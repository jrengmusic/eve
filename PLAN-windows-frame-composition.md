# PLAN: windows-frame-composition

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-16
**BLESSED Compliance:** verified against the reads cited below
**Language Constraints:** C++17 / JUCE 8.0.17 (patched, `eve/CMakeLists.txt:139`, `:144-158`) / JAM / Vulkan 1.2 (`jam_VulkanDevice.h:49`) / D3D11 + DXGI + DirectComposition (Windows SDK 10.0.26100.0)

## Context

ARCHITECT's direction, verbatim:
- *"fix the design, design by contract."*
- *"these changes should be on windows only, because on mac i have never had any issues with speed, nor blocking message thread"*
- *"NON NEGOTIABLE message thread should never blocked while keeping the adherence to LOCK-FREE architecture"*
- *"although if there was genuine defects tha violates contract must be fixed, if any."*
- *"review the plan with THOROUGH INVENTORY, jam, juce, vulkan. jam always priority over juce"*

ARCHITECT's rulings:

| # | Ruling (verbatim) | Decides |
|---|---|---|
| W1 | *"All three objects"* — move jam_VulkanComposition, jam_VulkanCompositionDevice and jam_VulkanSharedTextureExport to `context/native/*_windows.*` | Step 1 |
| W2 | *"JUCE D2D pattern (Recommended)"* — ExternalRenderContext follows D2DRenderContext: WM_PAINT validates and schedules, onVBlank paints | Step 2 |
| W3 | *"Shared fence, Mac order"* — D3D11 shared fence imported as a Vulkan timeline semaphore; `ID3D11DeviceContext4::Wait` before CopyResource; `jam_VulkanGraphics.cpp:499-500` goes, `:556` stays | Steps 3-4 |
| W4 | *"Include in Step 7"* — five contradicting doc blocks join the doxygen pass | Step 8 |
| W5 | *"Fix code"*, then *"fix the code then"* — Windows `BackgroundBlur::enable` applies the style it is given. Superseded by W6. | Step 5 |
| W6 | *"windows window glass always acrylic and blur behind. DEFAULT always"*; *"what's the point of data? what's the point of solid window on glass?"*; *"fix both code and docs"* — Windows has one glass style, acrylic. The blur-behind accent drew an opaque black background at runtime. | Step 5 |

The outcome: on Windows the Vulkan frame runs on VBlank, as JUCE's own Direct2D context does and as the Mac lane does. The composition path syncs D3D11 to Vulkan on the GPU, in Mac order. The Windows-only Vulkan objects live in `native/`. Windows glass applies its configured style.

## Evidence (read this session)

**Frame drive.**
- **Stock JUCE.** `D2DRenderContext::handlePaintMessage` validates and schedules (`juce_Windowing_windows.cpp:4966-4974`), and `onVBlank` paints (`:4989-4993`, `:5314-5358`).
- **Our `ExternalRenderContext`.** It paints only inside WM_PAINT (`:5414-5417`, `:5483-5497`), and its `onVBlank` is empty (`:5432`).
- **Learn** (About Messages and Message Queues): WM_PAINT messages *"are forwarded to the window procedure only when the queue contains no other messages."*
- **JUCE's queue.** One `PostMessage` wakes the loop, and one wake drains the whole batch (`juce_Messaging_windows.cpp:91-112`, `:228-247`).
- **Mac.** `onVBlank` calls `setNeedsDisplayInRect` (`juce_NSViewComponentPeer_mac.mm:1097-1124`).
- **Measured** (`benchmark/seq-100k-windows-report.md`): `editor blocks` runs 1,370 times (`:93`) and `vulkan end frame` 536 times (`:99`), per epoch.

**JAM has no frame driver of its own on the OS-window path.**
- Its window code adds no WM_PAINT handling: no hit in JAM for `WM_PAINT`, `WndProc` or `GWLP_WNDPROC`.
- JAM's VBlank users are JUCE `VBlankAttachment`s (`jam_TextEditor.h:1530`, `jam_PreviewComponent.h:541`).
- So the frame drive is a JUCE-patch change.

