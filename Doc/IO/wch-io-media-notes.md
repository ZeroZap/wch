# WCH IO, Audio, CAN, And Camera Interface Notes

This document extracts CAN, I2S, SAI, and DVP camera guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future HAL metadata, streaming templates, and board-level example routing.

Official EVT examples, RM, DS, bus specifications, sensor/audio codec datasheets, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/can_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/can_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/can_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/i2s_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/i2s_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/i2s_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/sai_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/dvp_camera.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/dvp_camera.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/dvp_camera.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`

## Interface Coverage

| Interface | Families from source notes | Main constraints |
|---|---|---|
| CAN | CH32V general, CH32F-style sources, CH32H417 | Bit timing, filter banks, transceiver pins, termination, and bus-off recovery are board/protocol decisions. |
| I2S | CH32V general, CH32F-style sources, CH32H417 | Audio clock accuracy, frame format, master/slave role, DMA double-buffering, and codec control are required for stable audio. |
| SAI | CH32H417 | Multi-slot audio framing, master clock, FIFO/DMA, and codec synchronization need explicit metadata. |
| DVP camera | CH56x, CH32F-style sources, CH32H417 | Sensor clock/reset/config bus, pixel format, sync polarity, DMA buffer placement, and frame size are sensor/board-specific. |

## Classification Rules

- Keep CAN, I2S, SAI, DVP, USB audio/video, and ordinary SPI/UART streams as separate metadata classes.
- Streaming peripherals need buffer ownership, DMA, interrupt, timing, and backpressure rules; do not model them as simple blocking read/write calls.
- Board dependencies such as CAN transceiver standby pins, audio codec control bus, camera reset/power pins, and external clocks must be explicit.
- Protocol-layer settings belong above the peripheral driver: CANopen/J1939, audio file/codec stack, and image processing are not low-level HAL responsibilities.

## CAN Rules

CAN setup is split between MCU peripheral timing and board-level physical bus behavior.

Rules:

- Store CAN instance, RX/TX pins, remap, clock source, IRQ, filter-bank ownership, and transceiver control pins in metadata.
- Derive prescaler, time segments, and sample point from peripheral clock and requested bitrate.
- Verify bus termination, transceiver voltage, standby/silent pin state, and common ground before debugging software filters.
- Configure acceptance filters intentionally; permissive filters are useful for bring-up but unsafe as final protocol policy.
- Handle error warning, error passive, bus-off, arbitration lost, RX overflow, and TX mailbox empty separately.
- Define bus-off recovery policy explicitly; automatic recovery may hide wiring or bitrate faults.

## I2S And SAI Rules

Audio interfaces require stable clocks and continuous buffers.

Rules:

- Record sample rate, bit depth, channel count, frame format, clock polarity, master/slave role, MCLK use, and codec requirements.
- Generate audio clocks from a source that can meet sample-rate error requirements; do not reuse approximate system-clock divisors blindly.
- Configure DMA circular or double-buffer mode for continuous playback/capture.
- Keep DMA buffers aligned, static, and sized for latency plus interrupt/service jitter.
- Separate audio data movement from codec register configuration, which may use I2C/SPI/GPIO control pins.
- For SAI, model slot count, slot size, frame length, sync polarity, FIFO threshold, and block selection explicitly.

## DVP Camera Rules

DVP is a parallel camera capture interface with sensor-specific timing.

Rules:

- Store data width, pixel clock edge, HSYNC/VSYNC polarity, JPEG/RGB/YUV mode, frame dimensions, crop/window settings, and DMA buffer addresses.
- Configure sensor power, reset, SCCB/I2C control bus, and external clock before enabling capture.
- Match DVP pixel format with sensor output format and downstream buffer interpretation.
- Use DMA buffers in memory accessible by the DVP/DMA engine; CH56x-class sources require special attention to memory regions.
- Handle frame start, frame stop, row done, frame done, and FIFO overflow as distinct events.
- For JPEG mode, keep compressed byte count and buffer boundary handling separate from fixed-width RGB frame math.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| CAN bitrate copied across clock trees | No bus communication or error passive state | Recompute timing from peripheral clock. |
| CAN transceiver standby pin ignored | TX/RX pins toggle but bus is silent | Model transceiver GPIO and board power. |
| Audio clock approximation too large | Pitch error, underruns, codec lock failure | Verify sample-rate clock error. |
| Audio DMA buffer on stack | Glitches or memory corruption | Use aligned static/global buffers. |
| Codec configuration mixed into I2S driver | Board templates become non-reusable | Keep codec control as board/device layer. |
| DVP sync polarity wrong | Shifted, blank, or torn frames | Match sensor datasheet and EVT settings. |
| DVP buffer in inaccessible memory | Capture fails or corrupts output | Place buffers in DMA-accessible memory. |
| JPEG and RGB frame sizing mixed | Buffer overflow or truncated images | Track compressed and raw modes separately. |

## Metadata Checklist

Future IO/media templates should represent these fields explicitly:

- Interface class: CAN, I2S, SAI, DVP camera, or related board-level codec/sensor profile.
- Target chip, instance, clock source, bus clock, pins, remap/AF, IRQ, DMA channel, and DMA-accessible memory region.
- CAN fields: bitrate, sample point, filters, transceiver pins, termination assumption, error handling, and recovery policy.
- Audio fields: sample rate, bit depth, channels, frame format, MCLK, master/slave role, DMA buffering, and codec control bus.
- DVP fields: sensor model, control bus, reset/power pins, input clock, sync polarity, pixel format, frame size, DMA buffers, and overflow policy.
- Verification source: exact EVT example, RM/DS chapter, schematic, bus/sensor/codec datasheet, and measured timing result.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, EVT source trees, RM, DS, board schematics, CAN/audio/camera external component datasheets, or timing measurements in this pass.
- Treat API names, bit timing values, clock divisors, DMA requirements, pin mappings, and buffer placement rules as preliminary until checked against the exact target chip, package, board, and official example.
