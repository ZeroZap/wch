# WCH HAL 归一化笔记

本文从 `Doc/Ref/wch-dev-skill` 中提取通用外设模式，整理为适用于本仓库未来 WCH HAL 统一工作的规则。

范围：

- 第一轮涵盖 GPIO、时钟/RCC、UART/USART、ADC、DMA 及 Flash 存储。
- 涵盖的系列：CH57x/CH58x BLE 风格 API、CH32V/CH32F/CH32X/CH32H StdPeriph 风格 API、CH561/CH563 寄存器级 API 及 CH5xx 8051 API。
- 本文并非最终 HAL API 规范，而是归一化指南和检查清单。

官方 EVT 示例、RM、DS、头文件及当前仓库源码仍是最终依据。

## 来源文件

- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/gpio_control.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/uart_comm.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/adc_reading.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/flash_storage.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/dma_transfer.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`

## API 风格系列

| 风格 | 系列 | 特征 | 对 HAL 适配器的影响 |
|---|---|---|---|
| BLE 通用 API | CH57x、CH58x/CH59x | `GPIOA_ModeCfg`、`GPIOA_SetBits`、`UART1_DefInit` 等函数；PFIC 中断 | 需要精简的直接辅助函数及感知 highcode 的 ISR 支持。 |
| StdPeriphDriver | CH32V、CH32M030、CH32F、CH32X、CH32H | `GPIO_InitTypeDef`、`USART_InitTypeDef`、`RCC_*ClockCmd`、`NVIC_Init` | 天然适合结构化 HAL 配置对象；CH32M030 是 QingKe RISC-V V3B，不得归入 CH32F ARM。 |
| 寄存器级 ARM7 | CH561/CH563（当前 SDK 未导入，来源未验证） | 来源笔记中的 `R32_PA_DIR`、`R32_PA_OUT`、`R8_*` 时钟/IRQ 寄存器 | 导入官方 SDK 前不生成适配器；来源线索指向位域写入而非 StdPeriph 调用。 |
| 8051 SFR | CH5xx | `Pn_MOD_OC`、`Pn_DIR_PU`、位寻址引脚、安全模式、`interrupt` 语法 | HAL 适配器必须对存储器限定符、SFR 位及编译器约束建模。 |

## 通用外设初始化顺序

各 WCH 系列的外设启动应遵循以下顺序：

1. 选择系统/外设时钟源并更新系统时钟状态。
2. 启用外设总线时钟或电源门控。
3. 配置 GPIO 引脚模式及复用功能。
4. 配置外设寄存器或初始化结构体。
5. 使用 DMA 时配置 DMA。
6. 使用中断时配置中断控制器。
7. 启用外设。
8. 进入主循环前清除待处理标志。

除非 EVT 示例明确要求，否则不要颠倒时钟/GPIO/外设的顺序。

## 时钟与 RCC 归一化

HAL 元数据应区分以下时钟操作：

| 操作 | StdPeriph 风格示例 | BLE 风格/寄存器风格等效操作 |
|---|---|---|
| 系统时钟选择/更新 | `SystemCoreClockUpdate()` | `SetSysClock(...)` |
| 外设时钟使能 | `RCC_APB2PeriphClockCmd(...)`、`RCC_AHBPeriphClockCmd(...)` | `PWR_PeriphClkCfg(...)`、寄存器时钟门控位或系列辅助函数 |
| ADC 时钟预分频 | `RCC_ADCCLKConfig(...)` | 系列特定的 ADC 时钟配置 |
| 低功耗门控 | RCC/PWR 门控 | 寄存器级器件上的休眠时钟禁用寄存器 |

规则：

- HAL 描述必须包含每个外设实例的总线域及时钟使能依赖关系。
- 许多 StdPeriph 系列的 GPIO 与 AFIO/重映射时钟相互独立。
- 寄存器级系列可能使用反相时钟门控位，例如 0 表示时钟已启用。

## GPIO 归一化

通用逻辑 GPIO 模型：

| HAL 概念 | StdPeriph 映射 | CH57x/CH58x 映射 | CH561/CH563 映射 | CH5xx 映射 |
|---|---|---|---|---|
| 浮空输入 | `GPIO_Mode_IN_FLOATING` | `GPIO_ModeIN_Floating` | 输入方向，无 PU/PD | `Pn_MOD_OC=0`、`Pn_DIR_PU=0` |
| 上拉输入 | `GPIO_Mode_IPU` | `GPIO_ModeIN_PU` | 输入方向 + PU 位 | 准双向模式或带上拉模式的输入，取决于芯片 |
| 下拉输入 | `GPIO_Mode_IPD` | `GPIO_ModeIN_PD` | 输入方向 + PD 位 | 芯片特定，通常受限 |
| 推挽输出 | `GPIO_Mode_Out_PP` | `GPIO_ModeOut_PP_*mA` | DIR 输出，可选驱动位 | `Pn_MOD_OC=0`、`Pn_DIR_PU=1` |
| 开漏输出 | `GPIO_Mode_Out_OD` 或 `GPIO_Mode_AF_OD` | 系列特定 | DIR 输出 + 开漏/PD 位 | `Pn_MOD_OC=1`、`Pn_DIR_PU=0` |
| 复用功能 | `GPIO_Mode_AF_PP` 或 `GPIO_Mode_AF_OD` 加重映射 | 系列引脚功能设置 | 可用时使用寄存器多路复用 | `PIN_FUNC` 位 |

GPIO HAL 要求：

- 分别表示端口、引脚、模式、上下拉、驱动强度、输出类型、速度及复用功能。
- 记录系列使用显式 AF 模式、重映射位还是专用引脚功能寄存器。
- 启用存在冲突的外设前，必须检查外设引脚归属。
- 对于 EXTI/GPIO 中断，应包含触发模式、极性、待处理标志清除行为及 IRQ 控制器使能。

GPIO 陷阱：

- StdPeriph 系列未启用端口时钟时，GPIO 写入会静默失败。
- 使用 CH32V 风格 API 时，没有 AFIO 时钟会导致 EXTI/重映射失败。
- 未配置外部或内部上下拉时，浮空输入不稳定。
- 8051 准双向模式不等同于推挽输出。

## UART/USART 归一化

通用 UART 模型：

| 概念 | 字段 |
|---|---|
| 实例 | UART/USART 索引及寄存器基地址 |
| 引脚 | TX、RX、可选 RTS/CTS、重映射选项 |
| 格式 | 波特率、数据位、停止位、奇偶校验 |
| 模式 | TX、RX、半双工、同步、流控 |
| 传输 | 轮询、中断 RX/TX、DMA TX/RX |
| 调试 | 默认 printf UART，通常为 115200 8N1 |

StdPeriph 初始化顺序：

1. 启用 GPIO 和 USART 时钟。
2. 将 TX 引脚配置为复用推挽输出。
3. 根据系列示例将 RX 引脚配置为浮空输入或上拉输入。
4. 填充 `USART_InitTypeDef`。
5. 调用 `USART_Init()`。
6. 根据需要启用中断或 DMA。
7. 调用 `USART_Cmd(..., ENABLE)`。

HAL 适配器规则：

- 不要假定 TX/RX 引脚固定；重映射因系列和实例而异。
- DMA 请求通道映射应与 UART 实例分开建模。
- 环形缓冲区属于模板关注点，而非底层 UART 外设要求。
- 对于 BLE 风格器件，`UART1_DefInit()` 等辅助函数可能配置默认值并隐藏引脚；生成可复用代码时，HAL 应公开显式引脚模式。

## ADC 归一化

通用 ADC 模型：

| 概念 | 字段 |
|---|---|
| 实例 | ADC 索引及通道数 |
| 通道 | 外部引脚通道、内部温度、Vrefint |
| GPIO 模式 | 模拟输入或系列等效模式 |
| 时钟 | ADC 时钟分频器及最大频率限制 |
| 序列 | 排位、采样时间、扫描数量 |
| 转换模式 | 单次、连续、触发、注入 |
| 传输 | 轮询 EOC、中断、DMA 环形缓冲区 |
| 校准 | 有要求时在首次转换前复位/启动/等待 |

从来源文档提取的 ADC 规则：

- 采样外部通道前配置模拟 GPIO 模式。
- 初始化 ADC 前启用 ADC 外设时钟。
- 配置 ADC 时钟分频器，使 ADC 时钟保持在系列限制内。
- 对于 CH32V 风格 ADC，在首次转换前运行校准。
- 多通道连续扫描通常使用 DMA 循环模式模板。
- 内部温度及参考通道可用时，需要显式调用使能函数。

陷阱：

- 跳过 ADC 校准会导致读数不准确或无效。
- 对模拟通道使用数字输入/输出引脚模式会破坏读数。
- 连续扫描时忘记使用 DMA 循环模式会丢失重复采样数据。
- 温度转换公式取决于系列/参考电压，必须对照 RM/DS 验证。

## DMA 归一化

通用 DMA 模型：

| 概念 | 字段 |
|---|---|
| 控制器/通道 | DMA 实例及通道或请求线 |
| 方向 | 存储器到存储器、外设到存储器、存储器到外设 |
| 地址 | 外设寄存器地址、存储器缓冲区地址 |
| 递增 | 外设递增、存储器递增 |
| 宽度 | 字节、半字、字 |
| 模式 | 正常或循环 |
| 优先级 | 低/中/高/极高 |
| 触发 | 外设请求映射 |
| 中断 | 传输完成、半传输、错误 |

DMA 规则：

- 配置通道前启用 DMA 时钟。
- 更改配置前反初始化或禁用通道。
- 外设数据寄存器地址必须与系列头文件一致，例如 CH32V 风格示例中的 `USARTx->DATAR` 或 `ADCx->RDATAR`。
- 请求/通道映射无法跨系列移植；应将其保存在元数据中。
- 除非应用程序有意进行有限次采集，否则连续 ADC 扫描应使用循环 DMA。
- 对于 UART TX DMA，每次传输都应在启用 DMA 通道前设置数据计数器。
- CH32H 双核适配还必须建模 IPC、HSEM 以及 `SysTick0`/`SysTick1`；共享外设或缓冲区不能只依靠单核临界区假设。

## Flash 存储归一化

通用 Flash 模型：

| 概念 | 字段 |
|---|---|
| 区域 | 代码 Flash、DataFlash、选项字节、SNV/NV 区域 |
| 擦除单元 | 每页/扇区/块的字节数 |
| 编程单元 | 字节、半字、字、页、系列特定的快速写入 |
| 解锁/锁定 | 所需序列及 API |
| 保护 | 写保护、选项字节、安全模式 |
| 安全性 | 读取-修改-写入、验证、避开当前代码区域 |

Flash 规则：

- 系列有要求时，擦除/编程前务必解锁。
- 使用 StdPeriph 风格 Flash API 时，开始新操作前清除状态标志。
- 写入前先擦除；Flash 编程无法将位从 0 改回 1。
- 页内局部更新应使用读取-修改-写入。
- 编程后锁定 Flash。
- 存储配置或启动元数据时验证写入的数据。
- 未检查链接器布局、IAP 偏移及 OTA/SNV/DataFlash 区域前，绝不要使用数据存储地址。

提取的笔记中已知的擦除单元：

| 系列 | 擦除单元 |
|---|---|
| CH57x | 256 字节 |
| CH58x/CH59x | 256 字节 |
| CH32V003 | 64 字节 |
| CH32V006/CH32L103 | 1024 字节 |
| CH32V103 | 1024 字节 |
| CH32V20x/CH32V307/CH32V407 | 4096 字节 |
| CH32M030 | 使用当前 `CH32M030EVT` Flash 驱动/RM 验证，不从 CH32F 或其他低资源器件推断 |
| CH32H417 | 4096 字节 |
| CH561/CH563 | 4096 字节 |
| CH5xx 8051 DataFlash | 芯片特定的字节导向 DataFlash，具有受保护写入规则 |

## 寄存器级与 8051 适配器规则

CH561/CH563:

- 当前 SDK 未导入对应 EVT，本节全部为来源未验证线索，不表示当前仓库已有驱动或示例覆盖。
- 来源笔记中没有 `GPIO_InitTypeDef` 或 StdPeriph GPIO 调用。
- GPIO 控制映射到方向、上下拉、驱动、清除、输出及中断寄存器。
- IRQ 设置使用专用的使能/状态/模式/极性寄存器及 `__irq` 处理程序。
- 时钟门控可能使用休眠时钟关闭寄存器，而非 RCC 辅助函数。

CH5xx 8051:

- GPIO 模式由 `Pn_MOD_OC` 与 `Pn_DIR_PU` 对控制。
- 某些引脚功能通过 `PIN_FUNC` 位选择。
- 中断使用编译器特定的向量声明，例如 `interrupt INT_NO_GPIO using 1`。
- Flash/DataFlash 写入需要安全模式及中断屏蔽。
- 大型缓冲区应放入 `xdata`，而非 `data`。

## 建议的 HAL 元数据基本项

未来提取元数据时使用以下概念：

- `family`：来自 `Doc/Family/family-routing.md` 的芯片系列路由。
- `instance`：外设实例名称及基地址。
- `clock`：时钟门控 API/寄存器、总线域、复位依赖关系。
- `pin`：端口、引脚、模式、上下拉、驱动、速度、AF/重映射、冲突列表。
- `irq`：IRQ 名称/向量、优先级模型、处理程序语法、标志清除规则。
- `dma`：请求源、控制器、通道、方向、宽度、循环支持。
- `flash`：擦除单元、编程单元、安全区域、解锁/锁定 API。
- `template`：最小初始化序列及所需包含文件/头文件集合。

## 跨系列 HAL 检查清单

将任何外设规则提升为共享 HAL 抽象前，验证：

- 目标系列使用 StdPeriph、BLE 辅助 API、寄存器级 SFR 还是 8051 SFR？
- 访问外设前必须启用哪个时钟？
- 需要哪种 GPIO 模式及重映射/复用功能路径？
- 哪种 IRQ 控制器及处理程序属性语法有效？
- 状态标志通过写 0、写 1、读取副作用还是 API 调用清除？
- DMA 请求/通道映射是固定、可重映射还是不存在？
- 持久化存储示例适用哪种 Flash 擦除/编程单元？
- 是否存在与预期模板匹配的 EVT 示例？

## 验证状态

- 仅提取自 `wch-dev-skill` Markdown。
- 生成代码前，必须对照仓库中的 EVT 头文件检查准确的 API 名称及请求映射。
- 下一轮验证应检查各已导入系列中具有代表性的 GPIO、USART、ADC、DMA 及 Flash EVT 示例。