**Composition sync.**
- **Windows composition today:** submit, host wait (`jam_VulkanGraphics.cpp:497-504`), CopyResource + Present (`:534`, `jam_VulkanComposition.cpp:101-103`), then `:556`.
- **Mac:** submit, present, `:556` (`:517`, `:539-542`, `:556`).
- **Why `:556` stays.** It keeps the next frame's `resetFences` (`:474`) after this frame's completion on both paths.
- **Learn:**
  - `ID3D11DeviceContext4::Wait`: *"Waits until the specified fence reaches or exceeds the specified value before future work can begin."*
  - `ID3D11Fence::CreateSharedHandle`: *"the fence must have been created with either the D3D11_FENCE_FLAG_SHARED or D3D11_FENCE_FLAG_SHARED_CROSS_ADAPTER flags"*, and *"Currently the only value this parameter accepts is GENERIC_ALL."*
- **Vulkan:**
  - `VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D11_FENCE_BIT` is *"an alias of … D3D12_FENCE_BIT with the same meaning"* (`vulkan_core.h:5662`).
  - VkD3D12FenceSubmitInfoKHR: *"applications should prefer importing such handle types into semaphores created with a VkSemaphoreType of VK_SEMAPHORE_TYPE_TIMELINE"*.

**Glass.**
- **Windows `BackgroundBlur::enable`** ignores `style`. It picks acrylic on build 22000 or later, and blur-behind otherwise (`jam_BackgroundBlur_windows.cpp:89-107`).
- **The header** documents `@param style  Transparency style to apply.` (`jam_BackgroundBlur.h:99-113`).
- **How the style is chosen.** `StyleTheme::prepareWindow` resolves it with `BackgroundBlur::fromString (getWindowBackendString())` (`jam_StyleTheme.cpp:280`).
- **What EVE gets.** EVE's layout value is `acrylic10` (`default-eve.md:230`). That is not a Windows key; the keys are `blurBehind` and `acrylic` (`jam/cast/gui.md:203-208`). So `fromString` returns the first entry, `blurBehind` (`jam_BackgroundBlur.h:168-170`, `jam_Bimap.h:221-225`).

## Language / Framework Constraints
- **LANGUAGE.md C++/JUCE: one object, one translation unit** (`:34`). The moved objects keep their header and `.cpp`.
- **The 30-line and 3-branch limits apply** (`:36`).
- **Windows only.**
  - Code changes are inside `#if JUCE_WINDOWS` regions, in Windows-only files, or in Windows hunks of JUCE patches.
  - The only shared lines that change are include paths: `jam_vulkan.h:115-117` and `jam_vulkan.cpp:17-18`.
- **JAM before JUCE (ARCHITECT).**
  - Each JAM-side API below is the JAM pattern found by the inventory.
  - Inside a JUCE patch only JUCE types exist, so the patch uses JUCE's own Direct2D-context types.
- **JUCE patches are diffs against JUCE 8.0.17.**
  - `jam/cast/juce.cast:44-64` applies them with `git apply --check`, then `git apply`.
  - The patch-set hash (`eve/CMakeLists.txt:167-178`) gives a new patched copy at the next configure.
- **Builds** run only through `cast` or the scout manifest's `## toolchain` rows.
- **Hygiene.**
  - No comment and no doxygen before the audit.
  - Design by Contract per CODING.md CRITICAL RULES.
  - Engineer runs no git and deletes files with `rm`.

## Dependency & API Inventory

**JAM (priority)**

