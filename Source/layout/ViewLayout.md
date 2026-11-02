---
version: 0.0.2
---

# EVE

This file is the virgin configuration of EVE. It carries the display settings, the key bindings, the
user configuration and the visual appearance. It is seeded to ~/.config/end/eve.md on first launch
and read at editor start. The disk copy wins.

## display

Editor scaling, initial window size and zoom step.

+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| key                 | type    | value            | choices        | description                                                           |
+=====================+=========+==================+================+=======================================================================+
| previewWidth        | float   | 0.4              |                | Width of the preview column, as a fraction of the window width.       |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| scaling             | string  | ZOOM             | UI_SCALE, ZOOM | UI_SCALE resizes the window; ZOOM scales text.                        |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| scrollDuration      | numbers | 100, 200         |                | Wheel scroll animation duration range {short, long}, in milliseconds. |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| scrollDurationRange | numbers | 120, 480         |                | Scroll distance at which the duration is long and short, in pixels.   |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| scrollEasing        | numbers | 0.42, 0, 0.58, 1 |                | Cubic Bézier control points of the wheel scroll easing.               |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| scrollStep          | float   | 40               |                | Scroll distance per wheel notch, in pixels.                           |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| size                | numbers | 800, 600         |                | Initial window size in pixels {width, height}.                        |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+
| zoomStep            | float   | 0.1              |                | Zoom step of the zoomIn and zoomOut keys.                             |
+---------------------+---------+------------------+----------------+-----------------------------------------------------------------------+

## keys

Key bindings. Format: "modifier+key", for example "cmd+=". Modifiers: cmd, ctrl, alt, shift. An
empty mode applies in every mode. A value of one or two characters is a text chord. It is matched by
text character and is case-sensitive. A two-character value is a prefix sequence. A key description
such as ctrl+u or escape names a non-printing key. In terminal mode every other key goes to the
shell.

+-----------------+--------+--------------+----------+-----------------------------------+
| key             | type   | value        | mode     | description                       |
+=================+========+==============+==========+===================================+
| clear           | string |              |          | Clear the screen and the history. |
+-----------------+--------+--------------+----------+-----------------------------------+
| preview         | string | ctrl+shift+p |          | Show or hide the preview column.  |
+-----------------+--------+--------------+----------+-----------------------------------+

## settings

Plugin settings. The chain populates the persisted-parameter subtree from this table on first
launch; the host state overrides it after restore.

+-------------------+--------+----------+-------------------------------------------------------------+
| key               | type   | value    | description                                                 |
+===================+========+==========+=============================================================+
| UI_SCALE          | string | MEDIUM   | Editor scale step.                                          |
+-------------------+--------+----------+-------------------------------------------------------------+
| APPEARANCE        | string | AUTO     | Light/dark appearance; AUTO follows the OS.                 |
+-------------------+--------+----------+-------------------------------------------------------------+
| RASTERIZER        | string | freetype | Glyph rasterization backend: edgeTable, freetype or native. |
+-------------------+--------+----------+-------------------------------------------------------------+
| GAMMA             | float  | 2.2      | Coverage gamma exponent of rasterized glyphs.               |
+-------------------+--------+----------+-------------------------------------------------------------+
| CONTRAST          | float  | 0.0      | Coverage contrast of rasterized glyphs.                     |
+-------------------+--------+----------+-------------------------------------------------------------+
| EMBOLDEN          | string | false    | Synthetic embolden of rasterized glyphs: true or false.     |
+-------------------+--------+----------+-------------------------------------------------------------+
| HISTORY_CAPACITY  | int    | 10000    | Lines of terminal history that the record keeps.            |
+-------------------+--------+----------+-------------------------------------------------------------+
| SHELL_INTEGRATION | bool   | true     | Load the EVE shell hooks: OSC 7 and OSC 133 A, C and D.     |
+-------------------+--------+----------+-------------------------------------------------------------+
| START_DIRECTORY   | string | ~        | Folder where the shell starts. ~ is the home folder.        |
+-------------------+--------+----------+-------------------------------------------------------------+

## window

Window chrome configuration. Entries are read by the runtime by id — names are the contract, used
verbatim.

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
