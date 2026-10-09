## index

+--------------------+----------------------------------------+----------+
| alias              | symbol                                 | format   |
+====================+========================================+==========+
| @code              | ../../jam/cast/code.cast               |          |
| @source            | source.cast                            |          |
| @cmake             | cmake.cast                             |          |
| @project-info      | ../project-info.md                     |          |
| @signing           | signing.md                             |          |
| @sign              | signing.cast                           |          |
| @Entitlements      | ../entitlements.plist                  |          |
| @ProjectInfo       | ../Source/generated/ProjectInfo.h      |          |
| @CMakeLists        | ../CMakeLists.txt                      |          |
| @space             | U+0020                                 | fromUTF8 |
| @semicolon         | ;                                      |          |
| @files             | files.md                               |          |
| @identifiers       | identifiers.md                         |          |
| @lookuptables      | lookuptables.md                        |          |
| @Files             | ../Source/generated/Files.h            |          |
| @FilesSource       | ../Source/generated/Files.cpp          |          |
| @Identifiers       | ../Source/generated/Identifiers.h      |          |
| @IdentifiersSource | ../Source/generated/Identifiers.cpp    |          |
| @LookupTables      | ../Source/generated/LookupTables.h     |          |
| @Generated         | ../Source/generated/Generated.h        |          |
| @credits           | jam::LookupTable<int, const char*, 22> |          |
+--------------------+----------------------------------------+----------+

## headers

+----------------+------------------------------------------------------------------------+-------------+
| file           | brief                                                                  | description |
+================+========================================================================+=============+
| ProjectInfo.h  | ```                                                                    |             |
|                | @file ProjectInfo.h                                                    |             |
|                | @brief Project metadata — the generated ProjectInfo namespace.         |             |
|                | ```                                                                    |             |
+----------------+------------------------------------------------------------------------+-------------+
| Files.h        | ```                                                                    |             |
|                | @file Files.h                                                          |             |
|                | @brief Product asset file names.                                       |             |
|                | ```                                                                    |             |
+----------------+------------------------------------------------------------------------+-------------+
| Identifiers.h  | ```                                                                    |             |
|                | @file Identifiers.h                                                    |             |
|                | @brief Product identifier vocabulary.                                  |             |
|                | ```                                                                    |             |
+----------------+------------------------------------------------------------------------+-------------+
| LookupTables.h | ```                                                                    |             |
|                | @file LookupTables.h                                                   |             |
|                | @brief Product lookup tables.                                          |             |
|                | ```                                                                    |             |
+----------------+------------------------------------------------------------------------+-------------+
| Generated.h    | ```                                                                    |             |
|                | @file Generated.h                                                      |             |
|                | @brief Generated-header umbrella — re-exports every generated concern. |             |
|                | ```                                                                    |             |
+----------------+------------------------------------------------------------------------+-------------+

## output