| Need | JAM pattern used | Cite |
|---|---|---|
| Owned Win32 HANDLE | `std::unique_ptr<void, decltype (&CloseHandle)>` | `jam_TerminalFIFO_windows.cpp:35`, `jam_ChildProcess_windows.cpp:24`, `jam_TerminalTTY_windows.h:261-279`, `jam_AsyncOutputStream_windows.cpp:217-219` |
| COM pointer | `juce::ComSmartPtr<…>` used directly (JAM has no wrapper; 13 files) | `jam_VulkanCompositionDevice.h:19-31`, `jam_VulkanComposition.h:64-70` |
| COM out-parameter | `resetAndGetPointerAddress()` | `jam_VulkanComposition.cpp:61` |
| Vulkan semaphore creation | `auto [semaphoreResult, createdSemaphore] { device.getDevice().createSemaphore (info) };` | `jam_VulkanSwapchain.cpp:21-27` |
| Vulkan NT-handle import | `importCompositionImage` → `VulkanImage::importD3D11` | `jam_VulkanGraphics.cpp:2096-2106`, `jam_VulkanImage.cpp:79-158` |
| Optional device extension | `optionalDeviceExtensions` + `isDeviceExtensionSupported` | `jam_VulkanDevice.cpp:8-19`, `:301-314` |
| Timeline semaphores | enabled already: `.setTimelineSemaphore (vk::True)` | `jam_VulkanDevice.cpp:286`, `:316` |
| Extension dispatch | `vk::raii::Device` dispatcher; asserts *"requires <VK_KHR_external_semaphore_win32>"* | `vulkan_raii.hpp:1093`, `:21251-21262` |
| Failure log | `jam::debug::Log::write (…)` then `deferDeviceReinitialise()` | `jam_VulkanComposition.cpp:105-110` |
| Native file layout | `<dir>/native/jam_<Object>_windows.{h,cpp}`, included side by side, self-guarded | `jam_terminal.h:111-115`, `jam_terminal.cpp:27-28` |
| Module file table | `user-modules-info.md` rows | `:1182-1184`, `:1226-1227` |

**JUCE (inside the patch only)**
- `RenderContext` (`juce_Windowing_windows.cpp:1169-1189`).
- `D2DRenderContext` (`:4940-5395`): `handlePaintMessage` `:4966-4974`, `onVBlank` `:4989-4993`, `handleShowWindow` `:4995-4999`, `handleDirect2DPaint` `:5333-5343`, members `:5371-5372`.
- `WrappedD2DHwndContextTransparent::deferredRepaints` (`:5311`); clear on success (`:5225-5228`).
- `UpdateRegion` (`juce_DirectX_windows.h:225-245`, `.cpp:363-411`). `findRECTAndValidate` validates the region with `ValidateRgn` / `ValidateRect`.
- `D2DUtilities::toRectangle` (`:4971`).
- The paint-message callers stay the same, and all reach `handlePaintMessage`:
  - WM_PAINT (`:3721-3723`), WM_NCPAINT (`:3725-3730`);
  - `repaintNowIfTransparent`, for `perPixel` peers after a resize (`:1286-1294`, `:4233`);
  - WM_SHOWWINDOW calls `handleShowWindow` (`:3978-3988`).
- `HWNDComponentPeer::onVBlank` calls the listeners first, then `renderContext->onVBlank()` (`:1659-1665`).
- **The factory can be cleared.** `setGpuAvailable (false)` sets `externalContextFactory` to `nullptr` on Windows (`jam_VulkanGraphicsEngine.cpp:302`), then calls `component.repaint()` (`:490-494`). The engine switch-back runs in `HWNDComponentPeer::handlePaintMessage` (`:5474-5475`).
- **The JAM consumer.** `peer.getLastPaintUpdateRect()` (`jam_VulkanGraphicsEngine.cpp:465`) is the only use. It takes physical pixels, and an empty rect records the full window (`:467-468`).

**Vulkan / D3D11 (SDK headers read)**
- `vk::SemaphoreTypeCreateInfo` with `vk::SemaphoreType::eTimeline`.
- `vk::ImportSemaphoreWin32HandleInfoKHR` with `vk::ExternalSemaphoreHandleTypeFlagBits::eD3D11Fence` (`vulkan_enums.hpp:4763`).
- `vk::TimelineSemaphoreSubmitInfo`.
- `ID3D11Device5::CreateFence` (`d3d11_4.h:1037-1045`), `D3D11_FENCE_FLAG_SHARED` (`d3d11_3.h:2206`), `ID3D11Fence::CreateSharedHandle` (`d3d11_3.h:4180-4188`), `ID3D11DeviceContext4::Wait` (`d3d11_3.h:4357-4361`).
- The composition device already holds `ID3D11Device5` and `ID3D11DeviceContext4` (`jam_VulkanCompositionDevice.h:21`, `:23`).

**New names (each from an established family, NAMES.md Rule 5)**

