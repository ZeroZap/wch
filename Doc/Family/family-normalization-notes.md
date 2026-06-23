# WCH Family Normalization Notes

This document extracts family-level normalization guidance from `Doc/Ref/wch-dev-skill` into a repository-specific index for future per-family HAL notes, metadata, and EVT verification.

Official EVT projects, RM, DS, startup files, linker/scatter files, board manuals, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/new_project.md`
- Existing repository topic notes under `Doc/BLE`, `Doc/Core`, `Doc/HAL`, `Doc/USB`, `Doc/USBPD`, `Doc/ETH`, `Doc/HMI`, `Doc/IAP`, `Doc/RTOS`, `Doc/IO`, `Doc/Storage`, `Doc/System`, `Doc/Templates`, and `Doc/Security`.

## Normalization Strategy

Family notes should answer these questions before a unified HAL or generator uses a chip:

- Which architecture, compiler, project shape, startup file, linker/scatter file, and interrupt syntax apply?
- Which EVT tree and source family are the closest truth source?
- Which topic notes apply directly, and which require family-specific overrides?
- Which memory, DMA, BLE stack, USB stack, Ethernet, crypto, or bootloader constraints affect generated templates?
- Which facts remain unverified against official EVT/RM/DS material?

## Family Matrix

| Family group | Architecture and toolchain | Primary source roots | Key topic docs | First verification focus |
|---|---|---|---|---|
| CH57x | RISC-V BLE, MounRiver | `CH572EVT`, `CH573EVT`, `Doc/Ref/wch-dev-skill/chips/ch57x` | `Doc/BLE`, `Doc/Core`, `Doc/HAL`, `Doc/Templates`, `Doc/System`, `Doc/HMI` | BLE init order, `config.h`, `Main_Circulation()`, linker RAM layout, KEYSCAN/CMP/USB examples. |
| CH58x/CH59x | RISC-V BLE, MounRiver | `CH583EVT`, `CH585EVT`, `CH592EVT`, `CH595EVT`, `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x` | `Doc/BLE`, `Doc/USB`, `Doc/HMI`, `Doc/IAP`, `Doc/RTOS`, `Doc/Templates` | BLE heap/buffers, USB host/device paths, LCD/KEYSCAN availability, IAP/OTA offsets, RTOS example roots. |
| CH32V low-cost / CH32L103 | RISC-V StdPeriph, MounRiver | `CH32X035EVT`, `CH32L103EVT`, `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost` | `Doc/HAL`, `Doc/Core`, `Doc/USBPD`, `Doc/System`, `Doc/Templates` | Small Flash/RAM linker values, debug output mode, GPIO/ADC/Timer/Flash API, OPA/PWR/watchdog availability. |
| CH32V general | RISC-V StdPeriph, MounRiver | `CH32V103EVT`, `CH32V20xEVT`, `CH32V307EVT`, `CH32V407EVT`, `Doc/Ref/wch-dev-skill/chips/ch32v-general` | `Doc/HAL`, `Doc/Core`, `Doc/ETH`, `Doc/USB`, `Doc/IO`, `Doc/Storage`, `Doc/IAP` | Header prefix per subfamily, ETH only on capable chips, CAN/FSMC/SDIO coverage, IAP offset variants, linker/startup differences. |
| CH32F / CH32M ARM | Cortex-M3-style ARM, Keil MDK or MounRiver | `CH32F103EVT`, `CH32F20xEVT`, `CH32M030EVT`, `Doc/Ref/wch-dev-skill/chips/ch32f-arm` | `Doc/HAL`, `Doc/Templates`, `Doc/IO`, `Doc/Storage`, `Doc/System`, `Doc/Security` | `ch32f10x` vs `ch32f20x` headers, Keil project metadata, RNG availability on CH32F20x only, CH32M030 USBPD differences. |
| CH32X / CH64x USB-PD | RISC-V StdPeriph, MounRiver | `CH32X035EVT`, `CH32X315EVT`, `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd` | `Doc/USBPD`, `Doc/USB`, `Doc/HAL`, `Doc/IAP`, `Doc/Templates`, `Doc/System` | USB-PD CC pins, comparator thresholds, `USBPD_PHY_V33`, PIOC remap/microcode, package-specific memory and USBPD instances. |
| CH569 / CH56x | RISC-V high-speed, MounRiver | `CH569EVT`, `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet` | `Doc/ETH`, `Doc/USB`, `Doc/Storage`, `Doc/Security`, `Doc/IO`, `Doc/Templates` | RAMX/DMA placement, ETH/USB3/eMMC/HSPI/DVP/ECDC interactions, `CH56x_common.h`, `Flash_Lib`, high-speed clocks. |
| CH561/CH563 | ARM7TDMI, Keil MDK | `CH561EVT`, `CH563EVT`, `Doc/Ref/wch-dev-skill/chips/ch561-ch563` | `Doc/ETH`, `Doc/Core`, `Doc/Templates`, `Doc/Storage`, `Doc/IO`, `Doc/Security` | Register-level `R8_/R16_/R32_` API, `SYSFREQ.H/C`, scatter file, no ECDC, CH563 uC/OS-II, ARM7 interrupt syntax. |
| CH32H417 | High-performance RISC-V, MounRiver | CH32H417 EVT material, `Doc/Ref/wch-dev-skill/chips/ch32h-highperf` | `Doc/HAL`, `Doc/USB`, `Doc/ETH`, `Doc/HMI`, `Doc/IO`, `Doc/Storage`, `Doc/Security`, `Doc/Templates` | V5F/V3F linker selection, `0x10000` app offset, ITCM/DTCM, LTDC/GPHA/FMC/QSPI/SAI/DVP/ECDC/CRC/RNG details. |
| CH5xx 8051 | 8051, Keil C51 / SDCC | CH5xx EVT material, `Doc/Ref/wch-dev-skill/chips/ch5xx-8051` | `Doc/Core`, `Doc/USB`, `Doc/HAL`, `Doc/HMI`, `Doc/IAP`, `Doc/Templates`, `Doc/USBPD` | C51 memory model, `CH5xx.H`, protected register safe mode, ISP bootloader ROM limit, Type-C/TouchKey/USB constraints. |

## Per-Family Note Template

When a family is actively normalized, create or update a dedicated note using this shape:

```text
# <Family> HAL Normalization Notes

