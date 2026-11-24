# PLAN: SIMD UTF-8 Encode/Decode and Cross-Platform Throughput Restoration

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-24
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (`LANGUAGE.md`: One object, one translation unit; 30 lines/function and 3 branches/switch unchanged)

## Overview

Implement a clean-room, cross-platform SIMD UTF-8 encoder/decoder and ASCII scanner in `jam_core`, wire it into `jam_terminal`, and re-profile across all `scout` lanes. Eliminate every proven cross-platform design bottleneck (`TerminalProcessor` event dispatch, swap virtual serialization, Vulkan mid-frame/offscreen stalls, and `TerminalClock` elapsed-time overshoot) by fixing the root contract violations until macOS matches or beats its `2026-10-29` baseline while preserving Windows Cygwin throughput.

## Language / Framework Constraints

- **One object, one translation unit** (`CODING.md:896-937`, `LANGUAGE.md:25-39`): Templates and `constexpr` stay in headers; every non-template body goes into the object's single `.cpp` file (`jam_Utf8.h` + `jam_Utf8.cpp` compiled once in `jam_core.cpp`). `jam_Simd.h` stays a foundation header because `findOffset` is a template and its byte kernels inline across modules.
- **Zero includes in submodule files** (`CODING.md:805-846`): Submodule `.h` and `.cpp` files include nothing; all includes live in `jam_core.h`, `jam_core.cpp`, `jam_terminal.h`, and `jam_vulkan.h`.
- **Deadline thread contract** (`MANIFESTO.md:269`, `SPEC.md:45-65`): `EVETerminalProcessor::process()` and every function it calls perform zero heap allocation, zero locks, zero waits, and zero blocking calls.
- **Cross-platform ISA support** (`jam_Simd.h:10-20, 136, 448, 699, 767`): Every SIMD primitive provides ARM64 NEON (`JAM_SIMD_NEON`), x86_64 SSE2/SSSE3 (`JAM_SIMD_SSE2` / `JAM_SIMD_SSSE3`), and scalar fallback (`#else`).
- **Pre-audit hygiene** (`CAROL.md` Code Hygiene): Write zero comments and zero doxygen blocks during implementation; comments and doxygen run in the dedicated post-audit Pathfinder pass.

## Framework Inventory

### 1. API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| 128-bit SIMD byte scan harness | `jam_core` | `jam::simd::findOffset`, `getNibbleMask`, `getFirstLane`, `getTrailingZeroCount`, `laneWidth`, `strideWidth` | `jam_core/jam_Simd.h:59-61, 140-174, 450-479` | 1, 2 |
| SIMD byte and nibble-LUT primitives | `jam_core` | `jam::simd::findByte`, `findNonPlainByte`, `countByte`, `findBytePair`, `findFingerprint` | `jam_core/jam_Simd.h:221-446, 527-765`; `jam/docs/xml/index.xml:22270-22283` | 1, 2 |
| Character and Unicode boundary constants | `jam_core` | `Chars::space`, `Chars::tilde`, `Chars::deleteCharacter`, `Chars::nonAsciiStart`, `Chars::replacementCharacter`, `Chars::surrogateRangeStart`, `Chars::surrogateRangeEnd`, `Chars::maximumCodePoint` | `jam/generated/jam_Core.h:265-350` | 1, 2, 3 |
| UTF-8 prefix trim and span decode pattern | `jam_core` | `jam::Format::getUtf8Prefix`, `jam::Format::appendUtf8`, `jam::Format::setPercentDecoding` | `jam_core/text/jam_Format.h:556-584, 791`; `jam_Format.cpp:603-631, 1170-1188` | 2, 3 |
| Grapheme cluster iteration and width | `jam_core` | `jam::AttributedChar::applyToClusters`, `getWidthClass`, `getColumns`, `setExtension`, `graphemeSegmentationStep` | `jam_core/text/jam_AttributedChar.h:466-534`; `jam_AttributedChar.cpp:3281-3360` | 3 |
| Row atom container and binary writer | `jam_core` | `jam::AttributedString::set`, `setAtom`, `prepare`, `resize`, `removeLink`, `write`, `read` | `jam_core/text/jam_AttributedString.h:122-195, 343-414`; `jam_AttributedString.cpp:144-291, 372-483` | 3, 6 |
| ANSI byte classification and tokeniser | `jam_terminal` | `AnsiDocument::getToken`, `getTextToken`, `getAnsiVocabulary`, `map::ansi`, `map::Byte::text` | `jam_terminal/document/jam_AnsiDocument.cpp:16-57`; `cast/terminal.md:441-490` | 3 |
| VT text run decoder and event publisher | `jam_terminal` | `TerminalProcessor::process`, `addTextRun`, `addCluster`, `addExtension`, `addTextCell`, `setEvents` | `jam_terminal/processor/jam_TerminalProcessor.cpp:64-96, 297-332, 487-587` | 3, 5 |
| VT row encoder | `jam_terminal` | `TerminalEncoder::writeRow` | `jam_terminal/buffers/jam_TerminalEncoder.h:46` | 3 |
| Row ring and swap writer | `jam_terminal` | `TerminalBuffer::writeRow`, `getRow`, `TerminalSwap::writeFromTerminalBuffer`, `writeLine`, `writeNextLine` | `jam_terminal/buffers/jam_TerminalBuffer.cpp:217-223, 438-454`; `jam_TerminalSwap.cpp:75-182` | 3, 6 |
| Cached Model parameter atomic write | `jam_data_structures` | `jam::Model::getParameter<jam::Parameter<Type>>`, `Parameter::store` | `jam_data_structures/model/jam_Model.h:967-1031`; `eve/Source/EVETerminalProcessor.cpp:162-168` | 5 |
| Offscreen and mid-frame Vulkan flush | `jam_vulkan` | `VulkanGraphics::beginOffscreenFrame`, `flushRecording`, `reservePrimitiveRecords`, `submitAndWait`, `VulkanImagePixelData::createLowLevelContext` | `jam_vulkan/context/jam_VulkanGraphics.cpp:791-889, 1005-1031, 1191-1210`; `jam_VulkanImagePixelData.cpp:25-57` | 4, 7 |
| Viewport image caching | `jam_gui` / `juce_gui_basics` | `juce::Component::setBufferedToImage`, `StandardCachedComponentImage::paint` | `jam_gui/widgets/jam_TextEditor.cpp:28, 1666`; `juce_StandardCachedComponentImage.h:67-76` | 4, 7 |
| Cross-platform readiness clock | `jam_terminal` | `jam::TerminalClock::prepare`, `process`, `listen`, `getPeriodMilliseconds`, `AsyncOutputStream::Native` | `jam_terminal/tty/jam_TerminalClock.h:1-94`; `jam_TerminalClock.cpp:1-54`; `jam_AsyncOutputStream_mac.cpp:1-62`; `jam_AsyncOutputStream_windows.cpp:1-191` | 8 |
| Profiler zones and sampling | `jam_profiler` / `scout` | `jam::Profiler::Zone`, `scout` manifest (`## run`, `## sample`, `## toolchain`) | `profile/SKILL.md:112-300`; `eve/benchmark/seq-10m-mac-now*.md` | 4, 8 |

