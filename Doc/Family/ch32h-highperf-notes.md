# CH32H High-Performance Family Notes

This document extracts CH32H417 high-performance guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for high-performance HAL metadata and templates.

Official EVT examples, RM, DS, startup files, linker scripts, board schematics, external memory/display/storage datasheets, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Family/source group | Expected EVT root | Architecture/toolchain | Key constraints |
|---|---|---|---|
| CH32H417 | Verify CH32H417 EVT import in repository | High-performance WCH RISC-V, MounRiver | V5F/V3F linker split, bootloader offset, ITCM/DTCM, high-speed peripherals. |

## Architecture, Toolchain, Startup, Linker

- Toolchain: MounRiver `.wvproj` / `.project` source flow.
- Source notes distinguish V5F and V3F core/linker profiles.
- Built-in bootloader occupies `0x00000000` to `0x0000FFFF`; V5F application starts at `0x00010000` in source notes.
- V5F source memory notes use ITCM at `0x200A0000` and DTCM at `0x200C0000`.
- `.highcode` can run performance-critical code from ITCM.

## Memory And Boot Layout

- Flash capacity examples in source notes: 480 KB and 960 KB variants; application region depends on capacity after the 64 KB bootloader region.
- Flash erase unit is 4 KB and program unit is 4 bytes in source notes.
- V5F DTCM holds `.data`, `.bss`, heap, and stack; ITCM holds `.highcode` and optional fast code.
- External FMC memory ranges are listed for banks from `0x60000000` through `0x9FFFFFFF`.

Rules:

- Do not use `0x00000000` as application origin for normal CH32H417 apps.
- Do not mix V5F and V3F linker scripts.
- Place framebuffers, DMA buffers, and high-speed peripheral buffers in memory accessible by the corresponding controller.
- Partial Flash updates require 4 KB read-modify-erase-write.

## Peripheral And Example Coverage

Source examples include ADC/HSADC, CAN1/2/3, CRC, DAC, DFSDM, DMA/MUX, DVP, ECDC, ETH, EXTI, Flash, FMC, GPIO, GPHA, I2C, I2S, I3C, IAP, watchdogs, LPTIM, LTDC, OPA/CMP, PIOC, PWR, QSPI, RCC, RNG, RTC, SAI, SDIO/SDMMC, SerDes, SPI, SWPMI, timers, TKey, UHSIF, USART, USBFS, USBHS, USBPD, USBSS, and WWDG.

## Topic Cross-References

- Common peripherals: `Doc/HAL/wch-hal-normalization.md`.
- USB/USB3: `Doc/USB/wch-usb-notes.md`.
- USB-PD/PIOC: `Doc/USBPD/wch-usbpd-notes.md`.
- Ethernet: `Doc/ETH/wch-ethernet-notes.md`.
- Display/HMI/FMC/QSPI/I3C/SWPMI/DFSDM: `Doc/HMI/wch-hmi-specialty-notes.md`.
- CAN/I2S/SAI/DVP: `Doc/IO/wch-io-media-notes.md`.
- SDIO/SDMMC/eMMC/QSPI storage: `Doc/Storage/wch-storage-notes.md`.
- ECDC/CRC/RNG: `Doc/Security/wch-security-crypto-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.

## Known Family Pitfalls

- Peripheral clocks must be enabled before configuration.
- `GPIO_PinAFConfig()` alternate-function number must match the datasheet.
- Bootloader space at `0x0000-0xFFFF` must not be overwritten.
- GPIO must be configured for alternate function before peripheral TX/RX.
- I2C bus hangs may require bus recovery and careful STOP handling.
- DMA buffers require correct alignment and controller-accessible memory.
- LTDC framebuffer placement is critical.
- Ethernet PHY needs reset, reference clock, and MDIO/MDC verification.
- CAN filter banks must be split correctly for multiple CAN controllers.
- USB-PD CC pull/comparator settings must match role and board.
- SerDes/USB3 requires PLL lock and PHY-ready checks.

## Verification Checklist

- Verify CH32H417 EVT root and all example categories imported into this repository.
- Verify V5F/V3F linker scripts, startup, system clock, bootloader offset, ITCM/DTCM section placement, and Flash size variant.
- Verify high-speed examples for USBSS/SerDes, ETH, SDIO/SDMMC, FMC/QSPI, LTDC/GPHA, DVP, SAI/I2S, ECDC/CRC/RNG.
- Verify DMA/cache/coherency/memory-access rules for framebuffers, Ethernet buffers, storage buffers, and crypto buffers.
- Verify board schematics for PHY, display, external memory, QSPI flash, USB-PD, camera, audio codec, and SD/eMMC wiring.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against CH32H417 EVT source trees, RM, DS, project files, startup files, linker scripts, packages, or board schematics in this pass.
- Treat high-performance peripheral availability, linker values, memory placement, and timing rules as preliminary until checked against exact official material.
