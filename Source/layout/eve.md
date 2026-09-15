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