| Name | Family |
|---|---|
| `context/native/jam_VulkanComposition_windows.{h,cpp}`, `jam_VulkanCompositionDevice_windows.{h,cpp}`, `jam_VulkanSharedTextureExport_windows.h` | `tty/native/jam_TerminalTTY_windows.{h,cpp}` |
| `HWNDComponentPeer::handleExternalPaint`, `ExternalRenderContext::handleExternalPaint` | `handleDirect2DPaint` (`:5314`), `handleExternalPaintMessage` |
| `ExternalRenderContext::updateRegion`, `deferredRepaints`, `schedulePaintOnVblank`, local `isPainted` | `D2DRenderContext` `:5371-5372`, `:5311` |
| `VulkanComposition::sharedFence`, `initialFenceValue` | `sharedTexture`; `numSwapchainBuffers`, `presentSyncInterval` |
| `VulkanGraphics::importCompositionFence`, `compositionSemaphore`, `compositionFenceValue` | `importCompositionImage`; `compositionImage`, `compositionFramebuffer` |
| locals `fenceExport`, `textureExport` (owned) | `ownToken` (`jam_ChildProcess_windows.cpp:24`), `section` (`jam_TerminalFIFO_windows.cpp:35`) |

## Validation Gate
COUNSELOR validates each step before the next one. The checks are:
- MANIFESTO.md (BLESSED)
- NAMES.md
- ~/.carol/CODING.md
- this PLAN, with no deviation and no scope drift.

@Auditor runs once, after the final code step. It never runs per step.

## Steps

### Step 0: Write the plan to the EVE root
**Scope:** `D:\Poems\dev\eve\PLAN-windows-frame-composition.md` (replaced whole).
**Action:** COUNSELOR copies this approved plan there.
**Validation:** the file content equals the approved plan.

### Step 1: Move the three Windows-only objects into `context/native/` (W1)
**Scope:** JAM `jam_vulkan/context/`, `jam_vulkan/jam_vulkan.h`, `jam_vulkan/jam_vulkan.cpp`, `user-modules-info.md`.
**Action:**
1. Create these files byte-equal to their old files:
   - `context/native/jam_VulkanCompositionDevice_windows.{h,cpp}`;
   - `context/native/jam_VulkanSharedTextureExport_windows.h`;
   - `context/native/jam_VulkanComposition_windows.{h,cpp}`.
2. `rm` the five old files.
3. In `jam_vulkan.h:115-117` and `jam_vulkan.cpp:17-18`, change only the paths. Keep the order, the `///<` text and the absence of a guard (`jam_terminal.h:111-115`).
4. In `user-modules-info.md:1182-1184` and `:1226-1227`, change only the file column.

**Validation:**
- The new files are byte-equal to the old ones.
- No old file remains.
- Only the paths change.
- Each file still starts with `#if JUCE_WINDOWS`.

### Step 2: ExternalRenderContext follows the D2DRenderContext pattern (W2)
**Scope:** JAM `patch/juce-vulkan-engine-hook.patch` and `patch/juce-paint-update-rect-hook.patch`, Windows hunks only. The Mac hunk (`juce-vulkan-engine-hook.patch:1-15`) and the Linux hunk (`:16-36`) do not change.

**Action, `juce-vulkan-engine-hook.patch`:**
1. **Declaration.** At `:45`, `void handleExternalPaintMessage();` becomes `bool handleExternalPaint (Rectangle<int> updateRect);`.
2. **`ExternalRenderContext`** (`:152-193`), mirroring `D2DRenderContext`:
   - **`handlePaintMessage`:**
     1. `updateRegion.findRECTAndValidate (peer.getHWND());`
     2. `for (const auto& rect : updateRegion.getRects()) deferredRepaints.add (D2DUtilities::toRectangle (rect));`
     3. `schedulePaintOnVblank = true;`
   - **`onVBlank`:** `if (std::exchange (schedulePaintOnVblank, false)) handleExternalPaint();`
   - **`handleShowWindow`:** `handleExternalPaint();` (the D2D form at `:4995-4999`).
   - **`updateConstantAlpha`, `repaint`, `performAnyPendingRepaintsNow` and `createSnapshot`:** unchanged.
   - **Private `void handleExternalPaint()`** (the `handleDirect2DPaint` result form at `:5333-5343`, and the clear-on-success at `:5225-5228`):
     1. `const auto isPainted { peer.handleExternalPaint (deferredRepaints.getBounds()) };`
     2. `if (isPainted) deferredRepaints.clear();`
     3. `schedulePaintOnVblank = not isPainted;`
   - **Private members:** `UpdateRegion updateRegion;`, `RectangleList<int> deferredRepaints;`, `bool schedulePaintOnVblank = false;`.
