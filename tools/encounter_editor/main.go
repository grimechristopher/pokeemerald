// Command encounter_editor is a local web UI for editing
// src/data/wild_encounters.json - every Hoenn route's land/water/
// rock-smash/fishing encounters, split out by time-of-day, plus a report
// of which species appear in NONE of them.
//
// Run from the repo root (it looks for src/data/wild_encounters.json and
// include/constants/species.h relative to cwd, or pass -repo):
//
//	go run ./tools/encounter_editor
//
// Then open http://localhost:8090. Saving writes wild_encounters.json
// directly; `make` regenerates wild_encounters.h from it as usual.
package main

import (
	"embed"
	"flag"
	"fmt"
	"html/template"
	"io/fs"
	"log"
	"net/http"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"sync"
)

//go:embed static
var staticFS embed.FS

type Server struct {
	mu            sync.Mutex
	jsonPath      string
	we            *WildEncounters
	species       []SpeciesInfo
	hoennSpecies  []SpeciesInfo
	regionSpecies map[string][]SpeciesInfo // "Kanto", "Johto", "Sinnoh", etc. - species of origin, see LoadRegionalDexSpecies
	regionOrder   []string                 // display order for the sidebar/index
	routes        []Route
}

// Regions (besides Hoenn, handled separately above) worth surfacing in the
// UI. LoadRegionalDexSpecies actually finds every region pokedex.h knows
// about (Unova/Kalos/Alola/Galar too) - this just controls which ones show
// up as their own "excluded species" report, since Kanto/Johto/Sinnoh are
// what's been relevant so far (see CREDITS.md / wild_encounters.json for
// the Johto-origin species already placed into Hoenn's tables).
var regionsShownInUI = []string{"Kanto", "Johto", "Sinnoh"}

func main() {
	repo := flag.String("repo", ".", "path to the pokeemerald repo root")
	addr := flag.String("addr", ":8090", "listen address")
	flag.Parse()

	jsonPath := filepath.Join(*repo, "src", "data", "wild_encounters.json")
	speciesPath := filepath.Join(*repo, "include", "constants", "species.h")
	pokedexPath := filepath.Join(*repo, "include", "constants", "pokedex.h")

	we, err := LoadWildEncounters(jsonPath)
	if err != nil {
		log.Fatalf("loading %s: %v\n(run this from the repo root, or pass -repo <path>)", jsonPath, err)
	}
	species, err := LoadSpeciesList(speciesPath)
	if err != nil {
		log.Fatalf("loading %s: %v", speciesPath, err)
	}
	hoennDex, err := LoadHoennDexSpecies(pokedexPath)
	if err != nil {
		log.Fatalf("loading %s: %v", pokedexPath, err)
	}
	hoennSpecies := FilterToHoenn(species, hoennDex)

	speciesByNum, err := loadSpeciesNumbers(speciesPath)
	if err != nil {
		log.Fatalf("loading %s: %v", speciesPath, err)
	}
	regionsRaw, err := LoadRegionalDexSpecies(pokedexPath, speciesByNum)
	if err != nil {
		log.Fatalf("loading %s: %v", pokedexPath, err)
	}
	regionSpecies := map[string][]SpeciesInfo{}
	var regionOrder []string
	for _, name := range regionsShownInUI {
		regionSpecies[name] = FilterToRegion(species, regionsRaw[name])
		regionOrder = append(regionOrder, name)
	}

	log.Printf("loaded %d wild_encounter_groups, %d species (%d in the Hoenn dex; %s)",
		len(we.WildEncounterGroups), len(species), len(hoennSpecies), regionCountsLog(regionSpecies, regionOrder))

	s := &Server{
		jsonPath:      jsonPath,
		we:            we,
		species:       species,
		hoennSpecies:  hoennSpecies,
		regionSpecies: regionSpecies,
		regionOrder:   regionOrder,
	}
	s.routes = BuildRoutes(we)

	mux := http.NewServeMux()
	mux.HandleFunc("/", s.handleIndex)
	mux.HandleFunc("/route/", s.handleRoute)
	mux.HandleFunc("/save/", s.handleSave)
	mux.HandleFunc("/excluded", s.handleExcluded)
	mux.HandleFunc("/excluded/", s.handleExcludedRegion)
	staticSub, err := fs.Sub(staticFS, "static")
	if err != nil {
		log.Fatal(err)
	}
	mux.Handle("/static/", http.StripPrefix("/static/", http.FileServer(http.FS(staticSub))))

	log.Printf("encounter editor: http://localhost%s  (data: %s)", *addr, jsonPath)
	log.Fatal(http.ListenAndServe(*addr, mux))
}

