## Component

Component inventory of the markdown view lane. Row order is creation order: the z cell is the juce child index (0 = back) and therefore the z-order. In this lane each component takes the view bounds reduced by the window padding. The style cell names the registered style of the component.

+----+----------+-----------+-------+------+----------+---------+-------+
| z  | type     | parameter | image | dark | style    | primary | event |
+====+==========+===========+=======+======+==========+=========+=======+
| 0  | markdown |           |       |      | markdown |         |       |
+----+----------+-----------+-------+------+----------+---------+-------+
