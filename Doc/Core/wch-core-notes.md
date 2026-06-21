# WCH Core, Memory, Startup, And Toolchain Notes

This document extracts core architecture, memory layout, startup/linker, interrupt, and Flash rules from `Doc/Ref/wch-dev-skill` into repository-specific notes.

Scope:

- RISC-V WCH families: CH57x, CH58x/CH59x, CH32V, CH32X, CH569, CH32H.
- ARM families: CH32F Cortex-M3 and CH561/CH563 ARM7TDMI.
- 8051 families: CH543 through CH559.
- Focus: headers/toolchains, linker layout, memory regions, section placement, interrupt attributes, Flash erase/write rules, and IAP offsets.

Official EVT examples, startup files, linker scripts, RM, and DS remain the final source of truth.

## Source Files

- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`

## Architecture And Toolchain Matrix

| Architecture | Families | Typical headers | Build tool | Linker or memory description |
|---|---|---|---|---|
| RISC-V BLE | CH57x, CH58x/CH59x | `CH57x_common.h`, `CH58x_common.h`, `CH59x_common.h` | MounRiver | `Ld/Link.ld` |
| RISC-V StdPeriph | CH32V103, CH32V20x, CH32V307, CH32V407 | `ch32v10x.h`, `ch32v20x.h`, `ch32v30x.h`, `ch32v4x7.h` | MounRiver | `Ld/Link.ld` |
| RISC-V low-cost | CH32V003, CH32V006, CH32L103 | `ch32v00x.h`, `ch32l103.h` | MounRiver | `Ld/Link.ld` |
| RISC-V USB-PD | CH32X035, CH32X315, CH641, CH643 | `ch32x035.h`, `ch32x3x5.h`, `ch641.h`, `ch643.h` | MounRiver | `Ld/Link.ld` |
| RISC-V high-performance | CH32H417 | `ch32h417.h` | MounRiver | `Link_v5f.ld`, core-specific linker files |
| RISC-V Ethernet | CH569 | `CH56x_common.h` | MounRiver | `Ld/Link.ld` |
| ARM Cortex-M3 | CH32F103, CH32F20x, CH32M030 | `ch32f10x.h`, `ch32f20x.h` | Keil MDK or MounRiver | `.sct` or `Ld/Link.ld` |
| ARM7TDMI | CH561, CH563 | `CH561SFR.H`, `CH563SFR.H`, `SYSFREQ.H` | Keil MDK | Scatter file `.sct` |
| 8051 | CH543 through CH559 | `CH5xx.H` or chip-specific `CH554.H` style header | Keil C51 or SDCC | 8051 memory spaces, no standard GCC linker script |

## Include And Startup Rules

- Do not mix headers across families. A CH32V10x driver should not include CH32V30x or CH32X headers unless the EVT proves it is shared.
- BLE projects include BLE stack headers and HAL headers before application role headers.
- CH32V/CH32F/CH32X/CH32H StdPeriph projects usually include the chip root header and peripheral headers, or the family `*_conf.h` umbrella.
- CH561/CH563 code is register-level and uses SFR macros such as `R8_`, `R16_`, and `R32_`.
- 8051 code depends on memory qualifiers and compiler-specific interrupt declarations.

## Standard Memory Model

Most WCH RISC-V and ARM Cortex-M style parts use this conceptual layout:

| Region | Typical start | Purpose |
|---|---|---|
| Flash | `0x00000000` | Vector table, code, constants, initialized-data load image |
| SRAM | `0x20000000` or family-specific RAM base | `.data`, `.bss`, heap, stack |
| Peripheral registers | `0x40000000` range for many CH32-style parts | APB/AHB peripherals |

Exceptions:

- CH57x CH572 uses RAM at `0x20003800` in the source notes.
- CH32H417 separates ITCM and DTCM: ITCM at `0x200A0000`, DTCM at `0x200C0000`.
- CH561/CH563 uses SFR at `0x00400000` and SRAM at `0x00808000`.
- 8051 has separate code, data, idata, xdata, bit, and SFR spaces.

## Section Placement

Common GCC linker sections:

| Section | Location | Meaning |
|---|---|---|
| `.init` | Flash | Startup and init code before `main` |
| `.text` | Flash | Application code and read-only data |
| `.data` | RAM, loaded from Flash | Initialized global/static variables copied at startup |
| `.bss` | RAM | Zero-initialized global/static variables cleared at startup |
| `.stack` | RAM top or explicit region | Call stack and interrupt context |
| `.highcode` | Family-specific RAM or fast memory | Latency-sensitive code and interrupt handlers |

Rules:

- Verify whether `.highcode` is actually copied to or executed from RAM in the active linker/startup pair.
- CH57x source notes show `.highcode` loaded from Flash and executed from RAM for fast interrupt paths.
- CH58x/CH59x source notes list `.highcode` as a dedicated linker section for interrupt handlers.
- CH32H417 uses ITCM (`RAM_CODE`) for `.highcode` and performance-critical routines.
- CH32V003 source notes state `.highcode` can be RAM-backed, while some related low-cost variants may differ.

## Interrupt Attributes

| Family or architecture | Typical interrupt declaration | Notes |
|---|---|---|
| RISC-V WCH | `__attribute__((interrupt("WCH-Interrupt-fast")))` | Used by CH32V/CH32X/CH57x/CH58x-style examples. |
| RISC-V BLE fast handlers | fast interrupt plus `__attribute__((section(".highcode")))` | Use only after linker/startup verification. |
| ARM Cortex-M3 CH32F | Source notes show WCH fast attribute in examples | Verify against actual compiler and EVT startup. |
| ARM7TDMI CH561/CH563 | `__irq` style handlers | Register-level code, Keil MDK scatter/startup. |
| 8051 | `__interrupt(n)` or compiler equivalent | Vector number matters; syntax is compiler-specific. |

## Flash Erase And Write Rules

Flash cannot be treated as byte-rewritable RAM. Always verify erase granularity before writing.

| Family | Erase/write notes from source material |
|---|---|
| CH57x | 256-byte sectors; partial updates require read-modify-erase-write. |
| CH58x/CH59x | 256-byte erase sectors; writes must follow erase; word alignment matters. |
| CH32V103 | 1KB page erase. |
| CH32V20x/CH32V307/CH32V407 | 4KB page erase in source notes. |
| CH32V003 | 64-byte erase unit in source notes. |
| CH32V006/CH32L103 | 1KB erase unit in source notes. |
| CH32H417 | 4KB page erase, 4-byte program unit. |
| CH561/CH563 | 4KB erase block, 4-byte minimum write; Data-Flash is separate. |
| CH5xx 8051 | DataFlash access requires safe mode and interrupt masking; code Flash and DataFlash rules vary by chip. |

Read-modify-write rule for partial page data:

1. Copy the whole page or sector to RAM.
2. Modify the target bytes in RAM.
3. Erase the whole page or sector.
4. Program the whole page or required aligned words back.

## IAP And Boot Offsets

IAP layouts are family-specific. Do not reuse offsets blindly.

| Family | Source-note pattern |
|---|---|
| CH57x CH572 | Bootloader at `0x00000000`, app commonly at `0x00001000`; OTA dual-image layouts split image A/B and DataFlash flags. |
| CH579 | Bootloader is at top of Flash in source notes; application starts at `0x00000000`. |
| CH58x/CH59x | IAP app commonly offset to `0x00001000`; Flash/RAM sizes vary by chip. |
| CH32V general | Bootloader often 16KB; app starts at `0x00004000`; vector table must be relocated. |
| CH32V low-cost | 4KB boot area is common in source notes; app starts at `0x00001000` for IAP. |
| CH32H417 | Source notes show built-in bootloader region at `0x00000000` with application from `0x00010000`. |
| CH561/CH563 | Typical 4KB IAP area at Flash start; verify scatter file. |
| 8051 | Bootloader location depends on chip, for example CH552 and CH559 differ in source notes. |

Validation checklist for any IAP design:

- Confirm bootloader size and start address from EVT linker or scatter file.
- Confirm application start address and vector table relocation behavior.
- Confirm Flash page size and erase/write APIs.
- Confirm NV/DataFlash/SNV/OTA flag regions do not overlap app code or backup image.
- Confirm stack pointer validity checks use the correct RAM range.

## CH32H417 Core-Specific Notes

CH32H417 needs special handling because the source notes describe multiple core and RAM regions.

Key points:

- V5F application Flash may start at `0x00010000` after a bootloader area.
- ITCM RAM at `0x200A0000` is used for fast code execution.
- DTCM RAM at `0x200C0000` is used for data, bss, heap, and stack.
- Linker files are core-specific; do not apply a generic CH32V linker script without checking the active core.

## ARM7TDMI CH561/CH563 Notes

CH561/CH563 are not StdPeriphDriver parts in the source notes.

Rules:

- Use register-level SFR macros, not CH32V/CH32F StdPeriph APIs.
- Configure `FREQ_SYS` and memory-related options before relying on clock or RAM layout.
- SRAM size can depend on `MEM_DATA` in `SYSFREQ.H`.
- Flash erase unit is 4KB.
- Interrupt handlers use Keil/ARM7 conventions, not GCC RISC-V attributes.
- CH563 has USB and GPIO port D features that CH561 lacks in the source notes.

## 8051 Notes

8051 memory is not a flat ARM/RISC-V address space.

Memory spaces:

| Qualifier | Space | Use case |
|---|---|---|
| `data` | Direct internal RAM | Small, fast variables. |
| `idata` | Indirect internal RAM | More internal RAM, stack-like uses. |
| `xdata` | External data RAM via MOVX | Large buffers, USB buffers. |
| `code` | Code memory | Constant tables and read-only data. |
| `bit` | Bit-addressable area | Boolean flags. |

Rules:

- Use safe mode before writing protected registers: `SAFE_MOD = 0x55; SAFE_MOD = 0xAA;` then write, then `SAFE_MOD = 0x00;`.
- Disable interrupts during Flash/DataFlash writes where required (`EA = 0`, then restore).
- Keep USB buffer addresses aligned as required by the EVT and endpoint DMA registers.
- Use `xdata` for large buffers rather than exhausting `data`.

## Cross-Family Core Checklist

Before adding a new HAL family adapter or template, verify:

- Exact chip variant and package.
- Architecture and compiler/toolchain.
- Correct root header and peripheral header naming.
- Startup file and vector table implementation.
- Linker/scatter file Flash and RAM sizes.
- Stack and heap sizing.
- `.highcode` or equivalent fast-code placement.
- Interrupt declaration syntax.
- Flash erase unit, write unit, unlock sequence, and protection rules.
- IAP offsets and vector relocation, if bootloader support is needed.
- Whether the family uses StdPeriphDriver or register-level SFR access.

## Verification Status

- Extracted from `wch-dev-skill` Markdown only.
- Exact addresses and sizes must be checked against `Doc/DS`, `Doc/RM`, and repository EVT linker/startup files before code generation.
- Next verification pass should compare the notes above against `Ld/Link.ld`, `.sct`, startup files, and Flash driver implementations in each imported EVT tree.
