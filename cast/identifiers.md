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

## CommandId

```
@brief EVE command-id to command-name map.

Maps each command-id ordinal to the command name a config `keys` row uses.
The key is the juce::CommandID; the value is the row key.
```

+-----------------+-----------+-------------------+
| name            | key       | value             |
+=================+===========+===================+
| clear           | 0x4600012 | `clear`           |
| preview         | 0x4600027 | `preview`         |
+-----------------+-----------+-------------------+
