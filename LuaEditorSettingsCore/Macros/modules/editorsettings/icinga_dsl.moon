-- Icinga configuration files have '.conf' as their file extension, hence
-- some additional criterion is needed to distinguish from other file types
-- with the same extension. Since these files have no specific header,
-- keeping them at predictable paths seems to fit best: this preset will
-- match whenever any part of the full path starts with 'icinga' or
-- 'icinga2'.
import
  Java
  from require'editorsettings.java'
IcingaMask="*.conf"
IcingaFolder=(Compare,Mask,FileName)->
  if FileName\match[[\icinga2?]]
    Compare Mask,FileName
  else
    false
class Icinga extends Java
  Title: "Icinga2 DSL"
  Type: (Compare,FileName)->IcingaFolder Compare,IcingaMask,FileName
{
  :Icinga
}
