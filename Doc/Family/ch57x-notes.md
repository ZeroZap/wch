# CH57x Family Notes

This document extracts CH57x guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for BLE, USB, RF, IAP, HMI, and template work.

Official EVT examples, BLE libraries, RM, DS, startup files, linker/scatter files, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/config_reference.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/ble_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Chip/source group | Repository EVT root | Architecture/toolchain from source notes | Key constraints |
|---|---|---|---|
| CH572 / CH573-style CH57x | `CH572EVT/`, `CH573EVT/` | RISC-V, MounRiver, `Link.ld` | BLE stack, `.highcode`, 18 KB RAM source layout, DataFlash/image flags. |
| CH579 legacy source note | Source notes mention CH579EVT | ARM Cortex-M0, Keil `.sct` | Different architecture, Flash/RAM map, NET/LCD examples. Verify whether this repository carries the exact EVT before using. |

## Architecture, Toolchain, Startup, Linker

- CH572-style sources use MounRiver with linked `Ld`, `RVMSIS`, `Startup`, and `StdPeriphDriver` directories.
- CH579 source notes use Keil scatter files and ARM Cortex-M0 startup, not the CH572 RISC-V flow.
- `.highcode` is loaded from Flash and executed from RAM for interrupt handlers, Flash routines, and timing-sensitive BLE paths.
- BLE projects require `config.h`, BLE stack library, HAL, role/profile source, and `Main_Circulation()`.

## Memory And Boot Layout

- CH572 source note: Flash app area from `0x00001000` to `0x0006FFFF`, 512-byte DataFlash at `0x00077E00`, RAM at `0x20003800` length 18 KB.
- CH572 IAP source note: 4 KB bootloader at start, app origin `0x1000`; BackupUpgrade layout uses Image A/B, IAP image, and DataFlash image flags.
- CH579 source note: app starts at `0x00000000`, DataFlash near `0x3E800`, bootloader near top at `0x3F000`, RAM split into two banks.
- BLE heap is configured by `BLE_MEMHEAP_SIZE`; source notes mention CH572 default around 6 KB and CH579 around 8 KB.

## Peripheral And Example Coverage

Source examples include ADC/TouchKey, CMP, Flash/DataFlash, I2C, IAP over USB/UART, KEYSCAN, power management, PWMX, RF, SPI, Timer, UART, USB device/host, and BLE roles/profiles. CH579 source notes additionally mention segment LCD and WCHNET-style NET examples.

## Topic Cross-References

- BLE: `Doc/BLE/wch-ble-notes.md`.
- USB: `Doc/USB/wch-usb-notes.md`.
- IAP/OTA: `Doc/IAP/wch-iap-ota-notes.md`.
- KEYSCAN/TouchKey/LCD: `Doc/HMI/wch-hmi-specialty-notes.md`.
- Low power/system: `Doc/System/wch-system-analog-power-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.

## Known Family Pitfalls

- BLE init order is strict: clock, BLE library, HAL, GAP role, app init, then `Main_Circulation()`.
- `Main_Circulation()` must run the BLE/TMOS event loop; `__WFI()` alone does not process BLE events.
- BLE heap and `BLE_BUFF_MAX_LEN` must be sized for role, MTU, services, and connection count.
- ATT MTU is `BLE_BUFF_MAX_LEN - 4` in source notes.
- GAP/GATT/DeviceInfo services must register before custom services.
- Interrupt handlers should use WCH fast interrupt attributes and `.highcode` placement where required.
- Flash writes require 256-byte erase/read-modify-write handling.
- IAP app origin differs between CH572-style and CH579-style layouts.

## Verification Checklist

- Verify exact EVT roots and whether CH579 material is present and relevant in this repository.
- Verify BLE library path, header names, `config.h`, heap/buffer settings, role example, and `Main_Circulation()` call site.
- Verify linker/scatter files, RAM origin, DataFlash address, IAP/OTA image flags, and startup vector behavior.
- Verify USB, RF, KEYSCAN, CMP, TouchKey, and power examples against active headers and board schematics.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all CH57x EVT trees, BLE libraries, RM, DS, project files, startup files, linker/scatter files, packages, or board schematics in this pass.
- Treat CH579-related details especially as source-note leads until matched to actual repository EVT material.