### 2. Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Three-tier SIMD dispatch (`NEON`, `SSE2`/`SSSE3`, scalar) with `findOffset` | `jam_core/jam_Simd.h:162-258, 467-565, 767-800` | `jam_Simd.h:221-875` (`findByte`, `findNonPlainByte`, `countByte`, `findBytePair`, `findFingerprint`) | 1, 2 |
| Unity-build submodule registration (header + `.cpp`, zero submodule includes) | `jam_core/jam_core.h:91-114`, `jam_core/jam_core.cpp:4-32` | All submodules in `jam_core`, `jam_terminal`, `jam_vulkan` | 2 |
| Caller-buffer span writer returning written element count | `jam_core/text/jam_Format.cpp:1170-1188` (`setPercentDecoding`) | `jam_Format.h:791` | 2 |
| Cached `jam::Parameter<Type>&` bound once at event registration | `eve/Source/EVETerminalProcessor.cpp:162-168` (`registerImageEvents`) | `EVETerminalProcessor.cpp:116-169` (`registerEvents`, `registerImageEvents`) | 5 |
| Static dispatch table replacing branch chains (`LookupTable` / `std::array`) | `jam_core/text/jam_Format.cpp:605-615`; `jam_AsyncOutputStream_windows.cpp:119-131` | `jam_TerminalProcessor.cpp:1237-1499`, `jam_AsyncOutputStream_windows.cpp:15-16` | 2, 5 |
| Binary serialization matching `AttributedString::write` / `read` | `jam_core/text/jam_AttributedString.cpp:372-483` | `jam_TerminalBuffer.cpp:438-492`, `jam_TerminalSwap.cpp:103-112` | 6 |
| OS driver seam pacing `TerminalClock` on serial `AsyncUpdater::Source` | `jam_AsyncOutputStream_mac.cpp:13-31`, `jam_AsyncOutputStream_windows.cpp:26-60` | `jam_TerminalTTY_posix.cpp:234`, `jam_TerminalCygwinTTY_windows.cpp`, `jam_TerminalTTY_windows.cpp` | 8 |

### 3. Names

