# CH561 And CH563 ARM7TDMI Family Notes

This document extracts CH561 and CH563 guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for ARM7TDMI, Ethernet, USB, storage, RTOS, and template work.

Official EVT examples, RM, DS, startup files, scatter files, board schematics, PHY/storage datasheets, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Chip | Repository EVT root | Architecture/toolchain | Key differences |
|---|---|---|---|
| CH561 | Verify `CH561EVT/` availability | ARM7TDMI, Keil MDK | Ethernet, no USB, PA/PB GPIO, no RTOS in source table. |
| CH563 | Verify `CH563EVT/` availability | ARM7TDMI, Keil MDK | Ethernet, USB, GPIO PD, uC/OS-II, more NET examples. |

## Architecture, Toolchain, Startup, Linker

- Architecture: ARM7TDMI with register-level `R8_`, `R16_`, and `R32_` access.
- Toolchain: Keil MDK `.uvproj` / `.uvopt` with scatter file and ARM startup.
- Shared source files include `CH561SFR.H`, `CH563SFR.H`, `CH561BAS.H`, `CH563BAS.H`, `SYSFREQ.C/H`, `STARTUP.S`, `ISPXT56X.H`, and `ISPXT56X.O`.
- System frequency and SRAM size are configured in `SYSFREQ.H`; startup/scatter must match.

## Memory And Boot Layout

- Source notes list 224 KB code Flash at `0x00000000`, 28 KB Data-Flash at `0x00038000`, SFR at `0x00400000`, SRAM from `0x00808000`, and optional XBUS at `0x00C00000`.
- Flash erase unit is 4 KB and minimum write is 4 bytes in source notes.
- Data-Flash is byte-writable and intended for frequently updated config/counters.
- SRAM is configurable by `MEM_DATA`: 32/64/96 KB source options.
- Scatter files define code and RAM regions; do not use CH32 RISC-V linker scripts.

## Peripheral And Example Coverage

Source examples include ADC, Flash/EEPROM, GPIO, IAP/ISP, interrupts, Ethernet NET examples, SPI0/1, timers, UART0/1, watchdog, and shared source. CH563 adds USB, GPIO PD, parallel/external bus, power examples, uC/OS-II, and broader NET examples.

## Topic Cross-References

- Ethernet/WCHNET-style rules: `Doc/ETH/wch-ethernet-notes.md`.
- Templates: `Doc/Templates/wch-project-template-notes.md`.
- Storage/eMMC/HSPI source boundaries: `Doc/Storage/wch-storage-notes.md`.
- IO/media: `Doc/IO/wch-io-media-notes.md`.
- Security note explicitly records no ECDC engine: `Doc/Security/wch-security-crypto-notes.md`.
- RTOS/uC/OS-II: `Doc/RTOS/wch-rtos-notes.md`.

## Known Family Pitfalls

- CH561/CH563 are ARM7TDMI, not CH569 RISC-V; do not reuse CH569 APIs or MounRiver project assumptions.
- CH563 has USB and GPIO PD; CH561 does not.
- Peripheral clocks are controlled by `R8_SLP_CLK_OFF0/1` with inverse-style off bits in source notes.
- SRAM size and stack top depend on `MEM_DATA` and startup/scatter configuration.
- Data-Flash behavior differs from code Flash and from CH32 Flash APIs.
- Network examples use CH561NET/CH563 NET library flows; verify task/polling model and PHY details.
- ECDC is unavailable on CH561/CH563 per source notes.

## Verification Checklist

- Verify EVT roots, Keil projects, scatter files, startup, `SYSFREQ.H/C`, and `MEM_DATA` settings.
- Verify SFR headers, register naming, interrupt syntax, and vector handler names.
- Verify Ethernet NET library, PHY address, board reset pins, and network stack task requirements.
- Verify CH563-only USB, GPIO PD, UCOS, parallel bus, and power examples before generating templates.
- Verify Data-Flash write/erase rules and IAP/ISP flows.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against CH561/CH563 EVT trees, RM, DS, Keil project files, startup files, scatter files, packages, or board schematics in this pass.
- Treat memory maps, NET library details, and CH563-only feature availability as preliminary until checked against exact official material.
