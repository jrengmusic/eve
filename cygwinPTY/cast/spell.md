## index

+---------------+-----------------------------------+
| alias         | symbol                            |
+===============+===================================+
| @code         | ../../../jam/cast/code.cast       |
| @cmake        | cmake.cast                        |
| @project-info | ../project-info.md                |
| @modules      | ../../../jam/user-modules-info.md |
| @juce         | ../../../jam/cast/juce.cast       |
| @ProjectInfo  | ../Source/generated/ProjectInfo.h |
| @CMakeLists   | ../CMakeLists.txt                 |
| @semicolon    | ;                                 |
+---------------+-----------------------------------+

## headers

+---------------+-----------------------------------------------------------------+-------------------------------------------+
| file          | brief                                                           | description                               |
+===============+=================================================================+===========================================+
| ProjectInfo.h | ```                                                             | Product name, version, and source commit. |
|               | @file ProjectInfo.h                                             |                                           |
|               | @brief Project metadata — the ProjectInfo namespace, generated. |                                           |
|               | ```                                                             |                                           |
+---------------+-----------------------------------------------------------------+-------------------------------------------+

## output

+----------------------------------------------+----------------------+------------------------------------+--------------+
| list                                         | separator            | structure                          | file         |
+==============================================+======================+====================================+==============+
| - [list]: @project-info:cmake                | - [list]: @semicolon | @cmake:cmake                       | @CMakeLists  |
| - [list]: @project-info:project info         |                      |                                    |              |
| - [list]: @project-info:release:stage=       | - [list]: @semicolon | - [list]: @cmake:mac               |              |
| - [list]: @project-info:release:stage=linker | - [list]: @semicolon | - [list]: @cmake:mac               |              |
| - [list]: @project-info:debug:stage=         | - [list]: @semicolon | - [list]: @cmake:mac               |              |
| - [list]: @project-info:release:stage=       | - [list]: @semicolon | - [list]: @cmake:win               |              |
| - [list]: @project-info:release:stage=linker | - [list]: @semicolon | - [list]: @cmake:win               |              |
| - [list]: @project-info:debug:stage=         | - [list]: @semicolon | - [list]: @cmake:win               |              |
| - [list]: @project-info:release:stage=       | - [list]: @semicolon | - [list]: @cmake:mac               |              |
| - [list]: @project-info:release:stage=linker | - [list]: @semicolon | - [list]: @cmake:win               |              |
| - [list]: @project-info:debug:stage=         | - [list]: @semicolon | - [list]: @cmake:mac               |              |
| - [list]: @project-info:user module          |                      | - [list]: @cmake:module            |              |
| - [list]: @project-info:source glob          |                      | - [list]: @cmake:glob-pattern      |              |
| - [list]: @project-info:macro                |                      | - [list]: @cmake:value             |              |
| - [list]: @project-info:include              |                      | - [list]: @cmake:value             |              |
| - [list]: @project-info:juce module          |                      | - [list]: @cmake:value             |              |
| - [list]: @project-info:user module          |                      | - [list]: @cmake:link              |              |
| - [list]: @project-info:pack                 |                      | - [list]: @cmake:pack-value        |              |
|                                              |                      | - patched-juce: @juce:patched-juce |              |
| - [list]: @modules:submodule:module=patch    |                      | - [list]: @juce:patch              |              |
+----------------------------------------------+----------------------+------------------------------------+--------------+
| > - [list]: @project-info:project info       |                      | @code:namespace                    | @ProjectInfo |
|                                              |                      | - macro: #pragma once              |              |
|                                              |                      | - name: ProjectInfo                |              |
|                                              |                      | - [description]: @headers:brief    |              |
|                                              |                      | > - [list]: @code:constant         |              |
+----------------------------------------------+----------------------+------------------------------------+--------------+
