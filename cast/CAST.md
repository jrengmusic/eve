## index

+---------------+-----------------------------------+
| alias         | symbol                            |
+===============+===================================+
| @template     | ../../jam/cast/template.cast      |
| @project-info | ../project-info.md                |
| @ProjectInfo  | ../Source/generated/ProjectInfo.h |
| @CMakeLists   | ../CMakeLists.txt                 |
+---------------+-----------------------------------+

## index comment

+-------------+----------------------------------------------------------------------+
| alias       | comment                                                              |
+=============+======================================================================+
| @CMakeLists | ```                                                                  |
|             | @file CMakeLists.txt                                                 |
|             | @brief EVE build manifest — a self-sufficient JUCE plugin project.   |
|             |                                                                      |
|             | Generated from project-info.md; every value traces to one table row. |
|             | Edit the table, run cast, then configure.                            |
|             | ```                                                                  |
+-------------+----------------------------------------------------------------------+

## output

+--------------------------------------+-----------+---------------------------------+--------------+
| list                                 | separator | structure                       | file         |
+======================================+===========+=================================+==============+
| > - list: @project-info:project info |           | template:namespace              | @ProjectInfo |
|                                      |           | - macro: #pragma once           |              |
|                                      |           | - name: ProjectInfo             |              |
|                                      |           | > - list: template:constant     |              |
+--------------------------------------+-----------+---------------------------------+--------------+
| - list: @project-info:baseline       |           | template:cmake                  | @CMakeLists  |
| - list: @project-info:project        |           | - list: template:cmakeToolchain |              |
| > - list: @project-info:clang flags  |           | - list: template:cmakeProject   |              |
| > - list: @project-info:msvc flags   |           | > - list: template:cmakeEntry   |              |
| - list: @project-info:paths          |           | > - list: template:cmakeEntry   |              |
| - list: @project-info:juce           |           | - list: template:cmakePath      |              |
| > - list: @project-info:patches      |           | - list: template:cmakeJuce      |              |
| - list: @project-info:modules        |           | > - list: template:cmakePatch   |              |
| - list: @project-info:plugin         |           | - list: template:cmakeModule    |              |
| > - list: @project-info:sources      |           | - list: template:cmakePlugin    |              |
| > - list: @project-info:defines      |           | > - list: template:cmakeEntry   |              |
| > - list: @project-info:includes     |           | > - list: template:cmakeEntry   |              |
| > > - list: @project-info:libraries  |           | > - list: template:cmakeEntry   |              |
| > - list: @project-info:layout globs |           | > > - list: template:cmakeEntry |              |
| - list: @project-info:plugin         |           | > - list: template:cmakeEntry   |              |
|                                      |           | - list: template:cmakeFormat    |              |
+--------------------------------------+-----------+---------------------------------+--------------+
