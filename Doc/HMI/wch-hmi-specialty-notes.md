# WCH Display, HMI, And Specialty Peripheral Notes

This document extracts display, human-machine-interface, external-memory, and specialty-peripheral guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future HAL metadata and templates.

Scope:

- Segment LCD, TFT LCD via FSMC/FMC/LTDC, and GPHA graphics acceleration.
- TouchKey and KEYSCAN input peripherals.
- CH32H specialty peripherals: DFSDM, I3C, QSPI, FMC, and SWPMI.
- CH56x specialty peripherals: PWMX and BUS8 external bus.

Official EVT examples, RM, DS, schematics, display panel datasheets, memory datasheets, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/lcd_display.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/fsmc_lcd.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/fsmc_lcd.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/lcd_display.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/gpha_graphics.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/touch_key.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/touchkey_sensing.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/touchkey.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/touchkey.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/keyscan.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/dfsdm_filter.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/i3c_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/qspi_flash.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/fmc_storage.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/swpmi_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/pwmx_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/bus8_interface.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`

## Peripheral Coverage

| Topic | Families from source notes | Main constraints |
|---|---|---|
| Segment LCD | CH57x CH579 notes, CH58x/CH59x CH592/CH595 notes | LCD pins are shared with GPIO; duty, bias, scan clock, COM/SEG mapping, and analog power must match the panel. |
| FSMC/FMC LCD or external memory | CH32V307/CH32V407, CH32F-style recipes, CH32H417 FMC notes | Parallel bus timing, address-line mapping, data width, GPIO AF mode, and memory bank base address must match hardware. |
| LTDC RGB display | CH32H417 | Requires panel timing, pixel format, framebuffer location, layer configuration, and enough RAM bandwidth. |
| GPHA/DMA2D | CH32H417 | Useful for fills, copies, pixel conversion, and blending; output buffers must remain valid until transfer complete. |
| TouchKey | CH5xx, CH32X/CH64x, CH32V/CH32H source recipes | Touch pins must be floating inputs, baseline calibration and hysteresis are required, thresholds are board-dependent. |
| KEYSCAN | CH57x/CH58x/CH59x source notes | Scan pins can conflict with debug/ISP or GPIO output; low-power wake requires extra power configuration on CH595. |
| DFSDM | CH32H417 | Channel, filter, oversampling, data source, DMA mux, and data scaling must be configured as one pipeline. |
| I3C | CH32H417 | Requires I3C timing, FIFO, dynamic address assignment, CCC flow, DMA mux, and I2C-legacy compatibility planning. |
| QSPI | CH32H417 | External flash command set, line mode, dummy cycles, address size, quad enable, and memory-mapped mode are device-specific. |
| SWPMI | CH32H417 | Single-wire smart-card protocol requires line reset, ready flag wait, activation, and frame-state sequencing. |
| PWMX | CH56x | Independent PWM clock divider, cycle length, duty width, polarity, and pin conflicts such as reset-pin sharing matter. |
| BUS8 | CH56x | External bus maps devices at `0x80000000`; address lines, width, setup/hold timing, and external chip protocol are board-specific. |

## Classification Rules

Do not flatten these peripherals into a single generic display or GPIO API.

Rules:

- Segment LCD, TFT parallel LCD, RGB LTDC panel, and memory-mapped external bus are different display/memory models.
- TouchKey and KEYSCAN are input peripherals with analog/scan timing requirements; they are not simple GPIO buttons.
- GPHA accelerates memory operations but does not own the display panel; keep it separate from LTDC or LVGL metadata.
- FMC/FSMC and BUS8 expose memory-mapped external devices; generated code must model external device timing and board routing.
- Specialty serial/control peripherals such as I3C, SWPMI, DFSDM, and QSPI should keep protocol-specific metadata instead of being forced into SPI/I2C/ADC abstractions.

## Display Rules

Segment LCD rules:

- Enable the LCD controller and LCD power before expecting visible output.
- Configure duty and bias to match the segment LCD glass.
- Configure scan clock from the panel and power requirements.
- Maintain a COM/SEG map; wrong data register or segment mapping causes garbled output.
- LCD-enabled segment pins are no longer normal GPIO pins. Disable or avoid conflicting GPIO use.

FSMC/FMC LCD rules:

- Enable GPIO/AFIO and memory-controller clocks before configuring the bus.
- Configure all bus pins as alternate-function push-pull or the mode required by the EVT example.
- Match data width to wiring: 8-bit, 16-bit, or 32-bit where supported.
- Record which address line is connected to LCD RS/DC. The register/data offset depends on that line.
- Use separate read/write timing when the controller and display require it.
- Respect external memory bank base addresses such as `0x60000000`, `0x64000000`, `0x68000000`, and `0x6C000000` in source notes.

LTDC rules:

- Configure pixel clock, HSYNC, VSYNC, DE, and active/back/front porch values from the panel datasheet.
- Put framebuffers in RAM accessible by LTDC with correct alignment and bandwidth.
- Keep pixel format, line length, pitch, and layer dimensions consistent.
- Use `LTDC_ReloadConfig(...)` after changing layer attributes that require reload.
- LVGL or GUI flush callbacks should copy only the invalidated area and call the GUI flush-ready hook after transfer.

GPHA rules:

- Enable GPHA clock before configuring transfers.
- Configure mode explicitly: R2M fill, M2M copy, M2M pixel-format conversion, or blending.
- Wait for transfer-complete flag before reading or reusing the output buffer.
- Keep foreground/background layer formats and alpha settings explicit in metadata.

## Touch And Keyscan Rules

TouchKey rules:

- Configure touch-capable pins as floating inputs. Pull-ups, outputs, or alternate functions corrupt measurements.
- Select one channel at a time and wait for measurement-complete flag before reading data.
- Calibrate a no-touch baseline at startup or after environmental changes.
- Use averaged samples plus press/release hysteresis thresholds.
- Treat thresholds as board-specific values affected by pad shape, enclosure, humidity, grounding, and sampling time.
- Interrupt-mode scanning still needs channel sequencing and baseline comparison.

KEYSCAN rules:

- Release pins from debug/ISP or other alternate functions before enabling matrix scan when the pins are shared.
- Configure scan pins as input pull-up before enabling keyscan hardware when the source pitfall requires it.
- Use hardware repeat/debounce plus application-level debounce when needed.
- For sleep wake, enable key wake source before entering low power.
- On CH595-style source notes, call `KeyScanPowAdj()` before `KeyScan_Cfg(...)` when using keyscan wake from sleep; this changes ULDO and can affect LSI frequency.

## Specialty Peripheral Rules

DFSDM rules:

- Configure channel, filter, regular conversion, and data source as a matched pipeline.
- Set Sinc order and oversampling based on required bandwidth, resolution, and latency.
- Configure DMA and DMA mux value for the chosen filter before starting continuous conversion.
- Scale or shift 24-bit/32-bit results consistently before passing them to application code.

I3C rules:

- Configure bus timing, controller settings, FIFO thresholds, and GPIO AF before enabling the controller.
- Model dynamic address assignment and common command codes as part of bus bring-up.
- Separate legacy I2C device support from native I3C features such as DAA and IBI.
- Check completion and error flags for CCC and private messages.

QSPI rules:

- External flash command opcodes, address size, dummy cycles, and quad-enable sequences are device-specific.
- Configure 1-line, 2-line, or 4-line instruction/address/data phases explicitly for each command.
- Use auto-polling or status reads for write-enable and write-complete sequencing.
- Memory-mapped mode should be entered only after the flash protocol and cache/bus constraints are verified.

FMC/BUS8 rules:

- Treat external devices as board-level dependencies. Timing is not determined by MCU alone.
- Keep setup time, hold time, bus turnaround, data width, and address-line count in metadata.
- Use `volatile` memory-mapped accesses for external device registers.
- Do not let compiler optimizations merge or remove register accesses that have bus side effects.

SWPMI rules:

- Configure the single-wire pin and pull it low first when the source recipe requires bus reset.
- Wait for the ready flag before activation.
- Treat ACT, U-frame, and I-frame sequences as protocol states, not simple byte writes.

PWMX rules:

- Derive PWM frequency from `Fsys / ((div + 1) * cycle_length)`.
- Keep cycle mode, duty range, polarity, and channel pin mapping explicit.
- Check pin conflicts, especially PWM0 sharing PB15 with external reset in CH56x source notes.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| LCD segment pin reused as GPIO | GPIO writes do nothing or LCD corrupts output | Reserve LCD COM/SEG pins once LCD is enabled. |
| LCD duty/bias mismatch | Faint or unreadable display | Match LCD controller duty/bias to panel glass. |
| FSMC/FMC RS address line wrong | LCD commands and data are swapped or offset | Record external wiring and compute register/data address from it. |
| Framebuffer in inaccessible RAM | LTDC/GPHA output corrupt or blank | Place buffers in controller-accessible RAM with required alignment. |
| Touch pins configured with pull-up/output | No touch, unstable touch, or false trigger | Use floating input before sensing. |
| No TouchKey baseline/hysteresis | False touches or stuck state | Calibrate baseline and use separate press/release thresholds. |
| KEYSCAN used in sleep without power adjustment | Key wake fails | Apply family-specific low-power preparation such as `KeyScanPowAdj()`. |
| QSPI dummy cycles copied across flash devices | Read data shifted or invalid | Use the external flash datasheet. |
| External bus timing copied blindly | Intermittent read/write failures | Derive timing from external device datasheet and board speed. |
| GPHA buffer reused before completion | Torn or corrupt graphics output | Wait for transfer-complete flag. |

## Metadata Checklist

Future HMI/specialty templates should represent these fields explicitly:

- Peripheral class: segment LCD, LTDC RGB panel, FSMC/FMC LCD, external memory, TouchKey, KEYSCAN, GPHA, DFSDM, I3C, QSPI, SWPMI, PWMX, or BUS8.
- Target chip, package, pin map, alternate-function map, and board-level conflicts.
- Clock gate, bus domain, DMA channel, DMA mux, interrupt, and low-power wake dependencies.
- External device parameters: panel timing, LCD glass COM/SEG map, memory timing, flash opcode table, touch pad layout, or key matrix layout.
- Buffer and memory placement: framebuffer, DMA buffers, external memory base, cache/coherency needs, and alignment.
- Calibration/tuning values: touch baselines, thresholds, key debounce, LCD scan clock, PWM divider, DFSDM oversampling.
- Verification source: exact EVT example, RM/DS chapter, external component datasheet, and board schematic.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, EVT source trees, RM, DS, board schematics, display/memory/touch component datasheets, or timing measurements in this pass.
- Treat all API names, pin mappings, timing values, DMA mux values, and feature availability as preliminary until checked against the exact target chip, package, board, and official example.
