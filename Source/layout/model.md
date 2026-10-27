This file defines EVE's terminal state machine, read by jam::ModelLayout into the jam::Model lane. These values are not host parameters.

## model

+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| id                | type   | default | description                                                                                    |
+===================+========+=========+================================================================================================+
| applicationCursor | bool   | 0       | DECCKM state that the terminal processor reports                                               |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| bracketedPaste    | bool   | 0       | Bracketed paste state that the terminal processor reports                                      |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| mouseTracking     | choice | 0       | Mouse tracking level that the terminal processor reports: a key of map::MouseTracking          |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| mouseSgr          | bool   | 0       | SGR mouse encoding state that the terminal processor reports                                   |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| cursorVisible     | bool   | 1       | Cursor visibility that the terminal processor reports                                          |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| alternateScreen   | bool   | 0       | Alternate screen state that the terminal processor reports                                     |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| isRecording       | bool   | 0       | Command output state between the OSC 133 C and D marks                                         |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
| cursor            | int    | 0       | Live-area cursor, jam::Cell::Point::pack(): column in the high 16 bits, row in the low 16 bits |
+-------------------+--------+---------+------------------------------------------------------------------------------------------------+
