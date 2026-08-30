package main

const baseTemplate = `
{{define "base"}}
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Hoenn Encounter Editor</title>
<script src="/static/htmx.min.js"></script>
<style>
  :root { color-scheme: light dark; }
  body { font-family: ui-sans-serif, system-ui, sans-serif; margin: 0; display: flex; min-height: 100vh; }
  #sidebar { width: 260px; flex: none; border-right: 1px solid #8884; padding: 12px; overflow-y: auto; max-height: 100vh; position: sticky; top: 0; }
  #sidebar h3 { font-size: 11px; text-transform: uppercase; letter-spacing: .06em; opacity: .6; margin: 16px 0 4px; }
  #sidebar a { display: block; padding: 3px 6px; border-radius: 4px; text-decoration: none; color: inherit; font-size: 13px; }
  #sidebar a:hover, #sidebar a.active { background: #8882; }
  #sidebar input { width: 100%; box-sizing: border-box; padding: 4px 6px; margin-bottom: 6px; }
  #main { flex: 1; padding: 20px 28px; max-width: 1100px; }
  h1 { font-size: 20px; margin: 0 0 4px; }
  .sub { opacity: .6; font-size: 13px; margin-bottom: 18px; }
  table { border-collapse: collapse; width: 100%; margin-bottom: 10px; font-size: 13px; }
  th, td { border: 1px solid #8883; padding: 4px 6px; text-align: left; }
  th { background: #8881; font-weight: 600; }
  input[type=number] { width: 56px; }
  input[list] { width: 220px; }
  .block { border: 1px solid #8884; border-radius: 8px; padding: 12px 14px; margin-bottom: 16px; }
  .block h2 { font-size: 14px; margin: 0 0 10px; }
  .rate { opacity: .6; font-weight: 400; font-size: 12px; }
  .save-bar { position: sticky; bottom: 0; background: Canvas; padding: 10px 0; border-top: 1px solid #8884; margin-top: 10px; }
  button { padding: 8px 18px; font-size: 14px; cursor: pointer; }
  .msg { color: #2a7; font-size: 13px; margin-left: 12px; }
  .rod { font-size: 11px; opacity: .55; }
  code { background: #8882; padding: 1px 4px; border-radius: 3px; }
  .excl-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(160px, 1fr)); gap: 4px 12px; font-size: 13px; }
</style>
</head>
<body>
{{template "sidebar" .}}
<div id="main">
{{template "content" .}}
</div>
</body>
</html>
{{end}}
`

const sidebarTemplate = `
{{define "sidebar"}}
<div id="sidebar">
  <input type="text" id="route-filter" placeholder="Filter routes..." onkeyup="filterRoutes(this.value)">
  <a href="/">&larr; Overview</a>
  <a href="/excluded">Excluded species (Hoenn)</a>
  <a href="/excluded/Kanto">Excluded species (Kanto)</a>
  <a href="/excluded/Johto">Excluded species (Johto)</a>
  <a href="/excluded/Sinnoh">Excluded species (Sinnoh)</a>
  {{range .Groups}}
    <h3>{{.Label}}</h3>
    {{range .Routes}}
      <a class="route-link" data-name="{{.Display}}" href="/route/{{.GroupIdx}}/{{.Map}}"
         hx-get="/route/{{.GroupIdx}}/{{.Map}}" hx-target="#main" hx-push-url="true">{{.Display}}</a>
    {{end}}
  {{end}}
</div>
<script>
function filterRoutes(q) {
  q = q.toLowerCase();
  document.querySelectorAll('.route-link').forEach(function(a) {
    a.style.display = a.dataset.name.toLowerCase().includes(q) ? '' : 'none';
  });
}
</script>
{{end}}
`

const indexTemplate = `
{{define "content"}}
<h1>Hoenn Encounter Editor</h1>
<p class="sub">Editing <code>src/data/wild_encounters.json</code> directly. Pick a route on the left.</p>
<p>{{.Excluded}} Hoenn-dex species appear in <strong>zero</strong> wild encounter tables right now.
<a href="/excluded">See the full list &rarr;</a></p>
<p class="sub">Every slot count below is fixed by the game engine (12 land / 5 water / 5 rock smash / 10 fishing) -
this editor only lets you change <em>which species and levels</em> sit in each slot, never add or remove slots,
so it's not possible to accidentally create an unreachable encounter.</p>
{{end}}
`

