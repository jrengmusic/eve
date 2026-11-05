---
version: 0.0.1
---

# EVE

This file is the virgin configuration of EVE. It carries the display settings, the key bindings, the
user configuration and the visual appearance. It is the binary default of ~/.config/end/eve.md, and
it is written there when that file is missing or older. A valid disk copy overlays it row by row.
Keys are lower-case words that the reader formats to the reserved identifiers.

## display

Editor scaling, initial window size and zoom step.

+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| key                   | type    | value            | choices        | description                                                                            |
+=======================+=========+==================+================+========================================================================================+
| ligatures             | bool    | true             |                | Draw the ligature glyphs of the mono font.                                             |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| preview width         | float   | 0.4              |                | Width of the preview panel, as a fraction of the window width at the first LIST entry. |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| scaling               | string  | ZOOM             | UI_SCALE, ZOOM | UI_SCALE resizes the window; ZOOM scales the view by `zoom`.                           |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| scroll duration       | numbers | 100, 200         |                | Wheel scroll animation duration range {short, long}, in milliseconds.                  |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| scroll duration range | numbers | 120, 480         |                | Scroll distance at which the duration is long and short, in pixels.                    |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| scroll easing         | numbers | 0.42, 0, 0.58, 1 |                | Cubic Bézier control points of the wheel scroll easing.                                |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| scroll step           | float   | 40               |                | Scroll distance per wheel notch, in pixels.                                            |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| size                  | numbers | 800, 600         |                | Initial window size in pixels {width, height}.                                         |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| tree depth            | int     | 2                |                | Folder levels above the terminal folder in the tree of LIST mode.                      |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| tree width            | float   | 0.2              |                | Width of the tree panel, as a fraction of the window width at the first LIST entry.    |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+
| zoom step             | float   | 0.1              |                | Zoom step of the zoom in and zoom out keys.                                            |
+-----------------------+---------+------------------+----------------+----------------------------------------------------------------------------------------+

## keys

Key bindings. Format: "modifier+key", for example "cmd+=". Modifiers: cmd, ctrl, alt, shift. A row
here replaces the default row of its command name. An empty mode applies in every mode. A value of
one or two characters is matched by text character and is case-sensitive; a two-character value is
a sequence (gg). A key description such as ctrl+u or escape names a non-printing key. The prefix
key (\`) enters the prefix layer, and ] there enters normal mode; press \` twice to send it to the
shell. sequence timeout (ms) ends a sequence or a layer. Under END, END owns the prefix layer and
normal mode. In terminal mode every other key goes to the shell.

+------------------+--------+--------------+-------+-----------------------------------+
| key              | type   | value        | mode  | description                       |
+==================+========+==============+=======+===================================+
| clear            | string |              |       | Clear the screen and the history. |
+------------------+--------+--------------+-------+-----------------------------------+
| dismiss          | string | escape q     | panel | Close the panel.                  |
+------------------+--------+--------------+-------+-----------------------------------+
| open file        | string | return       | list  | Open the highlighted file.        |
+------------------+--------+--------------+-------+-----------------------------------+
| sequence timeout | int    | 1000         |       | Time in ms to wait for the next   |
|                  |        |              |       | key of a sequence or a layer.     |
+------------------+--------+--------------+-------+-----------------------------------+

## open with

What opens a file, by extension. The directory row gives the value of a directory. A row with no
extension in its key matches a file name, such as makefile. A value that names a command runs that
command. Any other value is a shell command, with the file as its last argument. An empty value
opens the file with the system default application, and so does any shell command while a program
runs in the terminal. Add any row.

+---------------+--------+----------+---------------------------------------+
| key           | type   | value    | description                           |
+===============+========+==========+=======================================+
| .c            | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .cpp          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .h            | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .hpp          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .cc           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .hh           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .rs           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .go           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .py           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .js           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .ts           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .jsx          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .tsx          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .java         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .kt           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .swift        | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .rb           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .lua          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .sh           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .bash         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .zsh          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .fish         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .pl           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .zig          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .asm          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .s            | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .cs           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .fs           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .ex           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .exs          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .clj          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .scala        | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .r            | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .m            | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .mm           | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .md           | string | markdown | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .txt          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .json         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .yaml         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .yml          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .toml         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .xml          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .html         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .css          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .csv          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .sql          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .graphql      | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .proto        | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .cmake        | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .gitignore    | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .editorconfig | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .env          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .ini          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .cfg          | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| .conf         | string | nvim     | Opens a file with this extension.     |
+---------------+--------+----------+---------------------------------------+
| makefile      | string | nvim     | Opens a file with this name.          |
+---------------+--------+----------+---------------------------------------+
| dockerfile    | string | nvim     | Opens a file with this name.          |
+---------------+--------+----------+---------------------------------------+
| directory     | string | cd       | Opens a directory.                    |
+---------------+--------+----------+---------------------------------------+
| any           | string |          | Opens a file that no other row names. |
+---------------+--------+----------+---------------------------------------+

## settings

Plugin settings. The chain populates the persisted-parameter subtree from this table on first
launch; the host state overrides it after restore.

+-------------------+--------+----------+-------------------------------------------------------------+
| key               | type   | value    | description                                                 |
+===================+========+==========+=============================================================+
| ui scale          | string | MEDIUM   | Editor scale step.                                          |
+-------------------+--------+----------+-------------------------------------------------------------+
| appearance        | string | AUTO     | Light/dark appearance; AUTO follows the OS.                 |
+-------------------+--------+----------+-------------------------------------------------------------+
| rasterizer        | string | freetype | Glyph rasterization backend: edgeTable, freetype or native. |
+-------------------+--------+----------+-------------------------------------------------------------+
| gamma             | float  | 2.2      | Coverage gamma exponent of rasterized glyphs.               |
+-------------------+--------+----------+-------------------------------------------------------------+
| contrast          | float  | 0.0      | Coverage contrast of rasterized glyphs.                     |
+-------------------+--------+----------+-------------------------------------------------------------+
| embolden          | string | false    | Synthetic embolden of rasterized glyphs: true or false.     |
+-------------------+--------+----------+-------------------------------------------------------------+
| history capacity  | int    | 10000    | Lines of terminal history that the record keeps.            |
+-------------------+--------+----------+-------------------------------------------------------------+
| shell integration | bool   | true     | Load the EVE shell hooks: OSC 7 and OSC 133 A, C and D.     |
+-------------------+--------+----------+-------------------------------------------------------------+
| start directory   | string | ~        | Folder where the shell starts. ~ is the home folder.        |
+-------------------+--------+----------+-------------------------------------------------------------+

## window

Window chrome configuration. Keys are lower-case words that the reader formats to the reserved
identifiers.

+---------+--------+----------------+-------------------------------------------------------+
| key     | type   | value          | description                                           |
+=========+========+================+=======================================================+
| mac     | string | backgroundBlur | macOS window visual effect style.                     |
+---------+--------+----------------+-------------------------------------------------------+
| win     | string | acrylic10      | Windows window visual effect style.                   |
+---------+--------+----------------+-------------------------------------------------------+
| opacity | float  | 0.75           | Window background opacity (0.0 - 1.0).                |
+---------+--------+----------------+-------------------------------------------------------+
| blur    | int    | 20             | Background blur radius in pixels.                     |
+---------+--------+----------------+-------------------------------------------------------+
| padding | int    | 8              | Padding in pixels reserved around the window content. |
+---------+--------+----------------+-------------------------------------------------------+
