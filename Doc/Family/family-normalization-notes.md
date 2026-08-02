# WCH 系列归一化说明

本文档从 `Doc/Ref/wch-dev-skill` 提取系列级归一化指导，形成仓库专用索引，供未来各系列 HAL 说明、元数据和 EVT 验证使用。

官方 EVT 工程、RM、DS、启动代码文件、链接器/分散加载文件、开发板手册和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/new_project.md`
- `Doc/BLE`、`Doc/Core`、`Doc/HAL`、`Doc/USB`、`Doc/USBPD`、`Doc/ETH`、`Doc/HMI`、`Doc/IAP`、`Doc/RTOS`、`Doc/IO`、`Doc/Storage`、`Doc/System`、`Doc/Templates` 和 `Doc/Security` 下的现有仓库主题说明。

## 归一化策略

统一 HAL 或生成器使用某款芯片前，系列说明应回答以下问题：

- 适用哪种架构、编译器、工程结构、启动代码文件、链接器/分散加载文件和中断语法？
- 哪个 EVT 树和源系列最接近事实来源？
- 哪些主题说明可直接应用，哪些需要系列专用覆盖？
- 哪些内存、DMA、BLE 栈、USB 栈、以太网、加密或引导加载程序约束会影响生成的模板？
- 哪些事实尚未根据官方 EVT/RM/DS 资料验证？

## 系列矩阵

| 系列组 | 架构和工具链 | 主要源根目录 | 关键主题文档 | 首要验证重点 |
|---|---|---|---|---|
| CH57x | RISC-V BLE、MounRiver | `CH572EVT`, `CH573EVT`, `Doc/Ref/wch-dev-skill/chips/ch57x` | `Doc/BLE`, `Doc/Core`, `Doc/HAL`, `Doc/Templates`, `Doc/System`, `Doc/HMI` | BLE 初始化顺序、`config.h`、`Main_Circulation()`、链接器 RAM 布局、KEYSCAN/CMP/USB 示例。 |
| CH58x/CH59x | RISC-V BLE、MounRiver | `CH583EVT`, `CH585EVT`, `CH592EVT`, `CH595EVT`, `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x` | `Doc/BLE`, `Doc/USB`, `Doc/HMI`, `Doc/IAP`, `Doc/RTOS`, `Doc/Templates` | BLE 堆/缓冲区、USB 主机/设备路径、LCD/KEYSCAN 可用性、IAP/OTA 偏移、RTOS 示例根目录。 |
| CH32V 低成本 / CH32L103 | RISC-V StdPeriph、MounRiver | 当前低资源 EVT 与 `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost` | `Doc/HAL`, `Doc/Core`, `Doc/USBPD`, `Doc/System`, `Doc/Templates` | 小型 Flash/RAM 链接器值、调试输出模式、GPIO/ADC/定时器/Flash API、OPA/PWR/看门狗可用性。 |
| CH32M030 | QingKe RISC-V V3B、MounRiver | `CH32M030EVT` | `Doc/HAL`, `Doc/Core`, `Doc/USBPD`, `Doc/Templates` | 主头 `ch32m030.h`、64 KB Flash/12 KB RAM、电机/电源与 USB-PD 示例；不得并入 ARM/CH32F。 |
| CH32V 通用 | RISC-V StdPeriph、MounRiver | `CH32V103EVT`, `CH32V20xEVT`, `CH32V307EVT`, `CH32V407EVT`, `Doc/Ref/wch-dev-skill/chips/ch32v-general` | `Doc/HAL`, `Doc/Core`, `Doc/ETH`, `Doc/USB`, `Doc/IO`, `Doc/Storage`, `Doc/IAP` | 各子系列的头文件前缀、仅有能力的芯片支持 ETH、CAN/FSMC/SDIO 覆盖范围、IAP 偏移变体、链接器/启动代码差异。 |
| CH32F ARM | Cortex-M3 风格 ARM、Keil MDK 或 MounRiver | `Doc/Ref/wch-dev-skill/chips/ch32f-arm`；当前 SDK 未导入 CH32F EVT | `Doc/HAL`, `Doc/Templates`, `Doc/IO`, `Doc/Storage`, `Doc/System`, `Doc/Security` | `ch32f10x`/`ch32f20x` 结论均标为来源未验证，不从 CH32M030 反推。 |
| CH32X / CH64x USB-PD | RISC-V StdPeriph、MounRiver | `CH32X035EVT`, `CH32X315EVT`, `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd` | `Doc/USBPD`, `Doc/USB`, `Doc/HAL`, `Doc/IAP`, `Doc/Templates`, `Doc/System` | USB-PD CC 引脚、比较器阈值、`USBPD_PHY_V33`、PIOC 重映射/微代码、封装专用内存和 USBPD 实例。 |
| CH569 / CH56x | RISC-V 高速、MounRiver | `CH569EVT`, `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet` | `Doc/ETH`, `Doc/USB`, `Doc/Storage`, `Doc/Security`, `Doc/IO`, `Doc/Templates` | RAMX/DMA 放置位置、ETH/USB3/eMMC/HSPI/DVP/ECDC 交互、`CH56x_common.h`、`Flash_Lib`、高速时钟。 |
| CH561/CH563 | ARM7TDMI、Keil MDK（来源线索） | `Doc/Ref/wch-dev-skill/chips/ch561-ch563`；当前 SDK 未导入对应 EVT | `Doc/ETH`, `Doc/Core`, `Doc/Templates`, `Doc/Storage`, `Doc/IO`, `Doc/Security` | 寄存器 API、工程结构和外设差异均为来源未验证，导入官方 SDK 前不得提升为 HAL 能力。 |
| CH32H415/416/417 | 高性能 RISC-V V3F/V5F、MounRiver | `CH32H47TEVT`, `Doc/Ref/wch-dev-skill/chips/ch32h-highperf` | `Doc/HAL`, `Doc/USB`, `Doc/ETH`, `Doc/HMI`, `Doc/IO`, `Doc/Storage`, `Doc/Security`, `Doc/Templates` | 共享头/启动覆盖、IPC/HSEM/双 SysTick、V3F/V5F RAM 模型及按示例区分的 IAP 偏移。 |
| CH5xx 8051 | 8051、Keil C51 / SDCC | CH5xx EVT 资料、`Doc/Ref/wch-dev-skill/chips/ch5xx-8051` | `Doc/Core`, `Doc/USB`, `Doc/HAL`, `Doc/HMI`, `Doc/IAP`, `Doc/Templates`, `Doc/USBPD` | C51 内存模型、`CH5xx.H`、受保护寄存器安全模式、ISP 引导加载程序 ROM 限制、Type-C/TouchKey/USB 约束。 |

## 各系列说明模板

主动归一化某个系列时，使用以下结构创建或更新专用说明：

```text
# <系列> HAL 归一化笔记