const shellRouteTemplate = `
{{define "content"}}
<div id="route-panel">
{{template "route-panel" .}}
</div>
{{end}}
`

const routePanelTemplate = `
{{define "route-panel"}}
<h1>{{.Display}}</h1>
<p class="sub"><code>{{.Map}}</code>{{if .Message}} <span class="msg">{{.Message}}</span>{{end}}</p>
<form hx-post="/save/{{.GroupIdx}}/{{.Map}}" hx-target="#route-panel" hx-swap="outerHTML">
{{range .Blocks}}
  <div class="block">
    <h2>{{if .Suffix}}{{.Suffix}}{{else}}(no time-of-day split){{end}}</h2>

    {{if .HasLand}}
      <table>
        <caption style="caption-side:top;text-align:left;font-size:12px;opacity:.7;margin-bottom:4px">
          Land / grass <span class="rate">(encounter rate: {{.LandRate}}%)</span>
        </caption>
        <thead><tr><th>Slot</th><th>Weight</th><th>Species</th><th>Min Lv</th><th>Max Lv</th></tr></thead>
        <tbody>
        {{$suf := .Suffix}}
        {{range .Land}}
          <tr>
            <td>{{.Index}}</td>
            <td>{{.Weight}}%</td>
            <td><input list="species-list" name="land_{{$suf}}_{{.Index}}_species" value="{{.Species}}"></td>
            <td><input type="number" name="land_{{$suf}}_{{.Index}}_min" value="{{.MinLvl}}" min="1" max="100"></td>
            <td><input type="number" name="land_{{$suf}}_{{.Index}}_max" value="{{.MaxLvl}}" min="1" max="100"></td>
          </tr>
        {{end}}
        </tbody>
      </table>
    {{end}}

    {{if .HasWater}}
      <table>
        <caption style="caption-side:top;text-align:left;font-size:12px;opacity:.7;margin-bottom:4px">
          Water / surfing <span class="rate">(encounter rate: {{.WaterRate}}%)</span>
        </caption>
        <thead><tr><th>Slot</th><th>Weight</th><th>Species</th><th>Min Lv</th><th>Max Lv</th></tr></thead>
        <tbody>
        {{$suf := .Suffix}}
        {{range .Water}}
          <tr>
            <td>{{.Index}}</td>
            <td>{{.Weight}}%</td>
            <td><input list="species-list" name="water_{{$suf}}_{{.Index}}_species" value="{{.Species}}"></td>
            <td><input type="number" name="water_{{$suf}}_{{.Index}}_min" value="{{.MinLvl}}" min="1" max="100"></td>
            <td><input type="number" name="water_{{$suf}}_{{.Index}}_max" value="{{.MaxLvl}}" min="1" max="100"></td>
          </tr>
        {{end}}
        </tbody>
      </table>
    {{end}}

    {{if .HasRockSmash}}
      <table>
        <caption style="caption-side:top;text-align:left;font-size:12px;opacity:.7;margin-bottom:4px">
          Rock Smash <span class="rate">(encounter rate: {{.RockSmashRate}}%)</span>
        </caption>
        <thead><tr><th>Slot</th><th>Weight</th><th>Species</th><th>Min Lv</th><th>Max Lv</th></tr></thead>
        <tbody>
        {{$suf := .Suffix}}
        {{range .RockSmash}}
          <tr>
            <td>{{.Index}}</td>
            <td>{{.Weight}}%</td>
            <td><input list="species-list" name="rock_{{$suf}}_{{.Index}}_species" value="{{.Species}}"></td>
            <td><input type="number" name="rock_{{$suf}}_{{.Index}}_min" value="{{.MinLvl}}" min="1" max="100"></td>
            <td><input type="number" name="rock_{{$suf}}_{{.Index}}_max" value="{{.MaxLvl}}" min="1" max="100"></td>
          </tr>
        {{end}}
        </tbody>
      </table>
    {{end}}

    {{if .HasFishing}}
      <table>
        <caption style="caption-side:top;text-align:left;font-size:12px;opacity:.7;margin-bottom:4px">
          Fishing <span class="rate">(bite rate: {{.FishingRate}}%)</span>
        </caption>
        <thead><tr><th>Slot</th><th>Rod</th><th>Weight</th><th>Species</th><th>Min Lv</th><th>Max Lv</th></tr></thead>
        <tbody>
        {{$suf := .Suffix}}
        {{range .Fishing}}
          <tr>
            <td>{{.Index}}</td>
            <td class="rod">{{.RodTier}}</td>
            <td>{{.Weight}}%</td>
            <td><input list="species-list" name="fish_{{$suf}}_{{.Index}}_species" value="{{.Species}}"></td>
            <td><input type="number" name="fish_{{$suf}}_{{.Index}}_min" value="{{.MinLvl}}" min="1" max="100"></td>
            <td><input type="number" name="fish_{{$suf}}_{{.Index}}_max" value="{{.MaxLvl}}" min="1" max="100"></td>
          </tr>
        {{end}}
        </tbody>
      </table>
    {{end}}

    {{if .HasHeadbutt}}
      <table>
        <caption style="caption-side:top;text-align:left;font-size:12px;opacity:.7;margin-bottom:4px">
          Headbutt tree <span class="rate">(encounter rate: {{.HeadbuttRate}}%)</span>
        </caption>
        <thead><tr><th>Slot</th><th>Weight</th><th>Species</th><th>Min Lv</th><th>Max Lv</th></tr></thead>
        <tbody>
        {{$suf := .Suffix}}
        {{range .Headbutt}}
          <tr>
            <td>{{.Index}}</td>
            <td>{{.Weight}}%</td>
            <td><input list="species-list" name="headbutt_{{$suf}}_{{.Index}}_species" value="{{.Species}}"></td>
            <td><input type="number" name="headbutt_{{$suf}}_{{.Index}}_min" value="{{.MinLvl}}" min="1" max="100"></td>
            <td><input type="number" name="headbutt_{{$suf}}_{{.Index}}_max" value="{{.MaxLvl}}" min="1" max="100"></td>
          </tr>
        {{end}}
        </tbody>
      </table>
    {{end}}
  </div>
{{end}}

  <div class="save-bar">
    <button type="submit">Save route</button>
  </div>
</form>

<datalist id="species-list">
{{range .AllSpecies}}<option value="{{.Const}}">{{.DisplayName}}</option>
{{end}}
</datalist>
{{end}}
`

