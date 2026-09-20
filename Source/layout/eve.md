This file defines EVE's user configuration and visual appearance. It is
seeded to ~/.config/end/eve.md on first launch and reloaded while EVE runs.
Colour format: 0xAARRGGBB hex integer (Alpha|Red|Green|Blue). The style
section references colours by their palette name. The `value` column is the
light appearance; the `dark` column is the dark appearance.

## settings

Plugin settings. The chain populates the persisted-parameter subtree from
this table on first launch; the host state overrides it after restore.

+------------+--------+---------+------------------------------------------------------------------+
| key        | type   | value   | description                                                      |
+============+========+=========+==================================================================+
| UI_SCALE   | string | MEDIUM  | Editor scale step.                                               |
+------------+--------+---------+------------------------------------------------------------------+
| APPEARANCE | string | DESKTOP | Light/dark appearance; DESKTOP follows the OS.                   |
+------------+--------+---------+------------------------------------------------------------------+

## UI_size

Editor window size in pixels. Updated by the Model at runtime; the host
state overrides it after restore.

+---------+--------+-------+-----------------------------------------------------------------------+
| key     | type   | value | description                                                           |
+=========+========+=======+=======================================================================+
| width   | int    | 600   | Initial editor width in pixels.                                       |
+---------+--------+-------+-----------------------------------------------------------------------+
| height  | int    | 400   | Initial editor height in pixels.                                      |
+---------+--------+-------+-----------------------------------------------------------------------+

## colours

Named palette. Every style entry below resolves its colour through this
table by name. Names and values follow the product colour scheme.

+---------------+--------+------------+------------------------------------------------------------+
| key           | type   | value      | description                                                |
+===============+========+============+============================================================+
| blank         | colour | 0x00000000 | Fully transparent.                                         |
+---------------+--------+------------+------------------------------------------------------------+
| bunker        | colour | 0xff090d12 | Background.                                                |
+---------------+--------+------------+------------------------------------------------------------+
| corbeau       | colour | 0xff0d141c | Highlight background.                                      |
+---------------+--------+------------+------------------------------------------------------------+
| paradiso      | colour | 0xff4e8c93 | Normal text.                                               |
+---------------+--------+------------+------------------------------------------------------------+
| caribbeanBlue | colour | 0xff01c2d2 | Bright text.                                               |
+---------------+--------+------------+------------------------------------------------------------+
| mediterranea  | colour | 0xff33535b | Dim text.                                                  |
+---------------+--------+------------+------------------------------------------------------------+
| carbon        | colour | 0xff333435 | Scrollbar thumb.                                           |
+---------------+--------+------------+------------------------------------------------------------+

## window

Window chrome configuration. Entries are read by the runtime by id — names
are the contract, used verbatim.

+---------+--------+----------------+--------------------------------------------------------------+
| key     | type   | value          | description                                                  |
+=========+========+================+==============================================================+
| mac     | string | backgroundBlur | macOS window visual effect style.                            |
+---------+--------+----------------+--------------------------------------------------------------+
| win     | string | acrylic10      | Windows window visual effect style.                          |
+---------+--------+----------------+--------------------------------------------------------------+
| opacity | float  | 0.75           | Window background opacity (0.0 - 1.0).                       |
+---------+--------+----------------+--------------------------------------------------------------+
| blur    | int    | 20             | Background blur radius in pixels.                            |
+---------+--------+----------------+--------------------------------------------------------------+
| padding | int    | 8              | Padding in pixels reserved around the window content.        |
+---------+--------+----------------+--------------------------------------------------------------+

## style

Component colour assignments. The key is the LookAndFeel colour id; the
`value` (light) and `dark` cells name a palette colour above.

+---------------------------------------+--------+----------+----------+---------------------------+
| key                                   | type   | value    | dark     | description               |
+=======================================+========+==========+==========+===========================+
| --ResizableWindow--backgroundColourId | string | bunker   | bunker   | Window background.        |
+---------------------------------------+--------+----------+----------+---------------------------+
| --TextEditor--textColourId            | string | paradiso | paradiso | Default terminal text.    |
+---------------------------------------+--------+----------+----------+---------------------------+
| --ScrollBar--thumbColourId            | string | carbon   | carbon   | Scrollbar thumb.          |
+---------------------------------------+--------+----------+----------+---------------------------+
| --ScrollBar--trackColourId            | string | blank    | blank    | Scrollbar track.          |
+---------------------------------------+--------+----------+----------+---------------------------+