3. **`HWNDComponentPeer::handleExternalPaintMessage`** (`:229-238`) is replaced by `bool HWNDComponentPeer::handleExternalPaint (Rectangle<int> updateRect)`:
   - When `externalContextFactory != nullptr` and the factory gives a context, it calls `handlePaint (*ctx)` and returns `true`. Otherwise it returns `false`.
   - The guard names its threat: `setGpuAvailable (false)` clears the factory between WM_PAINT and the VBlank (`jam_VulkanGraphicsEngine.cpp:302`). The `component.repaint()` that follows (`:493`) brings a WM_PAINT, and its `handlePaintMessage` switches the engine (`juce_Windowing_windows.cpp:5474-5475`).
   - It has no `BeginPaint` and no `EndPaint`, because `handlePaintMessage` validated the region, as `D2DRenderContext` does.
4. **Hunk headers.** Update the `@@` counts of each edited hunk.

**Action, `juce-paint-update-rect-hook.patch`:**
1. The `juce_Windowing_windows.cpp` hunk (`:26-40`) changes:
   - its context line becomes `bool HWNDComponentPeer::handleExternalPaint (Rectangle<int> updateRect)`;
   - its one added line is `lastPaintUpdateRect = updateRect;` at the top of the body.
2. Update the hunk header.
3. The `juce_ComponentPeer.h` hunks do not change.

**Validation:**
- The paint logic matches the cited D2D lines.
- No WM_PAINT path calls `BeginPaint`.
- `lastPaintUpdateRect` holds the bounds of every physical update region since the last painted frame. Each WM_PAINT no longer overwrites it.
- A frame that does not start keeps its region and paints on the next VBlank.
- No call goes through a null factory.
- Only Windows hunks change.
- The patches apply in Step 6's configure.

### Step 3: Request `VK_KHR_external_semaphore_win32` (W3)
**Scope:** JAM `jam_vulkan/device/jam_VulkanDevice.cpp`.
**Action:**
1. Add `VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME` to `optionalDeviceExtensions` inside `#if JUCE_WINDOWS` (`:10-13`).
2. At `:307`, set `compositionInteropSupported` to `isDeviceExtensionSupported (VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) and isDeviceExtensionSupported (VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)`.

**Validation:**
- The edits stay inside `#if JUCE_WINDOWS`.
- The extension is enabled only when the device reports it (`:312-314`).
- A device without both extensions keeps today's path (`:334`; `jam_VulkanGraphics.cpp:66`).

### Step 4: The shared fence, in Mac order (W3); the owned texture handle (contract)
**Scope:** JAM `jam_vulkan/context/native/jam_VulkanComposition_windows.{h,cpp}` and `jam_vulkan/context/jam_VulkanGraphics.{h,cpp}`, `#if JUCE_WINDOWS` regions only.

**Action, `VulkanComposition`:**
1. Add `static constexpr UINT64 initialFenceValue { 0 };` after `presentSyncInterval`.
2. Add the member `juce::ComSmartPtr<ID3D11Fence> sharedFence;` after `sharedTexture`.
3. In `create`, after `createSharedTexture`:
   - call `compositionDevice.direct3DDevice->CreateFence (initialFenceValue, D3D11_FENCE_FLAG_SHARED, __uuidof (ID3D11Fence), reinterpret_cast<void**> (composition->sharedFence.resetAndGetPointerAddress()))`;
   - on `FAILED`, return `nullptr` (the form of `:11-12`, `:26-42`).
4. `present` takes `UINT64 fenceValue`. It calls `compositionDevice.immediateContext->Wait (sharedFence, fenceValue);` before `CopyResource`. On `FAILED`, it takes the log + `deferDeviceReinitialise` form of `:105-110`.

