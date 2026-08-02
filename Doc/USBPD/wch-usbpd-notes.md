# WCH USB-PD、Type-C 与 PIOC 笔记

本文从 `Doc/Ref/wch-dev-skill` 提取 USB-PD、USB Type-C CC 检测及 PIOC 相关指导，整理为适用于本仓库的规则，供后续 HAL、驱动、模板及元数据工作使用。

范围：

- CH32X/CH64x、CH32M030 及 CH32H417 系列来源中的 USB-PD 供电端、受电端和 DRP 流程。
- CH5xx 来源中的 USB Type-C 连接、方向、Rp/Rd 及电流通告/检测笔记。
- CH32X035/CH643 系列来源中可编程 I/O 示例的 PIOC 启动及使用规则。
- CC、比较器、PHY 电压、BMC 时序、消息、中断及引脚重映射的常见陷阱。

最终应以官方 EVT 示例、RM、DS、原理图、USB-PD 规范、Type-C 规范及当前仓库源码为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/usbpd_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/pio_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/usbpd.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usbpd_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/type_c.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/example_list.md`

## 系列覆盖范围

| 系列或来源组 | 来源笔记中的 USB-PD / Type-C 支持 | 软件风格 | 必须保留的规则 |
|---|---|---|---|
| CH32X035、CH32X315、CH641、CH643 | 供电端、受电端、DRP/状态机流程的 USB-PD 示例；CH32X035/CH643 系列来源中的 PIOC 示例 | StdPeriph 加 USBPD 寄存器；PIOC SFR/微码 | 使能正确的 USBPD 时钟域，配置 CC 比较器，选择有效 CC 线，使 BMC 定时器与时钟匹配，并将 PIOC 视为独立的可编程引擎。 |
| CH32M030 | 内置 USBPD 外设；USBPD0/USBPD1 可用性因封装而异 | `PD_Process` 示例模块 | 正确选择 `PD_SEL`，考虑受电端模式所需的外部 5.1K Rd，维持 1 ms PD 时序并监测 VBUS。 |
| CH32L103 | 当前 EVT 提供 `USBPD_SRC`、`USBPD_SNK` 和 `USBPD_CH211` | StdPeriph 加 `PD_Process`/CH211 示例 | 分别从 `CH32L103EVT/EXAM/USBPD/USBPD_SRC/`、`USBPD_SNK/`、`USBPD_CH211/` 起步，不把角色或外部 CH211 控制流程混为一套。 |
| CH32H417 | 集成 USBPD 控制器，来源笔记面向 PD 3.0 | 类 StdPeriph USBPD 寄存器 | 使能正确的总线时钟，配置 `CONFIG`、`CONTROL`、`PORT_CC1`、`PORT_CC2`，并根据 VDD 使用 `USBPD_PHY_V33`。 |
| CH543-CH559 Type-C 来源 | 通过 `USB_C_CTRL` 及 UCC 引脚上的 ADC 检测 Type-C 连接、方向和电流 | 8051 SFR/寄存器级 | DFP 通过 Rp 通告电流；UFP 使能 Rd 并测量 Rp 电压；ADC 读取前 UCC 引脚必须为浮空输入。 |

## 概念边界

在 HAL 元数据中将 USB、Type-C、USB-PD 和 PIOC 分开。

| 概念 | 用途 | 不得混同于 |
|---|---|---|
| USB D+/D- 控制器 | USB 数据枚举及类通信 | USB-PD CC BMC 通信。 |
| Type-C CC 检测 | 连接、线缆方向、通告默认/1.5A/3.0A 电流 | 完整的 USB-PD 消息协商。 |
| USB-PD 控制器 | BMC 信令、GoodCRC、PDO/RDO 协商、复位处理 | USBFS 端点或描述符逻辑。 |
| PIOC | 用于自定义串行协议的可编程 I/O 引擎 | 主 CPU 执行的 GPIO 位操作。 |

规则：

- USB-PD 通过 CC1/CC2 通信，而非 D+/D-。
- 通过 Rp/Rd 进行的 Type-C 电流通告不同于 PD PDO 协商。
- USB-PD 示例可与 USBFS 示例共存，但两者的时钟、引脚、中断及状态机必须独立建模。
- PIOC 应作为独立外设建模，包括代码 SRAM、SFR/数据寄存器、IO 映射及中断行为。

## Type-C CC 规则

Type-C 角色决定 CC 上下拉配置。

| 角色 | CC 上下拉 | 来源笔记中的行为 | 对 HAL 的影响 |
|---|---|---|---|
| DFP / 面向供电端的主机 | 连接前在两个 CC 引脚上启用 Rp 上拉 | CH5xx 使用 `USB_C_CTRL` PU 位；CH32 USBPD 来源使用 `CC_PU_*` 值 | 将通告电流与电源角色分开表示。 |
| UFP / 面向受电端的设备 | 连接前在两个 CC 引脚上启用 Rd 下拉 | CH5xx 使能 `bUCC1_PD_EN | bUCC2_PD_EN`；CH32M030 受电端笔记可能要求外部 5.1K Rd | 元数据必须记录 Rd 是内部还是外部。 |
| DRP | 交替执行供电端/受电端行为 | CH32X 来源笔记提到供电端、受电端及 DRP 流程 | DRP 需要显式策略/状态机支持，而非静态角色标志。 |

方向及连接规则：

- 读取两个 CC 引脚，并在连接后仅选择一条有效通信线。
- 将两个 CC 均连接或均未连接的结果视为异常状态，需要消抖、断开处理或附件专用处理。
- 在 8051 Type-C 示例中，ADC 测量前将 UCC 引脚设为浮空输入；输出模式会导致读数错误或为零。
- 在 CH32 USBPD 示例中，BMC 通信前使用 `CC_SEL` 选择 CC1 或 CC2。
- 验证各实例的开发板引脚映射；CH32M030 USBPD0 和 USBPD1 使用不同的 CC 引脚，且可用性因封装而异。

## USB-PD 启动顺序

通用 USB-PD 初始化顺序：

1. 配置系统时钟，并更新延时/定时器代码使用的时钟状态。
2. 为所选系列及实例使能 USBPD 外设总线时钟。
3. 使能 CC 引脚或实例选择所需的 GPIO、AFIO 或重映射时钟。
4. 为两个 CC 引脚配置角色上下拉及比较器阈值。
5. 根据实际 VDD，使用 `USBPD_PHY_V33` 配置 PHY 电压模式。
6. 根据当前系统时钟配置 BMC 时序值。
7. 使能 PD 输入滤波及相关 RX/TX/复位中断。
8. 检测连接并选择有效 CC 通道。
9. 在主循环或专用处理函数中启动 PD 状态机。
10. 清除并处理各中断标志，避免在 ISR 中执行繁重的策略工作。

系列差异：

- CH32X 来源笔记使用 `RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBPD, ENABLE)`，CH32X035 的 CC 引脚为 PC14/PC15。
- CH32H417 当前 EVT 使用 `RCC_HBPeriphClockCmd(RCC_HBPeriph_USBPD, ENABLE)`，不得写成 APB1 时钟使能。
- CH32M030 来源笔记使用 `PD_Process` 模块、`PD_SEL` 及 1 ms TIM1 节拍维持 PD 时序。
- 根据来源方案，CH32M030 的 USBPD1 可能需要 AFIO 时钟及 `GPIO_Remap_SDI_Disable`。

## CC 比较器与 PHY 电压规则

BMC 接收检测需要配置 CC 比较器阈值。

规则：

- 不得仅配置 `CC_PD` 或 `CC_PU_*`；角色上下拉设置必须配合合适的比较器阈值。
- 受电端示例根据系列方案使用下拉以及 `CC_CMP_22` 或 `CC_CMP_45` 等低比较器阈值。
- 供电端示例使用 `CC_PU_330`、`CC_PU_180` 或 `CC_PU_80` 等上拉电流设置，并配合 `CC_CMP_66` 等比较器阈值。
- 阈值名称及确切支持值取决于头文件；生成常量前应阅读目标芯片的 USBPD 头文件。

`USBPD_PHY_V33` 必须与开发板 VDD 匹配：

- 仅在 3.3 V VDD 直通模式下设置 `USBPD_PHY_V33`。
- VDD 大于 4 V 时清除 `USBPD_PHY_V33`，由内部 LDO 限制 PHY 电压。
- 此项属于安全规则。设置错误可能导致无法通信或损坏 PHY。

## BMC 时序与消息规则

USB-PD 协议处理对时序敏感且具有状态。

BMC 时序规则：

- BMC TX/RX 定时器值必须根据实际系统时钟推导。
- 来源笔记列出了 48 MHz、24 MHz 和 12 MHz 定时器预设；更改时钟后不得继续使用 48 MHz 对应值。
- CH32M030 来源笔记维持 1 ms TIM1 节拍，并从主循环调用检测/PD 主处理。

消息规则：

- 为供电端、受电端、DRP、复位、电压调整及等待状态保留真正的 PD 状态机。
- 消息后处理 `GoodCRC`，否则对端可能认为消息已丢失。
- 按协议要求，在每个成功消息序列后递增 `Msg_ID`。
- 发送 `Accept` 并改变电压前，根据可用供电端 PDO 验证受电端请求。
- 仅在请求的电源状态实际就绪后发送 `PS_RDY`。
- Hard Reset 是 PHY 级 BMC 信令，不能仅建模为普通 PD 消息。
- PD 2.0/3.0 基本协商示例并不代表完整支持 PPS、AVS 或扩展消息。

PDO/RDO 规则：

- 来源方案中，固定 PDO 电压以 50 mV 为单位编码，电流以 10 mA 为单位编码。
- PPS/APDO 字段使用不同单位及位位置；不得按固定电源 PDO 解析。
- 避免硬编码不安全的电压请求。来源笔记明确警告，请求错误电压可能损坏已连接器件。
- 监测 VBUS 以判断连接/移除及电源状态，尤其是在受电端示例中。

## 中断与状态处理

USBPD ISR 规则：

- 仅读取一次状态，仅处理已置位的标志，并按系列规定的方式清除每个已处理标志。
- 分别处理接收完成、发送完成、复位及缓冲区错误标志。
- 使用 `volatile` 标志或小型事件队列，将策略/状态机工作延后到 ISR 之外执行。
- 明确处理复位；收到 `IF_RX_RESET` 时，应将 PD 状态机转入复位/恢复状态。

主循环规则：

- 按 EVT 示例要求的频率调用 PD 检测及 PD 主处理。
- 定时器 ISR 更新共享毫秒计数器或差值时，应予以保护。
- 保持功率级控制与 PD 策略状态同步，而不能只依据收到的消息。

## PIOC 规则

PIOC 是小型可编程 I/O 引擎，具有自己的代码、SFR/数据寄存器、IO 引脚及中断请求路径。

初始化顺序：

1. 使能所选 PIOC 引脚需要的 GPIOC 和 AFIO 时钟。
2. 需要时使用 `GPIO_PinRemapConfig(GPIO_Remap_PIOC, ENABLE)` 使能 PIOC 引脚重映射。
3. 仅当所选开发板/引脚需要占用相关引脚时，才禁用冲突的 SWJ/SDI 重映射。
4. 将 TX 引脚配置为复用推挽，将 RX 引脚配置为上拉输入或协议要求的模式。
5. 使能 `PIOC_IRQn` 并设置其优先级。
6. 按要求对齐，将预编译的 PIOC 微码加载到 `PIOC_SRAM_BASE`。
7. 按来源规定的顺序复位/释放 PIOC，并使能 `RB_MST_IO_EN0`、`RB_MST_IO_EN1` 和 `RB_MST_CLK_GATE`。
8. 通过 `R8_CTRL_WR` 启动传输前，先在数据寄存器中配置协议参数。

PIOC 设计规则：

- 将 PIOC 微码视为协议专用固件，而非普通 C 逻辑。
- 波特率/时序寄存器预设应与系统时钟关联；来源中的 UART 预设假定 48 MHz。
- 将数据寄存器建模为主机/PIOC 邮箱。不得隐藏其缓冲区大小限制。
- 生成辅助函数时，明确双缓冲 TX/RX 的所有权。
- 按来源方案所示，通过写入 `R8_CTRL_RD` 清除 PIOC 中断请求。
- 若 PIOC 不运行，应先检查时钟门控、复位释放、微码加载、重映射及 SWJ 引脚所有权，再调试协议逻辑。

来源中的 PIOC 内存/寄存器笔记：

- CH32X035 的 PIOC SFR 基地址 `PIOC_BASE` 为 `0x40026C00`。
- `PIOC_SRAM_BASE` 是独立的代码 RAM 地址，当前头文件定义为 `SRAM_BASE + 0x4000`；不得把它与 PIOC SFR 基地址混用。
- `R8_DATA_REG0` 至 `R8_DATA_REG31` 为通用数据寄存器；部分方案使用其 32 位别名配置时序。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| 缺少 CC 比较器阈值 | 无法解码 PD 消息 | CC 上下拉配置必须始终与比较器配置配合。 |
| `USBPD_PHY_V33` 与 VDD 不匹配 | 无法通信或可能损坏 PHY | 3.3 V 直通模式下设置；VDD 大于 4 V 时清除。 |
| 时钟改变后复用 BMC 定时器常量 | 协议错误 | 根据当前系统时钟推导定时器值。 |
| 在错误的 CC 线上通信 | 检测到连接，但 PD 通信失败 | 检测方向并更新 `CC_SEL`。 |
| 将 Type-C 电流通告当作 PD 协商 | 电源策略错误 | 将 Rp/Rd 电流级别与 PDO/RDO 协商分开。 |
| CH32M030 受电端设计缺少外部 Rd | 受电端连接/协商失败 | 芯片/封装没有内部 Rd 时，添加所需的 5.1K CC 下拉。 |
| 未监测 VBUS | 未发现断开或进入不安全的电源状态 | 在受电端/供电端策略中纳入 VBUS 状态。 |
| 忘记使能 AFIO/重映射时钟 | 引脚重映射无效 | 在 `GPIO_PinRemapConfig` 前使能 AFIO。 |
| PIOC 引脚仍由 SWJ/SDI 占用 | PIOC RX/TX 引脚不工作 | 仅禁用引脚布局所需的冲突调试/重映射功能。 |
| 在错误的复位/时钟顺序后加载 PIOC 微码 | PIOC 不运行 | 遵循 EVT 示例中的复位、SRAM 加载、IO 使能及时钟门控顺序。 |
| 未核实 CC/PHY 电压、Rp/Rd、VBUS 或功率级参数 | 协商失败、电气过应力或器件损坏 | 未经目标芯片 RM/DS、板卡原理图及器件额定值确认，不得接入或切换实际电源；示例常量不能作为安全依据。 |

## HAL 元数据检查清单

后续 HAL 或模板元数据应显式表示以下字段：

- USBPD 实例、寄存器基地址、IRQ、总线时钟域及时钟使能函数。
- CC1/CC2 引脚、封装可用性、复用/重映射要求及开发板连接器方向映射。
- 角色策略：供电端、受电端、DRP、数据角色、电源角色，以及已实现时的 VCONN 支持。
- CC 上下拉类型、Rp 通告电流、内部/外部 Rd 可用性及比较器阈值。
- PHY 电压模式及开发板 VDD 假设。
- BMC 定时器源时钟及生成的定时器常量。
- PD 版本及支持的功能级别，包括是否实现 PPS/APDO 或扩展消息。
- PDO/RDO 表、安全电压/电流限制及 VBUS 测量通道。
- ISR/事件标志及状态机入口点。
- PIOC 代码镜像、SRAM 基地址、IO 引脚、重映射依赖项、数据寄存器用途及协议时序时钟。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮未根据芯片实物、原理图、RM、DS、EVT 源码树或 USB-IF 一致性工具进行验证。
- 实现前，必须根据确切的目标芯片头文件及开发板原理图检查寄存器名、位名、总线域、引脚映射及阈值常量。
- 所有未验证的 PHY 电压、CC 上下拉、VBUS 检测和供电端输出配置都具有电气损坏风险，必须先查目标 RM/DS 和板级原理图，不得仅凭本文或 EVT 示例上电。
