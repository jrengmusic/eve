## index

+---------------+-----------------------------------+----------+
| alias         | symbol                            | format   |
+===============+===================================+==========+
| @code         | ../../jam/cast/code.cast          |          |
| @cmake        | cmake.cast                        |          |
| @project-info | ../project-info.md                |          |
| @ProjectInfo  | ../Source/generated/ProjectInfo.h |          |
| @CMakeLists   | ../CMakeLists.txt                 |          |
| @space        | U+0020                            | fromUTF8 |
| @semicolon    | ;                                 |          |
+---------------+-----------------------------------+----------+

## headers

+----------------+----------------------------------------------------------------------+---------+
| file           | brief                                                                | comment |
+================+======================================================================+=========+
| ProjectInfo.h  | ```                                                                  |         |
|                | @file ProjectInfo.h                                                  |         |
|                | @brief Project metadata — the generated ProjectInfo namespace.       |         |
|                | ```                                                                  |         |
+----------------+----------------------------------------------------------------------+---------+
| CMakeLists.txt | ```                                                                  |         |
|                | @file CMakeLists.txt                                                 |         |
|                | @brief EVE build manifest — a self-sufficient JUCE plugin project.   |         |
|                |                                                                      |         |
|                | Generated from project-info.md; every value traces to one table row. |         |
|                | Edit the table, run cast, then configure.                            |         |
|                | ```                                                                  |         |
+----------------+----------------------------------------------------------------------+---------+

## output

+--------------------------------------------+--------------------+-----------------------------------------------+--------------+
| list                                       | separator          | structure                                     | file         |
+============================================+====================+===============================================+==============+
| > - list: @project-info:project info       |                    | @code:namespace                               | @ProjectInfo |
|                                            |                    | - macro: #pragma once                         |              |
|                                            |                    | - name: ProjectInfo                           |              |
|                                            |                    | - comment: @headers:brief                     |              |
|                                            |                    | > - list: @code:constant                      |              |
+--------------------------------------------+--------------------+-----------------------------------------------+--------------+
| - list: @project-info:project info         | - list:            | @cmake:cmake                                  | @CMakeLists  |
|                                            |                    | - comment: @headers:brief                     |              |
| - list: @project-info:cmake                |                    |                                               |              |
| - list: @project-info:signing              |                    |                                               |              |
| - list: @project-info:architecture         | - list: @semicolon | - list: @cmake:value                          |              |
| - list: @project-info:format               | - list: @space     | - list: @cmake:value                          |              |
| - list: @project-info:release:stage=       | - list: @semicolon | - list: @cmake:mac                            |              |
| - list: @project-info:release:stage=linker | - list: @semicolon | - list: @cmake:mac                            |              |
| - list: @project-info:debug:stage=         | - list: @semicolon | - list: @cmake:mac                            |              |
| - list: @project-info:release:stage=       | - list: @semicolon | - list: @cmake:win                            |              |
| - list: @project-info:release:stage=linker | - list: @semicolon | - list: @cmake:win                            |              |
| - list: @project-info:debug:stage=         | - list: @semicolon | - list: @cmake:win                            |              |
| - list: @project-info:patch                |                    | - list: @cmake:patch                          |              |
| - list: @project-info:jam module           |                    | - list: @cmake:module                         |              |
| > - list: @project-info:source             |                    | > - list: @cmake:entry                        |              |
| > - list: @project-info:define             |                    | > - list: @cmake:entry                        |              |
| > - list: @project-info:include            |                    | > - list: @cmake:entry                        |              |
| > > - list: @project-info:juce module      |                    | > > - list: @cmake:value                      |              |
| > > - list: @project-info:jam module       |                    | > > - list: @cmake:link                       |              |
| > - list: @project-info:layout glob        |                    | > - list: @cmake:entry                        |              |
| - list: @project-info:format               |                    | - list: @cmake:format-install-directory       |              |
|                                            |                    | - xattr: @cmake:xattr                         |              |
|                                            |                    | - codesign: @cmake:codesign                   |              |
|                                            |                    | - verify: @cmake:verify                       |              |
|                                            |                    | - zip: @cmake:zip                             |              |
|                                            |                    | - notarize: @cmake:notarize                   |              |
|                                            |                    | - staple: @cmake:staple                       |              |
|                                            |                    | - wraptool: @cmake:wraptool                   |              |
|                                            |                    | - qa-directory: @cmake:qa-directory           |              |
|                                            |                    | - qa-copy: @cmake:qa-copy                     |              |
|                                            |                    | - install-directory: @cmake:install-directory |              |
|                                            |                    | - install-copy: @cmake:install-copy           |              |
|                                            |                    | - clangd: @cmake:clangd                       |              |
+--------------------------------------------+--------------------+-----------------------------------------------+--------------+
