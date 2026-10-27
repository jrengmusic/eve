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

+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| type    | name                          | format  | value                      | format    | description                                         |
+=========+===============================+=========+============================+===========+=====================================================+
| @string | view layout                   | toCamel | ViewLayout.md              | toLiteral | Virgin settings, seeded to eve.md.                  |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | settings layout               | toCamel | SettingsLayout.html        | toLiteral | Settings dialog layout.                             |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | parameters layout             | toCamel | parameters.md              | toLiteral | Parameter descriptor tables.                        |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | model layout                  | toCamel | model.md                   | toLiteral | Terminal state machine table, the Model lane.       |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | markdown settings directory   | toCamel | .config/end                | toLiteral | User settings directory, relative to home.          |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | markdown settings file name   | toCamel | eve.md                     | toLiteral | User settings file name on disk.                    |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | standalone settings file name | toCamel | EVE.settings               | toLiteral | Standalone plugin state file name on disk.          |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | up normal                     | toCamel | up_normal.svg              | toLiteral | Up-button normal icon.                              |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | up over                       | toCamel | up_over.svg                | toLiteral | Up-button over icon.                                |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | up down                       | toCamel | up_down.svg                | toLiteral | Up-button down icon.                                |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | down normal                   | toCamel | down_normal.svg            | toLiteral | Down-button normal icon.                            |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | down over                     | toCamel | down_over.svg              | toLiteral | Down-button over icon.                              |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | down down                     | toCamel | down_down.svg              | toLiteral | Down-button down icon.                              |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | shell integration directory   | toCamel | shell                      | toLiteral | Shell hook folder, relative to the settings folder. |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | zsh environment               | toCamel | zsh_zshenv.zsh             | toLiteral | zsh .zshenv that loads the EVE hooks.               |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | zsh integration               | toCamel | zsh_integration.zsh        | toLiteral | zsh EVE hooks.                                      |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | bash integration              | toCamel | bash_integration.bash      | toLiteral | bash EVE hooks.                                     |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
| @string | powershell integration        | toCamel | powershell_integration.ps1 | toLiteral | PowerShell EVE hooks.                               |
+---------+-------------------------------+---------+----------------------------+-----------+-----------------------------------------------------+
