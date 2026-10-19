# PLAN: Optional Panel, Settings Shortcut, One Settings Vocabulary

**RFC:** none — objective from ARCHITECT prompt (RFC-EVE-terminal-grand-scheme.md covers another objective)
**Date:** 2026-10-19
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (KANJUT mirror) — LANGUAGE.md C++/JUCE: one object, one translation unit (:33-35)
**On approval:** this file is copied to `/Users/jreng/Documents/Poems/dev/eve/PLAN-optional-panel-settings.md` (goplan rule). The last step is the log.

## Context

EVE has no `PANEL_HEIGHT` settings row (ViewLayout.md:54-60). Thus `AudioModel::getUIPanelHeight()` reads an absent entry and returns 0 (jam_AudioModel.cpp:124). The panel object still exists (EVEView.cpp:33-34) and gets bounds (jam_PluginEditor.cpp:47). The result is correct only by accident. ARCHITECT: *"without PanelLayout.html, determinisitically Panel should not be created at all, without breaking JFS, since the exact same plugin bootstrap will be synced across repo"*.

EVE keeps Settings, opened by a key: *"EVE still need 'Settings' … on demand created as now, but not via Button::Dialog, but Dialog created with keyboard shortcut"*.

The names follow one vocabulary: *"stick to EVERYTHING using 'Settings', only diverge with markdown vs xml"*.

## Rulings (ARCHITECT, verbatim)

1. Panel owner: **"Bootstrap builds it (Recommended)"**. The bootstrap builds the panel when `PanelLayout.html` exists. `initialisePanels()` and its overrides go. whelmed now, jfs at the KANJUT sync.
2. About: **"Delete About"**.
3. Geometry: **"Validator rule (Recommended)"**. A `PANEL_HEIGHT` settings row exists if and only if the panel layout resource exists.
4. Key: **"cmd+,"**.
5. Rename: **"This sprint"**. Vocabulary: *"stick to EVERYTHING using 'Settings', only diverge with markdown vs xml"*.
6. Tree root: **"Root `settings`"**.
7. Extension: **"Name only"**. `settingExtension` → `settingsExtension`; the value stays `.setting`.
8. KANJUT/jfs: **"This sprint"**.
9. Other projects: **"Derive, like XML lane"**. `ConfigDocument`, `ConfigValidator` and `Id::config` stay for end, ggwp, cast, sandbox and stamp.
10. Lane type: **"SettingsDocument<FormatDocumentType>"**; placement *"how about at plugin_bootstrap?"*.

## Dependency & API Inventory

- **JAM `BinaryData::Raw::exists()`** (jam_Raw.h:45): the one existence test of `PanelLayout.html`.
- **JAM `files` table** of jam_plugin_bootstrap (cast/plugin_bootstrap.md:203-204, rows `component layout`, `default settings`): the sibling for the new `panel layout` row. jam `files` is the global namespace `files` (jam_DataStructures.h:80).
- **JAM `map::CommandId`** (cast/gui.md:197-203, `zoomIn 0x4600001` … `zoomReset 0x4600003`): the sibling for `settings 0x4600004`. `CommandManager::setKeyPresses` maps `## keys` rows by type (jam_CommandManager.cpp:10-17).
- **JAM `ButtonDialog::Popup`** (jam_ButtonDialog.h:124-228) over `jam::ModalWindow` (jam_ModalWindow.h:63-66): the dialog lane, reused, not rewritten. `Popup` is the only `Popup` type in jam.
- **JAM `Registry::makeContent`**: `reg.makeContent.get (content, modelToUse, getLookAndFeel())` (jam_ButtonDialog.h:68). EVE, whelmed and jfs register `Id::settings` (EVEViewComponents.cpp:151-156, WHELMEDViewComponents.cpp:79, FilterStripViewComponents.cpp:268).
- **Dismiss on lost focus = false** for the settings dialog in all three projects (EVEViewRegistration.cpp:55, WHELMEDViewRegistration.cpp:55, FilterStripViewRegistration.cpp:353).
- **JAM document bases**: `XmlDocument : Document` (jam_Xml.h:11); `MarkdownDocument : Document` (jam_MarkdownDocument.h:21); `ConfigDocument : MarkdownDocument` (jam_ConfigDocument.h:19).
- **Module layers**: jam_plugin_bootstrap depends on jam_core, jam_data_structures, jam_markdown (jam_plugin_bootstrap.h:10-17). jam_data_structures depends on jam_core only (jam_data_structures.h:10-14).
- **Settings rules**: `PluginEditorLayout::getSettingsRules` per lane (jam_PluginEditorLayout.cpp:214-319). The XML sibling reads a row with `Xml::get<juce::String> (document, getSettingsKey().toString(), Id::toTag (Id::UIScale), Id::value)` (:247). The md sibling reads with `markdown.getTableRow (Id::settings, Id::toType (Id::UIScale))` (:285).
- **Module file lists**: `user-modules-info.md` `## submodule` rows generate the `BEGIN_SUBMODULE_INCLUDES` blocks (jam user-modules-info.md:177-180; SettingsDocument rows :242, :261; ModalWindow :978, :1028; ButtonDialog :983; PluginEditorLayout :1068, :1077). KANJUT: kuassa user_modules/user-modules-info.md:125-128.
- **Commands**: jam regenerate `cast cast/spell.md` (jam .claude/CLAUDE.md:106). KANJUT sync `cast --sync ~/Documents/Poems/dev/jam ~/Documents/Poems/kuassa/user_modules` (kuassa carol/SPRINT-LOG.md:798). EVE / whelmed / jfs regenerate and build `cast cast/spell.md --debug` (eve CLAUDE.md Build 2; kuassa SPRINT-LOG.md:97).
- **Consumers of renamed symbols** (Pathfinder, verified): eve, whelmed (jam); jreng-filter-strip (KANJUT). end, ggwp, cast, sandbox, stamp use `ConfigDocument` / `ConfigValidator` / `Id::config` only, and those stay (ruling 9).

