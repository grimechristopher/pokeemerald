package main

import (
	"encoding/json"
	"fmt"
	"os"
	"regexp"
	"sort"
	"strconv"
	"strings"
)

// ---- wild_encounters.json schema -----------------------------------------
//
// Mirrors src/data/wild_encounters.json exactly (verified by hand against
// the live file and against src/wild_encounter.c's slot-selection logic -
// see README.md in this directory for how that was confirmed). Slot counts
// per encounter type are FIXED and hardcoded in the C engine:
//   land_mons:       12 slots (weights 20/20/10/10/10/10/5/5/4/4/1/1)
//   water_mons:       5 slots (weights 60/30/5/4/1)
//   rock_smash_mons:  5 slots (weights 60/30/5/4/1)
//   fishing_mons:    10 slots (old rod 0-1, good rod 2-4, super rod 5-9)
// Adding or removing slots produces entries the game can never roll -
// ChooseWildMonIndex_Land()/_Water()/_RockSmash()/_Fishing() are fixed
// if/else chains over compile-time ENCOUNTER_CHANCE_*_SLOT_N constants,
// not loops over the array length. This tool enforces the slot counts so
// that mistake (made once, by hand, before this tool existed) can't
// happen again.

type Mon struct {
	MinLevel int    `json:"min_level"`
	MaxLevel int    `json:"max_level"`
	Species  string `json:"species"`
}

type MonList struct {
	EncounterRate int   `json:"encounter_rate"`
	Mons          []Mon `json:"mons"`
}

type FieldSpec struct {
	Type           string `json:"type"`
	EncounterRates []int  `json:"encounter_rates"`
	// Note: Go's encoding/json always emits map keys in sorted order, so a
	// save through this tool will reorder these alphabetically (e.g.
	// fishing's old_rod/good_rod/super_rod becomes good_rod/old_rod/super_rod)
	// even though nothing about the encounter data actually changes - the
	// JSON->header generator reads .encounter_rates by index, not by the
	// order groups appear in this map, so it's cosmetic only.
	Groups map[string][]int `json:"groups,omitempty"`
}

type Encounter struct {
	// omitempty matters here: the Battle Pyramid/Pike groups have no "map"
	// key at all in the source JSON (for_maps: false) - without omitempty a
	// round-trip save would inject a spurious "map": "" into every one of
	// their entries (this happened once; see git history around this line).
	Map           string   `json:"map,omitempty"`
	BaseLabel     string   `json:"base_label"`
	LandMons      *MonList `json:"land_mons,omitempty"`
	WaterMons     *MonList `json:"water_mons,omitempty"`
	RockSmashMons *MonList `json:"rock_smash_mons,omitempty"`
	FishingMons   *MonList `json:"fishing_mons,omitempty"`
	HeadbuttMons  *MonList `json:"headbutt_mons,omitempty"`
}

type Group struct {
	Label      string      `json:"label"`
	ForMaps    bool        `json:"for_maps"`
	Fields     []FieldSpec `json:"fields,omitempty"`
	Encounters []Encounter `json:"encounters"`
}

type WildEncounters struct {
	WildEncounterGroups []Group `json:"wild_encounter_groups"`
}

const (
	LandSlots      = 12
	WaterSlots     = 5
	RockSmashSlots = 5
	FishingSlots   = 10
	HeadbuttSlots  = 5
)

var FishingRodLabel = map[int]string{
	0: "Old Rod", 1: "Old Rod",
	2: "Good Rod", 3: "Good Rod", 4: "Good Rod",
	5: "Super Rod", 6: "Super Rod", 7: "Super Rod", 8: "Super Rod", 9: "Super Rod",
}

// ---- load / save -----------------------------------------------------

func LoadWildEncounters(path string) (*WildEncounters, error) {
	raw, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	var we WildEncounters
	if err := json.Unmarshal(raw, &we); err != nil {
		return nil, err
	}
	// Sanity-check slot counts on load so a hand-edit outside the tool
	// (or a bug in the tool itself) surfaces immediately instead of
	// silently producing more dead slots.
	for _, g := range we.WildEncounterGroups {
		for _, e := range g.Encounters {
			if e.LandMons != nil && len(e.LandMons.Mons) != LandSlots {
				return nil, fmt.Errorf("%s: land_mons has %d slots, want %d", e.BaseLabel, len(e.LandMons.Mons), LandSlots)
			}
			if e.WaterMons != nil && len(e.WaterMons.Mons) != WaterSlots {
				return nil, fmt.Errorf("%s: water_mons has %d slots, want %d", e.BaseLabel, len(e.WaterMons.Mons), WaterSlots)
			}
			if e.RockSmashMons != nil && len(e.RockSmashMons.Mons) != RockSmashSlots {
				return nil, fmt.Errorf("%s: rock_smash_mons has %d slots, want %d", e.BaseLabel, len(e.RockSmashMons.Mons), RockSmashSlots)
			}
			if e.FishingMons != nil && len(e.FishingMons.Mons) != FishingSlots {
				return nil, fmt.Errorf("%s: fishing_mons has %d slots, want %d", e.BaseLabel, len(e.FishingMons.Mons), FishingSlots)
			}
			if e.HeadbuttMons != nil && len(e.HeadbuttMons.Mons) != HeadbuttSlots {
				return nil, fmt.Errorf("%s: headbutt_mons has %d slots, want %d", e.BaseLabel, len(e.HeadbuttMons.Mons), HeadbuttSlots)
			}
		}
	}
	return &we, nil
}