| Family | Members | Intent | Step |
|---|---|---|---|
| `jam::simd` scanner family | `findByte`, `findNonPlainByte`, `findBytePair`, `findFingerprint`, `countByte` (`jam_Simd.h:221-446`) | Join with `findNonPrintableAscii`, `findControlByte` — return byte index of first match, or `size` when absent | 1 |
| `jam::Utf8` codec family (`NAMES.md` Rule 1 Verb Contract + `jam_Format.h:584, 791`) | `Format::getUtf8Prefix`, `Format::setPercentDecoding`, `Validator::isValid`, `CharPointer_UTF8::isValidString` | `Utf8::isValid` (`is` bool), `getCodepointCount`, `getEncodedBytes`, `getPrefix` (`get` value), `setCodepoints`, `setBytes` (`set` writes into output span and returns count) | 2 |
| `AttributedString::set` family | `set (int, AttributedChar)`, `set (int, AttributedChar, const Grapheme::Entry&)`, `set (int, AttributedChar, const AttributedString&)`, `set (int, juce::Span<const char32_t>, AttributedChar)` (`jam_AttributedString.h:147-195`) | Join with `set (int index, std::string_view asciiText, AttributedChar attributes)` — write contiguous printable ASCII atoms at `index` | 3 |
| `TerminalProcessor` event setters | `setEvents`, `oscConstruction`, `eveConstruction` (`jam_TerminalProcessor.cpp:297-332`) | Emit each Model event through `events.get (Id::..., value)` when state changes | 5 |
| `TerminalClock` timing family | `prepare`, `process`, `listen`, `getPeriodMilliseconds` (`jam_TerminalClock.h:40-66`) | Keep exact public API; `getPeriodMilliseconds()` returns remaining wait period after subtracting elapsed processing time | 8 |

### 4. Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "One object, one translation unit" (`CODING.md:1092`) | One header `.h` + one `.cpp` included in the module unity `.cpp`; templates and `constexpr` in `.h`, non-template bodies in `.cpp` | `jam_core/jam_core.h:92`, `jam_core/jam_core.cpp:14` (`jam_Format.h` + `jam_Format.cpp`) |
| "No bail-out guards — positive nesting and assert" (`CODING.md:1068-1069`) | Assert preconditions with `jassert`; use nested positive `if` blocks; use result returns only when the answer is final | `jam_core/text/jam_Format.cpp:619-630` |
| "Use `.at()` for container access" (`CODING.md:1071`) | Bounds-checked `.at()` everywhere except proven hot loops on raw pointers/spans | `jam_core/text/jam_AttributedString.cpp:146-152, 275` |
| "Use C++ alternative tokens" (`CODING.md:1072`) | `not`, `and`, `or` in C++ code; `!`, `&&`, `||` in `#if` / `#elif` directives only | `jam_core/jam_Simd.h:10-20, 856`; `jam_AttributedString.cpp:192` |
| "No anonymous namespaces; no `namespace detail`" (`CODING.md:1073-1074`) | Use `static` file-local helpers in `.cpp` and class `private:` members in `.h` | `jam_core/text/jam_Format.cpp:5-6, 953-1010` |
| "No magic values — owner constants" (`CODING.md:1086`, `MANIFESTO.md:89`) | Reference `Chars::*` from `generated/jam_Core.h` and named `static constexpr` in narrowest scope | `jam_core/text/jam_Format.cpp:755, 1172` |
| "Deadline threads: zero allocation, lock, wait, or blocking call" (`CODING.md:1093`) | Stack fixed buffers (`std::array`) or pre-sized `prepare()` buffers; zero `std::vector` / `std::string` allocation on hot path | `jam_core/text/jam_Format.h:570` (`std::array<char, utf8MaxBytes>`) |
| "Structured bindings for all pair/tuple results" (`CODING.md:1076`) | Named destructuring (`const auto& [range, linkNumber]`), never `.first`/`.second`, never `_` or `it` | `jam_core/text/jam_AttributedString.cpp:106, 275` |

### 5. Absence

| Need | Query | Result |
|---|---|---|
| SIMD UTF-8 encode, decode, validate, or codepoint count in `jam` | `rg -n -i "(simd.*utf|utf.*simd)" jam/docs/xml/index.xml` and `rg -n -i "(encodeUtf|decodeUtf|validateUtf|scanUtf|isValidUtf|utf8Decode|utf8Encode|utf8Validate|utf8Scan)" jam/docs/xml/index.xml`; direct read of `jam_Simd.h:1-878` and `jam_Format.h` | 0 matches (exit code 1); `jam::simd` has only ASCII/nibble/pixel helpers and `jam::Format` has only scalar UTF-8 helpers |
| SIMD UTF-8 encode, decode, validate, or count in JUCE | Direct read of `juce_CharPointer_UTF8.h:1-596`; `rg -n -i "(simd.*utf|utf.*simd)" JUCE/docs/xml/index.xml` | 0 matches (exit code 1); `CharPointer_UTF8` is 100% scalar byte loops |
| SIMD printable-ASCII or ANSI control-byte scanner in `jam::simd` | Direct read of `jam_Simd.h:1-878` and `docs/xml/index.xml:22270-22293` | Absent; `findNonPlainByte` checks identifier chars (`a-zA-Z0-9_ `) only, not `[0x20, 0x7E]` or ANSI control `< 0x20 / 0x7F` |
| Bulk printable-ASCII row writer on `jam::AttributedString` | Direct read of `jam_AttributedString.h:29-354` | Absent; `set` accepts one `AttributedChar` or one `Span<const char32_t>` grapheme cluster at a time |

