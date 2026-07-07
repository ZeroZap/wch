# wch-dev-skill Markdown Extraction Plan

This document guides the ongoing extraction of Markdown knowledge from `Doc/Ref/wch-dev-skill` into the appropriate documentation areas under `Doc/`.

`Doc/Ref/wch-dev-skill` is the source knowledge base. Do not delete it during this phase. The goal is to progressively digest its `.md` files into repository-specific notes, indexes, and HAL design references while keeping official RM / DS / EVT sources as the final source of truth.

## Objectives

- Keep `Doc/Ref/wch-dev-skill` available as raw reference input.
- Extract useful `.md` content into focused documents under `Doc/`.
- Convert generic AI-skill guidance into repository-specific WCH HAL notes.
- Avoid copying large documents verbatim when a concise, traceable summary is better.
- Preserve source links back to `Doc/Ref/wch-dev-skill/...` for traceability.
- Build a long-term documentation structure that supports HAL unification and future CubeX metadata generation.

## Source Material

- `Doc/Ref/wch-dev-skill/SKILL.md`: chip-family routing, global WCH rules, BLE rules, workflows, pitfalls.
- `Doc/Ref/wch-dev-skill/AGENTS.md`: coding conventions, include patterns, main loop templates, checklists.
- `Doc/Ref/wch-dev-skill/chips/*/recipes/*.md`: scenario guides by chip family and peripheral.
- `Doc/Ref/wch-dev-skill/chips/*/resources/*.md`: API references, pitfalls, memory layout, example indexes.

## Target Documentation Areas

Use existing directories first. Create new directories only when the topic does not fit an existing area.

| Source topic | Target area | Notes |
|---|---|---|
| BLE initialization, GAP/GATT, config, BLE examples | `Doc/BLE/` | Extract CH57x and CH58x/CH59x BLE rules into focused BLE notes. |
| RISC-V core, interrupt attributes, startup, linker, memory layout | `Doc/Core/` | Merge with core/toolchain references and keep architecture differences explicit. |
| Ethernet examples and pitfalls | `Doc/ETH/` | Extract CH569, CH32V307, CH32F20x, CH561/CH563, CH32H417 Ethernet notes. |
| USB device/host/USB3 notes | `Doc/USB/` | Create when USB extraction starts. |
| USB-PD, Type-C, PIOC | `Doc/USBPD/` | Create when USB-PD extraction starts. |
| Common GPIO/RCC/UART/SPI/I2C/ADC/Timer/Flash/DMA patterns | `Doc/HAL/` | Create as HAL normalization notes, not raw chip examples. |
| Chip-family routing, memory map, examples, pitfalls | `Doc/Family/` | Create per-family notes when actively normalizing a family. |
| New project templates, toolchain metadata, minimal main loops | `Doc/Templates/` | Create when project-template extraction starts. |
| IAP/OTA/bootloader/linker offset | `Doc/IAP/` | Create when IAP extraction starts. |
| FreeRTOS/RT-Thread/HarmonyOS/UCOS references | `Doc/RTOS/` | Create when RTOS extraction starts. |
| Display, HMI, and specialty peripherals | `Doc/HMI/` | Create when display/HMI extraction starts. |
| CAN, I2S, SAI, DVP camera streaming interfaces | `Doc/IO/` | Create when IO/media extraction starts. |
| SD/SDIO/eMMC/HSPI/QSPI storage interfaces | `Doc/Storage/` | Create when storage interface extraction starts. |
| Low power, RTC, watchdog, analog, CRC/RNG utilities | `Doc/System/` | Create when system utility extraction starts. |
| ECDC, CRC, RNG, security boundaries | `Doc/Security/` | Create when security and crypto extraction starts. |

## Extraction Rules

- Do not delete `Doc/Ref/wch-dev-skill` while extraction is in progress.
- Do not treat `wch-dev-skill` as the final authority. Cross-check with `Doc/DS`, `Doc/RM`, and `*EVT` sources.
- Prefer summaries, tables, and HAL-relevant conclusions over wholesale copied recipes.
- For every extracted note, include a `Source` section with original `Doc/Ref/wch-dev-skill/...` paths.
- Keep chip-specific behavior separated from cross-family HAL design rules.
- If a recipe describes runnable code, map it to the closest EVT example path before converting it into repository guidance.
- Keep generated target documents small enough to be maintainable by family or topic.

## Retained Development Rules

These rules should be carried into target docs where relevant.

### Chip Family Routing

| Chip or family | Source family |
|---|---|
| CH57x, CH572, CH573, CH579 | `ch57x` |
| CH58x, CH59x, CH583, CH585, CH592, CH595 | `ch58x-ch59x` |
| CH32V103, CH32V20x, CH32V307, CH32V407 | `ch32v-general` |
| CH32V003, CH32V006, CH32L103 | `ch32v-lowcost` |
| CH32F103, CH32F20x, CH32M030 | `ch32f-arm` |
| CH32X035, CH32X315, CH641, CH643 | `ch32x-usbpd` |
| CH569 | `ch56x-ethernet` |
| CH32H417 | `ch32h-highperf` |
| CH561, CH563 | `ch561-ch563` |
| CH543 through CH559 | `ch5xx-8051` |

