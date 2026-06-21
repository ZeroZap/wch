# WCH Storage Interface Notes

This document extracts removable and high-speed storage guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future HAL metadata, board templates, and example routing.

Official EVT examples, RM, DS, board schematics, card/device datasheets, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/sd_card.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/emmc_storage.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/hspi_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/emmc_storage.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/hspi_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/sdio_sdcard.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/sdio_sdcard.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/sdio_sdcard.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/qspi_flash.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`

## Interface Coverage

| Interface | Families from source notes | Main constraints |
|---|---|---|
| SD card over SDIO/SDMMC | CH32V307/CH32V407, CH32F-style sources, CH32H417 | Card detect/write protect are board-specific; clock must start slow for identification and then switch after card negotiation. |
| SD card over CH56x storage examples | CH569 / CH56x | Buffer placement and DMA accessibility must be checked against CH56x memory rules. |
| eMMC | CH56x and CH561/CH563 source notes | Fixed soldered storage; bus width, partition, block length, and initialization flow differ from removable SD card policy. |
| HSPI | CH56x and CH561/CH563 source notes | High-speed host/slave style interface with DMA/FIFO/timing requirements; do not model it as ordinary low-speed SPI. |
| QSPI external flash | CH32H417 source notes | Flash command set, dummy cycles, address width, quad-enable, and memory-mapped mode are device-specific. |

## Classification Rules

- Keep SD card, eMMC, HSPI, QSPI, ordinary SPI flash, and internal MCU Flash as separate metadata classes.
- SDIO/SDMMC storage APIs need card-state, bus-width, block-size, clock, DMA, and filesystem integration fields.
- eMMC templates should not assume card-detect or removable media behavior.
- HSPI should preserve its dedicated FIFO, DMA, clock, and role configuration instead of being folded into the generic SPI abstraction.
- QSPI memory-mapped mode is an external memory/bus decision, not a replacement for internal program Flash unless the target explicitly supports execute-in-place and cache rules.

## SDIO And SD Card Rules

Generic SD initialization sequence:

1. Enable GPIO, SDIO/SDMMC, DMA, and card-detect clocks as required.
2. Configure command, clock, and data pins in the correct alternate-function mode.
3. Start with a safe identification clock before high-speed transfer.
4. Reset and identify the card, read CID/CSD, and determine capacity/version.
5. Select bus width only after checking card and board wiring support.
6. Increase transfer clock only after initialization completes.
7. Align data buffers for DMA and keep them alive for the full transfer.
8. Handle card removal, write protection, timeout, CRC, and transfer-complete status.

Rules:

- Do not assume 4-bit mode is available; many boards wire only 1-bit mode.
- Do not put sector buffers on short-lived stacks when DMA is used.
- Separate block-device access from filesystem integration such as FATFS.
- Treat card-detect and write-protect pins as board metadata, not MCU features.
- Verify whether DMA buffers must reside in a specific RAM region for the target family.

## eMMC Rules

eMMC is soldered managed NAND and needs a different policy than removable SD:

- Model bus width, voltage, clock, partition selection, boot partition use, and erase group size explicitly.
- Preserve command/response timeout handling; eMMC busy states can be longer than simple register polling expects.
- Keep block size and address mode tied to the device and initialization result.
- Avoid power-cycling assumptions that only work with removable SD sockets.
- If eMMC is used for firmware images, cross-check with `Doc/IAP/wch-iap-ota-notes.md` before defining boot/update layouts.

## HSPI Rules

HSPI examples belong to high-speed data movement, not generic SPI snippets.

Rules:

- Keep master/slave role, data width, FIFO threshold, DMA channel, and interrupt ownership explicit.
- Derive bus timing from the selected clock tree and external device requirements.
- Use static or long-lived DMA buffers with required alignment.
- Use flow control, ready flags, or packet framing where the peer device requires it.
- Do not reuse low-speed SPI blocking-transfer helpers for HSPI throughput paths without EVT verification.

## QSPI Rules

QSPI external flash support is device-specific.

Rules:

- Store flash opcode tables, address width, dummy cycles, line mode, page size, erase size, and status-register layout in board/device metadata.
- Execute write-enable, erase, program, and wait-busy sequences exactly as the flash datasheet requires.
- Enter quad mode only after setting and verifying the flash vendor's quad-enable bit.
- Memory-mapped mode should be enabled only after command timing, dummy cycles, cache behavior, and address range are verified.
- Keep QSPI erase/program operations separate from internal Flash HAL APIs.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| SD clock switched too fast during identification | Card does not initialize | Start slow and increase only after card selection. |
| Assuming 4-bit SD bus wiring | No data transfer or CRC errors | Record board bus width and pin map. |
| DMA buffer on stack or wrong RAM | Transfer failure or corrupted data | Use aligned static/global buffers in DMA-accessible memory. |
| Treating eMMC like removable SD | Wrong detect, power, or partition handling | Model fixed-media behavior and eMMC partitions. |
| Folding HSPI into generic SPI | Missing FIFO/DMA/timing behavior | Keep HSPI as a dedicated high-speed interface. |
| Copying QSPI dummy cycles between flash chips | Invalid reads | Use the external flash datasheet. |

## Metadata Checklist

Future storage templates should represent these fields explicitly:

- Interface class: SDIO/SDMMC SD card, eMMC, HSPI, QSPI flash, or ordinary SPI flash.
- Target chip, package, clock source, bus clock, pins, alternate-function map, DMA channel, interrupt, and memory region.
- Board fields: card detect, write protect, power switch, voltage, bus width, external flash/eMMC part number, and routing limits.
- Transfer fields: block size, sector size, erase group size, FIFO threshold, alignment, timeout, and CRC/error policy.
- Integration fields: FATFS/block-device adapter, firmware-update storage role, boot partition use, and cache/coherency constraints.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, EVT source trees, RM, DS, board schematics, SD/eMMC/QSPI device datasheets, filesystem ports, or timing measurements in this pass.
- Treat API names, DMA requirements, memory placement, command sequences, and supported bus modes as preliminary until checked against the exact target chip, board, and official example.