*(Note: `carol_ling` MCP returned `no daemon for manifest: /Users/jreng/.gemini/config/plugins/carol`; all absence proofs use full Doxygen XML index scans plus complete header reads.)*

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against `MANIFESTO.md` (BLESSED), `NAMES.md`, `~/.carol/CODING.md`, and the locked PLAN decisions (no deviation, no scope drift). `@Auditor` runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Add Cross-Platform SIMD Printable-ASCII and ANSI Control Scanners to `jam::simd`
**Scope:** `jam/jam_core/jam_Simd.h`
**Action:**
1. Add `findControlByte (const char* data, size_t size, size_t offset) noexcept -> size_t` across `#if JAM_SIMD_NEON`, `#elif JAM_SIMD_SSE2`, and `#else` scalar fallback.
   - Match condition: `byte < 0x20` or `byte == 0x7F` (exact equivalence with `map::ansi` in `cast/terminal.md:453-490`).
   - Use `jam::simd::findOffset` (`32`-byte unrolled strides + `16`-byte tail + scalar tail) on NEON (`vcltq_u8` + `vceqq_u8` + `vorrq_u8`) and SSE2 (unsigned compare via XOR `0x80` + `_mm_cmplt_epi8` + `_mm_cmpeq_epi8` + `_mm_or_si128`).
2. Add `findNonPrintableAscii (const char* data, size_t size, size_t offset) noexcept -> size_t` across `#if JAM_SIMD_NEON`, `#elif JAM_SIMD_SSE2`, and `#else` scalar fallback.
   - Match condition: `static_cast<uint8_t> (byte - 0x20) >= 0x5F` (flags C0 `0x00..0x1F`, DEL `0x7F`, and non-ASCII `0x80..0xFF` in one unsigned subtract + compare).
   - Use `jam::simd::findOffset` on NEON (`vsubq_u8` + `vcgeq_u8`) and SSE2 (signed shift + `_mm_cmpgt_epi8` / `_mm_cmplt_epi8`).
3. Keep zero magic numbers (named `static constexpr uint8_t` constants inside narrowest scope; note `jam_Simd.h` sits before `generated/jam_Core.h` at `jam_core.h:72`, so declare named `inline constexpr uint8_t` bounds in `namespace jam::simd`).
**Inventory:** API rows 1–2; Pattern usage row 1; Names row 1; Synthesis rows 2, 4, 6; Absence row 3.
**Validation:** COUNSELOR reads `jam_Simd.h` at all edited lines; verifies NEON, SSE2, and scalar paths implement identical predicates, zero bail-out guards, `not`/`and`/`or` tokens, zero comments, and clean Release build through `scout`.

### Step 2: Implement Clean-Room Cross-Platform SIMD `jam::Utf8` Codec
**Scope:** `jam/jam_core/text/jam_Utf8.h` (new), `jam/jam_core/text/jam_Utf8.cpp` (new), `jam/jam_core/jam_core.h`, `jam/jam_core/jam_core.cpp`, `jam/jam_core/text/jam_Format.cpp`
**Action:**
1. Create `jam_core/text/jam_Utf8.h` (zero `#include` directives) declaring `struct Utf8` with static `noexcept` methods:
   - `static bool isValid (std::string_view bytes) noexcept;`
   - `static size_t getCodepointCount (std::string_view bytes) noexcept;`
   - `static size_t getEncodedBytes (juce::Span<const char32_t> codepoints) noexcept;`
   - `static std::string_view getPrefix (std::string_view bytes) noexcept;`
   - `static size_t setCodepoints (std::string_view bytes, juce::Span<char32_t> output) noexcept;`
   - `static size_t setBytes (juce::Span<const char32_t> codepoints, juce::Span<char> output) noexcept;`