// ---- sidebar / index -----------------------------------------------------

func (s *Server) handleIndex(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/" {
		http.NotFound(w, r)
		return
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	data := struct {
		Groups   []GroupSummary
		Excluded int
	}{
		Groups:   s.groupSummaries(),
		Excluded: len(ExcludedSpecies(s.we, s.hoennSpecies)),
	}
	renderPage(w, "index", data)
}

type GroupSummary struct {
	Idx    int
	Label  string
	Routes []RouteSummary
}

type RouteSummary struct {
	GroupIdx int
	Map      string
	Display  string
}

func regionCountsLog(regionSpecies map[string][]SpeciesInfo, order []string) string {
	parts := make([]string, len(order))
	for i, name := range order {
		parts[i] = fmt.Sprintf("%d in %s", len(regionSpecies[name]), name)
	}
	return strings.Join(parts, ", ")
}

func (s *Server) groupSummaries() []GroupSummary {
	var out []GroupSummary
	seen := map[int]*GroupSummary{}
	var order []int
	for _, rt := range s.routes {
		gs, ok := seen[rt.GroupIdx]
		if !ok {
			label := s.we.WildEncounterGroups[rt.GroupIdx].Label
			gs = &GroupSummary{Idx: rt.GroupIdx, Label: label}
			seen[rt.GroupIdx] = gs
			order = append(order, rt.GroupIdx)
		}
		gs.Routes = append(gs.Routes, RouteSummary{GroupIdx: rt.GroupIdx, Map: rt.Map, Display: RouteDisplayName(rt.Map)})
	}
	sort.Ints(order)
	for _, gi := range order {
		out = append(out, *seen[gi])
	}
	return out
}

// ---- route editor ---------------------------------------------------------

type SlotView struct {
	Index   int
	Weight  int
	RodTier string // only set for fishing
	Species string
	MinLvl  int
	MaxLvl  int
}

type TimeBlock struct {
	Suffix        string // Morning/Day/Evening/Night/""
	EncIdx        int    // index into group.Encounters
	HasLand       bool
	HasWater      bool
	HasRockSmash  bool
	HasFishing    bool
	HasHeadbutt   bool
	Land          []SlotView
	Water         []SlotView
	RockSmash     []SlotView
	Fishing       []SlotView
	Headbutt      []SlotView
	LandRate      int
	WaterRate     int
	RockSmashRate int
	FishingRate   int
	HeadbuttRate  int
}

type RoutePage struct {
	GroupIdx   int
	Map        string
	Display    string
	Blocks     []TimeBlock
	AllSpecies []SpeciesInfo
	Groups     []GroupSummary
	Message    string
}

func (s *Server) findRoute(groupIdx int, mapName string) *Route {
	for i := range s.routes {
		if s.routes[i].GroupIdx == groupIdx && s.routes[i].Map == mapName {
			return &s.routes[i]
		}
	}
	return nil
}

func fieldWeights(g Group, typ string) []int {
	for _, f := range g.Fields {
		if f.Type == typ {
			return f.EncounterRates
		}
	}
	return nil
}

