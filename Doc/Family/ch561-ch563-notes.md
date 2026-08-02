# CH561 和 CH563 ARM7TDMI 系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH561 和 CH563 指导，形成针对 ARM7TDMI、以太网、USB、存储、RTOS 和模板工作的仓库专用归一化说明。

官方 EVT 示例、RM、DS、启动代码文件、分散加载文件、开发板原理图、PHY/存储器件数据手册和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 芯片 | 仓库 EVT 根目录 | 架构/工具链 | 关键差异 |
|---|---|---|---|
| CH561 | 验证 `CH561EVT/` 可用性 | ARM7TDMI、Keil MDK | 以太网、无 USB、PA/PB GPIO，源表中无 RTOS。 |
| CH563 | 验证 `CH563EVT/` 可用性 | ARM7TDMI、Keil MDK | 以太网、USB、GPIO PD、uC/OS-II，更多 NET 示例。 |

## 架构、工具链、启动代码、链接器

- 架构：ARM7TDMI，使用寄存器级 `R8_`、`R16_` 和 `R32_` 访问。
- 工具链：带分散加载文件和 ARM 启动代码的 Keil MDK `.uvproj` / `.uvopt`。
- 共享源文件包括 `CH561SFR.H`、`CH563SFR.H`、`CH561BAS.H`、`CH563BAS.H`、`SYSFREQ.C/H`、`STARTUP.S`、`ISPXT56X.H` 和 `ISPXT56X.O`。
- 系统频率和 SRAM 大小在 `SYSFREQ.H` 中配置；启动代码/分散加载文件必须匹配。

## 内存和引导布局

- 源说明列出：`0x00000000` 处有 224 KB 代码 Flash，`0x00038000` 处有 28 KB Data-Flash，SFR 位于 `0x00400000`，SRAM 从 `0x00808000` 开始，可选 XBUS 位于 `0x00C00000`。
- 源说明中的 Flash 擦除单元为 4 KB，最小写入单位为 4 字节。
- Data-Flash 可按字节写入，适合存放频繁更新的配置/计数器。
- SRAM 可通过 `MEM_DATA` 配置：源中提供 32/64/96 KB 选项。
- 分散加载文件定义代码和 RAM 区域；不要使用 CH32 RISC-V 链接脚本。

## 外设和示例覆盖范围

源示例包括 ADC、Flash/EEPROM、GPIO、IAP/ISP、中断、以太网 NET 示例、SPI0/1、定时器、UART0/1、看门狗和共享源代码。CH563 还增加了 USB、GPIO PD、并行/外部总线、电源示例、uC/OS-II 和更广泛的 NET 示例。

## 主题交叉引用

- 以太网/WCHNET 风格规则：`Doc/ETH/wch-ethernet-notes.md`。
- 模板：`Doc/Templates/wch-project-template-notes.md`。
- 存储/eMMC/HSPI 源边界：`Doc/Storage/wch-storage-notes.md`。
- IO/媒体：`Doc/IO/wch-io-media-notes.md`。
- 安全说明明确记录无 ECDC 引擎：`Doc/Security/wch-security-crypto-notes.md`。
- RTOS/uC/OS-II：`Doc/RTOS/wch-rtos-notes.md`。

## 已知系列陷阱

- CH561/CH563 是 ARM7TDMI，而非 CH569 RISC-V；不要复用 CH569 API 或 MounRiver 工程假设。
- CH563 具有 USB 和 GPIO PD；CH561 没有。
- 源说明中外设时钟由 `R8_SLP_CLK_OFF0/1` 控制，采用反向风格的关闭位。
- SRAM 大小和栈顶取决于 `MEM_DATA` 及启动代码/分散加载配置。
- Data-Flash 行为不同于代码 Flash，也不同于 CH32 Flash API。
- 网络示例使用 CH561NET/CH563 NET 库流程；需验证任务/轮询模型和 PHY 细节。
- 根据源说明，CH561/CH563 不支持 ECDC。

## 验证清单

- 验证 EVT 根目录、Keil 工程、分散加载文件、启动代码、`SYSFREQ.H/C` 和 `MEM_DATA` 设置。
- 验证 SFR 头文件、寄存器命名、中断语法和向量处理程序名称。
- 验证以太网 NET 库、PHY 地址、开发板复位引脚和网络栈任务要求。
- 生成模板前验证 CH563 专有的 USB、GPIO PD、UCOS、并行总线和电源示例。
- 验证 Data-Flash 写入/擦除规则和 IAP/ISP 流程。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对 CH561/CH563 EVT 树、RM、DS、Keil 工程文件、启动代码文件、分散加载文件、封装或开发板原理图进行完整验证。
- 在根据确切官方资料完成核查前，应将内存映射、NET 库细节和 CH563 专有功能可用性视为初步信息。
