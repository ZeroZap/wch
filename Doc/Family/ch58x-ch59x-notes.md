# CH58x And CH59x Family Notes

This document extracts CH583, CH585, CH592, and CH595 guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for BLE, USB, HMI, RTOS, IAP, and template work.

Official EVT examples, BLE stack libraries, RM, DS, startup files, linker scripts, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/config_reference.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Chip | Repository EVT root | Source-note memory | Key extra features from source notes |
|---|---|---|---|
| CH583 | `CH583EVT/` | 448 KB Flash, 32 KB RAM | BLE, dual USB on source table, common CH58x examples. |
| CH585 | `CH585EVT/` | 448 KB Flash, 128 KB RAM | BLE, NFCA, LED controller, LCD, dual USB on source table. |
| CH592 | `CH592EVT/` | 448 KB Flash, 26 KB RAM | BLE, LCD, smaller RAM than CH583/CH585. |
| CH595 | `CH595EVT/` | 240 KB Flash, 32 KB RAM | BLE, LCD, ENCODER, KEYSCAN, LED controller. |

## Architecture, Toolchain, Startup, Linker

- Architecture: WCH RISC-V BLE MCU family.
- Toolchain: MounRiver Studio project files with `Ld/Link.ld`, `Startup`, `RVMSIS`, and `StdPeriphDriver` linked or copied from EVT sources.
- Main headers: `CH58x_common.h` or `CH59x_common.h` depending selected chip.
- BLE stack libraries: source notes list `libCH58xBLE.a` / `libCH59xBLE.a` under BLE examples; verify exact library path per EVT tree.
- Template rule: choose the closest BLE, UART, USB, LCD, or GPIO example before modifying application code.

## Memory And Boot Layout

- Standard application starts at `0x00000000` in source notes.
- IAP application uses a 4 KB bootloader and application origin `0x00001000` in the source note.
- Flash erase granularity from source notes is 256 bytes; write granularity is 4 bytes.
- BLE heap and buffers are configured in `config.h`, not linker-only.

Rules:

- CH592's 26 KB RAM and CH595's 240 KB Flash require tighter heap, stack, and feature budgeting.
- CH585's larger 128 KB RAM can support larger BLE heap or buffering, but still requires EVT/linker verification.
- Do not copy one `Link.ld` across CH583/CH585/CH592/CH595 without checking memory lengths.
- IAP, OTA, and BackupUpgrade flows must use the matching linker scripts and image flags from the EVT examples.

## Peripheral And Example Coverage

Common source examples include:

- BLE roles: Peripheral, Central, Broadcaster, Observer, CentPeri, MultiCentral, MultiCentPeri, HID, HeartRate, BLE_UART, BLE_USB, DTM, RF_PHY, Mesh, OTA/IAP variants.
- USB: device COM/HID/vendor and host enumeration/AOA/U_DISK where present.
- Common peripherals: ADC/TouchKey, Flash, I2C, IAP, PM, PWMX, SPI, Timer, UART1, RTOS examples.
- Chip-specific: CH585 NFCA, CH595 ENCODER and KEYSCAN, CH585/CH595 LED controller, LCD on CH585/CH592/CH595 according to source notes.

## Topic Cross-References

- BLE rules: `Doc/BLE/wch-ble-notes.md`.
- USB rules: `Doc/USB/wch-usb-notes.md`.
- IAP/OTA rules: `Doc/IAP/wch-iap-ota-notes.md`.
- RTOS/TMOS boundaries: `Doc/RTOS/wch-rtos-notes.md`.
- LCD/TouchKey/KEYSCAN: `Doc/HMI/wch-hmi-specialty-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.
- Low power/system utilities: `Doc/System/wch-system-analog-power-notes.md`.

## Known Family Pitfalls

- BLE init order must be exact: system clock, BLE stack init, HAL init, GAP role init, app init, `Main_Circulation()`.
- `Main_Circulation()` is the TMOS/BLE scheduler and must not be omitted or allowed to return.
- `BLE_MEMHEAP_SIZE`, `BLE_BUFF_MAX_LEN`, connection limits, and sleep policy must match the role/profile.
- ATT MTU is `BLE_BUFF_MAX_LEN - 4` in source notes.
- GATT services should register GAP/GATT/DeviceInfo before custom services.
- Interrupt handlers may require WCH fast interrupt attributes and `.highcode` placement.
- CH583/CH585 dual USB examples require the correct USB instance functions.
- Flash partial updates require read-modify-write over 256-byte sectors.
- CH595 KEYSCAN sleep wake requires family-specific power preparation such as `KeyScanPowAdj()` per source notes.
- CH585 NFCA requires analog/antenna control pin setup when the board uses the NFC_CTR path.

## Verification Checklist

- Verify each EVT root and selected `.wvproj/.project/.cproject` structure.
- Verify exact chip header, BLE library, `config.h`, linker memory length, startup file, and debug UART pins.
- Verify BLE role example paths, stack init order, service registration, heap/buffer sizes, and sleep settings.
- Verify USB controller instance, endpoint buffer rules, and dual-USB availability by chip.
- Verify CH585 NFCA and CH595 KEYSCAN/ENCODER/LED examples against board schematics and DS.
- Verify IAP/OTA linker offsets, image flags, and backup-upgrade flows before generating update templates.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all CH583/CH585/CH592/CH595 EVT trees, BLE libraries, RM, DS, project files, startup files, linker scripts, packages, or board schematics in this pass.
- Treat feature availability, memory values, and chip-specific examples as preliminary until checked against exact EVT and official documents.