func (s *Server) buildRoutePage(groupIdx int, mapName string, message string) (*RoutePage, error) {
	rt := s.findRoute(groupIdx, mapName)
	if rt == nil {
		return nil, fmt.Errorf("route not found: group %d map %s", groupIdx, mapName)
	}
	g := s.we.WildEncounterGroups[groupIdx]
	landW := fieldWeights(g, "land_mons")
	waterW := fieldWeights(g, "water_mons")
	rockW := fieldWeights(g, "rock_smash_mons")
	fishW := fieldWeights(g, "fishing_mons")
	headbuttW := fieldWeights(g, "headbutt_mons")

	page := &RoutePage{
		GroupIdx:   groupIdx,
		Map:        mapName,
		Display:    RouteDisplayName(mapName),
		AllSpecies: s.species,
		Groups:     s.groupSummaries(),
		Message:    message,
	}

	for _, re := range rt.Entries {
		e := g.Encounters[re.Idx]
		tb := TimeBlock{Suffix: re.Suffix, EncIdx: re.Idx}
		if e.LandMons != nil {
			tb.HasLand = true
			tb.LandRate = e.LandMons.EncounterRate
			tb.Land = buildSlots(e.LandMons.Mons, landW, nil)
		}
		if e.WaterMons != nil {
			tb.HasWater = true
			tb.WaterRate = e.WaterMons.EncounterRate
			tb.Water = buildSlots(e.WaterMons.Mons, waterW, nil)
		}
		if e.RockSmashMons != nil {
			tb.HasRockSmash = true
			tb.RockSmashRate = e.RockSmashMons.EncounterRate
			tb.RockSmash = buildSlots(e.RockSmashMons.Mons, rockW, nil)
		}
		if e.FishingMons != nil {
			tb.HasFishing = true
			tb.FishingRate = e.FishingMons.EncounterRate
			tb.Fishing = buildSlots(e.FishingMons.Mons, fishW, FishingRodLabel)
		}
		if e.HeadbuttMons != nil {
			tb.HasHeadbutt = true
			tb.HeadbuttRate = e.HeadbuttMons.EncounterRate
			tb.Headbutt = buildSlots(e.HeadbuttMons.Mons, headbuttW, nil)
		}
		page.Blocks = append(page.Blocks, tb)
	}
	return page, nil
}

func buildSlots(mons []Mon, weights []int, rodLabel map[int]string) []SlotView {
	out := make([]SlotView, len(mons))
	for i, m := range mons {
		sv := SlotView{Index: i, Species: m.Species, MinLvl: m.MinLevel, MaxLvl: m.MaxLevel}
		if i < len(weights) {
			sv.Weight = weights[i]
		}
		if rodLabel != nil {
			sv.RodTier = rodLabel[i]
		}
		out[i] = sv
	}
	return out
}

// URL shape: /route/{groupIdx}/{MAP_CONST}
func parseRoutePath(prefix, path string) (int, string, bool) {
	rest := strings.TrimPrefix(path, prefix)
	parts := strings.SplitN(rest, "/", 2)
	if len(parts) != 2 {
		return 0, "", false
	}
	gi, err := strconv.Atoi(parts[0])
	if err != nil {
		return 0, "", false
	}
	return gi, parts[1], true
}

func (s *Server) handleRoute(w http.ResponseWriter, r *http.Request) {
	gi, mapName, ok := parseRoutePath("/route/", r.URL.Path)
	if !ok {
		http.NotFound(w, r)
		return
	}
	s.mu.Lock()
	page, err := s.buildRoutePage(gi, mapName, "")
	s.mu.Unlock()
	if err != nil {
		http.Error(w, err.Error(), 404)
		return
	}
	if r.Header.Get("HX-Request") == "true" {
		renderFragment(w, "route-panel", page)
		return
	}
	renderPage(w, "shell-route", page)
}

// ---- save -------------------------------------------------------------

func (s *Server) handleSave(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "POST only", 405)
		return
	}
	gi, mapName, ok := parseRoutePath("/save/", r.URL.Path)
	if !ok {
		http.NotFound(w, r)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, err.Error(), 400)
		return
	}

	s.mu.Lock()
	defer s.mu.Unlock()

	rt := s.findRoute(gi, mapName)
	if rt == nil {
		http.Error(w, "route not found", 404)
		return
	}
	g := &s.we.WildEncounterGroups[gi]

	for _, re := range rt.Entries {
		e := &g.Encounters[re.Idx]
		applySlots(r.Form, re.Suffix, "land", e.LandMons)
		applySlots(r.Form, re.Suffix, "water", e.WaterMons)
		applySlots(r.Form, re.Suffix, "rock", e.RockSmashMons)
		applySlots(r.Form, re.Suffix, "fish", e.FishingMons)
		applySlots(r.Form, re.Suffix, "headbutt", e.HeadbuttMons)
	}

	if err := SaveWildEncounters(s.jsonPath, s.we); err != nil {
		http.Error(w, "save failed: "+err.Error(), 500)
		return
	}

	page, err := s.buildRoutePage(gi, mapName, "Saved to wild_encounters.json - run `make` to rebuild.")
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}
	renderFragment(w, "route-panel", page)
}

