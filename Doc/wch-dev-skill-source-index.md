# wch-dev-skill Markdown 源文件索引

本索引跟踪 `Doc/Ref/wch-dev-skill` 下应逐步提取到仓库专用文档中的 Markdown 文件。

本索引的覆盖范围：

- 包含：顶层技能文档、`chips/*/recipes/*.md` 和 `chips/*/resources/*.md`。
- 首轮不包含：`chips/*/resources/EXAM/**/*.md`，因为这些文件大多属于复制的示例或第三方软件包，仅应在迁移相关示例时单独审查。

## 顶层技能文档

| 源文件 | 用途 | 初始目标位置 |
|---|---|---|
| `Doc/Ref/wch-dev-skill/SKILL.md` | 全局路由、规则、工作流、场景和陷阱 | `Doc/Family/`、`Doc/HAL/`、主题文档 |
| `Doc/Ref/wch-dev-skill/AGENTS.md` | 编码约定、头文件包含模式、主循环模板和检查清单 | `Doc/Core/`、`Doc/HAL/`、主题文档 |
| `Doc/Ref/wch-dev-skill/README.md` | 中文概述和安装说明 | 仅供参考 |
| `Doc/Ref/wch-dev-skill/README_EN.md` | 英文概述和安装说明 | 仅供参考 |
| `Doc/Ref/wch-dev-skill/CHANGELOG.md` | 上游技能变更历史 | 仅供参考 |

## 芯片系列资源文档

这些文件通常包含最具提取价值的精炼参考资料。

| 芯片系列 | 资源文档 | 主要目标位置 |
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

## 按目标主题划分的操作指南组

### 项目和芯片系列设置

源文件：

- `chips/*/recipes/new_project.md`

目标位置：

- `Doc/Family/`
- `Doc/Core/`
- `Doc/Templates/`

### 通用 HAL 外设

源文件：

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

目标位置：

- `Doc/HAL/`
- `Doc/Core/`
- `Doc/Family/`

### BLE 和 2.4 GHz RF

源文件：

- `chips/ch57x/recipes/ble_*.md`
- `chips/ch57x/recipes/rf_comm.md`
- `chips/ch58x-ch59x/recipes/ble_*.md`
- `chips/ch57x/resources/ble_api.md`
- `chips/ch57x/resources/config_reference.md`
- `chips/ch58x-ch59x/resources/config_reference.md`

目标位置：

- `Doc/BLE/`
- `Doc/Core/`
- `Doc/Family/`

### USB 和 USB3

源文件：

- `chips/*/recipes/usb_device.md`
- `chips/*/recipes/usb_host.md`
- `chips/ch56x-ethernet/recipes/usb3_device.md`
- `chips/ch32h-highperf/recipes/usb3_device.md`
- `chips/ch32h-highperf/recipes/usbhs_device.md`
- `chips/ch32h-highperf/recipes/usbfs_device.md`

目标位置：

- `Doc/USB/`
- `Doc/Core/`
- `Doc/Family/`

### USB-PD、Type-C 和 PIOC

源文件：

- `chips/ch32x-usbpd/recipes/usbpd_config.md`
- `chips/ch32x-usbpd/recipes/pio_config.md`
- `chips/ch32f-arm/recipes/usbpd.md`
- `chips/ch32h-highperf/recipes/usbpd_config.md`
- `chips/ch5xx-8051/recipes/type_c.md`

目标位置：

- `Doc/USBPD/`
- `Doc/USB/`
- `Doc/HAL/`

### 以太网和高速接口

源文件：

- `chips/*/recipes/eth_comm.md`
- `chips/ch32f-arm/recipes/ethernet.md`
- `chips/ch56x-ethernet/recipes/emmc_storage.md`
- `chips/ch561-ch563/recipes/emmc_storage.md`
- `chips/ch56x-ethernet/recipes/hspi_comm.md`
- `chips/ch561-ch563/recipes/hspi_comm.md`
- `chips/ch56x-ethernet/recipes/ecdc_crypto.md`
- `chips/ch561-ch563/recipes/ecdc_crypto.md`
- `chips/ch32h-highperf/recipes/ecdc_crypto.md`

目标位置：

- `Doc/ETH/`
- `Doc/Security/`
- `Doc/HAL/`
- `Doc/Core/`

### 安全、密码学、CRC 和 RNG

源文件：

- `chips/*/recipes/ecdc_crypto.md`
- `chips/*/recipes/crc_calculation.md`
- `chips/*/recipes/rng_random.md`

目标位置：

- `Doc/Security/`
- `Doc/System/`
- `Doc/IAP/`

### 存储接口

源文件：

- `chips/*/recipes/sdio_sdcard.md`
- `chips/ch56x-ethernet/recipes/sd_card.md`
- `chips/*/recipes/emmc_storage.md`
- `chips/*/recipes/hspi_comm.md`
- `chips/ch32h-highperf/recipes/qspi_flash.md`

目标位置：

- `Doc/Storage/`
- `Doc/HAL/`
- `Doc/Family/`

### IO、音频、CAN 和摄像头接口

源文件：

- `chips/*/recipes/can_comm.md`
- `chips/*/recipes/i2s_audio.md`
- `chips/*/recipes/sai_audio.md`
- `chips/*/recipes/dvp_camera.md`

目标位置：

- `Doc/IO/`
- `Doc/HAL/`
- `Doc/Family/`

