## index

+---------------+-----------------------------------+
| alias         | symbol                            |
+===============+===================================+
| @template     | ../../jam/cast/template.cast      |
| @project-info | ../project-info.md                |
| @ProjectInfo  | ../Source/generated/ProjectInfo.h |
+---------------+-----------------------------------+

## output

+--------------------------------------+-----------+-----------------------------+--------------+
| list                                 | separator | structure                   | file         |
+======================================+===========+=============================+==============+
| > - list: @project-info:project info |           | template:namespace          | @ProjectInfo |
|                                      |           | - macro: #pragma once       |              |
|                                      |           | - name: ProjectInfo         |              |
|                                      |           | > - list: template:constant |              |
+--------------------------------------+-----------+-----------------------------+--------------+