func applySlots(form map[string][]string, suffix, kind string, ml *MonList) {
	if ml == nil {
		return
	}
	for i := range ml.Mons {
		prefix := fmt.Sprintf("%s_%s_%d_", kind, suffix, i)
		if v := formVal(form, prefix+"species"); v != "" {
			ml.Mons[i].Species = v
		}
		if v, err := strconv.Atoi(formVal(form, prefix+"min")); err == nil {
			ml.Mons[i].MinLevel = v
		}
		if v, err := strconv.Atoi(formVal(form, prefix+"max")); err == nil {
			ml.Mons[i].MaxLevel = v
		}
	}
}

func formVal(form map[string][]string, key string) string {
	if v, ok := form[key]; ok && len(v) > 0 {
		return v[0]
	}
	return ""
}

// ---- excluded species report -------------------------------------------

func (s *Server) handleExcluded(w http.ResponseWriter, r *http.Request) {
	s.mu.Lock()
	defer s.mu.Unlock()
	excluded := ExcludedSpecies(s.we, s.hoennSpecies)
	data := struct {
		Excluded []SpeciesInfo
		Total    int
		AllCount int
		Groups   []GroupSummary
	}{
		Excluded: excluded,
		Total:    len(excluded),
		AllCount: len(s.hoennSpecies),
		Groups:   s.groupSummaries(),
	}
	renderPage(w, "excluded", data)
}

// handleExcludedRegion serves /excluded/{Region} for each of regionsShownInUI
// (Kanto/Johto/Sinnoh) - same idea as handleExcluded but scoped to species
// that originated in that region instead of the Hoenn dex. Since none of
// these regions have maps in this ROM, "excluded" here just means "hasn't
// been placed into one of Hoenn's wild tables yet" - i.e. it's a shortlist
// of candidates, not a gap to necessarily fill.
func (s *Server) handleExcludedRegion(w http.ResponseWriter, r *http.Request) {
	region := strings.TrimPrefix(r.URL.Path, "/excluded/")
	s.mu.Lock()
	defer s.mu.Unlock()
	list, ok := s.regionSpecies[region]
	if !ok {
		http.NotFound(w, r)
		return
	}
	excluded := ExcludedSpecies(s.we, list)
	data := struct {
		Region   string
		Excluded []SpeciesInfo
		Total    int
		AllCount int
		Groups   []GroupSummary
	}{
		Region:   region,
		Excluded: excluded,
		Total:    len(excluded),
		AllCount: len(list),
		Groups:   s.groupSummaries(),
	}
	renderPage(w, "excluded-region", data)
}

// ---- templating ----------------------------------------------------------

var funcMap = template.FuncMap{
	"add": func(a, b int) int { return a + b },
}

func renderPage(w http.ResponseWriter, name string, data any) {
	t := template.Must(template.New("base").Funcs(funcMap).Parse(baseTemplate))
	t = template.Must(t.Parse(sidebarTemplate))
	switch name {
	case "index":
		t = template.Must(t.Parse(indexTemplate))
	case "shell-route":
		t = template.Must(t.Parse(routePanelTemplate))
		t = template.Must(t.Parse(shellRouteTemplate))
	case "excluded":
		t = template.Must(t.Parse(excludedTemplate))
	case "excluded-region":
		t = template.Must(t.Parse(excludedRegionTemplate))
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := t.ExecuteTemplate(w, "base", data); err != nil {
		log.Println("render error:", err)
	}
}

func renderFragment(w http.ResponseWriter, name string, data any) {
	t := template.Must(template.New("frag").Funcs(funcMap).Parse(routePanelTemplate))
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := t.ExecuteTemplate(w, name, data); err != nil {
		log.Println("render error:", err)
	}
}
