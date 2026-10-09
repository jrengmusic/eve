---
version: 0.0.5
---

# EVE

This file is the virgin configuration of EVE. It carries the display settings,
the key bindings, the user configuration and the visual appearance. It is seeded to
~/.config/end/eve.md on first launch and read at editor start. The disk copy
wins.
Colour format: 0xAARRGGBB hex integer (Alpha|Red|Green|Blue). The style
section references colours by their colour name. The `value` column is the
light appearance; the `dark` column is the dark appearance.

## display

Editor scaling, initial window size and zoom step.

+----------+---------+----------+----------------+------------------------------------------------+
| key      | type    | value    | choices        | description                                    |
+==========+=========+==========+================+================================================+
| scaling  | string  | ZOOM     | UI_SCALE, ZOOM | UI_SCALE resizes the window; ZOOM scales text. |
+----------+---------+----------+----------------+------------------------------------------------+
| size     | numbers | 600, 400 |                | Initial window size in pixels {width, height}. |
+----------+---------+----------+----------------+------------------------------------------------+
| zoomStep | float   | 0.1      |                | Zoom step of the zoomIn and zoomOut keys.      |
+----------+---------+----------+----------------+------------------------------------------------+

## keys

Key bindings. Format: "modifier+key", for example "cmd+=". Modifiers: cmd,
ctrl, alt, shift.

+-----------+--------+-------+---------+--------------------------------+
| key       | type   | value | choices | description                    |
+===========+========+=======+=========+================================+
| zoomIn    | string | cmd+= |         | Increase the zoom by zoomStep. |
+-----------+--------+-------+---------+--------------------------------+
| zoomOut   | string | cmd+- |         | Decrease the zoom by zoomStep. |
+-----------+--------+-------+---------+--------------------------------+
| zoomReset | string | cmd+0 |         | Reset the zoom to its default. |
+-----------+--------+-------+---------+--------------------------------+

## settings

Plugin settings. The chain populates the persisted-parameter subtree from
this table on first launch; the host state overrides it after restore.

+------------+--------+---------+------------------------------------------------------------------+
| key        | type   | value   | description                                                      |
+============+========+=========+==================================================================+
| UI_SCALE   | string | MEDIUM  | Editor scale step.                                               |
+------------+--------+---------+------------------------------------------------------------------+
| APPEARANCE | string | AUTO    | Light/dark appearance; AUTO follows the OS.                      |
+------------+--------+---------+------------------------------------------------------------------+

## colours

Named colours. Every style entry below resolves its colour through this
table by name. Names and values follow the product colour scheme.

+-----------------+--------+------------+-----------------------+
| key             | type   | value      | description           |
+=================+========+============+=======================+
| --blank         | colour | 0x00000000 | Fully transparent.    |
+-----------------+--------+------------+-----------------------+
| --bunker        | colour | 0xff090d12 | Background.           |
+-----------------+--------+------------+-----------------------+
| --corbeau       | colour | 0xff0d141c | Highlight background. |
+-----------------+--------+------------+-----------------------+
| --paradiso      | colour | 0xff4e8c93 | Normal text.          |
+-----------------+--------+------------+-----------------------+
| --caribbeanBlue | colour | 0xff01c2d2 | Bright text.          |
+-----------------+--------+------------+-----------------------+
| --mediterranea  | colour | 0xff33535b | Dim text.             |
+-----------------+--------+------------+-----------------------+
| --carbon        | colour | 0xff333435 | Scrollbar thumb.      |
+-----------------+--------+------------+-----------------------+

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
`value` (light) and `dark` cells name a colour above.

+---------------------------------------+--------+------------+------------+------------------------+
| key                                   | type   | value      | dark       | description            |
+=======================================+========+============+============+========================+
| --ResizableWindow--backgroundColourId | string | --bunker   | --bunker   | Window background.     |
+---------------------------------------+--------+------------+------------+------------------------+
| --TextEditor--textColourId            | string | --paradiso | --paradiso | Default terminal text. |
+---------------------------------------+--------+------------+------------+------------------------+
| --ScrollBar--thumbColourId            | string | --carbon   | --carbon   | Scrollbar thumb.       |
+---------------------------------------+--------+------------+------------+------------------------+
| --ScrollBar--trackColourId            | string | --blank    | --blank    | Scrollbar track.       |
+---------------------------------------+--------+------------+------------+------------------------+
