## index

+--------------------+-------------------------------------+----------+
| alias              | symbol                              | format   |
+====================+=====================================+==========+
| @code              | ../../jam/cast/code.cast            |          |
| @cmake             | cmake.cast                          |          |
| @project-info      | ../project-info.md                  |          |
| @signing           | signing.md                          |          |
| @ProjectInfo       | ../Source/generated/ProjectInfo.h   |          |
| @CMakeLists        | ../CMakeLists.txt                   |          |
| @Entitlements      | ../entitlements.plist               |          |
| @space             | U+0020                              | fromUTF8 |
| @semicolon         | ;                                   |          |
| @files             | files.md                            |          |
| @Files             | ../Source/generated/Files.h         |          |
| @Generated         | ../Source/generated/Generated.h     |          |
| @source            | source.cast                         |          |
| @identifiers       | identifiers.md                      |          |
| @Identifiers       | ../Source/generated/Identifiers.h   |          |
| @IdentifiersSource | ../Source/generated/Identifiers.cpp |          |
| @FilesSource       | ../Source/generated/Files.cpp       |          |
+--------------------+-------------------------------------+----------+

## headers

+----------------+--------+------------------------------------------------------------------------------------------+-------------+
| file           | type   | brief                                                                                    | description |
+================+========+==========================================================================================+=============+
| ProjectInfo.h  | header | ```                                                                                      |             |
|                |        | @file ProjectInfo.h                                                                      |             |
|                |        | @brief Project metadata — the generated ProjectInfo namespace.                           |             |
|                |        | ```                                                                                      |             |
+----------------+--------+------------------------------------------------------------------------------------------+-------------+
| Identifiers.h  | header | ```                                                                                      |             |
|                |        | @file Identifiers.h                                                                      |             |
|                |        | @brief Identifier and name-string constants for EVE's own vocabulary.                    |             |
|                |        | ```                                                                                      |             |
+----------------+--------+------------------------------------------------------------------------------------------+-------------+
| Files.h        | header | ```                                                                                      |             |
|                |        | @file Files.h                                                                            |             |
|                |        | @brief Product asset file names.                                                         |             |
|                |        | ```                                                                                      |             |
+----------------+--------+------------------------------------------------------------------------------------------+-------------+
| Generated.h    | header | ```                                                                                      |             |
|                |        | @file Generated.h                                                                        |             |
|                |        | @brief Generated-header umbrella — includes every generated product header.              |             |
|                |        | ```                                                                                      |             |
+----------------+--------+------------------------------------------------------------------------------------------+-------------+
| CMakeLists.txt |        | ```                                                                                      |             |
|                |        | @file CMakeLists.txt                                                                     |             |
|                |        | @brief EVE build manifest — a self-sufficient JUCE plugin project.                       |             |
|                |        |                                                                                          |             |
|                |        | Generated from project-info.md and cast/signing.md; every value traces to one table row. |             |
|                |        | Edit the table, run cast, then configure.                                                |             |
|                |        | ```                                                                                      |             |
+----------------+--------+------------------------------------------------------------------------------------------+-------------+

## output

+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| list                                          | separator                 | structure                                     | file               |
+===============================================+===========================+===============================================+====================+
| > - [list]: @project-info:project info        |                           | @code:namespace                               | @ProjectInfo       |
|                                               |                           | - macro: #pragma once                         |                    |
|                                               |                           | - name: ProjectInfo                           |                    |
|                                               |                           | - [description]: @headers:brief               |                    |
|                                               |                           | > - [list]: @code:constant                    |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @files:files                      | - [list]: @code:linebreak | @code:line                                    | @Files             |
|                                               |                           | - macro: #pragma once                         |                    |
|                                               |                           |                                               |                    |
|                                               |                           | @code:namespace                               |                    |
|                                               |                           | - name: files                                 |                    |
|                                               |                           | > - [list]: @code:identifier                  |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @files:files                      | - [list]: @code:linebreak | @source:source                                | @FilesSource       |
|                                               |                           | - file: Files.h                               |                    |
|                                               |                           |                                               |                    |
|                                               |                           | @code:namespace                               |                    |
|                                               |                           | - name: files                                 |                    |
|                                               |                           | > - [list]: @code:identifier-definition       |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @identifiers                        |                           | @code:namespace                               | @Identifiers       |
|                                               |                           | - macro: #pragma once                         |                    |
|                                               |                           | - name: Id                                    |                    |
|                                               |                           | - [description]: @headers:brief               |                    |
|                                               |                           | - [list]: @code:identifier                    |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @identifiers                      |                           | @source:source                                | @IdentifiersSource |
|                                               |                           | - file: Identifiers.h                         |                    |
|                                               |                           |                                               |                    |
|                                               |                           | @code:namespace                               |                    |
|                                               |                           | - name: Id                                    |                    |
|                                               |                           | > - [list]: @code:identifier-definition       |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @headers:type=header                |                           | @code:line                                    | @Generated         |
|                                               |                           | - macro: #pragma once                         |                    |
|                                               |                           |                                               |                    |
|                                               |                           | - [list]: @code:include                       |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @project-info:project info          | - [list]:                 | @cmake:cmake                                  | @CMakeLists        |
|                                               |                           | - [description]: @headers:brief               |                    |
| - [list]: @project-info:cmake                 |                           |                                               |                    |
| - [list]: @signing:signing                    |                           |                                               |                    |
| - [list]: @project-info:architecture          | - [list]: @semicolon      | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:format                | - [list]: @space          | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:release:stage=        | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:release:stage=linker  | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:debug:stage=          | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:release:stage=        | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:release:stage=linker  | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:debug:stage=          | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:patch                 |                           | - [list]: @cmake:patch                        |                    |
| - [list]: @project-info:user module           |                           | - [list]: @cmake:module                       |                    |
| > - [list]: @project-info:source              |                           | > - [list]: @cmake:entry                      |                    |
| > - [list]: @project-info:define              |                           | > - [list]: @cmake:entry                      |                    |
| > - [list]: @project-info:include             |                           | > - [list]: @cmake:entry                      |                    |
| > > - [list]: @project-info:juce module       |                           | > > - [list]: @cmake:value                    |                    |
| > > - [list]: @project-info:user module       |                           | > > - [list]: @cmake:link                     |                    |
| > - [list]: @project-info:layout glob         |                           | > - [list]: @cmake:entry                      |                    |
| - [list]: @project-info:format                |                           | - [list]: @cmake:format-install-directory     |                    |
|                                               |                           | - xattr: @cmake:xattr                         |                    |
|                                               |                           | - codesign: @cmake:codesign                   |                    |
|                                               |                           | - verify: @cmake:verify                       |                    |
|                                               |                           | - zip: @cmake:zip                             |                    |
|                                               |                           | - notarize: @cmake:notarize                   |                    |
|                                               |                           | - staple: @cmake:staple                       |                    |
|                                               |                           | - wraptool: @cmake:wraptool                   |                    |
|                                               |                           | - qa-directory: @cmake:qa-directory           |                    |
|                                               |                           | - qa-copy: @cmake:qa-copy                     |                    |
|                                               |                           | - install-directory: @cmake:install-directory |                    |
|                                               |                           | - install-copy: @cmake:install-copy           |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @signing:signing:type=entitlement |                           | @code:[xml]entitlements                       | @Entitlements      |
|                                               |                           | > - [list]: @code:entitlement                 |                    |
+-----------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
