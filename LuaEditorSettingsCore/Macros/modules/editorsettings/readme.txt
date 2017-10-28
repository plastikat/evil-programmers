== Some hints about user modules ==

The resulting schemes list is sorted alphabetically. The search for the
matching file type goes from the top down. Thus, if you want to define a
scheme to match some type that is already defined (in
editorsettings_base module, for instance), you've got two options to
make your module win this search:

1) Give it such a Title that would make it alphabetically higher on the
   list. In this case the original scheme stays on the list and can be
   selected manually when necessary.

2) Give it exactly the same Title as the original scheme defines and
   make sure your module is loaded last in editorsettings_user module.
   In this case you simply replace the original scheme with your own.

Choose the option that fits you best.


Now, to define your own scheme it is basically enough to import
"editorsettings_helpers" module and set some attributes ("Title" and
"Type" being a required minimum). To get a better picture look, for
example, inside python.moon (it's next to this file).

To _augment_ one of existing schemes you need to import the module that
defines it. Let's assume we want to augment XML scheme from
editorsettings_base module. Here's an example way do it:

-- moonscript start ----------------------------------------------------------

import
  Xml
  from require'editorsettings_base'

class Xml1 extends Xml
  TabSize: 4

{
  :Xml1
}

-- moonscript end ------------------------------------------------------------