## Validation Gate

COUNSELOR validates each step before the next — against MANIFESTO.md (BLESSED), NAMES.md, CODING.md, and the locked rulings above. No deviation, no scope drift. Every Engineer prompt carries Design by Contract per CODING.md CRITICAL RULES, the MVP data-flow contract verbatim, doxygen-first, no comments, no doxygen, and an explicit file set. @Auditor runs ONCE, after the final code step.

## Steps

### Step 1: jam cast tables
**Scope:** jam `cast/plugin_bootstrap.md`, `cast/gui.md`, `cast/data_structures.md`; jam `generated/`.
**Action:**
1. `plugin_bootstrap.md` `## files`: add `| @string | panel layout | toCamel | PanelLayout.html | toLiteral | Top/bottom bar layout. |` after `component layout` (:203). Description from jfs Files.h:30.
2. `gui.md` `## CommandId`: add `| settings | 0x4600004 | \`settings\` |` after `zoomReset` (:202).
3. `data_structures.md:163`: `setting extension` → `settings extension`; value `.setting` unchanged.
4. Run jam `cast cast/spell.md`.
**Validation:** generated `files::panelLayout`, `map::CommandId::settings`, `files::settingsExtension` exist; no other generated diff; `Id::config` row (core.md:63) untouched (ruling 9).

### Step 2: Popup leaves ButtonDialog
**Scope:** jam `jam_gui/buttons/jam_ButtonDialog.h`; new `jam_gui/windows/jam_Popup.h`, `jam_Popup.cpp`; `user-modules-info.md` rows after :978 and :1028.
**Action:** Delete the nested `Popup` from ButtonDialog first. Then add `jam::Popup` as one object (one header, one `.cpp`, bodies from jam_ButtonDialog.h:126-228 verbatim). `~Popup()` calls `dismiss()` (the owner dismisses; RAII). ButtonDialog holds `Popup popup;` and its destructor becomes `= default` (was `{ popup.dismiss(); }`, :21). Add the two manifest rows; run jam `cast cast/spell.md`.
**Validation:** no behaviour change in ButtonDialog; no manual RAII in any destructor (CODING); no new names beyond `Popup` (existing name, NAMES Rule 5).

### Step 3: `SettingsDocument<FormatDocumentType>` in jam_plugin_bootstrap
**Scope:** delete jam `jam_core/xml/jam_SettingsDocument.h/.cpp` and rows :242, :261. New `jam_plugin_bootstrap/layout/jam_SettingsDocument.h/.cpp` with rows beside :1068/:1077. Delete `getSettingsValue` / `setSettingsValue` from `jam_ConfigDocument.h:85-107` / `.cpp:303-349`.
**Action:**
1. `template <typename FormatDocumentType> struct SettingsDocument : FormatDocumentType` declares `parse (const juce::String& text, const juce::String& origin)`, `getVersion() const`, `getSettingsValue (const juce::File&, juce::StringRef)`, `setSettingsValue (const juce::File&, juce::StringRef, const juce::String&)`.
2. Explicit specializations for `XmlDocument` (bodies from jam_SettingsDocument.cpp:5-53; `getSettingsValue` parses with `settingsFile.getFullPathName()` as origin) and `ConfigDocument` (parse body from jam_ConfigDocument.cpp:5-10; `getVersion` returns `ConfigDocument::getVersion()`; settings bodies from jam_ConfigDocument.cpp:303-349). Declarations in the header, definitions in the one `.cpp`.
3. Run jam `cast cast/spell.md`.
**Validation:** one object, one TU (CODING); jam_core and jam_markdown carry no settings-lane code; `ConfigDocument` keeps its general API (ruling 9).

