## index

+---------+------------------+
| alias   | symbol           |
+=========+==================+
| @id     | juce::Identifier |
+---------+------------------+
| @string | juce::String     |
+---------+------------------+

## identifiers

```
@brief Product identifier and name-string vocabulary — branding, buttons, view component types, and images — not already carried by jam.
```

+------+------------------+--------------------+-------------+
| type | name             | value              | description |
+======+==================+====================+=============+
| @id  | args             | `args`             |             |
+------+------------------+--------------------+-------------+
| @id  | button           | `button`           |             |
+------+------------------+--------------------+-------------+
| @id  | colorMode        | `colorMode`        |             |
+------+------------------+--------------------+-------------+
| @id  | commandCount     | `commandCount`     |             |
+------+------------------+--------------------+-------------+
| @id  | deviceClock      | `deviceClock`      |             |
+------+------------------+--------------------+-------------+
| @id  | historyCapacity  | `historyCapacity`  |             |
+------+------------------+--------------------+-------------+
| @id  | previewWidth     | `previewWidth`     |             |
+------+------------------+--------------------+-------------+
| @id  | profiler         | `profiler`         |             |
+------+------------------+--------------------+-------------+
| @id  | selector         | `selector`         |             |
+------+------------------+--------------------+-------------+
| @id  | shell            | `shell`            |             |
+------+------------------+--------------------+-------------+
| @id  | shellIntegration | `shellIntegration` |             |
+------+------------------+--------------------+-------------+
| @id  | startDirectory   | `startDirectory`   |             |
+------+------------------+--------------------+-------------+
| @id  | terminalInput    | `terminalInput`    |             |
+------+------------------+--------------------+-------------+
| @id  | textEditor       | `textEditor`       |             |
+------+------------------+--------------------+-------------+
| @id  | winsize          | `winsize`          |             |
+------+------------------+--------------------+-------------+

## CommandId

```
@brief EVE command-id to command-name map.

Maps each command-id ordinal to the command name a config `keys` row uses.
The key is the juce::CommandID; the value is the row key.
```

+-----------------+-----------+-------------------+
| name            | key       | value             |
+=================+===========+===================+
| zoomIn          | 0x4600001 | `zoomIn`          |
| zoomOut         | 0x4600002 | `zoomOut`         |
| zoomReset       | 0x4600003 | `zoomReset`       |
| settings        | 0x4600004 | `settings`        |
| left            | 0x4600005 | `left`            |
| down            | 0x4600006 | `down`            |
| up              | 0x4600007 | `up`              |
| right           | 0x4600008 | `right`           |
| wordForward     | 0x4600009 | `wordForward`     |
| wordBackward    | 0x460000a | `wordBackward`    |
| wordEnd         | 0x460000b | `wordEnd`         |
| lineStart       | 0x460000c | `lineStart`       |
| lineEnd         | 0x460000d | `lineEnd`         |
| documentStart   | 0x460000e | `documentStart`   |
| documentEnd     | 0x460000f | `documentEnd`     |
| halfPageUp      | 0x4600010 | `halfPageUp`      |
| halfPageDown    | 0x4600011 | `halfPageDown`    |
| clear           | 0x4600012 | `clear`           |
| normal          | 0x4600018 | `normal`          |
| visualCharacter | 0x4600019 | `visualCharacter` |
| visualLine      | 0x460001a | `visualLine`      |
| visualBlock     | 0x460001b | `visualBlock`     |
| yank            | 0x460001c | `yank`            |
| terminal        | 0x4600026 | `terminal`        |
| preview         | 0x4600027 | `preview`         |
+-----------------+-----------+-------------------+
