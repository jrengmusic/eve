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

+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| type    | name                        | format  | value               | format    | description                                |
+=========+=============================+=========+=====================+===========+============================================+
| @string | view layout                 | toCamel | ViewLayout.md       | toLiteral | Virgin settings, seeded to eve.md.         |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | settings layout             | toCamel | SettingsLayout.html | toLiteral | Settings dialog layout.                    |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | parameters layout           | toCamel | parameters.md       | toLiteral | Parameter descriptor tables.               |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | markdown settings directory | toCamel | .config/end         | toLiteral | User settings directory, relative to home. |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | markdown settings file name | toCamel | eve.md              | toLiteral | User settings file name on disk.           |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | up normal                   | toCamel | up_normal.svg       | toLiteral | Up-button normal icon.                     |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | up over                     | toCamel | up_over.svg         | toLiteral | Up-button over icon.                       |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | up down                     | toCamel | up_down.svg         | toLiteral | Up-button down icon.                       |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | down normal                 | toCamel | down_normal.svg     | toLiteral | Down-button normal icon.                   |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | down over                   | toCamel | down_over.svg       | toLiteral | Down-button over icon.                     |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
| @string | down down                   | toCamel | down_down.svg       | toLiteral | Down-button down icon.                     |
+---------+-----------------------------+---------+---------------------+-----------+--------------------------------------------+
