# CH32V General Family Notes

This document extracts CH32V103, CH32V20x, CH32V307, and CH32V407 family guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for future HAL metadata and template work.

Official EVT examples, RM, DS, startup files, linker scripts, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Subfamily | Example EVT roots in this repository | Header pattern | Notes |
|---|---|---|---|
| CH32V103 | `CH32V103EVT/` | `ch32v10x.h`, `ch32v10x_*` | General StdPeriph-style RISC-V baseline; feature tables in source notes should be verified per chip. |
| CH32V20x | `CH32V20xEVT/` | `ch32v20x.h`, `ch32v20x_*` | V20x/V208 variants add BLE/USB/high-speed features depending package. |
| CH32V307 | `CH32V307EVT/` | `ch32v30x.h`, `ch32v30x_*` | Higher-performance family with Ethernet examples and 64 KB RAM variants. |
| CH32V407 | `CH32V407EVT/` | `ch32v4x7.h`, `ch32v4x7_*` | High-feature CH32V family with USB HS, Ethernet, SDIO, DVP, FSMC/LTDC-style source references. |

## Architecture, Toolchain, Startup, Linker

- Architecture: WCH RISC-V with StdPeriphDriver-style peripheral APIs.
- Toolchain: MounRiver Studio project files (`.project`, `.cproject`, `.wvproj`) with GCC-style `Ld/Link.ld`.
- Shared project shape: `User/`, `SRC/Peripheral/inc`, `SRC/Peripheral/src`, `SRC/Startup`, `SRC/RVMSIS`, and `Ld/Link.ld`.
- Template starting point: copy the closest EVT example rather than building from snippets.
- Debug convention: `SystemCoreClockUpdate()`, delay/debug init, `USART_Printf_Init(115200)`, and chip ID print are common in source templates.

## Memory And Boot Layout

Memory size varies strongly by subfamily and package.

| Subfamily | Source-note Flash/RAM examples | Flash erase note |
|---|---|---|
| CH32V103 | 32/64/128 KB Flash, 10/20 KB RAM | Source note lists 1 KB pages. |
| CH32V20x | 32/64/128 KB common, larger D8 example noted; 10/20/64 KB RAM | Source note lists 4 KB pages. |
| CH32V307 | 128/256 KB Flash, 64 KB RAM | Source note lists 4 KB pages. |
| CH32V407 | 256 KB or 1 MB Flash, 64 KB RAM | Source note lists 4 KB pages. |

Rules:

- Do not reuse one `Link.ld` across chip memory densities.
- IAP layouts in source notes include a 16 KB bootloader with app at `0x00004000` for the CH32V general example. Verify against the actual EVT IAP project before applying.
- Flash write requires erase first; partial updates need read-modify-erase-write around the erase page.
- Stack and heap sizes are linker-controlled and must be adjusted for RTOS, USB, ETH, SDIO, graphics, or large buffers.

## Peripheral Coverage Highlights

The source example index lists broad coverage, but exact support must be verified by chip:

- Common: GPIO, USART, SPI, I2C, ADC, Timer/PWM, DMA, EXTI, Flash, RCC, PWR, watchdogs.
- Communications: CAN on all listed subfamilies in the source matrix; USB FS broadly, USB HS on CH32V407-style sources.
- High-speed and media: Ethernet on capable V208/V307/V407 variants, SDIO, FSMC, DVP, I2S, LTDC/ARGB/I3C/PSRAM where source examples list them.
- System and analog: RTC, CRC, RNG, DAC, OPA, TouchKey, PMP, FPU depending subfamily.
- RTOS: source index lists FreeRTOS/RT-Thread/HarmonyOS/TencentOS for CH32V103/V307/V407, but not CH32V20x in the table; verify repository EVT coverage.

## Topic Cross-References

- Common peripheral rules: `Doc/HAL/wch-hal-normalization.md`.
- Startup/linker/interrupt rules: `Doc/Core/wch-core-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.
- Ethernet: `Doc/ETH/wch-ethernet-notes.md`.
- USB: `Doc/USB/wch-usb-notes.md`.
- CAN/I2S/DVP: `Doc/IO/wch-io-media-notes.md`.
- SDIO/storage: `Doc/Storage/wch-storage-notes.md`.
- IAP: `Doc/IAP/wch-iap-ota-notes.md`.
- RTOS: `Doc/RTOS/wch-rtos-notes.md`.

## Known Family Pitfalls

- Peripheral clocks must be enabled on the correct bus before register access.
- Alternate-function GPIO modes must be selected before enabling peripheral signals.
- I2C examples require event checks at each bus phase; skipping them causes hangs or garbage transfers.
- CAN receive requires filter configuration; no filter means incoming messages can be rejected.
- ADC calibration is required before accurate conversion.
- `printf()` requires retargeting through `_write()` or equivalent debug helper.
- Ethernet, USB, SDIO, DVP, and large DMA buffers should not be allocated on small stacks.
- Do not assume CH32V103/V20x/V307/V407 have identical high-speed peripheral sets.

## Verification Checklist

- Verify each subfamily EVT root and exact example paths for GPIO, USART, ADC, Timer, Flash, DMA, USB, CAN, ETH, SDIO, IAP, and RTOS.
- Verify header prefix, startup file, system file, linker script, `*_conf.h`, and interrupt handler names per subfamily.
- Verify Flash/RAM size and package-specific peripheral availability against DS/RM.
- Verify IAP linker offsets and vector relocation functions against the selected IAP EVT example.
- Verify high-speed peripheral buffer placement and DMA requirements from active headers and examples.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all CH32V103/V20x/V307/V407 EVT trees, RM, DS, project files, startup files, linker scripts, packages, or board schematics in this pass.
- Treat feature availability and memory values as leads until checked against the exact target chip and official EVT material.