### 系统、模拟、电源和实用外设

源文件：

- `chips/*/recipes/power_management.md`
- `chips/*/recipes/lowpower.md`
- `chips/*/recipes/watchdog.md`
- `chips/*/recipes/rtc_config.md`
- `chips/*/recipes/rtc_clock.md`
- `chips/*/recipes/cmp.md`
- `chips/*/recipes/compare.md`
- `chips/*/recipes/opa_config.md`
- `chips/*/recipes/dac_output.md`
- `chips/*/recipes/rng_random.md`
- `chips/*/recipes/crc_calculation.md`

目标位置：

- `Doc/System/`
- `Doc/HAL/`
- `Doc/Core/`

### 显示、HMI 和专用外设

源文件：

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

目标位置：

- `Doc/HMI/`
- `Doc/HAL/`
- `Doc/Family/`

### IAP、OTA、引导加载程序和闪存布局

源文件：

- `chips/*/recipes/iap_ota.md`
- `chips/ch5xx-8051/recipes/iap_bootloader.md`
- `chips/*/resources/memory_layout.md`

目标位置：

- `Doc/IAP/`
- `Doc/Core/`
- `Doc/Family/`

### RTOS 集成

源文件：

- `chips/*/resources/example_list.md`
- `chips/*/resources/pitfalls.md`
- `chips/ch57x/` 和 `chips/ch58x-ch59x/` 下与 BLE/TMOS 相关的操作指南和参考资料

目标位置：

- `Doc/RTOS/`
- `Doc/BLE/`
- `Doc/Core/`

## 首轮提取顺序

1. `Doc/Family/family-routing.md`：全局芯片系列路由和源目录映射。已完成。
2. `Doc/Family/family-normalization-notes.md`：芯片系列级规范化入口和未来各芯片系列说明模板。首轮 Markdown 提取已完成；各芯片系列的 EVT/RM/DS 验证仍待进行。
3. `Doc/Family/ch32v-general-notes.md`、`Doc/Family/ch58x-ch59x-notes.md`、`Doc/Family/ch56x-ethernet-notes.md`：首批各芯片系列提取说明。首轮 Markdown 提取已完成；准确的 EVT/RM/DS 验证仍待进行。
4. `Doc/Family/ch57x-notes.md`、`Doc/Family/ch32x-usbpd-notes.md`、`Doc/Family/ch32h-highperf-notes.md`：第二批各芯片系列提取说明。首轮 Markdown 提取已完成；准确的 EVT/RM/DS 验证仍待进行。
5. `Doc/Family/ch32f-arm-notes.md`、`Doc/Family/ch32v-lowcost-notes.md`、`Doc/Family/ch561-ch563-notes.md`、`Doc/Family/ch5xx-8051-notes.md`：其余各芯片系列提取说明。首轮 Markdown 提取已完成；准确的 EVT/RM/DS/工具链验证仍待进行。
6. `Doc/Templates/wch-project-template-notes.md`：新建项目、工具链、链接器、启动和最小主循环说明。首轮 Markdown 提取已完成；EVT 项目文件验证仍待进行。
7. `Doc/Security/wch-security-crypto-notes.md`：ECDC、CRC、RNG 和安全边界说明。首轮 Markdown 提取已完成；EVT、测试向量和安全审查验证仍待进行。
8. `Doc/BLE/wch-ble-notes.md`：CH57x 和 CH58x/CH59x 的 BLE 规则、配置和示例。首轮 Markdown 提取已完成；EVT 验证仍待进行。
9. `Doc/Core/wch-core-notes.md`：内存布局、中断属性和启动/链接器规则。首轮 Markdown 提取已完成；EVT 链接器/启动验证仍待进行。
10. `Doc/HAL/wch-hal-normalization.md`：通用外设规范化说明。首轮 Markdown 提取已完成；EVT API 验证仍待进行。
11. `Doc/ETH/wch-ethernet-notes.md`：以太网 MAC/PHY/DMA/WCHNET 说明。首轮 Markdown 提取已完成；EVT 和板级验证仍待进行。
12. `Doc/USB/`、`Doc/USBPD/`、`Doc/IAP/`、`Doc/RTOS/`：专用主题提取文档。首轮 Markdown 提取已完成；EVT 和集成验证仍待进行。
13. `Doc/HMI/wch-hmi-specialty-notes.md`：显示、HMI 和专用外设说明。首轮 Markdown 提取已完成；板级和特定外设验证仍待进行。
14. `Doc/IO/wch-io-media-notes.md`：CAN、I2S、SAI 和 DVP 摄像头接口说明。首轮 Markdown 提取已完成；EVT、板级和外部组件验证仍待进行。
15. `Doc/Storage/wch-storage-notes.md`：SD/SDIO/eMMC/HSPI/QSPI 存储接口说明。首轮 Markdown 提取已完成；EVT、板级和设备验证仍待进行。
16. `Doc/System/wch-system-analog-power-notes.md`：低功耗、RTC、看门狗、模拟、CRC/RNG 和实用外设说明。首轮 Markdown 提取已完成；EVT、板级、模拟和测量验证仍待进行。

## 说明

- 本索引应专注于提取规划。详细结论应放入目标主题文档。
- 当源操作指南包含 API 名称时，应先对照仓库中的 EVT 头文件进行验证，再将其作为 HAL 事实。
- 随着提取工作的推进，将新的目标文档添加到本索引中。
