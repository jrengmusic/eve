# END Metadata

## project

```
@brief Projucer-equivalent project fields — config-time truth for the build toolchain.

Read by CAST at config time; the CMakeLists writer derives the JUCE project
declaration from these rows. No downstream file restates a value.
```

+-------------+--------------------+-------------------------------+
| key         | value              | comment                       |
+=============+====================+===============================+
| description | END Audio Plugin   | Product description.          |
| formats     | Standalone VST3 AU | juce_add_plugin FORMATS list. |
+-------------+--------------------+-------------------------------+

## project info

```
@brief Project metadata — the ProjectInfo namespace, generated.

Every field is a complete literal; nothing downstream derives, concatenates, or restates a value.
```

+-------------------+------------------+--------+--------------------------+-----------+--------------------------------------------------+
| type              | name             | format | value                    | format    | comment                                          |
+===================+==================+========+==========================+===========+==================================================+
| const char* const | projectName      |        | END                      | toLiteral | Product name.                                    |
| const char* const | companyName      |        | JRENG                    | toLiteral | Company name.                                    |
| const char* const | legalCompanyName |        | PT JRENG Teknika         | toLiteral | Full legal company name.                         |
| const char* const | versionString    |        | 0.1.0                    | toLiteral | Product version string.                          |
| int               | versionNumber    |        | 0x100                    |           | Product version, JUCE hex encoding.              |
| const char* const | productWebsite   |        | `https://jrengmusic.com` |           | Product website URL.                             |
| const char* const | presetExtension  |        | endp                     | toLiteral | Preset file extension, without the leading dot.  |
| const char* const | presetDefault    |        | INIT                     | toLiteral | Default init preset name, without the extension. |
+-------------------+------------------+--------+--------------------------+-----------+--------------------------------------------------+