## Source Files
## Supported Chips And EVT Roots
## Architecture, Toolchain, Startup, Linker
## Memory And Boot Layout
## Project Template Rules
## Peripheral Coverage Matrix
## Topic Cross-References
## Known Family Pitfalls
## Verification Checklist
## Verification Status
```

Rules:

- Keep verified facts and Markdown-extracted leads separate.
- Use one family note per source family when enough EVT/RM/DS verification exists.
- Cross-link topic documents rather than duplicating detailed USB/BLE/ETH/IAP rules.
- Record exact EVT paths and header files before marking an API or feature as supported.

## Cross-Family Split Rules

- Do not merge CH57x and CH58x/CH59x BLE details until their BLE stack, RAM, USB, and example differences are checked.
- Do not treat all CH32V chips as one feature set; CH32V103, V20x, V307, and V407 differ in high-speed peripherals.
- Do not infer CH32F20x RNG or Ethernet availability for CH32F10x or CH32M030.
- Do not use CH569 high-speed/RAMX/ECDC assumptions for CH561/CH563 ARM7TDMI.
- Do not apply CH32H417 V5F linker layout to V3F or smaller CH32 families.
- Do not apply RISC-V interrupt attributes or linker scripts to CH5xx 8051 or CH561/CH563 ARM7 projects.

## Verification Checklist

For each family, verify:

- EVT roots present in this repository and closest example paths for GPIO, USART/UART, timer, ADC, Flash, DMA, USB/BLE/ETH/CAN/storage as applicable.
- Main chip header, peripheral driver include folder, system file, startup file, linker/scatter file, and debug helper file.
- Flash/RAM sizes, bootloader/application offsets, stack/heap layout, and DMA-accessible memory regions.
- Interrupt syntax, vector table relocation mechanism, priority grouping, and ISR section attributes.
- Toolchain project metadata: MounRiver `.project/.cproject/.wvproj`, Keil `.uvproj/.uvprojx`, C51 options, include paths, and defines.
- Board-level dependencies: oscillator, debug UART pins, LEDs/buttons, USB connector, Type-C CC pins, Ethernet PHY, external storage, display, codec, camera, or transceiver.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and current repository topic notes listed above.
- Not fully verified against every EVT tree, RM, DS, project file, startup assembly, linker/scatter file, board schematic, or package variant in this pass.
- Treat this as a family-normalization routing document. Dedicated per-family files should be created only after the corresponding EVT/RM/DS verification pass begins.