### Universal WCH Rules

- Verify WCH APIs against EVT sources, headers, examples, RM, or DS before using them in HAL design.
- Enable peripheral clocks before accessing peripherals.
- Configure GPIO mode and alternate function before peripheral initialization.
- Erase Flash by the correct page or sector size before writing.
- Do not mix headers, startup files, linker scripts, or interrupt attributes across chip families.
- For runnable examples, start from the closest EVT example and adapt it.
- For existing code, make the smallest correct in-place change.

### BLE Rules

- Preserve strict initialization order: system clock, BLE stack init, HAL init, GAP role init, app init, then event loop.
- `Main_Circulation()` is the BLE event loop and must not return.
- Register GATT services in order: GAP, GATT, Device Information, custom services.
- Keep BLE interrupt handlers in fast interrupt form and RAM/highcode section where required by the chip family.
- Check `config.h` for heap size, buffer size, MTU, sleep policy, TX power, and connection limits.

### Architecture-Specific Rules

- RISC-V WCH interrupt handlers commonly use `__attribute__((interrupt("WCH-Interrupt-fast")))`.
- CH57x/CH58x/CH59x BLE interrupt handlers may additionally require `.highcode` placement.
- CH32V/CH32F/CH32X/CH32H StdPeriphDriver code should configure NVIC priority grouping before interrupt setup.
- ARM7TDMI CH561/CH563 code is register-level and uses `R8_`, `R16_`, and `R32_` style access rather than StdPeriphDriver.
- 8051 code must use protected-register safe mode sequences and correct `data` or `xdata` qualifiers.

## Extraction Workflow

Use this workflow for each topic or chip family.

1. Select a topic, such as BLE peripheral, GPIO, Flash, Ethernet, USB-PD, or memory layout.
2. Identify all related `Doc/Ref/wch-dev-skill/**/*.md` files.
3. Read the corresponding EVT example paths referenced by the skill docs.
4. Cross-check key API names and behavior against repository headers and official documents.
5. Create or update the target note under `Doc/<topic>/`.
6. Add source paths and verification notes to the target note.
7. Update the TODO list in this document.
8. Leave `Doc/Ref/wch-dev-skill` in place for future extraction.

## Migration TODO List

- [x] Restore `Doc/Ref/wch-dev-skill` as the raw source reference.
- [x] Replace the old removal-focused plan with this extraction-focused plan.
- [x] Inventory all top-level skill docs: `SKILL.md`, `AGENTS.md`, `README.md`, `README_EN.md`, `CHANGELOG.md`.
- [x] Generate a complete source index of `Doc/Ref/wch-dev-skill/**/*.md` grouped by family and topic.
- [x] Extract global chip-family routing into a repository-specific family index under `Doc/Family/`.
- [x] Create a family-normalization entry note for future per-family HAL notes and EVT verification.
- [x] Create first per-family notes for CH32V general, CH58x/CH59x, and CH56x Ethernet families.
- [x] Create second per-family notes for CH57x, CH32X/CH6xx USB-PD, and CH32H high-performance families.
- [x] Create `Doc/Templates/` and extract new-project, toolchain, linker, startup, and minimal main-loop notes.
- [x] Extract BLE notes from `ch57x` and `ch58x-ch59x` into `Doc/BLE/`.
- [x] Extract memory layout, startup, linker, interrupt, and architecture pitfalls into `Doc/Core/`.
- [x] Extract Ethernet notes into `Doc/ETH/`.
- [x] Create `Doc/HAL/` and extract common peripheral normalization notes for GPIO, RCC, UART, SPI, I2C, ADC, Timer, Flash, and DMA.
- [x] Create `Doc/USB/` and extract USB device, host, USB HS, and USB3 notes.
- [x] Create `Doc/USBPD/` and extract USB-PD, Type-C, and PIOC notes.
- [x] Create `Doc/IAP/` and extract IAP, OTA, bootloader, and linker-offset notes.
- [x] Create `Doc/RTOS/` and extract RTOS-related references where useful.
- [x] For each extracted target document, include source paths and verification status.
- [x] Keep `README.md` aligned with the current extraction status.
- [x] Create `Doc/HMI/` and extract display, HMI, and specialty peripheral notes.
- [x] Create `Doc/IO/` and extract CAN, I2S, SAI, and DVP camera interface notes.
- [x] Create `Doc/Storage/` and extract SD/SDIO/eMMC/HSPI/QSPI storage interface notes.
- [x] Create `Doc/System/` and extract low-power, RTC, watchdog, analog, CRC/RNG, and utility peripheral notes.
- [x] Create `Doc/Security/` and extract ECDC, CRC, RNG, and security-boundary notes.

## Acceptance Criteria

- `Doc/Ref/wch-dev-skill` remains present during extraction.
- Extracted notes live in topic-appropriate `Doc/` directories.
- Each extracted note is traceable to source `.md` files and, where possible, official EVT/RM/DS material.
- README describes `Doc/Ref/wch-dev-skill` as a source reference, not as a directory scheduled for immediate deletion.
- Deletion of `Doc/Ref/wch-dev-skill` is not considered until all useful Markdown content has been migrated or intentionally skipped.