2. Create `jam_core/text/jam_Utf8.cpp` (zero `#include` directives, `static` internal helpers, zero `namespace detail`, zero anonymous namespaces) implementing clean-room cross-platform SIMD algorithms:
   - `isValid`: `64`-byte (`4×16B`) unrolled ASCII fast-path (`reduce_or` MSB check via `vmaxvq_u8 < 0x80` on NEON, `_mm_movemask_epi8 == 0` on SSE2) + branchless Keiser-Lemire 3-table nibble-LUT transition validation (`vqtbl1q_u8` on NEON, `_mm_shuffle_epi8` under `#if JAM_SIMD_SSSE3`, scalar fallback on baseline SSE2/other) + `prev2 >= 0xE0` / `prev3 >= 0xF0` continuation check + trailing incomplete check.
   - `getCodepointCount`: counts non-continuation bytes (`int8_t(b) > -65`) in `16`-byte SIMD lane accumulators (NEON `vcgtq_s8` flushed via `vaddlvq_u8` before overflow; SSE2 `_mm_cmpgt_epi8` flushed via `_mm_sad_epu8`; scalar tail).
   - `getPrefix`: O(1) trailing boundary trim checking at most the last 3 bytes for incomplete 2/3/4-byte lead sequences.
   - `setCodepoints`: caller-buffer zero-allocation UTF-8 → UTF-32 decode. `16`-byte ASCII fast-path checks MSB `< 0x80` and widens `uint8_t` → `uint32_t` directly (`vmovl_u8` + `vmovl_u16` on NEON; `_mm_unpacklo/hi_epi8` + `_mm_unpacklo/hi_epi16` on SSE2) with strict input/output bounds (never over-reads or over-writes). Non-ASCII sequences decode via continuation-mask / branchless 2/3/4-byte assembly with explicit `Chars::replacementCharacter` (`U+FFFD`) on ill-formed bytes (matching RFC 3629 / Unicode maximal subpart).
   - `setBytes`: caller-buffer zero-allocation UTF-32 → UTF-8 encode. `8`/`16`-codepoint ASCII fast-path checks all lanes `< 0x80` (`vmaxvq_u32` on NEON; `_mm_or_si128` + compare on SSE2) and narrows `uint32_t` → `uint16_t` → `uint8_t` directly (`vmovn_u32` + `vmovn_u16` on NEON; `_mm_packus_epi16` on SSE2). Non-ASCII codepoints encode 2/3/4 bytes safely (clamping surrogates `Chars::surrogateRangeStart..surrogateRangeEnd` and `> Chars::maximumCodePoint` to `Chars::replacementCharacter`).
   - `getEncodedBytes`: SIMD ASCII fast-path + branchless length tally (`1..4` bytes per codepoint).
3. Include `"text/jam_Utf8.h"` in `jam_core.h` (before `jam_Format.h`) and `"text/jam_Utf8.cpp"` in `jam_core.cpp`.
4. Delegate `Format::getUtf8Prefix (bytes)` (`jam_Format.cpp:603-631`) directly to `Utf8::getPrefix (bytes)` (SSOT).
**Inventory:** API rows 1–4; Pattern usage rows 1–3, 5; Names row 2; Synthesis rows 1–7; Absence rows 1–2.
**Validation:** COUNSELOR reads `jam_Utf8.h`, `jam_Utf8.cpp`, `jam_core.h`, `jam_core.cpp`, and `jam_Format.cpp`; checks every CODING.md CRITICAL RULE, `30`-line function decomposition, `3`-branch limit, zero heap allocation, strict bounds (no buffer over-read/over-write), and clean compilation.