**Action, `VulkanGraphics`:**
1. In `jam_VulkanGraphics.h:1103-1107`, add `vk::raii::Semaphore compositionSemaphore { nullptr };` and `uint64_t compositionFenceValue { VulkanComposition::initialFenceValue };`.
2. In `:788-790`, add `bool importCompositionFence();`.
3. **`importCompositionFence`**, next to `importCompositionImage`:
   1. `composition->sharedFence->CreateSharedHandle (nullptr, GENERIC_ALL, nullptr, &handle)`.
   2. Own the handle: `const std::unique_ptr<void, decltype (&CloseHandle)> fenceExport { handle, &CloseHandle };`
   3. Create the semaphore: `vk::SemaphoreTypeCreateInfo { vk::SemaphoreType::eTimeline, VulkanComposition::initialFenceValue }`, chained in `vk::SemaphoreCreateInfo`, in the `createSemaphore` binding form of `jam_VulkanSwapchain.cpp:21-27`.
   4. Import with `device.getDevice().importSemaphoreWin32HandleKHR (vk::ImportSemaphoreWin32HandleInfoKHR { *compositionSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eD3D11Fence, fenceExport.get() })`.
   5. Return `true` only when every call succeeds.
4. **`importCompositionImage`** (`:2096-2106`). This is a genuine contract defect: the handle is closed by hand at `:2103`. CODING.md, MEMORY MANAGEMENT: *"Never manually manage RAII objects"*. MANIFESTO B: *"No raw owning pointers. No manual cleanup."* The fix:
   - own the released handle as `const std::unique_ptr<void, decltype (&CloseHandle)> textureExport { composition->releaseTextureExport(), &CloseHandle };`;
   - pass `textureExport.get()` to `VulkanImage::importD3D11`;
   - delete `CloseHandle (textureExport);`.
5. **`create`.** After `:107`, add `ready = ready and (graphics->composition == nullptr or graphics->importCompositionFence());`.
6. **`endFrame`, composition branch** (`:485-504`):
   1. increment `compositionFenceValue`;
   2. the submit at `:488` signals `compositionSemaphore` with a chained `vk::TimelineSemaphoreSubmitInfo` that carries `compositionFenceValue`;
   3. delete the host-wait block `:497-504`.
7. **`endFrame` present** (`:534`): `composition->present (device.getCompositionDevice(), compositionFenceValue);`.
8. `:556` does not change.

**Validation:**
- The order is: submit with signal N, D3D11 `Wait` N, `CopyResource`, `Present (0, 0)`, `:556`.
- No host wait sits between the composition submit and `present`.
- Every addition is inside `#if JUCE_WINDOWS`.
- `resize` (`:1089-1116`) leaves the fence and the semaphore alone, because `VulkanComposition::resize` recreates only the texture.
- Both NT handles close by RAII.
- A failed import falls through the existing `create` failure, as an unsupported interop does today.

### Step 5: Windows glass is always acrylic (W6, supersedes W5)
**Runtime evidence:** with W5 in place, EVE's `acrylic10` fell back to `blurBehind`. The `accentEnableBlurBehind` state drew text on an opaque black background (ARCHITECT's test).

**Scope:** JAM `cast/gui.md`, `jam_gui/desktop/jam_BackgroundBlur.h`, `jam_gui/desktop/native/jam_BackgroundBlur_windows.cpp`, `jam_gui/windows/jam_Window.cpp`; EVE `Source/layout/default-eve.md`.
**Action:**
1. `gui.md` `## windowFX windows` holds one row: `acrylic | 0`.
2. The Windows `enum class WindowFX` holds `acrylic = 0` only. `setBlurBehind` is deleted (declaration and definition).
3. `BackgroundBlur::enable` on Windows: `result` starts `true`. When `blur > 0.0f`, `result = setAcrylic (component, colour);`. `style` stays named and unused, as the macOS `enable` leaves `colour` (`jam_BackgroundBlur_mac.mm:94-128`). Both platforms share the call sites (`jam_StyleTheme.cpp:269-270`, `:280`; `jam_Window.cpp:130`).
4. `jam_Window.cpp:86` passes `WindowFX::acrylic`.
5. `default-eve.md:230` value is `acrylic`.
6. Docs: `gui.md:186-189`, `jam_BackgroundBlur.h:17`, `:68`, `:98-110` state that Windows glass is acrylic only. `generated/jam_Gui.h` regenerates from `gui.md` at the JAM cast run.

