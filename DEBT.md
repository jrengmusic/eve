# DEBT — Inter-Sprint Ledger

## DEBT-20260908T000000 — Editor lane join: component.md z/primary + ViewLayout.svg numbered ids

**Captured:** 2026-09-08, ARCHITECT directive, jfs sprint editor-lane-z-index in flight.
**Precondition:** none — enters the next EVE sprint scope (JRENG law).

The jam editor lane now joins by number (jam_PluginEditorLayout.h `addComponents`,
jam_ViewManagerEditor.cpp `build`/`make`/`bindComponent`): `MarkdownDocument
(files::componentLayout)` must carry a `## Component` table with columns
`z | type | parameter | image | dark | style | primary | event`, `z` 0-based and
contiguous in row order (= juce child index = z-order), `primary` = the z of the
owner, composite children as a block-quoted list in the type cell with every
other cell's list keyed `> - childType: value`; the view SVG's rects carry
`id`/`serif:id` = `NN-type-PARAM` whose leading number joins the row. The
`event` cell holds the registered chain-events key value (jfs: `AUTO_LISTEN`).
Validator rules `rect`, `z`, `primary`, `li` (jam_PluginEditorLayout.h `getRules`)
reject anything else.

EVE today: `Source/layout/` holds `DefaultSettings.xml`, `style.css`,
`ViewLayout.md` — no `component.md`, no view SVG. Author both to the convention
above before EVE builds against current jam.

## DEBT-20260908T000001 — Processor registries: getter and chain-event types

**Captured:** 2026-09-08, ARCHITECT directive, jfs sprint editor-lane-z-index in flight.
**Precondition:** none — enters the next EVE sprint scope (JRENG law).

`jam::ViewManager::Getters` is now `jam::HashMap<juce::String,
Function::Map<juce::Identifier, void>>` (inner map keyed by `juce::Identifier`)
and `jam::ViewManager::Events` is `jam::HashMap<juce::String,
Function::Array<void>>` (one callback array per event key, built once at
registration). `EVEAudioProcessor.h:10` (`userInterfaceGetters`) and `:12`
(`chainEvents`) still declare the old `Function::Map<juce::String, void>` shapes;
`EVEView.cpp:37` passes both to `ViewEditor::create`. Retype both members and
their registration sites to match jam before EVE builds.
