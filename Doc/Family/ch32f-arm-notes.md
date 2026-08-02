# CH32F ARM 系列说明

本文档保留 `Doc/Ref/wch-dev-skill` 中 CH32F10x、CH32F20x ARM 系列的来源路由。当前 SDK 未导入对应 CH32F EVT，因此以下 CH32F 结论均为来源未验证；CH32M030 已从本组拆出。

官方 EVT 示例、RM、DS、启动代码文件、链接器/分散加载文件、开发板原理图和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 源组 | 仓库 EVT 根目录 | 头文件系列 | 关键约束 |
|---|---|---|---|
| CH32F10x / CH32F103 | 当前 SDK 未导入 | `ch32f10x.h`, `ch32f10x_*`（来源线索） | Cortex-M3、内存和 USB API 均待导入官方 SDK 后验证。 |
| CH32F20x | 当前 SDK 未导入 | `ch32f20x.h`, `ch32f20x_*`（来源线索） | 以太网、DVP、I2S、SDIO、USBHS、RNG、BLE 等均为未验证来源线索。 |

CH32M030 实际为 QingKe RISC-V V3B，当前 EVT 主头为 `ch32m030.h`，默认链接脚本为 64 KB Flash、12 KB RAM；应独立归一化，或与低资源 RISC-V StdPeriph 器件分组，不得使用本文件的 ARM 启动和 Flash 基址假设。

## 架构、工具链、启动代码、链接器

- 架构：使用 StdPeriphDriver 风格 API 的 ARM Cortex-M 风格系列。
- 工具链：源说明中有 Keil MDK 和 MounRiver 工程；应保留原始 EVT 工程元数据。
- Flash 基址为 `0x08000000`，不同于从零开始的 CH32V RISC-V 布局。
- 工程文件包括 `*_conf.h`、`*_it.c/h`、`system_*.c/h`、调试辅助程序、启动代码和链接器/分散加载元数据。

## 内存和引导布局

- CH32F10x 源说明列出 32 KB 至 512 KB Flash 和 10 KB 至 64 KB SRAM 变体。
- CH32F20x 源说明列出 64 KB 至 256 KB Flash 和 20 KB 或 64 KB SRAM 变体。
- CH32F10x IAP 示例使用应用起始地址 `0x08001000`；CH32F20x IAP 示例使用应用起始地址 `0x08002000`。
- 向量表从 Flash 基址开始且可重定位；需验证 IAP 工程中的 `SCB->VTOR` 处理。

## 外设和示例覆盖范围

来源索引声称涵盖 ADC、BKP、CAN、CRC、DAC、DMA、EXTI、Flash、GPIO、I2C、看门狗、PWR、RCC、RTC、SPI、SysTick、TIM、TouchKey、USART、USB、RTOS 和 IAP，并为 CH32F20x 列出 DVP、以太网、FSMC、I2S、OPA、RNG、SDIO、USBHS、BLE。由于当前 SDK 无 CH32F EVT，这些不能写成当前仓库已验证覆盖。

## 主题交叉引用

- 通用外设：`Doc/HAL/wch-hal-normalization.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。
- USB/USBHS：`Doc/USB/wch-usb-notes.md`。
- 以太网：`Doc/ETH/wch-ethernet-notes.md`。
- CAN/I2S/DVP：`Doc/IO/wch-io-media-notes.md`。
- SDIO/存储：`Doc/Storage/wch-storage-notes.md`。
- USB-PD：`Doc/USBPD/wch-usbpd-notes.md`。
- 安全/RNG/CRC：`Doc/Security/wch-security-crypto-notes.md`。

## 已知系列陷阱

- 不要混用 `ch32f10x` 和 `ch32f20x` 头文件或驱动文件。
- 配置外设前启用 RCC 时钟。
- 设置中断优先级前配置 NVIC 优先级分组。
- 写入 Flash 前必须先擦除；CH32F10x 和 CH32F20x 的页大小不同。
- GPIO 复用功能模式必须与外设类型匹配，尤其是 I2C 开漏模式和 ADC/DAC 模拟模式。
- CH32F USB 使用 WCH 自定义寄存器 API，而非 STM32 USB OTG API。
- 中断处理程序名称必须与向量表完全匹配。
- ISR 中的工作应保持简短；较长的处理应使用标志或队列。
- 避免在 USB、以太网、SDIO、DVP 和 RTOS 示例中使用大型栈缓冲区。

## 验证清单

- 验证每款芯片的确切 EVT 根目录、工程文件、编译器、启动代码、系统、链接器/分散加载和调试辅助文件。
- 生成链接器文件前验证 `0x08000000` 绝对 Flash 起始地址和 IAP 偏移。
- 根据确切芯片/封装验证 CH32F20x 专有功能，例如 RNG、以太网、SDIO、DVP、USBHS、BLE 和 OPA。
- CH32M030 的 USBPD、OPA/CMP/PGA 和电机控制应在独立 RISC-V 路由下按 `CH32M030EVT/` 验证。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 当前 SDK 未导入 CH32F EVT；本轮未验证 CH32F 的 RM、DS、工程、启动代码、链接器/分散加载文件、封装或开发板资料。
- 在根据确切官方资料完成核查前，应将功能可用性和内存值视为初步信息。
