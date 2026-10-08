## index

+---------+------------------+
| alias   | symbol           |
+=========+==================+
| @id     | juce::Identifier |
+---------+------------------+
| @string | juce::String     |
+---------+------------------+

## files

```
@brief Product layout file names.

Each constant is the literal file name of an embedded layout resource,
resolved against the binary-data / asset search path at load time.
```

+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| type    | name              | format  | value               | format    | description                               |
+=========+===================+=========+=====================+===========+===========================================+
| @string | view layout       | toCamel | ViewLayout.md       | toLiteral | Editor geometry and UI size.              |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | panel layout      | toCamel | PanelLayout.html    | toLiteral | Top panel: settings and about buttons.    |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | settings layout   | toCamel | SettingsLayout.html | toLiteral | Settings dialog layout.                   |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | about layout      | toCamel | AboutLayout.html    | toLiteral | About dialog layout.                      |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | parameters layout | toCamel | parameters.md       | toLiteral | Parameter descriptor tables.              |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | config directory  | toCamel | .config/end         | toLiteral | User config directory, relative to home.  |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | default config    | toCamel | eve.md              | toLiteral | User config document seeded when missing. |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | settings normal   | toCamel | settings_normal.svg | toLiteral | Settings-button normal icon.              |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | settings over     | toCamel | settings_over.svg   | toLiteral | Settings-button over icon.                |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | settings down     | toCamel | settings_down.svg   | toLiteral | Settings-button down icon.                |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | up normal         | toCamel | up_normal.svg       | toLiteral | Up-button normal icon.                    |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | up over           | toCamel | up_over.svg         | toLiteral | Up-button over icon.                      |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | up down           | toCamel | up_down.svg         | toLiteral | Up-button down icon.                      |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | down normal       | toCamel | down_normal.svg     | toLiteral | Down-button normal icon.                  |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | down over         | toCamel | down_over.svg       | toLiteral | Down-button over icon.                    |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
| @string | down down         | toCamel | down_down.svg       | toLiteral | Down-button down icon.                    |
+---------+-------------------+---------+---------------------+-----------+-------------------------------------------+
