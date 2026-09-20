This file defines EVE's automatable parameters, read by jam::ParameterLayout
to build the host-visible parameter layout.

## parameter

+------------+--------------+--------+--------+------------------------+---------+
| id         | name         | group  | type   | choices                | default |
+============+==============+========+========+========================+=========+
| mode       | mode         | master | choice | normal, insert, visual | 0       |
+------------+--------------+--------+--------+------------------------+---------+