### Step 3: Wire SIMD UTF-8 and Bulk Printable-ASCII Path into `jam_core/text` and `jam_terminal`
**Scope:** `jam/jam_core/text/jam_AttributedChar.h`, `jam/jam_core/text/jam_AttributedString.h`, `jam/jam_core/text/jam_AttributedString.cpp`, `jam/jam_terminal/document/jam_AnsiDocument.cpp`, `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `jam/jam_terminal/buffers/jam_TerminalEncoder.cpp`
**Action:**
1. `AnsiDocument::getTextToken` (`jam_AnsiDocument.cpp:44-57`): Replace byte-by-byte `while` loop with `jam::simd::findControlByte (cursor.source.data(), cursor.source.size(), start)`. Keep `Utf8::getPrefix` trim when `end == cursor.source.size()`.
2. `AttributedString::set (int index, std::string_view asciiText, AttributedChar attributes)` (`jam_AttributedString.h`, `jam_AttributedString.cpp`):
   - Assert every byte is printable ASCII in debug (`jassert (simd::findNonPrintableAscii (asciiText.data(), asciiText.size(), 0) == asciiText.size())`).
   - Compute `endIndex { index + static_cast<int> (asciiText.size()) }`; resize `atoms` once if `endIndex > atoms.size()`; release any overwritten grapheme clusters and links in `[index, juce::jmin (endIndex, oldSize))` once (skipping link scan completely when `linkRuns.isEmpty()`).
   - Prepare base `AttributedChar` (`contentTag = contentCodepoint`, `widthClass = narrow`, `codepoint = 0`) and fill `atoms` across `asciiText` in a tight loop setting `codepoint = static_cast<uint8_t> (byte)`.
3. `AttributedChar::applyToClusters (std::string_view text, Function&& function)` (`jam_AttributedChar.h:496-534`):
   - Decode UTF-8 chunks using `Utf8::setCodepoints` into a fixed stack `std::array<char32_t, chunkSize>` (zero heap allocation) instead of per-byte `CharPointer_UTF8::getAndAdvance()`.
   - Preserve exact byte-offset reporting and 8-codepoint cluster splitting contract.
4. `TerminalProcessor::addTextRun` (`jam_TerminalProcessor.cpp:487-495`):
   - Skip `addExtension()` when `not text.empty()` and the first byte is printable ASCII (`>= Chars::space and < Chars::deleteCharacter`) because printable ASCII never extends a preceding UAX #29 cluster (verify against `graphemeSegT1/T2/T3` before implementing).
   - Loop over `text` using `jam::simd::findNonPrintableAscii`:
     - For a contiguous printable-ASCII span when `not insertMode`: if a non-ASCII byte immediately follows the span in `text`, hold the final 1 byte of the span for the cluster path so combining marks attach cleanly to it. Otherwise slice the span into row-sized chunks up to `bounds.width - cursor.x`, handling `autoWrap` / `isWrapPending` / `addLine` / `setWrapped` once per row wrap, calling `terminalBuffer.getRow (cursor.getY()).set (cursor.x, chunk, pen.attributes)` once per chunk, marking `outputBlock` once when `isRecording`, and adding `activeLink` range once when `activeLink != noLink`.
     - For any non-ASCII byte (or when `insertMode` is active), decode through `AttributedChar::applyToClusters` → `addCluster` (zero duplication of Unicode/Kitty/insert logic).
5. `TerminalEncoder::writeRow` (`jam_TerminalEncoder.cpp`): Check its UTF-8 output path; encode codepoint runs through `jam::Utf8::setBytes` into a fixed stack buffer instead of per-codepoint `CharPointer_UTF8::write`.
**Inventory:** API rows 1–9; Pattern usage rows 1–3; Names rows 1–3; Synthesis rows 1–7; Absence rows 1–4.
**Validation:** COUNSELOR reads every edited file; verifies exact VT wrap/pending-wrap/combining-mark/insertMode/OSC-8/OSC-133 invariants, zero heap allocation on the deadline thread, and zero comments.

### Step 4: Re-Profile All Lanes and Trace `VulkanGraphics::flushRecording`
**Scope:** `eve/benchmark/seq-10m-mac-simd*.md` (manifests/reports)
**Action:**
1. Run sequential `scout` profiles (one at a time, Release, zero overlap, per `profile/SKILL.md`):
   - Live `How long` (`5` epochs, `seq 1 10000000`, baseline = `seq-10m-mac-now-report.md`).
   - Live `Where` + `cswitch` (`3` epochs, `intervalMs = 1`, `stackDepth = 64`).
   - Memory Replay `How long` (`5` epochs, `seq 1 1000000`, baseline = `seq-1m-mac-now-replay-report.md`).
2. Run a targeted `whatdbg` breakpoint trace via `@Pathfinder` (`trace` skill) on `VulkanGraphics::flushRecording` during `seq` to capture exact runtime call stacks, `extent`, `swapchain.getExtent()`, and `primitiveRecordBuffer` counts.
3. Reconcile measured numbers against the `2026-10-29` baseline (`0.543 s` / `5.474 s`) and confirm exact targets for Steps 5–8.
**Inventory:** API rows 12–13, 15.
**Validation:** COUNSELOR inspects `dropped == 0`, `build type == Release`, zone deltas, `## sample` / `## cswitch` top frames, and `whatdbg` call stacks verbatim before executing Steps 5–8.

### Step 5: Eliminate Deadline-Thread Event-Dispatch Overhead (Cross-Platform)
**Scope:** `eve/Source/EVETerminalProcessor.cpp`, `jam/jam_terminal/processor/jam_TerminalProcessor.h`, `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`
**Action:**
1. In `EVETerminalProcessor::registerEvents()` (`EVETerminalProcessor.cpp:116-117`): Bind `auto& parameter { *state.getParameter<jam::Parameter<int>> (terminalTag, id) };` once at registration and store via `parameter.store (value)` directly — matching `registerImageEvents()` (`EVETerminalProcessor.cpp:162-168`) and eliminating `46` `HashMap::doFind` lookups per drained tick.
2. In `TerminalProcessor` (`jam_TerminalProcessor.cpp:64-96, 297-332`): Emit mode/state events (`applicationCursor`, `bracketedPaste`, `mouseSgr`, `cursorVisible`, `isRecording`, `alternateScreen`, `mouseTracking`, `cursorShape`, `keyboardFlags`, `autoWrap`, `originMode`, `insertMode`, `activeLink`, `savedCursors`, `scrollingRegion`, `pen`, `savedPens`, `keyboardModes`, `syncStart`, `outputBlock`, `previewBox`, `numPrompts`) at the exact transition sites (`reset()`, CSI/OSC/DEC/ESC state updates) rather than re-dispatching all `30` keys through `jam::Function::Map` on every drained tick. Keep only tick-changed fields (`cursor`, `isWrapPending`, `shellExited`, `syncOutput`) on tick drain.
   - Verify `syncOutput` gating: when `syncOutput` transitions from `true` to `false`, flush the full Model snapshot via `setEvents (events)` once so `DEC 2026` synchronized updates remain lossless.