const excludedTemplate = `
{{define "content"}}
<h1>Excluded species</h1>
<p class="sub">{{.Total}} of {{.AllCount}} Hoenn-dex species (the local Hoenn Pokedex only - this count excludes the
extra Kanto/Johto/Sinnoh/mega/custom-form species this fork also carries, which were swamping the count with
species that were never going to be Hoenn wild encounters anyway) appear in <strong>zero</strong> wild
encounter slots (land, water, rock smash, or fishing - across every group, not just overworld routes).
This does <em>not</em> mean unobtainable - starters, gifts, trades, static/legendary encounters, and
evolution-only forms are expected to show up here too. It just means "not findable by walking, surfing,
fishing, or smashing a rock."</p>
<div class="excl-grid">
{{range .Excluded}}<div title="{{.Const}}">{{.DisplayName}}</div>
{{end}}
</div>
{{end}}
`

const excludedRegionTemplate = `
{{define "content"}}
<h1>Excluded species ({{.Region}})</h1>
<p class="sub">{{.Total}} of {{.AllCount}} {{.Region}}-origin species (species that first appeared in
{{.Region}}, per the National Dex order in <code>include/constants/pokedex.h</code> - not an in-game
{{.Region}} regional dex, since no {{.Region}} maps exist in this ROM) appear in <strong>zero</strong> wild
encounter slots anywhere in Hoenn's tables. This is a shortlist of candidates for backfilling Hoenn's routes
with non-Hoenn species (the same idea already used for Aipom/Spinarak/Pineco/Ledyba/Heracross on the
Headbutt trees) - not a gap that needs filling. Starters/gifts/trades/legendaries are expected here too.</p>
<div class="excl-grid">
{{range .Excluded}}<div title="{{.Const}}">{{.DisplayName}}</div>
{{end}}
</div>
{{end}}
`
