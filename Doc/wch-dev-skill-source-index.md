# wch-dev-skill Markdown Source Index

This index tracks the Markdown files under `Doc/Ref/wch-dev-skill` that should be progressively extracted into repository-specific documentation.

Scope for this index:

- Included: top-level skill docs, `chips/*/recipes/*.md`, and `chips/*/resources/*.md`.
- Excluded for the first pass: `chips/*/resources/EXAM/**/*.md`, because those files mostly belong to copied examples or third-party packages and should be reviewed separately only when the related example is migrated.

## Top-Level Skill Docs

| Source | Purpose | Initial target |
|---|---|---|
| `Doc/Ref/wch-dev-skill/SKILL.md` | Global routing, rules, workflows, scenarios, pitfalls | `Doc/Family/`, `Doc/HAL/`, topic docs |
| `Doc/Ref/wch-dev-skill/AGENTS.md` | Coding conventions, include patterns, main loop templates, checklists | `Doc/Core/`, `Doc/HAL/`, topic docs |
| `Doc/Ref/wch-dev-skill/README.md` | Chinese overview and installation notes | Reference only |
| `Doc/Ref/wch-dev-skill/README_EN.md` | English overview and installation notes | Reference only |
| `Doc/Ref/wch-dev-skill/CHANGELOG.md` | Upstream skill history | Reference only |

## Family Resource Docs

These files usually contain the highest-value compact reference material for extraction.

| Family | Resource docs | Primary target |
|---|---|---|
| `ch57x` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `hal_reference.md`, `example_list.md`, `config_reference.md`, `ble_api.md` | `Doc/BLE/`, `Doc/Core/`, `Doc/Family/` |
| `ch58x-ch59x` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md`, `config_reference.md` | `Doc/BLE/`, `Doc/USB/`, `Doc/Family/` |
| `ch32v-general` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/HAL/`, `Doc/Core/`, `Doc/Family/` |
| `ch32v-lowcost` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/HAL/`, `Doc/Core/`, `Doc/USBPD/`, `Doc/Family/` |
| `ch32f-arm` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/HAL/`, `Doc/Core/`, `Doc/Family/` |
| `ch32x-usbpd` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/USBPD/`, `Doc/USB/`, `Doc/HAL/`, `Doc/Family/` |
| `ch56x-ethernet` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/ETH/`, `Doc/USB/`, `Doc/Core/`, `Doc/Family/` |
| `ch561-ch563` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/ETH/`, `Doc/Core/`, `Doc/Family/` |
| `ch32h-highperf` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/HAL/`, `Doc/USB/`, `Doc/ETH/`, `Doc/Core/`, `Doc/Family/` |
| `ch5xx-8051` | `pitfalls.md`, `peripheral_api.md`, `memory_layout.md`, `example_list.md` | `Doc/Core/`, `Doc/USB/`, `Doc/Family/` |

## Recipe Groups By Target Topic

### Project And Family Setup

Sources:

- `chips/*/recipes/new_project.md`

Targets:

- `Doc/Family/`
- `Doc/Core/`
- future template or project-layout notes

### Common HAL Peripherals

Sources:

- `chips/*/recipes/gpio_control.md`
- `chips/*/recipes/uart_comm.md`
- `chips/*/recipes/spi_comm.md`
- `chips/*/recipes/i2c_comm.md`
- `chips/*/recipes/adc_reading.md`
- `chips/*/recipes/timer_pwm.md`
- `chips/*/recipes/flash_storage.md`
- `chips/*/recipes/dma_transfer.md`
- `chips/*/recipes/exti_interrupt.md`
- `chips/*/recipes/power_management.md`
- `chips/*/recipes/watchdog.md`
- `chips/*/recipes/rtc_config.md`
- `chips/*/recipes/rtc_clock.md`

Targets:

- `Doc/HAL/`
- `Doc/Core/`
- `Doc/Family/`

### BLE And 2.4 GHz RF

Sources:

- `chips/ch57x/recipes/ble_*.md`
- `chips/ch57x/recipes/rf_comm.md`
- `chips/ch58x-ch59x/recipes/ble_*.md`
- `chips/ch57x/resources/ble_api.md`
- `chips/ch57x/resources/config_reference.md`
- `chips/ch58x-ch59x/resources/config_reference.md`

Targets:

- `Doc/BLE/`
- `Doc/Core/`
- `Doc/Family/`

### USB And USB3

Sources:

- `chips/*/recipes/usb_device.md`
- `chips/*/recipes/usb_host.md`
- `chips/ch56x-ethernet/recipes/usb3_device.md`
- `chips/ch32h-highperf/recipes/usb3_device.md`
- `chips/ch32h-highperf/recipes/usbhs_device.md`
- `chips/ch32h-highperf/recipes/usbfs_device.md`

Targets:

- `Doc/USB/`
- `Doc/Core/`
- `Doc/Family/`

### USB-PD, Type-C, And PIOC

Sources:

- `chips/ch32x-usbpd/recipes/usbpd_config.md`
- `chips/ch32x-usbpd/recipes/pio_config.md`
- `chips/ch32f-arm/recipes/usbpd.md`
- `chips/ch32h-highperf/recipes/usbpd_config.md`
- `chips/ch5xx-8051/recipes/type_c.md`

Targets:

- `Doc/USBPD/`
- `Doc/USB/`
- `Doc/HAL/`

### Ethernet And High-Speed Interfaces

Sources:

- `chips/*/recipes/eth_comm.md`
- `chips/ch32f-arm/recipes/ethernet.md`
- `chips/ch56x-ethernet/recipes/emmc_storage.md`
- `chips/ch561-ch563/recipes/emmc_storage.md`
- `chips/ch56x-ethernet/recipes/hspi_comm.md`
- `chips/ch561-ch563/recipes/hspi_comm.md`
- `chips/ch56x-ethernet/recipes/ecdc_crypto.md`
- `chips/ch561-ch563/recipes/ecdc_crypto.md`
- `chips/ch32h-highperf/recipes/ecdc_crypto.md`

Targets:

- `Doc/ETH/`
- `Doc/HAL/`
- `Doc/Core/`

### Display, HMI, And Specialty Peripherals

Sources:

- `chips/*/recipes/fsmc_lcd.md`
- `chips/*/recipes/lcd_display.md`
- `chips/*/recipes/touchkey*.md`
- `chips/ch57x/recipes/keyscan.md`
- `chips/ch32h-highperf/recipes/gpha_graphics.md`
- `chips/ch32h-highperf/recipes/dfsdm_filter.md`
- `chips/ch32h-highperf/recipes/i3c_comm.md`
- `chips/ch32h-highperf/recipes/qspi_flash.md`
- `chips/ch32h-highperf/recipes/swpmi_comm.md`
- `chips/ch32h-highperf/recipes/fmc_storage.md`
- `chips/ch56x-ethernet/recipes/pwmx_output.md`
- `chips/ch56x-ethernet/recipes/bus8_interface.md`

Targets:

- `Doc/HAL/`
- `Doc/Family/`

### IAP, OTA, Bootloader, And Flash Layout

Sources:

- `chips/*/recipes/iap_ota.md`
- `chips/ch5xx-8051/recipes/iap_bootloader.md`
- `chips/*/resources/memory_layout.md`

Targets:

- `Doc/IAP/`
- `Doc/Core/`
- `Doc/Family/`

## First-Pass Extraction Order

1. `Doc/Family/family-routing.md`: global family routing and source directory mapping. Done.
2. `Doc/BLE/wch-ble-notes.md`: CH57x and CH58x/CH59x BLE rules, config, and examples. Done for first Markdown extraction pass; EVT verification remains.
3. `Doc/Core/wch-core-notes.md`: memory layout, interrupt attributes, startup/linker rules. Done for first Markdown extraction pass; EVT linker/startup verification remains.
4. `Doc/HAL/wch-hal-normalization.md`: common peripheral normalization notes. Done for first Markdown extraction pass; EVT API verification remains.
5. `Doc/ETH/wch-ethernet-notes.md`: Ethernet MAC/PHY/DMA/WCHNET notes. Done for first Markdown extraction pass; EVT and board-level verification remains.
6. `Doc/USB/`, `Doc/USBPD/`, `Doc/IAP/`: remaining specialized topics.

## Notes

- Keep this index focused on extraction planning. Detailed conclusions belong in target topic documents.
- When a source recipe contains API names, verify them against repository EVT headers before treating them as HAL facts.
- Add new target documents to this index as extraction progresses.
