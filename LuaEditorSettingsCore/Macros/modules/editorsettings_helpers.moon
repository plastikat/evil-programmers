class Sources
  KillSpaces: true
  KillEmptyLines: true
  TabSize: 2
  AutoIndent: true
  CursorBeyondEol: true
  CharCodeBase: 2
  Lines: 64
  SmartHome: true
  SmartTab: true
  SmartBs: true
class UnixSources extends Sources
  Eol: "\n"

class Schemes
  new: =>
    @items = {}

  add_items: (items) =>
    for k,v in pairs items
      @items[v.Title] = v

  get_schemes: =>
    titles = {}
    values = {}
    for k,v in pairs @items
      table.insert titles, k
    table.sort titles, (a,b)->string.lower(a) < string.lower(b)
    for title in *titles
      table.insert values, @items[title]
    values

{
  :Sources, :UnixSources
  :Schemes
}