func SaveWildEncounters(path string, we *WildEncounters) error {
	out, err := json.MarshalIndent(we, "", "  ")
	if err != nil {
		return err
	}
	out = append(out, '\n')
	tmp := path + ".tmp"
	if err := os.WriteFile(tmp, out, 0o644); err != nil {
		return err
	}
	return os.Rename(tmp, path)
}

// ---- species list ------------------------------------------------------

var speciesDefineRe = regexp.MustCompile(`^#define\s+(SPECIES_[A-Z0-9_]+)\s+(\d+)\s*$`)

// species we never want to offer/count - not real catchable species.
var excludedSpeciesNames = map[string]bool{
	"SPECIES_NONE":        true,
	"SPECIES_EGG":         true,
	"SPECIES_SHINY_TAG":   true,
	"SPECIES_OLD_UNOWN_B": true, // legacy alias present in some forks; harmless if absent
}

type SpeciesInfo struct {
	Const       string // SPECIES_FOO
	DisplayName string // Foo
}

func LoadSpeciesList(constantsPath string) ([]SpeciesInfo, error) {
	raw, err := os.ReadFile(constantsPath)
	if err != nil {
		return nil, err
	}
	var list []SpeciesInfo
	for _, line := range strings.Split(string(raw), "\n") {
		line = strings.TrimRight(line, "\r")
		m := speciesDefineRe.FindStringSubmatch(line)
		if m == nil {
			continue
		}
		name := m[1]
		if excludedSpeciesNames[name] {
			continue
		}
		if _, err := strconv.Atoi(m[2]); err != nil {
			continue
		}
		list = append(list, SpeciesInfo{
			Const:       name,
			DisplayName: displayNameFor(name),
		})
	}
	sort.Slice(list, func(i, j int) bool { return list[i].DisplayName < list[j].DisplayName })
	return list, nil
}

// loadSpeciesNumbers is LoadSpeciesList's data but keyed by number instead
// of display-sorted - needed by resolveSpeciesConst to pick the
// lowest-numbered form of a multi-form species.
func loadSpeciesNumbers(constantsPath string) (map[string]int, error) {
	raw, err := os.ReadFile(constantsPath)
	if err != nil {
		return nil, err
	}
	out := map[string]int{}
	for _, line := range strings.Split(string(raw), "\n") {
		m := speciesDefineRe.FindStringSubmatch(strings.TrimRight(line, "\r"))
		if m == nil {
			continue
		}
		if n, err := strconv.Atoi(m[2]); err == nil {
			out[m[1]] = n
		}
	}
	return out, nil
}

func displayNameFor(constName string) string {
	rest := strings.TrimPrefix(constName, "SPECIES_")
	words := strings.Split(rest, "_")
	for i, w := range words {
		if w == "" {
			continue
		}
		words[i] = strings.ToUpper(w[:1]) + strings.ToLower(w[1:])
	}
	return strings.Join(words, " ")
}

// ---- grouping helper: one "route" = all time-of-day copies of a map ----

type RouteEntry struct {
	Suffix string // "Morning" / "Day" / "Evening" / "Night" / "" (no time-of-day variant)
	Idx    int    // index into the owning Group.Encounters slice
}

type Route struct {
	GroupIdx int
	Map      string
	Entries  []RouteEntry // usually 4 (time-of-day), sometimes 1
}

func BuildRoutes(we *WildEncounters) []Route {
	var routes []Route
	for gi, g := range we.WildEncounterGroups {
		byMap := map[string]*Route{}
		var order []string
		for ei, e := range g.Encounters {
			r, ok := byMap[e.Map]
			if !ok {
				order = append(order, e.Map)
				r = &Route{GroupIdx: gi, Map: e.Map}
				byMap[e.Map] = r
			}
			suffix := timeSuffix(e.BaseLabel, e.Map)
			r.Entries = append(r.Entries, RouteEntry{Suffix: suffix, Idx: ei})
		}
		for _, m := range order {
			routes = append(routes, *byMap[m])
		}
	}
	return routes
}

