# PLAN: Open and Gesture

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

**Contract for each step (binds this plan):** ARCHITECT builds and tests: *"you dont build. architect build and test runtime"*. *"no new semantics, no magic strings allowed anywhere, no handroll state management, no manual arithmetic, no new pattern, use framework API to its fullest extent, follow established pattern verbatim"*. *"framework API(s) are rich jam first then juce. you probably only need to use it OOTB. if there's genuienely no abstract as SSOT you can use, extend the existing."* Data in transit has *"0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state"* (`MANIFESTO.md:267`). *"we only use juce::String as Input or final Output materialisation to communicate with juce's API. otherwise we always use std::string/string_view everywhere."*

## Overview

A click on a link opens its target through one registered command, `openFile`. A drag starts VISUAL
only after 2 cells. The `## open with` table chooses what opens a file. A user adds any row, as in
endless.

ARCHITECT: *"the open file must be a registered command, so clicking, jump open, or pressing enter on
highlighted at LIST mode will call the exact SSOT method of `open file`"*; *"state changes trigger a
registered command"*; *"registrable callback isnt not new pattern, we use no `handlers` we have
commandManager"*; *"even endless could freely add any open with by extension to its config"*;
*"dragging in range of 2 or more cell then start VISUAL and select"*. Busy shell: *"Use the system
default app"*.

## Language / Framework Constraints

- `applyMouse` is 46 lines and must grow. Terminal bytes and the cell gesture are two responsibilities.
- A view write to a Model row: `winsize` (`jam_TextEditor.cpp:1062`), `background` (`EVEView.cpp:28`).

## Dependency & API Inventory

**JUCE:** `juce::URL` (`isLocalFile`, `getScheme`, `getLocalFile`, `launchInDefaultBrowser`).
`fileFromFileSchemeURL` makes a host the first path segment (`juce_URL.cpp:423-428`), so the host
leaves first. `juce::File::getFileExtension`, `getFileName`, `exists`, `startAsProcess`.
`juce::Point::getDistanceFrom`. `juce::String::fromFirstOccurrenceOf`, `replace`.

**JAM:** `CommandManager::getCommandForName`, `invokeDirectly`; `PluginEditor::perform` runs the
registry callback of a command name (`jam_PluginEditor.cpp:102-111`); `StyleManager::getConfig`;
`AttributedString::getLinkRuns`, `getLinks` (reader shape: `jam_MarkdownGraphics.cpp:429-443`).

**Lexicon:** `Id::url` exists (`core.md:158`). New rows: jam `openFile`, `any`; EVE `openWith`,
`failNoOpenFile`; `map::CommandId::openFile`. New names: `applyTerminalMouse`, `applyCellMouse`,
`applyLink`, `addInput`, `applyOpenCommand`, `appendQuoted`, `appendPath`, `openFile`, `getUriPath`,
`getOpenCommand`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: A config table that holds an `any` row takes any key
**Scope:** `jam/cast/core.md`, `jam/jam_markdown/document/jam_ConfigDocument.cpp`
**Action:** Add the `@id` row `any`. In `getTableFailures` (`:452-453`), the unknown key failure also
needs `not defaultTableTree.hasProperty (Format::toType (Id::any))`. The `any` row is the value of
every key that the table does not name.
**Validation:** This is the one rule that ARCHITECT's *"Free rows: change jam"* needs. A new property
survives the overlay with no other change: `Model::setValuesFrom` writes each source property
(`jam_Model.cpp:254-272`), and a property row is the property form (`jam_ConfigDocument.cpp:179-194`).
Tables with no `any` row keep the failure. A wrong key gives no error in this table, by ruling.

### Step 2: The open-with table
**Scope:** `eve/Source/layout/default-eve.md`, `eve/cast/identifiers.md`
**Action:** Add after `## keys`, in the grid format of `## window`:
```
## open with

What opens a file, by extension. A row with no extension in its key matches a file name, such as
makefile. A value that names a command runs that command. Any other value is a shell command, with
the file as its last argument. An empty value opens the file with the system default application,
and so does any shell command while a program runs in the terminal. Add any row.
```
Columns `key | type | value | description`, type `string`. One row per extension of `LinkDetector.h:84-97`,
less the image formats, with value `nvim`, plus `makefile`, `dockerfile`, and the row `any` with an
empty value. Add `openWith` and `failNoOpenFile` (`open with: no file `) to `eve/cast/identifiers.md`.
Add the `## CommandId` row `| openFile | 0x4600028 | `openFile` |`.
**Validation:** A disk file with no such table keeps the binary default (`jam_ConfigDocument.cpp:477`,
`PluginEditorLayout.cpp:71`). The version does not change (`SPEC.md:587`).