**Inventory:** API rows 8, 11; Pattern usage row 4; Names row 4; Synthesis rows 2, 7.
**Validation:** COUNSELOR reads `EVETerminalProcessor.cpp` and `jam_TerminalProcessor.cpp`; verifies zero shadow state (`MANIFESTO.md` **S**), every mode transition emits its Model row on both macOS and Windows, and `DEC 2026` flush remains intact.

### Step 6: Eliminate Per-Field Virtual Stream Serialization in `TerminalSwap` (Cross-Platform)
**Scope:** `jam/jam_core/text/jam_AttributedString.cpp`, `jam/jam_terminal/buffers/jam_TerminalBuffer.cpp`, `jam/jam_terminal/document/jam_TerminalSwap.cpp`
**Action:**
1. Inspect `AttributedString::write` (`jam_AttributedString.cpp:372-406`) and `TerminalBuffer::writeRow` (`jam_TerminalBuffer.cpp:438-454`).
2. Pack fixed-layout little-endian headers (`atoms.size`, `clusters.size`, `freeClusterIndices.size`, `fontIds.size`, `links.size`, `linkRuns.size`, `placements.size`, `outputBlock`, `gridStart`, `isWrapped`, `isFilled`) into contiguous local byte blocks using `juce::ByteOrder::littleEndian*` / `std::memcpy` and emit in minimal `output.write(...)` calls (one contiguous write for empty-metadata rows — the 99.99% case — instead of 14+ virtual `writeInt`/`writeInt64`/`writeBool` calls per row).
3. Keep the serialized byte stream **bit-for-bit identical** to the existing format so `AttributedString::read` (`jam_AttributedString.cpp:408-483`) and every `TerminalSwap` reader work unchanged on macOS and Windows.
**Inventory:** API rows 6, 10; Pattern usage row 6; Synthesis rows 2, 6, 7.
**Validation:** COUNSELOR reads `jam_AttributedString.cpp`, `jam_TerminalBuffer.cpp`, and `jam_TerminalSwap.cpp`; verifies exact byte-layout parity with `AttributedString::read`, endianness safety, and zero heap allocation.

### Step 7: Eliminate Vulkan Mid-Frame / Offscreen CPU-GPU Stalls (Cross-Platform)
**Scope:** `jam/jam_vulkan/context/jam_VulkanGraphics.h`, `jam/jam_vulkan/context/jam_VulkanGraphics.cpp`, `jam/jam_vulkan/resource/jam_VulkanImagePixelData.cpp`, `jam/jam_gui/widgets/jam_TextEditor.cpp` (exact subset pinned by Step 4 `whatdbg` trace)
**Action:**
1. Consume the exact call stacks captured in Step 4 (`whatdbg` on `VulkanGraphics::flushRecording`).
2. Fix the root design violation(s) proven by Step 4 (both macOS MoltenVK and Windows Vulkan):
   - If `reservePrimitiveRecords()` (`jam_VulkanGraphics.cpp:1191-1195`) triggers mid-frame `flushRecording()` due to `primitiveRecordCapacity == 1024` (`jam_VulkanGraphics.h:168`): size the primitive record buffer to hold a full viewport frame without mid-frame `submitAndWait()` stalls (using the existing `VulkanPrimitiveRecordBuffer` owner API).
   - If `TextEditor`'s `viewport.setBufferedToImage (true)` (`jam_TextEditor.cpp:28`) + `VulkanImagePixelData::needsBackup() = true, canBackup() = false` (`jam_VulkanImagePixelData.cpp:56-57`) forces redundant full-viewport offscreen `submitAndWait()` / `beginOffscreenFrame()` on every repaint: fix the ownership/caching contract so `TextEditor` paints directly into the window Vulkan pass (or Vulkan offscreen backup contract holds cleanly without per-frame invalidation and `vkWaitForFences` stalls).
3. Never patch around `flushRecording()` with a guard or flag — fix the capacity/ownership contract at the owner.
**Inventory:** API rows 12–13; Synthesis rows 2, 6.
**Validation:** COUNSELOR reads all modified files; verifies `vulkan flush recording` drops from `549` calls / `3.32 s` back to near-zero and zero visual regression in `TextEditor` rendering.