var timeSuffixes = []string{"Morning", "Day", "Evening", "Night"}

func timeSuffix(baseLabel, mapName string) string {
	for _, s := range timeSuffixes {
		if strings.HasSuffix(baseLabel, "_"+s) {
			return s
		}
	}
	return ""
}

// RouteDisplayName turns MAP_ROUTE102 into "Route102", MAP_METEOR_FALLS_1F_1R
// into "Meteor Falls 1F 1R", etc. - purely cosmetic for the sidebar.
func RouteDisplayName(mapConst string) string {
	rest := strings.TrimPrefix(mapConst, "MAP_")
	rest = strings.ReplaceAll(rest, "_", " ")
	return rest
}

// ---- Hoenn dex scoping ---------------------------------------------------
//
// src/pokemon.c defines sHoennToNationalOrder[HOENN_DEX_COUNT-1], built via
// entries shaped `HOENN_TO_NATIONAL(NAME)` where NAME is the same token used
// in both HOENN_DEX_##NAME and NATIONAL_DEX_##NAME - and, not coincidentally,
// in SPECIES_##NAME too. So the Hoenn dex roster can be read straight off
// that array's argument list without needing to cross-reference national dex
// numbers at all.
var hoennToNationalRe = regexp.MustCompile(`HOENN_TO_NATIONAL\(([A-Z0-9_]+)\)`)

// LoadHoennDexSpecies returns the set of SPECIES_ constants that are in the
// Hoenn Pokedex (gen 3's ~200-ish local dex, not the full national roster),
// parsed directly out of sHoennToNationalOrder in src/pokemon.c.
func LoadHoennDexSpecies(pokemonCPath string) (map[string]bool, error) {
	raw, err := os.ReadFile(pokemonCPath)
	if err != nil {
		return nil, err
	}
	text := string(raw)
	start := strings.Index(text, "sHoennToNationalOrder")
	if start == -1 {
		return nil, fmt.Errorf("sHoennToNationalOrder not found in %s", pokemonCPath)
	}
	end := strings.Index(text[start:], "\n};")
	if end == -1 {
		return nil, fmt.Errorf("sHoennToNationalOrder: no closing }; found in %s", pokemonCPath)
	}
	block := text[start : start+end]
	matches := hoennToNationalRe.FindAllStringSubmatch(block, -1)
	if len(matches) == 0 {
		return nil, fmt.Errorf("sHoennToNationalOrder: no HOENN_TO_NATIONAL(...) entries found")
	}
	set := make(map[string]bool, len(matches))
	for _, m := range matches {
		set["SPECIES_"+m[1]] = true
	}
	return set, nil
}

// ---- other regions' dexes (by origin, not an in-game regional dex) ------
//
// Unlike Hoenn, this fork never built a real in-game regional dex for
// Kanto/Johto/Sinnoh (no maps for those regions exist here), so there's no
// sHoennToNationalOrder-style reordered array to parse for them. What we
// have instead is include/constants/pokedex.h's `enum NationalDexOrder`,
// which lays out every species in one contiguous block per region of
// origin (`// Kanto`, `// Johto`, `// Hoenn`, `// Sinnoh`, ...), unreordered.
// That block tells you "this species first appeared in region X" - not
// "region X's in-game dex would list this species" the way the Hoenn
// extraction above does (which pulls in cross-gen evolutions like Roserade
// that originated elsewhere). For picking candidates to backfill into
// Hoenn's wild tables (the same idea already used for Johto-origin species
// like Aipom on the Headbutt trees), origin is exactly the right question
// to ask.
var regionCommentRe = regexp.MustCompile(`^\s*//\s*([A-Za-z]+)\s*$`)
var nationalDexTokenRe = regexp.MustCompile(`NATIONAL_DEX_([A-Z0-9_]+)`)

// non-region headers that can appear as `// Word` inside the enum.
var regionSkipHeaders = map[string]bool{"National": true, "Pokédex": true, "order": true, "Unknown": true}