### Step 4: ParameterManager takes the lane functions
**Scope:** jam `jam_data_structures/parameter/jam_ParameterManager.h/.cpp`.
**Action:** Delete ctor 1 (jam_ParameterManager.h:23, .cpp:4-8; zero construction sites). Ctor 2 becomes `(ValueMap newDefaultValues, ValueMap newUnits, std::function<…> newGetSettingsValue, std::function<…> newSetSettingsValue, juce::String newSettingsExtension = files::settingsExtension)`; it binds the two functions as ctor 3 does (.cpp:16-17). `newSettingExtension` → `newSettingsExtension`.
**Validation:** jam_data_structures references no settings document type (layer, MANIFESTO E); parameter names from ctor 3 (jam_ParameterManager.h:41-44).

### Step 5: PluginEditorLayout on the Settings vocabulary
**Scope:** jam `jam_plugin_bootstrap/layout/jam_PluginEditorLayout.h/.cpp`.
**Action:**
1. `getOrCreateConfig` → `getOrCreateSettings` (public and template); member pointer `config` → `settings`; `getDefaultConfig` → `getDefaultSettings`; every `config` parameter or local → `settings`.
2. Lane overloads take `SettingsDocument<XmlDocument>` / `SettingsDocument<ConfigDocument>` (:131, :140, :154, :161, :170, :179; instantiations .cpp:39-40).
3. Root: `Id::toType (Id::config)` → `Id::toType (Id::settings)` (.cpp:488, :511, :523).
4. Settings rule `Id::panelHeight.toString()` in both `getSettingsRules`: the row exists if and only if `BinaryData::Raw { files::panelLayout }.exists()`. XML reads the row as at :247; md as at :285.
**Validation:** the rule is the one owner of the invariant (MANIFESTO D); no geometry code changes; dispatch stays by document type.

### Step 6: PluginEditor — optional panel, settings command
**Scope:** jam `jam_plugin_bootstrap/view/jam_PluginEditor.h/.cpp`.
**Action:**
1. Delete pure virtual `initialisePanels()` (.h:186). `initialise (const juce::ValueTree& settings)` builds the panel in a positive check of `BinaryData::Raw { files::panelLayout }.exists()`: `panel = ViewPanel::create (model, HtmlDocument::getOrCreate (juce::Identifier { files::panelLayout }));` then `addAndMakeVisible (panel.get());`.
2. `resized()` (.cpp:47) and `onSetState` (.cpp:166): the panel call sits inside `if (panel != nullptr)`.
3. `perform()`: the `map::CommandId::settings` command calls private `launchSettings()` (verb from `ButtonDialog::launchDialog`, jam_ButtonDialog.h:82). The zoom path is unchanged. `launchSettings()` shows `registry->makeContent.get (Id::settings.toString(), model, getLookAndFeel())` in member `Popup popup`, centred on `*getTopLevelComponent()`, at the content size, dismiss on lost focus `false` (named constant in function scope).
4. Rename `initialise`/`initialiseTheme`/`fileChanged` `config` names → `settings`; `getOrCreateConfig` call → `getOrCreateSettings`.
**Validation:** two positive checks, no bail-out (CODING); ≤3 branches in `perform`; no manual RAII; names only from rulings and families.

### Step 7: KANJUT sync
**Scope:** `~/Documents/Poems/kuassa/user_modules`.
**Action:** Run `cast --sync ~/Documents/Poems/dev/jam ~/Documents/Poems/kuassa/user_modules`; then KANJUT `cast cast/spell.md`.
**Validation:** kuassa mirrors Steps 1-6 (Popup, SettingsDocument template, ParameterManager, PluginEditorLayout, PluginEditor, cast rows); diff limited to the sprint files.

