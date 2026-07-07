# CH32F And CH32M ARM Family Notes

This document extracts CH32F10x, CH32F20x, and CH32M030 ARM-family guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes.

Official EVT examples, RM, DS, startup files, linker/scatter files, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Source group | Repository EVT roots | Header family | Key constraints |
|---|---|---|---|
| CH32F10x / CH32F103 | Verify `CH32F103EVT/` availability | `ch32f10x.h`, `ch32f10x_*` | Cortex-M3 style, Flash at `0x08000000`, 1 KB pages, USB custom register API. |
| CH32F20x | Verify `CH32F20xEVT/` availability | `ch32f20x.h`, `ch32f20x_*` | More RAM/features, Ethernet, DVP, I2S, SDIO, USBHS, RNG, BLE examples in source notes. |
| CH32M030 | `CH32M030EVT/` | CH32F-style headers in source notes | Motor/control and USB-PD-oriented variant; verify exact header and examples. |

## Architecture, Toolchain, Startup, Linker

- Architecture: ARM Cortex-M style family using StdPeriphDriver-like APIs.
- Toolchains: Keil MDK and MounRiver projects appear in source notes; preserve original EVT project metadata.
- Flash base is `0x08000000`, unlike zero-based CH32V RISC-V layouts.
- Project files include `*_conf.h`, `*_it.c/h`, `system_*.c/h`, debug helpers, startup, and linker/scatter metadata.

## Memory And Boot Layout

- CH32F10x source notes list 32 KB to 512 KB Flash and 10 KB to 64 KB SRAM variants.
- CH32F20x source notes list 64 KB to 256 KB Flash and 20 KB or 64 KB SRAM variants.
- CH32F10x IAP example uses app origin `0x08001000`; CH32F20x IAP example uses app origin `0x08002000`.
- Vector table starts at Flash base and can be relocated; verify `SCB->VTOR` handling in IAP projects.

## Peripheral And Example Coverage

Source examples cover ADC, BKP, CAN, CRC, DAC, DMA, EXTI, Flash, GPIO, I2C, watchdogs, PWR, RCC, RTC, SPI, SysTick, TIM, TouchKey, USART, USB, RTOS, and IAP. CH32F20x sources add DVP, Ethernet, FSMC, I2S, OPA, RNG, SDIO, USBHS, BLE examples, and more application examples. CH32M030 sources include OPA/CMP/PGA, USBFS, USBPD, motor/power examples, and RTOS examples.

## Topic Cross-References

- Common peripherals: `Doc/HAL/wch-hal-normalization.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.
- USB/USBHS: `Doc/USB/wch-usb-notes.md`.
- Ethernet: `Doc/ETH/wch-ethernet-notes.md`.
- CAN/I2S/DVP: `Doc/IO/wch-io-media-notes.md`.
- SDIO/storage: `Doc/Storage/wch-storage-notes.md`.
- USB-PD: `Doc/USBPD/wch-usbpd-notes.md`.
- Security/RNG/CRC: `Doc/Security/wch-security-crypto-notes.md`.

## Known Family Pitfalls

- Do not mix `ch32f10x` and `ch32f20x` headers or driver files.
- Enable RCC clocks before peripheral configuration.
- Configure NVIC priority grouping before interrupt priorities.
- Flash must be erased before write; CH32F10x and CH32F20x page sizes differ.
- GPIO alternate-function mode must match peripheral type, especially I2C open-drain and ADC/DAC analog mode.
- CH32F USB uses WCH custom register APIs, not STM32 USB OTG APIs.
- Interrupt handler names must exactly match the vector table.
- Keep ISR work short; use flags or queues for longer processing.
- Avoid large stack buffers in USB, Ethernet, SDIO, DVP, and RTOS examples.

## Verification Checklist

- Verify exact EVT roots, project files, compiler, startup, system, linker/scatter, and debug helper files for each chip.
- Verify `0x08000000` absolute Flash origins and IAP offsets before generating linker files.
- Verify CH32F20x-only features such as RNG, Ethernet, SDIO, DVP, USBHS, BLE, and OPA against exact chip/package.
- Verify CH32M030 USBPD, OPA/CMP/PGA, Flash ECC, and motor-control examples against EVT/RM/DS.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all CH32F/CH32M EVT trees, RM, DS, project files, startup files, linker/scatter files, packages, or board schematics in this pass.
- Treat feature availability and memory values as preliminary until checked against exact official material.