## 源文件
## 支持的芯片与 EVT 根目录
## 架构、工具链、启动与链接
## 内存与启动布局
## 项目模板规则
## 外设覆盖矩阵
## 主题交叉引用
## 已知系列陷阱
## 验证清单
## 验证状态
```

规则：

- 将已验证事实与从 Markdown 提取的线索分开。
- 完成足够的 EVT/RM/DS 验证后，每个源系列使用一份系列说明。
- 交叉链接主题文档，而不是重复详细的 USB/BLE/ETH/IAP 规则。
- 将 API 或功能标记为支持前，记录确切的 EVT 路径和头文件。

## 跨系列拆分规则

- 在检查 CH57x 与 CH58x/CH59x 的 BLE 栈、RAM、USB 和示例差异前，不要合并其 BLE 细节。
- 不要将所有 CH32V 芯片视为同一功能集；CH32V103、V20x、V307 和 V407 的高速外设不同。
- CH32M030 是 RISC-V V3B，不属于 CH32F；也不要从当前未导入的 CH32F 来源说明扩大 RNG、以太网等结论。
- 不要将 CH569 高速/RAMX/ECDC 假设用于 CH561/CH563 ARM7TDMI。
- 不要将 CH32H417 V5F 链接器布局应用于 V3F 或更小的 CH32 系列。
- 不要将 RISC-V 中断属性或链接脚本应用于 CH5xx 8051 或 CH561/CH563 ARM7 工程。

## 验证清单

对每个系列，验证：

- 本仓库中存在的 EVT 根目录，以及适用的 GPIO、USART/UART、定时器、ADC、Flash、DMA、USB/BLE/ETH/CAN/存储的最接近示例路径。
- 主芯片头文件、外设驱动包含目录、系统文件、启动代码文件、链接器/分散加载文件和调试辅助文件。
- Flash/RAM 大小、引导加载程序/应用偏移、栈/堆布局和 DMA 可访问内存区域。
- 中断语法、向量表重定位机制、优先级分组和 ISR 段属性。
- 工具链工程元数据：MounRiver `.project/.cproject/.wvproj`、Keil `.uvproj/.uvprojx`、C51 选项、包含路径和定义。
- 开发板级依赖：振荡器、调试 UART 引脚、LED/按钮、USB 连接器、Type-C CC 引脚、以太网 PHY、外部存储、显示器、编解码器、摄像头或收发器。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述当前仓库主题说明提取。
- 本轮尚未针对每个 EVT 树、RM、DS、工程文件、启动汇编、链接器/分散加载文件、开发板原理图或封装变体进行完整验证。
- 将本文档视为系列归一化路由文档。仅在开始相应 EVT/RM/DS 验证轮次后，才应创建各系列专用文件。