### Step 3: The gesture
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:**
1. Split `applyMouse` (`:1237-1283`). It becomes:
   ```cpp
   const auto cell { getCell (event.position.translated (0.0f, -getRowOffset())) };

   return applyTerminalMouse (type, button, cell, event.mods) or applyCellMouse (type, button, cell);
   ```
2. `bool applyTerminalMouse (TerminalMouse::Type type, int button, juce::Point<int> cell, juce::ModifierKeys mods);`
   holds lines `:1244-1258`, with `getMode()`.
3. `bool applyCellMouse (TerminalMouse::Type type, int button, juce::Point<int> cell);`:
   ```cpp
   static constexpr int selectionThreshold { 2 };
   const auto isVisual { getMode() == map::EditorMode::visual };
   auto isHandled { false };

   if (type == TerminalMouse::Type::press)
   {
       setCursor (cell.y, cell.x);
       state.setProperty (Id::anchorLine, cell.y, nullptr);
       state.setProperty (Id::anchorColumn, cell.x, nullptr);
       isHandled = true;
   }
   else if (type == TerminalMouse::Type::drag)
   {
       const juce::Point<int> anchor { static_cast<int> (state.getProperty (Id::anchorColumn)), static_cast<int> (state.getProperty (Id::anchorLine)) };

       if (isVisual or anchor.getDistanceFrom (cell) >= selectionThreshold)
       {
           if (not isVisual)
           {
               state.setProperty (Id::visualMode, map::VisualMode::getInstance()->get (map::VisualMode::character), nullptr);
               setMode (map::EditorMode::visual);
           }

           setCursor (cell.y, cell.x);
       }

       isHandled = true;
   }
   else if (type == TerminalMouse::Type::release and button == map::MouseButton::left and not isVisual)
   {
       applyLink (getCursor());
       isHandled = true;
   }

   return isHandled;
   ```
4. `void applyLink (juce::Point<int> cell);`:
   ```cpp
   const auto& cells { getCells (cell.y) };

   for (const auto& [run, ordinal] : cells.getLinkRuns())
   {
       if (run.contains (cell.x))
       {
           terminal.setProperty (Id::url, cells.getLinks().at (ordinal).uri, nullptr);

           if (const auto* command { commandManager->getCommandForName (Id::openFile) })
               commandManager->invokeDirectly (command->commandID, false);
       }
   }
   ```
**Validation:** The press writes the anchor, so the threshold adds no state. `getButton` is right on
release (`juce_MouseEvent.h:131-132`). A grid column indexes the merged cells
(`jam_TextEditor.cpp:1097-1126`). Live and history lines share `addLine` (`jam_TerminalSwap.cpp:205-217`).
A command, not a property event, so a second click on one link runs again.

### Step 4: The row and the PTY write
**Scope:** `eve/Source/layout/parameters.md`, `eve/Source/ShellIntegration.h`, `.cpp`,
`eve/Source/EVETerminalProcessor.h`, `.cpp`
**Action:**
1. Add the `## model` row `url | string | 1024 | | Target of the link that the view opens`.
2. Extend `jam::Format`, the family of `appendDecimal` and `appendUtf8`:
   `static void appendQuoted (std::string& output, std::string_view text, char quote, std::string_view escapedQuote);`.
   It appends `quote`, each byte of `text` with `escapedQuote` in place of `quote`, and `quote`.
3. Add `static constexpr std::string_view` constants to `ShellIntegration`, named `posixEscapedQuote`
   (`'\''`) and `powershellEscapedQuote` (`''`). Add `static void appendPath (std::string& output, const juce::String& shell, std::string_view path);`
   with the table shape of `getShellArguments` (`:73-93`), keyed `powershell` and `pwsh`.
4. Extract the `terminalInput` lambda body (`:188-195`) to private `void addInput (const char* bytes, int size);`.
5. Add public `void applyOpenCommand (std::string_view command, std::string_view path);`. It builds one
   `std::string`: the command, `Chars::space`, the quoted path, `Chars::carriageReturn`, and calls `addInput`.
**Validation:** The threat that the quote names: an OSC 8 URI comes from program output, and a double
quoted path runs `$()`. The input lane stays the one write (`SPEC.md:153`). `juce::String` meets only
the `shell` key, a JUCE value.

