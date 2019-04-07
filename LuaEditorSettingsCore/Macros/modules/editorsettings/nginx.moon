-- Nginx configuration files have '.conf' as their file extension, hence
-- some additional criterion is needed to distinguish from other file types
-- with the same extension. Since these files have no specific header,
-- keeping them at predictable paths seems to fit best: this preset will
-- match whenever any part of the full path starts with 'nginx'.
import
  UnixSources
  from require'editorsettings_helpers'
NginxMask="*.conf"
NginxFolder=(Compare,Mask,FileName)->
  if FileName\match[[\nginx]]
    Compare Mask,FileName
  else
    false
class Nginx extends UnixSources
  Title: "Nginx"
  Type: (Compare,FileName)->NginxFolder Compare,NginxMask,FileName
  CodePage: 65001
  TabSize: 4
  ExpandTabs: true
  SmartHome: true
  SetBOM: false
{
  :Nginx
}
