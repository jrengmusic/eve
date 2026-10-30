# EVE
**Embeddable Viewport Endpoint**

EVE is the fastest modern terminal emulator built with lock free audio architecture discipline. "Fastest" is profiled, measured, and compared head to head with the most popular modern terminal emulators available.

---

## Why

A terminal's content should never be destroyed — not by SIGWINCH, not by a window resize. EVE is a terminal whose content survives every resize, and whose surface composes with Finder-grade file navigation.

EVE is a JUCE application, not a terminal. Every widget is a `juce::Component`. The terminal is one Component — the center core — among ours.

---

## Architecture

EVE is built as an audio plugin is built: Model, View, Processor. The Model holds the state. The Processor — the clock's tick, under audio discipline — owns terminal emulation. The View Components are dumb: they paint and own nothing.

The clock's tick reads and writes the PTY and decodes the bytes. The message thread lays out and paints. Lock-free FIFOs join them. EVE starts no thread. The terminal chain owns its clock, and the TTY clocks it in the PTY's own rhythm.

Rendering never touches the terminal. Images from the terminal arrive as bytes, one way, to the message thread, and real `juce::Component` widgets draw them.

The specification is [SPEC.md](SPEC.md).

---

## What

### Speed

`seq 1 N` timed by the zsh `time` builtin inside each terminal, same machine. The full method and raw lines are in [comparison.md](comparison.md).

| terminal  | 1M      | 10M     | 100M     |
|-----------|---------|---------|----------|
| EVE       | 0.543 s | 5.474 s | 53.668 s |
| WezTerm   | 0.617 s | 6.299 s | 1:00.90  |
| Ghostty   | 0.642 s | 6.421 s | 1:02.08  |
| kitty     | 0.696 s | 6.915 s | 1:06.38  |
| Alacritty | 0.746 s | 7.422 s | 1:11.49  |

### Terminal

- Runs `$SHELL` (zsh by default) on macOS and PowerShell on Windows, with shell integration for zsh, bash and PowerShell: OSC 7 (working folder) and OSC 133 A, C, D (prompt and command marks).
- History lines are kept in a record on disk; `HISTORY_CAPACITY` sets how many (10000 by default).
- Reflow belongs to the view: wrapping follows the view's own width, never the PTY window size.
- Normal and visual modes with vim keys over the terminal content; every key is a row in the config.

### Shell API

The shell integration adds an `eve` function. It speaks EVE's envelope, `ESC ] 1337 ; EVE ; <operation> ; <args> BEL`:

- `eve command <name>` — runs an EVE command, the same command a key binding runs.
- `eve preview <file>` — shows the file in the Preview column; `eve preview` hides it.
- `eve image <file>` — sends the image bytes to EVE.

### Formats and platforms

Standalone, VST3 and AU, on macOS (Intel and Apple silicon) and Windows.

---

## How

**Requirements:** a C++17 compiler (Xcode clang on macOS, MSVC on Windows), CMake 4.2 or newer, Ninja, and the JAM framework beside this repository (`../jam`). JUCE 8.0.14 sits at `../../JUCE`; the build patches a cached copy of it.

```bash
cast cast/spell.md --debug
```

`project-info.md` is the source of the build metadata; `CMakeLists.txt` is generated from it.

On first launch, EVE seeds its config to `~/.config/end/eve.md`: display settings, key bindings, settings and window chrome. The copy on disk wins.

| Feature | Status |
|---------|--------|
| Preview column (images, GIF animation) | In progress |
| Terminal images taken over from iTerm2, Sixel and Kitty, shown by Preview | In progress |
| File Tree, `ls` as a Finder/Explorer | Planned |

---

## License

MIT.

---

*conceived with [CAROL](https://github.com/jrengmusic/carol)*

Rock 'n Roll!

**JRENG!**