### Step 8: Fix `TerminalClock` Elapsed-Time Control Law and Verify Cross-Platform Convergence
**Scope:** `jam/jam_terminal/tty/jam_TerminalClock.cpp`, `eve/benchmark/seq-10m-mac-final*.md`
**Action:**
1. Fix `TerminalClock::getPeriodMilliseconds()` (`jam_TerminalClock.cpp:48-51`) so it subtracts elapsed processing time (`juce::Time::getMillisecondCounterHiRes() - tickStartMilliseconds`) from `periodMilliseconds` and returns `juce::jmax (0.0, periodMilliseconds - elapsed)`.
   - Effect: total unread accumulation window (`elapsed + wait`) equals `targetPeriod` instead of `elapsed + targetPeriod`, preventing systematic overshoot past `bufferSize / periods` into XNU's `1024 B` `TTYCLSIZE` wall on macOS and keeping Windows Cygwin's `128 KiB` pipe unchoked.
2. Run final sequential `scout` measurements (Live `How long`, Live `Where` + `cswitch`, Replay `How long`) against `seq-10m-mac-now-report.md` and `seq-10m-mac-report.md`.
3. Present exact `scout` numbers to ARCHITECT for Release `time seq 1 1000000` and `time seq 1 10000000` Verdict (plus Windows Cygwin verification). If any residual gap remains in `scout` `## sample` / `## cswitch`, stop and report the exact citation.
**Inventory:** API rows 14–15; Pattern usage row 7; Names row 5; Synthesis rows 2, 7.
**Validation:** COUNSELOR reads `jam_TerminalClock.cpp`, inspects all three final `scout` reports (`dropped == 0`, `Release`), and verifies `avg bytes/tick`, `terminal process`, `swap history lines`, `vulkan flush recording`, and `epoch` converge to the hardware floor.

### Step 9: Sprint Audit and Post-Audit Doxygen Pass
**Scope:** All modified and new files across `jam` and `eve` from this sprint
**Action:**
1. Run `@Auditor` ONCE over the entire sprint file set against `MANIFESTO.md`, `CODING.md`, `NAMES.md`, `LANGUAGE.md`, `SPEC.md`, and this locked PLAN.
2. Filter every Auditor finding through COUNSELOR's Design by Contract Filter; resolve every validated finding in this sprint via `@Engineer` and re-verify.
3. Delegate the dedicated post-audit doxygen pass (`doxygen-writer` skill) to `@Pathfinder` with exact headers, symbols, implementation files, and doxygen regeneration command; validate every written block against code.
4. Hold `/log` and temporary benchmark manifest cleanup for ARCHITECT's explicit instruction.
**Inventory:** Full Framework Inventory (Sections 1–5 above).
**Validation:** Zero unresolved validated Auditor findings; every doxygen block verified against code; clean build verified.

## BLESSED Alignment

- **B (Bound):** `jam::Utf8` owns pure stateless UTF-8 codecs; `jam::simd` owns vector byte kernels; `jam::AttributedString` owns its atom/cluster/link mutations; `jam::TerminalClock` owns the inter-tick timing law; `EVETerminalProcessor` owns its cached `Parameter` bindings. Zero floating state or raw owning pointers.
- **L (Lean):** Each helper stays under 30 lines and 3 branches; table lookups replace decision chains; `jam_Utf8.h` + `jam_Utf8.cpp` follows `LANGUAGE.md` one-object-one-TU placement.
- **E (Explicit):** Zero magic numbers (`Chars::*` and named `constexpr` bounds); positive nesting and `jassert` preconditions; zero bail-out guards; caller-visible output spans.
- **S (Single Source of Truth):** `Format::getUtf8Prefix` delegates to `Utf8::getPrefix`; `map::ansi` and `findControlByte` share one byte-class contract; Model parameters stay the sole state truth with zero shadow state.
- **S (Stateless):** `jam::Utf8` and `jam::simd` hold zero persistent state; `TerminalProcessor` emits transitions to the Model without caller polling.
- **E (Encapsulation):** Unidirectional layer flow (`jam_core` → `jam_terminal` → `eve`); zero `#include` in submodules; zero allocation/copy in transit on deadline threads.
- **D (Deterministic):** Same UTF-8 / VT / swap byte input produces bit-identical `AttributedString` rows, swap extents, and rendered frames across macOS ARM64 NEON, Windows x86_64 SSE2/SSSE3, and scalar fallback.

## Risks / Open Questions

None. All design choices trace directly to CONTRACT (`MANIFESTO.md`, `CODING.md`, `NAMES.md`, `LANGUAGE.md`), `SPEC.md`, and measured evidence from this session. Step 7's exact edit subset is pinned by Step 4's `whatdbg` trace before any Vulkan edit runs.
