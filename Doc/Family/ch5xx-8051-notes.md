# CH5xx 8051 Family Notes

This document extracts CH543 through CH559 8051-family guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for C51 templates, USB, Type-C, IAP, TouchKey, and low-resource HAL work.

Official EVT examples, datasheets, register headers, Keil C51 projects, ISP bootloader behavior, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

Source notes cover CH543, CH545, CH547, CH549, CH552/CH551, CH554, CH555, CH557, CH558, and CH559. Some EVT packages contain only headers and should borrow examples from related chips only after register compatibility is verified.

## Architecture, Toolchain, Startup, Linker

- Architecture: 8051 Harvard architecture with separate code, data/idata, xdata, bit, and SFR spaces.
- Toolchains: Keil C51 and possibly SDCC-style flows; source recipes emphasize Keil C51 `.uvproj` projects.
- Common project roots include `Public/CH5xx.H`, `Debug.C/H`, app `Main.C`, and peripheral `.C/.H` files.
- C51 memory model, code banking, and `data`/`idata`/`xdata` qualifiers are template-critical.

## Memory And Boot Layout

- Code Flash varies from about 10 KB to 64 KB across chips in source notes.
- `data` is 128 bytes direct addressing; `idata` is 256 bytes indirect addressing; xdata ranges from 512 bytes to 4 KB depending chip.
- DataFlash appears at `0xC000` for several CH55x/CH54x chips, with chip-specific size.
- Built-in ISP bootloader is at upper code Flash for many CH5xx chips; application normally starts at `0x0000` and must not exceed the bootloader region.
- Interrupt vectors occupy low code addresses starting at reset vector `0x0000`.

## Peripheral And Example Coverage

Source examples include ADC, comparator, DataFlash/FlashRom, GPIO, I2C, USB-PD/Type-C on some chips, PWM, SPI, timers, TouchKey, UART, USB device/host, IAP, RGB/LED control, encryption/chip ID examples, and mass-storage-like USB examples depending chip.

## Topic Cross-References

- 8051 core/memory rules: `Doc/Core/wch-core-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.
- USB: `Doc/USB/wch-usb-notes.md`.
- Type-C: `Doc/USBPD/wch-usbpd-notes.md`.
- TouchKey/HMI: `Doc/HMI/wch-hmi-specialty-notes.md`.
- IAP/ISP: `Doc/IAP/wch-iap-ota-notes.md`.
- Common peripherals: `Doc/HAL/wch-hal-normalization.md`.

## Known Family Pitfalls

- Do not use CH32 or CH57x headers, startup, linker, interrupt attributes, or project structures.
- Use protected-register safe mode sequences before writing protected system registers.
- Keep `EA` interrupt state controlled during Flash/DataFlash writes.
- Use correct C51 memory qualifiers; large buffers belong in `xdata`, not scarce `data`.
- USB buffers may require even alignment and specific xdata placement.
- ISP bootloader occupies upper ROM; Keil code size/ROM range must prevent overlap.
- DataFlash access requires chip-specific ROM address registers and safe-mode/interrupt handling.
- TouchKey and comparator pins require analog/floating input configuration.

## Verification Checklist

- Verify which CH5xx EVT roots and headers are present in this repository.
- Verify chip-specific `CH5xx.H`, SFR names, interrupt numbers, vector layout, and C51 project settings.
- Verify DataFlash size/address, ISP bootloader address, code ROM limit, and IAP jump address per chip.
- Verify USB device/host examples, endpoint buffers, Type-C pins, TouchKey pads, and board-level constraints.
- Verify Keil memory model, code banking, xdata size, and debug UART setup for generated templates.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all CH5xx EVT trees, datasheets, Keil projects, headers, ISP bootloader behavior, or board schematics in this pass.
- Treat memory maps, bootloader addresses, DataFlash behavior, and example compatibility as preliminary until checked against exact chip material.