### Step 5: The command
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:**
1. `registerCallbacks`, in the shape of `:221-225`:
   ```cpp
   registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::openFile),
                                               [] (juce::Component& c)
                                               {
                                                   static_cast<EVEView&> (c).openFile();
                                               });
   ```
2. One path from a URI to a local path, for OSC 7 and OSC 8. In `jam_TerminalProcessor`:
   - Extract the lines of the OSC 7 lambda (`:970-979`) to a public
     `static std::string_view getUriPath (std::string_view uri);`. It keeps the Windows drive rule.
   - Make `applyToDecodedText` (`:938-945`) a public static template, defined in the header.
   - The OSC 7 lambda calls both, so the tick does not change.
3. Private `juce::String getOpenCommand (const juce::File& file) const;`:
   ```cpp
   const auto table { styleManager->getConfig (jam::Format::toType (Id::openWith)) };
   const auto extension { file.getFileExtension() };
   const auto key { jam::Format::toType (juce::Identifier { extension.isEmpty() ? file.getFileName() : extension }) };

   return table.getProperty (key, table.getProperty (jam::Format::toType (Id::any))).toString();
   ```
4. Private `void openFile();`:
   ```cpp
   const auto terminal { model.state.getChildWithName (jam::Format::toType (Id::terminal)) };
   const juce::URL url { terminal.getProperty (Id::url).toString() };

   if (url.isLocalFile())
   {
       jam::TerminalProcessor::applyToDecodedText (jam::TerminalProcessor::getUriPath (url.toString (false).toRawUTF8()), [this, &terminal] (std::string_view path)
       {
           openFile (path, terminal);
       });
   }
   else
   {
       url.launchInDefaultBrowser();
   }
   ```
5. Private `void openFile (std::string_view path, const juce::ValueTree& terminal);`:
   ```cpp
   const juce::File file { juce::String::fromUTF8 (path.data(), static_cast<int> (path.size())) };
   const auto value { getOpenCommand (file) };
   const auto* command { value.isEmpty() ? nullptr : commandManager->getCommandForName (juce::Identifier { value }) };
   const auto isBusy { static_cast<bool> (terminal.getProperty (Id::isRecording)) or static_cast<bool> (terminal.getProperty (Id::alternateScreen)) };

   if (file.exists())
   {
       if (command != nullptr) commandManager->invokeDirectly (command->commandID, false);
       else if (value.isEmpty() or isBusy) file.startAsProcess();
       else getTerminalProcessor().applyOpenCommand (value.toRawUTF8(), path);
   }
   else
   {
       messageOverlay.showMessage (Id::failNoOpenFile + file.getFileName().quoted (Chars::singleQuote));
   }
   ```
**Validation:** Click path, end to end: `applyCellMouse` → `applyLink` → row → `invokeDirectly` →
`PluginEditor::perform` → registry callback. A row value that names a command needs no new dispatch.
3 branches at most. The path stays a `std::string_view` from the row to the PTY. `juce::String` appears
at `juce::URL`, `juce::File` and `MessageOverlay` only. The decode buffer is `ParameterText::maxTextLength`,
the bound of the `url` row.

### Step 6: Docs
**Scope:** `SPEC.md:189`, `:311-320`, `PLAN-Grand-Scheme.md`
**Action:** COUNSELOR states the click, the table shape, the busy rule, and the gesture, with quotes.

### Step 7: Hand over, Step 8: Audit, Step 9: Doxygen pass
As PLAN-1 Steps 10 to 12.

## Acceptance (ARCHITECT builds and tests)

1. In TERMINAL, a click with a small slip does not enter VISUAL. A drag of 2 cells does.
2. `printf '\e]8;;file:///tmp/a.cpp\e\\a.cpp\e]8;;\e\\\n'`, then click: the shell runs `nvim '/tmp/a.cpp'`.
3. A link with `$(touch x)` in the path runs no substitution.
4. An `https://` link opens in the browser.
5. During `sleep 30`, the file click opens the system default application.
6. Add `.pdf | string | open -a Preview` to the disk table. It validates and hot reloads.

## BLESSED Alignment

- **SSOT:** one command, one link read, one input write, one table.
- **Stateless:** the threshold adds no state. `url` is one Model row.
- **Encapsulation:** the TextEditor writes a row and invokes a name. It knows no table.
- **Lean:** the table replaces a branch per type.

## Risks / Open Questions

- Closed: free rows, busy shell.
- A click writes after any text that the user typed. No read gives the prompt text to the view.
