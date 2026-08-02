# CH32X 和 CH6xx USB-PD 系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH32X035、CH32X315、CH641、CH643 及相关 CH6xx USB-PD 指导，形成仓库专用归一化说明。

官方 EVT 示例、RM、DS、启动代码文件、链接脚本、开发板原理图、USB-PD/Type-C 规范和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 芯片/源组 | 仓库 EVT 根目录 | 源说明中的内存 | 关键约束 |
|---|---|---|---|
| CH32X035 | `CH32X035EVT/` | 62 KB Flash、20 KB RAM | USB-PD、USB、PIOC、触摸/按键、小型 Flash 布局。 |
| CH32X315 | `CH32X315EVT/` | 192 KB Flash、64 KB RAM | 驱动层 `ch32x3x5.h`/`ch32x3x5_usbpd.h` 存在 USBPD 定义，但当前 `EXAM` 无 USBPD 示例；另有 USBHS/USBSS、CRC/RTC 等示例。 |
| CH643 | 使用前验证 EVT 根目录 | 192 KB Flash、64 KB RAM | 源说明中的 USB-PD、PIOC、OPA、RTOS 示例。 |
| CH641 | 使用前验证 EVT 根目录 | 16 KB Flash、2 KB RAM | 内存极小；仅支持 USB-PD 和基础外设。 |
| CH634 | 源说明称无 EXAM 目录 | N/A | 仅在硬件验证后将 CH32X035/CH643 示例用作参考。 |

## 架构、工具链、启动代码、链接器

- 架构：WCH RISC-V StdPeriph 风格系列。
- 工具链：MounRiver `.project` / `.wvproj`，源模板中共享 `SRC/Core`、`SRC/Debug`、`SRC/Ld`、`SRC/Peripheral` 和 `SRC/Startup`。
- 必须按确切内存容量选择链接脚本。源说明中 CH641 使用小得多的默认栈。
- 与 CH57x 不同，源内存说明指出这些芯片没有 `.highcode` 段。

## 内存和引导布局

- 标准应用起始地址为 `0x00000000`。
- IAP 偏移不统一为 `0x00001000`：当前 CH32X035/CH32X315 USB/UART IAP APP 从 `0x00005000` 开始；CH32X035 Host IAP APP 为 `0x00006000`，CH32X315 Host IAP APP 为 `0x00008000`。必须按具体示例选择链接脚本。
- 源说明中的 Flash 页擦除/编程单位为 256 字节页擦除和 4 字节字编程。
- CH641 仅有 2 KB RAM；必须尽量缩小栈、堆、USB-PD 状态和缓冲区。

## 外设和示例覆盖范围

源示例包括 ADC、DMA、EXTI、Flash、GPIO、I2C、IAP、看门狗、PWR、RCC、SPI、SysTick、TIM、USART、USB、USBPD、SDI printf，以及取决于芯片的 RTOS/PIOC/OPA/PMP/TOUCHKEY/USBHS/USBSS/CRC/RTC 示例。对 CH32X315 必须区分“驱动头存在 USBPD”与“当前 `CH32X315EVT/EXAM` 无 USBPD 示例”，后者不能由寄存器定义替代。

## 主题交叉引用

- USB-PD 和 PIOC：`Doc/USBPD/wch-usbpd-notes.md`。
- USB 设备/主机：`Doc/USB/wch-usb-notes.md`。
- 通用外设：`Doc/HAL/wch-hal-normalization.md`。
- IAP：`Doc/IAP/wch-iap-ota-notes.md`。
- RTOS：`Doc/RTOS/wch-rtos-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。

## 已知系列陷阱

- 访问外设前启用 RCC 总线时钟。
- 外设输出前配置 GPIO 复用功能模式。
- 编程前解锁 Flash 并按页对齐擦除。
- ADC 必须先校准，才能准确转换。
- 源说明中 TIM1 PWM 需要启用主输出（`MOE`）。
- USB-PD 需要设置 CC 上下拉和比较器阈值。
- `USBPD_PHY_V33` 必须与实际 VDD 匹配；错误设置可能损坏 PHY 或导致其无法工作。
- 引脚重映射前必须启用 AFIO 时钟。
- 应在设置各中断优先级前配置 NVIC 优先级分组。

## 验证清单

- 验证 CH32X035、CH32X315、CH641、CH643 的确切 EVT 根目录，以及本仓库中 CH634 是否有可用示例。
- 按芯片验证头文件、启动代码文件、链接脚本、内存容量、栈大小和 `debug.c/h`。
- 验证 USB-PD CC 引脚、CC 阈值、`USBPD_PHY_V33`、VBUS 检测和开发板 Type-C 上下拉电阻设计。
- 验证 PIOC SRAM 加载/重映射顺序和微代码示例。
- 根据当前 EVT 资料验证 CH32X315 类示例上的 USBHS/USBSS 可用性。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对所有 CH32X/CH6xx EVT 树、RM、DS、USB-PD 头文件、工程文件、启动代码文件、链接脚本、封装或开发板原理图进行完整验证。
- 在根据确切官方资料完成核查前，应将芯片支持、USB 控制器可用性、USB-PD 常量和内存值视为初步信息。