## fonts

Terminal font record. The file is an embedded binary resource; height is
the point height and kerning the kerning factor applied at registration.

+---------+--------+---------------------+---------------------------------------------------------+
| key     | type   | value               | description                                             |
+=========+========+=====================+=========================================================+
| mono    | string | DisplayMonoBook.ttf | Monospaced terminal font file.                          |
+---------+--------+---------------------+---------------------------------------------------------+
| height  | float  | 12                  | Point height.                                           |
+---------+--------+---------------------+---------------------------------------------------------+
| kerning | float  | 0.1                 | Kerning factor.                                         |
+---------+--------+---------------------+---------------------------------------------------------+

## keys

Editor key bindings. One row per action; the `normal`, `insert`, `visual`
cells hold the chord for that mode, empty = unbound. A chord is one
character (`h`, `$`, `G`), a two-character sequence (`gg`), or a JUCE key
description (`ctrl+u`, `escape`, `return`, `backspace`).

+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| key                  | type   | normal | insert    | visual | description                                                        |
+======================+========+========+===========+========+====================================================================+
| moveLeft             | string | h      |           | h      | Move the cursor one column left.                                   |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| moveDown             | string | j      |           | j      | Move the cursor one line down.                                     |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| moveUp               | string | k      |           | k      | Move the cursor one line up.                                       |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| moveRight            | string | l      |           | l      | Move the cursor one column right.                                  |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| wordForward          | string | w      |           | w      | Move the cursor to the start of the next word.                     |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| wordBackward         | string | b      |           | b      | Move the cursor to the start of the previous word.                 |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| wordEnd              | string | e      |           | e      | Move the cursor to the end of the current word.                    |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| lineStart            | string | 0      |           | 0      | Move the cursor to the first column of the line.                   |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| lineEnd              | string | $      |           | $      | Move the cursor to the last column of the line.                    |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| documentStart        | string | gg     |           | gg     | Move the cursor to the first line of the document.                 |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| documentEnd          | string | G      |           | G      | Move the cursor to the last line of the document.                  |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| halfPageUp           | string | ctrl+u |           | ctrl+u | Scroll the cursor up half a page.                                  |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| halfPageDown         | string | ctrl+d |           | ctrl+d | Scroll the cursor down half a page.                                |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterInsert          | string | i      |           |        | Enter insert mode before the cursor.                               |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterInsertAfter     | string | a      |           |        | Enter insert mode after the cursor.                                |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterInsertLineStart | string | I      |           |        | Enter insert mode at the first column of the line.                 |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterInsertLineEnd   | string | A      |           |        | Enter insert mode at the last column of the line.                  |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| openLineBelow        | string | o      |           |        | Open a new line below and enter insert mode.                       |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| openLineAbove        | string | O      |           |        | Open a new line above and enter insert mode.                       |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| leaveMode            | string | escape | escape    | escape | Leave the current mode and return to normal mode.                  |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterVisual          | string | v      |           | v      | Enter visual mode with character selection.                        |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterVisualLine      | string | V      |           | V      | Enter visual mode with line selection.                             |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| enterVisualBlock     | string | ctrl+v |           | ctrl+v | Enter visual mode with block selection.                            |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| yank                 | string | y      |           | y      | Yank the selected text.                                            |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| deleteOperator       | string | d      |           | d      | Delete the selected text or a motion span.                         |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| changeOperator       | string | c      |           | c      | Delete the selected text or a motion span, then enter insert mode. |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| deleteChar           | string | x      |           | x      | Delete the character under the cursor.                             |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| pasteAfter           | string | p      |           | p      | Paste the yanked text after the cursor.                            |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| pasteBefore          | string | P      |           | P      | Paste the yanked text before the cursor.                           |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| undo                 | string | u      |           |        | Undo the last change.                                              |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| redo                 | string | ctrl+r |           |        | Redo the last undone change.                                       |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| splitLine            | string |        | return    |        | Split the line at the cursor.                                      |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
| deleteBackward       | string |        | backspace |        | Delete the character before the cursor.                            |
+----------------------+--------+--------+-----------+--------+--------------------------------------------------------------------+
