# CH32H 高性能系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH32H417 高性能指导，形成针对高性能 HAL 元数据和模板的仓库专用归一化说明。

官方 EVT 示例、RM、DS、启动代码文件、链接脚本、开发板原理图、外部内存/显示/存储器件数据手册和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 系列/源组 | 预期 EVT 根目录 | 架构/工具链 | 关键约束 |
|---|---|---|---|
| CH32H415/416/417 | `CH32H47TEVT/` | 高性能 WCH RISC-V V3F/V5F、MounRiver | 当前 `ch32h417.h` 及 V3F/V5F startup 共享覆盖 415/416/417；链接器、IPC/HSEM 和内存模型按内核区分。 |

## 架构、工具链、启动代码、链接器

- 工具链：MounRiver `.wvproj` / `.project` 源流程。
- 源说明区分 V5F 和 V3F 内核/链接器配置。
- 当前默认 V3F 链接脚本从 `0x00000000` 启动，默认 V5F 从 `0x00010000` 启动；这不是一条“普通 H417 不得从零启动”的通用限制。
- V5F 源内存说明在 `0x200A0000` 使用 ITCM，在 `0x200C0000` 使用 DTCM。
- V3F 默认脚本在 `0x20100000` 配置 64 KB `RAM_CODE`，在 `0x20110000` 配置 256 字节 `RAM_LOAD`，其后的 448 KB 区域用于数据 RAM。
- `.highcode` 可从 ITCM 运行性能关键代码。
- 双核相关代码必须考虑 IPC 通道、HSEM 仲裁以及独立的 `SysTick0`/`SysTick1` 中断，不能套用单 SysTick 模板。

## 内存和引导布局

- Flash 可分配容量及零等待区按芯片、内核和工程而变；当前默认脚本本身不能被解释为固定的 64 KB 通用引导区。
- 源说明中的 Flash 擦除单位为 4 KB，编程单位为 4 字节。
- V5F DTCM 保存 `.data`、`.bss`、堆和栈；ITCM 保存 `.highcode` 和可选快速代码。
- 列出的外部 FMC 存储区范围从 `0x60000000` 至 `0x9FFFFFFF`。

规则：

- 普通 V3F 工程可按当前默认脚本从 `0x00000000` 启动；普通 V5F 默认从 `0x00010000` 启动。
- USB/UART IAP APP 的 V3F 起点为 `0x00006000`；USB Host IAP APP 的 V3F/V5F 起点分别为 `0x0000C000`/`0x0001C000`，不得统一成默认偏移。
- 不要混用 V5F 和 V3F 链接脚本。
- 将帧缓冲区、DMA 缓冲区和高速外设缓冲区放置在相应控制器可访问的内存中。
- Flash 局部更新需要执行 4 KB 读取-修改-擦除-写入。

## 外设和示例覆盖范围

源示例包括 ADC/HSADC、CAN1/2/3、CRC、DAC、DFSDM、DMA/MUX、DVP、ECDC、ETH、EXTI、Flash、FMC、GPIO、GPHA、I2C、I2S、I3C、IAP、看门狗、LPTIM、LTDC、OPA/CMP、PIOC、PWR、QSPI、RCC、RNG、RTC、SAI、SDIO/SDMMC、SerDes、SPI、SWPMI、定时器、TKey、UHSIF、USART、USBFS、USBHS、USBPD、USBSS 和 WWDG。

## 主题交叉引用

- 通用外设：`Doc/HAL/wch-hal-normalization.md`。
- USB/USB3：`Doc/USB/wch-usb-notes.md`。
- USB-PD/PIOC：`Doc/USBPD/wch-usbpd-notes.md`。
- 以太网：`Doc/ETH/wch-ethernet-notes.md`。
- 显示/HMI/FMC/QSPI/I3C/SWPMI/DFSDM：`Doc/HMI/wch-hmi-specialty-notes.md`。
- CAN/I2S/SAI/DVP：`Doc/IO/wch-io-media-notes.md`。
- SDIO/SDMMC/eMMC/QSPI 存储：`Doc/Storage/wch-storage-notes.md`。
- ECDC/CRC/RNG：`Doc/Security/wch-security-crypto-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。

## 已知系列陷阱

- 配置前必须启用外设时钟。
- `GPIO_PinAFConfig()` 复用功能编号必须与数据手册一致。
- 仅在所选映像布局确实预留引导区时禁止覆盖该区域；应以对应 V3F/V5F IAP 链接脚本为准。
- 外设 TX/RX 前必须将 GPIO 配置为复用功能。
- I2C 总线挂起可能需要总线恢复和谨慎的 STOP 处理。
- DMA 缓冲区需要正确对齐，并位于控制器可访问的内存中。
- LTDC 帧缓冲区的放置位置至关重要。
- 以太网 PHY 需要验证复位、参考时钟和 MDIO/MDC。
- 对多个 CAN 控制器，必须正确划分 CAN 滤波器组。
- USB-PD CC 上下拉/比较器设置必须与角色和开发板匹配。
- SerDes/USB3 需要检查 PLL 锁定和 PHY 就绪状态。

## 验证清单

- 验证 CH32H417 EVT 根目录和导入本仓库的所有示例分类。
- 验证 V5F/V3F 链接脚本、启动代码、系统时钟、引导加载程序偏移、ITCM/DTCM 段放置位置和 Flash 大小变体。
- 验证 USBSS/SerDes、ETH、SDIO/SDMMC、FMC/QSPI、LTDC/GPHA、DVP、SAI/I2S、ECDC/CRC/RNG 的高速示例。
- 验证帧缓冲区、以太网缓冲区、存储缓冲区和加密缓冲区的 DMA/缓存/一致性/内存访问规则。
- 验证 PHY、显示器、外部内存、QSPI flash、USB-PD、摄像头、音频编解码器和 SD/eMMC 接线的开发板原理图。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对 CH32H417 EVT 源代码树、RM、DS、工程文件、启动代码文件、链接脚本、封装或开发板原理图进行完整验证。
- 在根据确切官方资料完成核查前，应将高性能外设可用性、链接器值、内存放置位置和时序规则视为初步信息。