### Step 8: EVE
**Scope:** `Source/layout/PanelLayout.html`, `AboutLayout.html`, `Source/images/JRENG_logo.png`, `Source/buttons/settings_{normal,over,down}.svg` (delete); `cast/files.md`, `cast/identifiers.md`, `cast/lookuptables.md` (delete), `cast/spell.md`, `project-info.md`; `Source/EVEView.h/.cpp`, `EVEViewPanelCallbacks.cpp`, `EVEViewRegistration.cpp`, `EVEViewComponents.cpp`, `EVEProcessor.h/.cpp`; `Source/layout/ViewLayout.md`.
**Action:**
1. Delete: the files above; `files.md` rows `panel layout` (:25), `about layout` (:29), `settings normal/over/down` (:39-43); `lookuptables.md` and its `spell.md` rows (:21, :28, :146-155, and the `LookupTables.h` include row of `Generated.h`); `project-info.md:408` `images` glob row; identifier rows whose only uses go (`jrengLogo`, `aboutBox`, `buttonDialog`, `animationStrip`, `developer`, `jreng`), each confirmed by a read.
2. Delete `EVEView::initialisePanels`, `attachAboutDialog` and the About part of `attachPanelCallbacks` (EVEViewPanelCallbacks.cpp:11-48; the `UIScale` callback :5-9 stays). Delete the settings ButtonSVG (EVEViewRegistration.cpp:8-16), ButtonLogo (:35), ButtonDialog registration and config (:36, :51-56), AnimationStrip and ScrambledText registrations and configs and the `aboutBox` makeContent (EVEViewComponents.cpp:67-68, :110-135, :145-149).
3. Rename `files.md` `config directory` → `markdown settings directory`, `default config` → `markdown settings file name` (ruling 5: md diverges by format). `EVEProcessor.h:59` → `create<jam::SettingsDocument<jam::ConfigDocument>>`; `:61` → those constants and `&jam::SettingsDocument<jam::ConfigDocument>::getSettingsValue` / `setSettingsValue`. `getOrCreateConfig` → `getOrCreateSettings` (EVEProcessor.cpp:16, EVEView.cpp:9); `initialiseTheme` parameter → `settings`.
4. `ViewLayout.md`: `## keys` row `| settings | string | cmd+, | | Open the settings dialog. |`; version `0.0.1` → `0.0.2` (re-seeds eve.md).
5. Run `cast cast/spell.md --debug`.
**Validation:** no panel object; Settings opens by `cmd+,`; no dead registration, row or asset; `.config/end` literal unchanged (XDG).

### Step 9: whelmed
**Scope:** `whelmed/cast/files.md`, `Source/WHELMEDView.h/.cpp`, `Source/WHELMEDProcessor.h/.cpp`.
**Action:** Delete `files.md` row `panel layout` (collides with `files::panelLayout`) and `WHELMEDView::initialisePanels`. Same renames as Step 8.3. Run `cast cast/spell.md --debug`.
**Validation:** panel still built (PanelLayout.html present); rule passes (ViewLayout.md:64 has `PANEL_HEIGHT`).

### Step 10: jfs
**Scope:** `jreng-filter-strip/cast/files.md`, `Source/FilterStripView.h/.cpp`, `Source/FilterStripProcessor.h/.cpp`.
**Action:** Delete `files.md` row `panel layout` and `FilterStripView::initialisePanels` (FilterStripView.cpp:132-136, .h:132). `FilterStripProcessor.h:174` → `create<kuassa::SettingsDocument<kuassa::XmlDocument>>`; `:177-178` ctor 2 adds `&kuassa::SettingsDocument<kuassa::XmlDocument>::getSettingsValue` / `setSettingsValue`. `getOrCreateConfig` → `getOrCreateSettings` (FilterStripProcessor.cpp:34, FilterStripView.cpp:22); `initialiseTheme` parameter → `settings`. Run `cast cast/spell.md --debug`.
**Validation:** panel still built; rule passes (DefaultSettings.xml:10 has `PANEL_HEIGHT`); on-disk `.setting` unchanged.

### Step 11: Audit, doxygen, log
@Auditor sweeps the sprint once. COUNSELOR resolves every validated finding. Dedicated doxygen pass (headers only, zero warnings). Then `/log` and the commit messages in chat.

## Verification

1. Builds: jam generation, KANJUT sync, then `cast cast/spell.md --debug` in eve, whelmed and jreng-filter-strip — 0 errors, 0 warnings. Approval of this plan is the build request for these commands.
2. ARCHITECT runs EVE Standalone: no panel row; `cmd+,` opens the Settings dialog; APPEARANCE and UI_SCALE write to `~/.config/end/eve.md`; Escape and the close button dismiss.
3. ARCHITECT runs whelmed and jfs: the panel shows, Settings and About open from the panel buttons as before.

## BLESSED Alignment

- **B:** the editor owns the panel and the `Popup`; `Popup` dismisses in its own destructor.
- **L:** one template replaces two lane APIs; `perform` stays ≤3 branches.
- **E:** existence is one `Raw::exists()` test; no bail-out; named constant for the dismiss flag.
- **S (SSOT):** one panel-file name (jam `files::panelLayout`); one `Popup`; one settings vocabulary.
- **S (Stateless):** no flag tracks panel existence; `panel` ownership is the fact.
- **E (Encapsulation):** jam_data_structures no longer knows a settings document type.
- **D:** panel geometry is 0 by validated contract, not by an absent row.

## Risks / Open Questions

None. All points closed by the rulings above.