**Validation:**
- No `blurBehind` style remains in code, data or docs. `ACCENT_STATE::accentEnableBlurBehind` stays: it mirrors a Win32 value.
- One branch only.

### Step 6: Build and measure (Pathfinder)
**Scope:** `benchmark/seq-100k-windows.md`. Its `## toolchain` rows run `cast ../../jam/cast/spell.md` and `cast ../cast/spell.md --standalone-profile-symbols`.
**Action:**
1. Pathfinder runs `scout` with that manifest once, with a 15-minute watchdog.
2. A compile error or a `git apply --check` failure goes back through Engineer to the step that owns the file.
3. COUNSELOR compares the new report with `benchmark/seq-100k-windows-report.md`:
   - `vulkan end frame` calls (536) against `editor blocks` calls (1,370);
   - `editor blocks` time (3.34 s);
   - the epoch (7.86 s);
   - `user` and `system` (5.44 s, 5.72 s).

**Validation:** the report exists, and the numbers go to ARCHITECT.

### Step 7: Audit
**Action:** @Auditor sweeps once against CONTRACT and this PLAN, across every changed file. Each finding goes through the Design by Contract Filter and is resolved this sprint.

### Step 8: Doxygen pass (Pathfinder) (W4)
**Scope:**
- the five moved files;
- the changed patch hunks, including the `externalContextFactory` doc (`juce-vulkan-engine-hook.patch:290-296`, *"a nullptr result skips that frame"*) and the `getLastPaintUpdateRect` doc (`juce-paint-update-rect-hook.patch:8-12`);
- `VulkanComposition::present`, whose host-wait precondition text is at `jam_VulkanComposition.h:46-50`;
- the new members and functions of Step 4;
- `BackgroundBlur::enable`;
- the five contradicting blocks (W4): `jam_VulkanGraphicsEngine.h:47-49`, `:57-58`, `:64-65`; `jam_VulkanGraphics.h:159-163`; `jam_VulkanGraphics.h:809`.

**Action:** COUNSELOR gives Pathfinder the symbols, the LSP positions, the index paths and the regeneration command. COUNSELOR then reads each block against the code.

### Step 9: ARCHITECT builds and tests
**Action:** ARCHITECT builds Windows and macOS. On Windows, ARCHITECT tests:
- `seq` (message thread responsive, CPU use);
- resize;
- show and hide;
- the translucent window (composition path, acrylic glass).

macOS only compiles the moved paths.

## BLESSED Alignment
- **B (Bound).**
  - `ExternalRenderContext` owns its update region and deferred repaints, as `D2DRenderContext` does.
  - `VulkanComposition` owns `sharedFence`. `VulkanGraphics` owns `compositionSemaphore`.
  - Both NT handles are RAII-owned.
- **L (Lean).**
  - One host wait and one manual `CloseHandle` are deleted, along with the build-number selection.
  - No new type.
  - Functions stay within 30 lines and 3 branches.
- **E (Explicit).**
  - Each name comes from a cited family.
  - `initialFenceValue` is named.
  - The one new guard names its threat.
  - Failures take the existing result-return forms.
- **S (SSOT).**
  - The update region comes from Win32 (`UpdateRegion`).
  - The fence value has one source, `compositionFenceValue`.
  - The glass effect follows the theme's style.
- **S (Stateless).** `schedulePaintOnVblank` and `deferredRepaints` are JUCE's own established state for this role (`:5371-5372`, `:5311`).
- **E (Encapsulation).**
  - JAM patterns come first. The patch follows the established JUCE context in the same file. MANIFESTO: *"A new pattern where one already exists is always wrong."*
  - Platform code stays in Windows hunks, `#if JUCE_WINDOWS` regions and `native/`.
- **D (Deterministic).**
  - One frame per scheduled VBlank, with a retry on a frame that does not start.
  - GPU order is fixed by the timeline value.

## Risks / Open Questions
- None. W1-W6 are closed.