+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| list                                                         | separator                 | structure                                     | file               |
+==============================================================+===========================+===============================================+====================+
| > - [list]: @project-info:project info                       |                           | @code:namespace                               | @ProjectInfo       |
|                                                              |                           | - macro: #pragma once                         |                    |
|                                                              |                           | - name: ProjectInfo                           |                    |
|                                                              |                           | - [description]: @headers:brief               |                    |
|                                                              |                           | > - [list]: @code:constant                    |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @project-info:cmake                                | - [list]:                 | @cmake:cmake                                  | @CMakeLists        |
| - [list]: @project-info:project info                         |                           |                                               |                    |
| - [list]: @project-info:plugin format                        | - [list]: @space          | - [list]: @cmake:value                        |                    |
| - [list]: @project-info:compile option:configuration=all     | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:compile option:configuration=Release | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:link option:configuration=Release    | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:compile option:configuration=Debug   | - [list]: @semicolon      | - [list]: @cmake:mac                          |                    |
| - [list]: @project-info:compile option:configuration=all     | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:compile option:configuration=Release | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:link option:configuration=Release    | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:compile option:configuration=Debug   | - [list]: @semicolon      | - [list]: @cmake:win                          |                    |
| - [list]: @project-info:patch                                |                           | - [list]: @cmake:patch                        |                    |
| - [list]: @project-info:user module                          |                           | - [list]: @cmake:module                       |                    |
| > - [list]: @project-info:source                             |                           | > - [list]: @cmake:entry                      |                    |
| > - [list]: @project-info:macro                              |                           | > - [list]: @cmake:entry                      |                    |
| > - [list]: @project-info:include                            |                           | > - [list]: @cmake:entry                      |                    |
| > > - [list]: @project-info:juce module                      |                           | > > - [list]: @cmake:value                    |                    |
| > > - [list]: @project-info:user module                      |                           | > > - [list]: @cmake:link                     |                    |
| > - [list]: @project-info:layout glob                        |                           | > - [list]: @cmake:entry                      |                    |
| > - [list]: @project-info:binary                             |                           | > - [list]: @cmake:value                      |                    |
| - [list]: @project-info:plugin format                        |                           | - [list]: @cmake:format-directory             |                    |
| - [list]: @signing:signing                                   |                           | - [list]: @sign:signing-value                 |                    |
|                                                              |                           | - xattr: @sign:xattr                          |                    |
|                                                              |                           | - codesign: @sign:codesign                    |                    |
|                                                              |                           | - verify: @sign:verify                        |                    |
|                                                              |                           | - wraptool: @sign:wraptool                    |                    |
|                                                              |                           | - wraptool-win: @sign:wraptool-win            |                    |
|                                                              |                           | - signtool-win: @sign:signtool-win            |                    |
|                                                              |                           | - install-directory: @cmake:install-directory |                    |
|                                                              |                           | - install-copy: @cmake:install-copy           |                    |
|                                                              |                           | - install-copy-win: @cmake:install-copy-win   |                    |
|                                                              |                           | - install-file-win: @cmake:install-file-win   |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @signing:entitlement                             |                           | @sign:[xml]entitlements                       | @Entitlements      |
|                                                              |                           | > - [list]: @sign:entitlement                 |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @files:files                                     | - [list]: @code:linebreak | @code:line                                    | @Files             |
|                                                              |                           | - macro: #pragma once                         |                    |
|                                                              |                           |                                               |                    |
|                                                              |                           | @code:namespace                               |                    |
|                                                              |                           | - name: files                                 |                    |
|                                                              |                           | > - [list]: @code:identifier                  |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @files:files                                     | - [list]: @code:linebreak | @source:source                                | @FilesSource       |
|                                                              |                           | - file: Files.h                               |                    |
|                                                              |                           |                                               |                    |
|                                                              |                           | @code:namespace                               |                    |
|                                                              |                           | - name: files                                 |                    |
|                                                              |                           | > - [list]: @code:identifier-definition       |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @identifiers:identifiers                           |                           | @code:namespace                               | @Identifiers       |
|                                                              |                           | - macro: #pragma once                         |                    |
|                                                              |                           | - generated:                                  |                    |
|                                                              |                           | - name: Id                                    |                    |
|                                                              |                           | - [description]: @headers:brief               |                    |
|                                                              |                           | - [list]: @code:identifier                    |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @identifiers:tags                                  |                           | @code:namespace                               | @Identifiers       |
|                                                              |                           | - name: Id                                    |                    |
|                                                              |                           | - [list]: @code:identifier                    |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > - [list]: @identifiers:identifiers                         |                           | @source:source                                | @IdentifiersSource |
|                                                              |                           | - file: Identifiers.h                         |                    |
|                                                              |                           |                                               |                    |
|                                                              |                           | @code:namespace                               |                    |
|                                                              |                           | - name: Id                                    |                    |
|                                                              |                           | > - [list]: @code:identifier-definition       |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| - [list]: @identifiers:tags                                  |                           | @code:namespace                               | @IdentifiersSource |
|                                                              |                           | - name: Id                                    |                    |
|                                                              |                           | - [list]: @code:identifier-definition         |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+
| > > - [list]: @lookuptables:credits                          | - [list]: @code:linebreak | @code:namespace                               | @LookupTables      |
|                                                              |                           | - macro: #pragma once                         |                    |
|                                                              |                           | - generated:                                  |                    |
| > > - [list]: @lookuptables:credits:key                      | > > - [list]: @code:comma | - name: map                                   |                    |
|                                                              |                           | - [description]: @headers:brief               |                    |
| > > - [list]: @lookuptables:credits:value                    |                           |                                               |                    |
|                                                              |                           |                                               |                    |
|                                                              |                           | @code:lookup-table                            |                    |
|                                                              |                           | - type: @credits                              |                    |
|                                                              |                           | - name: credits                               |                    |
|                                                              |                           | > > - [list]: @code:entry                     |                    |
+--------------------------------------------------------------+---------------------------+-----------------------------------------------+--------------------+

## output index

+----------------------+-----------+-----------------------------------+------------+
| list                 | separator | structure                         | file       |
+======================+===========+===================================+============+
| - [list]: @headers   |           | @code:struct                      | @Generated |
|                      |           | - macro: #pragma once             |            |
| > - [list]: instance |           | - name: Generated                 |            |
|                      |           | - [description]: @headers:brief   |            |
|                      |           | - [list]: @code:include           |            |
|                      |           | > - [list]: @code:shared-instance |            |
+----------------------+-----------+-----------------------------------+------------+
