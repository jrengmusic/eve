# EVE Throughput Comparison

Raw-byte throughput of EVE against other terminals. ARCHITECT measures every run.

## Method

- Command: `seq 1 N` with N = 1 000 000, 10 000 000 and 100 000 000, timed by the zsh `time` builtin inside each terminal.
- Runs: one run per terminal and N, consecutive.
- Columns: `total` is wall-clock time. `cpu` is the CPU share that zsh reports. `user` and `system` are the seconds that the `seq` process uses.
- EVE: Release build with `jam::TerminalClock`, 2026-10-29 (`PLAN-terminal-clock-rhythm.md`).

## Total time

+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+
| terminal                 | date       | source           | 1M total  | 1M cpu  | 10M     | 10M cpu  | 100M     | 100M cpu |
|                          |            |                  |           |         | total   |          | total    |          |
+==========================+============+==================+===========+=========+=========+==========+==========+==========+
| EVE                      | 2026-10-29 | terminal-clock-  | 0.543 s   | 99%     | 5.474 s | 99%      | 53.668 s | 98%      |
|                          |            | rhythm           |           |         |         |          |          |          |
+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+
| endless (old END binary) | 2026-10-25 | Sprint 174       | 0.596 s   | 98%     | 6.127 s | 98%      | 59.609 s | 98%      |
+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+
| WezTerm                  | 2026-10-25 | Sprint 174       | 0.617 s   | 99%     | 6.299 s | 99%      | 1:00.90  | 99%      |
+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+
| Ghostty                  | 2026-10-25 | Sprint 174       | 0.642 s   | 99%     | 6.421 s | 99%      | 1:02.08  | 99%      |
+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+
| kitty                    | 2026-10-25 | Sprint 174       | 0.696 s   | 99%     | 6.915 s | 99%      | 1:06.38  | 99%      |
+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+
| Alacritty                | 2026-10-26 | eve-max-speed    | 0.746 s   | 99%     | 7.422 s | 99%      | 1:11.49  | 99%      |
+--------------------------+------------+------------------+-----------+---------+---------+----------+----------+----------+

The rows are in order of the 100M total, fastest first.

## User and system time at 100M

+--------------------------+---------+----------+----------+--------------------+
| terminal                 | user    | system   | total    | total vs endless   |
+==========================+=========+==========+==========+====================+
| EVE                      | 17.43 s | 35.33 s  | 53.668 s | −5.9 s             |
+--------------------------+---------+----------+----------+--------------------+
| endless (old END binary) | 19.31 s | 39.38 s  | 59.609 s | 0                  |
+--------------------------+---------+----------+----------+--------------------+
| WezTerm                  | 17.52 s | 43.37 s  | 1:00.90  | +1.3 s             |
+--------------------------+---------+----------+----------+--------------------+
| Ghostty                  | 17.64 s | 44.38 s  | 1:02.08  | +2.5 s             |
+--------------------------+---------+----------+----------+--------------------+
| kitty                    | 17.30 s | 49.04 s  | 1:06.38  | +6.8 s             |
+--------------------------+---------+----------+----------+--------------------+
| Alacritty                | 19.57 s | 51.89 s  | 1:11.49  | +11.9 s            |
+--------------------------+---------+----------+----------+--------------------+

## Raw measurements

The lines below are verbatim as ARCHITECT reported them.

### EVE, 2026-10-29

```
seq 1 100000000  17.43s user 35.33s system 98% cpu 53.668 total
seq 1 10000000  1.91s user 3.52s system 99% cpu 5.474 total
seq 1 1000000  0.18s user 0.36s system 99% cpu 0.543 total
```

### endless (old END binary), 2026-10-25

```
seq 1 1000000  0.20s user 0.39s system 98% cpu 0.596 total
seq 1 10000000  2.12s user 3.94s system 98% cpu 6.127 total
seq 1 100000000  19.31s user 39.38s system 98% cpu 59.609 total
```

### WezTerm, 2026-10-25

```
seq 1 1000000  0.18s user 0.44s system 99% cpu 0.617 total
seq 1 10000000  1.93s user 4.36s system 99% cpu 6.299 total
seq 1 100000000  17.52s user 43.37s system 99% cpu 1:00.90 total
```

### Ghostty, 2026-10-25

```
seq 1 1000000  0.18s user 0.46s system 99% cpu 0.642 total
seq 1 10000000  1.92s user 4.49s system 99% cpu 6.421 total
seq 1 100000000  17.64s user 44.38s system 99% cpu 1:02.08 total
```

### kitty, 2026-10-25

```
seq 1 1000000  0.18s user 0.51s system 99% cpu 0.696 total
seq 1 10000000  1.91s user 5.00s system 99% cpu 6.915 total
seq 1 100000000  17.30s user 49.04s system 99% cpu 1:06.38 total
```

### Alacritty, 2026-10-26

```
seq 1 1000000  0.21s user 0.53s system 99% cpu 0.746 total
seq 1 10000000  2.20s user 5.22s system 99% cpu 7.422 total
seq 1 100000000  19.57s user 51.89s system 99% cpu 1:11.49 total
```