// LoadRegionalDexSpecies parses every region's species-of-origin list out of
// pokedexHPath's `enum NationalDexOrder`, resolving NATIONAL_DEX_FOO to a
// real SPECIES_ constant via speciesByNum (see resolveSpeciesConst).
func LoadRegionalDexSpecies(pokedexHPath string, speciesByNum map[string]int) (map[string][]string, error) {
	raw, err := os.ReadFile(pokedexHPath)
	if err != nil {
		return nil, err
	}
	lines := strings.Split(string(raw), "\n")

	start := -1
	for i, l := range lines {
		if strings.Contains(l, "enum NationalDexOrder") {
			start = i
			break
		}
	}
	if start == -1 {
		return nil, fmt.Errorf("enum NationalDexOrder not found in %s", pokedexHPath)
	}
	end := -1
	for i := start; i < len(lines); i++ {
		if strings.TrimSpace(lines[i]) == "};" {
			end = i
			break
		}
	}
	if end == -1 {
		return nil, fmt.Errorf("enum NationalDexOrder: no closing }; found in %s", pokedexHPath)
	}

	regions := map[string][]string{}
	var order []string
	current := ""
	for _, line := range lines[start:end] {
		if m := regionCommentRe.FindStringSubmatch(line); m != nil {
			name := m[1]
			if regionSkipHeaders[name] {
				current = ""
				continue
			}
			current = name
			if _, ok := regions[current]; !ok {
				regions[current] = nil
				order = append(order, current)
			}
			continue
		}
		if current == "" {
			continue
		}
		if m := nationalDexTokenRe.FindStringSubmatch(line); m != nil {
			dexName := m[1]
			if resolved := resolveSpeciesConst(dexName, speciesByNum); resolved != "" {
				regions[current] = append(regions[current], resolved)
			}
		}
	}
	_ = order // regions map is all callers need today; order kept for future use
	return regions, nil
}

// resolveSpeciesConst maps a NATIONAL_DEX_FOO token to the right SPECIES_
// constant. Most species have a bare SPECIES_FOO; multi-form ones
// (Deoxys, Burmy, Giratina, Shellos, Basculin, ...) only exist as
// per-form constants (SPECIES_FOO_NORMAL, SPECIES_FOO_PLANT, ...) - in
// every case the form that kept the species' original dex-block number is
// its lowest-numbered SPECIES_FOO_* constant, since later-added forms get
// appended with much higher numbers, so that's the fallback.
func resolveSpeciesConst(dexName string, speciesByNum map[string]int) string {
	direct := "SPECIES_" + dexName
	if _, ok := speciesByNum[direct]; ok {
		return direct
	}
	prefix := direct + "_"
	best := ""
	bestNum := -1
	for name, num := range speciesByNum {
		if strings.HasPrefix(name, prefix) && (bestNum == -1 || num < bestNum) {
			best, bestNum = name, num
		}
	}
	return best
}

// FilterToHoenn narrows a species list down to just the ones in the Hoenn
// dex, preserving order.
//
// A couple of Hoenn dex entries (Deoxys, Castform) name a base species that
// this fork only exposes as a `#define SPECIES_FOO SPECIES_FOO_NORMAL` alias
// - not a numbered constant of its own, so LoadSpeciesList never lists
// "SPECIES_FOO" itself, only "SPECIES_FOO_NORMAL". Fall back to the _NORMAL
// form so those still count as present rather than silently dropping out.
func FilterToHoenn(all []SpeciesInfo, hoennDex map[string]bool) []SpeciesInfo {
	var out []SpeciesInfo
	for _, s := range all {
		if hoennDex[s.Const] {
			out = append(out, s)
			continue
		}
		if base, ok := strings.CutSuffix(s.Const, "_NORMAL"); ok && hoennDex[base] {
			out = append(out, s)
		}
	}
	return out
}

// FilterToRegion narrows a species list down to just the ones in the given
// set (built from LoadRegionalDexSpecies), preserving order. Unlike
// FilterToHoenn there's no alias fallback needed - resolveSpeciesConst
// already resolved multi-form species down to a real constant.
func FilterToRegion(all []SpeciesInfo, regionSpecies []string) []SpeciesInfo {
	set := make(map[string]bool, len(regionSpecies))
	for _, s := range regionSpecies {
		set[s] = true
	}
	var out []SpeciesInfo
	for _, s := range all {
		if set[s.Const] {
			out = append(out, s)
		}
	}
	return out
}

// ---- excluded species report -------------------------------------------

// ExcludedSpecies returns every enabled species that appears in NONE of the
// wild encounter tables, in any group, in any slot type. It does NOT know
// about gifts/trades/starters/legendaries/evolution-only mons - those are
// legitimately obtainable other ways, so "excluded" here means "not
// findable by walking, surfing, fishing, or rock-smashing", not "wholly
// unobtainable".
func ExcludedSpecies(we *WildEncounters, all []SpeciesInfo) []SpeciesInfo {
	present := map[string]bool{}
	for _, g := range we.WildEncounterGroups {
		for _, e := range g.Encounters {
			for _, ml := range []*MonList{e.LandMons, e.WaterMons, e.RockSmashMons, e.FishingMons} {
				if ml == nil {
					continue
				}
				for _, m := range ml.Mons {
					present[m.Species] = true
				}
			}
		}
	}
	var excluded []SpeciesInfo
	for _, s := range all {
		if !present[s.Const] {
			excluded = append(excluded, s)
		}
	}
	return excluded
}
