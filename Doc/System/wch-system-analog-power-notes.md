# WCH System, Analog, Power, And Utility Peripheral Notes

This document extracts low-power, watchdog, RTC, comparator, DAC, OPA, CRC, RNG, and related utility-peripheral guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future HAL metadata and templates.

Official EVT examples, RM, DS, analog electrical specifications, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/lowpower.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/lowpower.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/power_management.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/watchdog.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/watchdog.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/watchdog.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/rtc_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/rtc_clock.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/cmp.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/compare.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/opa_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/opa_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/opa_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/dac_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/dac_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/dac_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/crc_calculation.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`

## Coverage

| Topic | Families from source notes | Main constraints |
|---|---|---|
| Low power | CH57x, CH58x/CH59x, CH56x, CH561/CH563, CH32V, CH32X, CH32F, CH32H | Wake source, clock recovery, debug behavior, BLE/USB/ETH timing, and regulator state are family-specific. |
| Watchdog | CH32V low-cost/general, CH32H, CH5xx pitfalls | Timeout clock source and refresh ownership must be explicit; watchdog can recover hangs or brick updates if mishandled. |
| RTC | CH32V general, CH32H, BLE-family API references | LSE/LSI source, backup domain, calendar format, alarm/tamper/wakeup, and calibration must be selected per chip. |
| Comparator / CMP | CH57x and CH5xx source notes; CH32 low-cost analog sources | Input routing, reference, hysteresis, interrupt clearing, and analog power are board-dependent. |
| OPA | CH32V low-cost, CH32F, CH32H | Pin routing, gain mode, calibration, and analog supply constraints matter. |
| DAC | CH32V general, CH32F, CH32H | Output buffer, trigger, DMA, reference voltage, and pin conflicts matter. |
| CRC / RNG | CH32H CRC/RNG, CH32F RNG source notes | Polynomial/seed/output format and entropy readiness must be explicit. |

## Low-Power Rules

Low-power support should be modeled as a system policy, not as a single `sleep()` helper.

Rules:

- Record mode type, wake sources, wake latency, retained RAM, clock recovery path, and peripheral retention for each chip.
- Disable or quiesce peripherals before sleep only when the selected wake policy allows it.
- BLE projects must preserve stack timing and TMOS/BLE sleep rules; see `Doc/BLE/wch-ble-notes.md`.
- USB projects must account for suspend/resume and remote wake timing.
- Ethernet projects should define whether link, PHY, DMA descriptors, and network stack state survive low power.
- Debug probes and UART logging can prevent or distort low-power measurements; document debug-mode assumptions separately from production mode.
- Measure current on the actual board because pull-ups, LEDs, PHYs, external flash, sensors, and regulators often dominate MCU sleep current.

## Watchdog Rules

Watchdog ownership must be deterministic.

Rules:

- Choose independent watchdog or window watchdog based on reset policy and clock availability.
- Store watchdog clock source, prescaler, reload/window value, timeout, and reset/interrupt behavior in metadata.
- Feed the watchdog from one deliberate owner such as a supervisor task or main-loop health check.
- Do not refresh from unrelated ISRs; that hides deadlocks in the main application.
- During IAP/OTA erase/program, either service the watchdog at verified safe points or disable/reconfigure it according to the bootloader policy.
- If the watchdog can only be enabled until reset, expose that as a project-level irreversible runtime decision.

## RTC Rules

RTC bring-up touches clock and backup domains.

Rules:

- Select LSE, LSI, or other low-speed clock source explicitly and verify board crystal population.
- Unlock backup domain before changing RTC clock configuration where the family requires it.
- Preserve backup registers and time state across reset only when the power domain is retained.
- Keep calendar format, epoch, timezone policy, and daylight-saving handling out of the low-level RTC driver.
- Alarm, periodic wake, tamper, and calibration features should be separate capabilities in metadata.

## Analog Peripheral Rules

Comparator rules:

- Enable analog/CMP power before selecting inputs or starting conversions.
- Configure input pins as analog or floating mode as the family requires.
- Keep positive input, negative input, reference source, hysteresis/filter, and output polarity explicit.
- Clear comparator flags using the family-specific write semantics; some 8051-style flags clear by writing `0`.
- Debounce or filter noisy analog thresholds in software or hardware before using them as wake/reset decisions.

OPA rules:

- Treat OPA as an analog routing block with gain, input, output, calibration, and stabilization time.
- Verify pin conflicts with ADC, comparator, DAC, GPIO, and package availability.
- Do not assume rail-to-rail behavior, bandwidth, or load drive without DS verification.

DAC rules:

- Record reference voltage, resolution, output buffer setting, trigger source, DMA support, and output pin.
- Do not generate waveforms from blocking loops when timer-triggered DMA is required for stable output.
- Keep DAC output settling time and external load in board metadata.

## CRC And RNG Rules

CRC rules:

- Store polynomial, initial value, input bit order, output bit order, and final XOR so software and hardware CRC match.
- Keep protocol CRC profiles separate from firmware-image CRC profiles.
- Verify byte/word feeding order against the target CRC peripheral before using it for boot validation.

RNG rules:

- Enable required clock and entropy source before reading random data.
- Wait for ready/data-valid flags and handle error flags.
- Do not use unverified RNG output directly for cryptographic keys without conditioning and security review.
- Keep test-mode or pseudo-random sources separate from true hardware RNG metadata.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| Low-power mode entered without wake-source setup | Device never wakes | Configure and test wake source before sleep. |
| Clock tree not restored after wake | UART, USB, BLE, or timers run at wrong speed | Reinitialize or validate clocks on wake. |
| Watchdog refreshed from ISR | Deadlocked application appears healthy | Refresh only from a health-checked owner. |
| RTC crystal assumed present | RTC does not run or drifts badly | Verify board clock source and fallback policy. |
| Comparator input left as digital output/pull-up | Wrong threshold or no interrupt | Use analog/floating mode required by the chip. |
| OPA/DAC pin conflict ignored | Analog output missing or distorted | Reserve pins and check package mapping. |
| CRC polynomial mismatch | Firmware or packet validation fails | Store full CRC profile, not just width. |
| RNG ready/error flags ignored | Repeated or invalid random data | Wait for valid data and handle errors. |

## Metadata Checklist

Future system/analog/power templates should represent these fields explicitly:

- Low-power mode, retained domains, wake sources, clock recovery, debug behavior, and measured board current notes.
- Watchdog type, clock source, timeout/window, feed owner, reset/interrupt mode, and bootloader/update policy.
- RTC clock source, backup-domain behavior, calendar capability, alarm/wakeup/tamper support, and calibration.
- Analog blocks: comparator, OPA, DAC, ADC cross-links, pins, references, calibration, trigger/DMA, and package conflicts.
- Utility blocks: CRC profile, RNG readiness/error policy, security assumptions, and firmware-validation usage.
- Verification source: exact EVT example, RM/DS chapter, schematic, and measurement result.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, EVT source trees, RM, DS, board schematics, analog measurements, low-power current measurements, or security validation in this pass.
- Treat API names, wake behavior, analog ranges, clock sources, reset behavior, CRC/RNG details, and pin mappings as preliminary until checked against the exact target chip, package, board, and official example.
