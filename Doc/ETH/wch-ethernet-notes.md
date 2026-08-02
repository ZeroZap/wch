# WCH 以太网笔记

本文从 `Doc/Ref/wch-dev-skill` 提取以太网相关指导，整理为适用于本仓库的笔记，供后续 HAL、驱动及模板工作使用。

范围：

- 支持以太网和 USB3 的 CH569 / CH56x RISC-V 系列。
- 采用寄存器级以太网的 CH561/CH563 ARM7TDMI 系列。
- 采用 StdPeriph 风格以太网的 CH32V307/CH32V407 系列。
- 采用 WCHNET 以太网的 CH32F20x 系列。
- 高性能以太网系列 CH32H417。

最终应以官方 EVT 示例、开发板原理图、PHY 数据手册、RM、DS 及头文件为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/ethernet.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/pitfalls.md`

## 系列覆盖范围

| 系列 | 以太网模块 | PHY 模型 | 软件风格 | 说明 |
|---|---|---|---|---|
| CH569 | 当前 EVT 提供 10/100/1000 速率处理路径 | 通过 RGMII 连接外部 PHY | RISC-V 外设库 | 当前 EVT 的 PHY 状态处理会配置 10M、100M、1000M；DMA 缓冲区位于 RAMX / `.dmadata`。不得将旧来源笔记的 10/100M、MII/RMII 结论套到 CH569。 |
| CH561/CH563 | 集成 10/100M MAC | 通过 MII/RMII 连接外部 PHY | ARM7 寄存器级 | 无 StdPeriph；使用 `R32_ETH_*` 寄存器及 `__irq`。 |
| CH32V307/CH32V407 | 集成 MAC | CH32V307 来源笔记提到内部 10BASE-T PHY；100M 需要外部 PHY | StdPeriph ETH 驱动 | 需要 RCC 时钟、GPIO 重映射及 DMA 描述符。 |
| CH32F20x | 配合 WCHNET 的以太网 MAC | 取决于开发板/变体 | WCHNET TCP/IP 套接字 API | 必须周期性运行 `WCHNET_MainTask()`。 |
| CH32H417 | 当前 EVT 提供以太网 MAC/网络示例 | 当前 EVT 使用外部 PHY 和 RGMII | 类 StdPeriph 驱动及网络库 | 当前文档不宣称内置 PHY，也不把 MII/RMII 写成当前 EVT 接口；PTP 能力尚未充分验证，标记为待核。 |

## 硬件模型

通用以太网硬件栈：

```text
MCU MAC <-> MII/RMII/RGMII pins <-> PHY <-> magnetics/RJ45
        <-> MDC/MDIO management bus
        <-> DMA descriptor rings and frame buffers
```

HAL 元数据应建模以下内容：

- MAC 实例及寄存器基地址。
- PHY 地址及管理总线引脚。
- MII/RMII/RGMII 模式及引脚映射。
- PHY 复位 GPIO 及复位时序。
- 参考时钟源及 PHY 时钟要求。
- DMA 描述符数量、缓冲区大小、内存区域及对齐。
- MAC 地址来源及唯一性策略。
- 链路状态、速率、双工、自动协商及校验和卸载。

## 初始化顺序

通用以太网初始化顺序：

1. 初始化系统时钟及延时/定时器时基。
2. 使能以太网 MAC/DMA/时钟门控。
3. 为 MDC、MDIO、TX、RX、时钟及控制信号配置 GPIO 引脚及重映射/AF 模式。
4. 需要时通过 GPIO 或 PHY 寄存器复位外部 PHY。
5. 通过 MDC/MDIO 探测 PHY 地址。
6. 配置自动协商或固定速率/双工模式。
7. 软件复位 MAC/DMA 模块。
8. 配置 MAC 地址及帧过滤选项。
9. 分配并初始化 TX/RX DMA 描述符环。
10. 使能 DMA 中断或轮询路径。
11. 启动 MAC TX/RX 及 DMA TX/RX。
12. 发送应用帧前验证链路状态。

系列差异：

- CH56x 来源笔记使用 `PWR_PeriphClkCfg(ENABLE, BIT_SLP_CLK_ETH)`。
- CH32V/CH32H 使用 `RCC_*PeriphClockCmd(...)` 风格的时钟使能。
- CH561/CH563 使用 `R8_SLP_CLK_OFF*` 位等寄存器时钟门控。
- CH32F20x WCHNET 使用周期性网络任务及套接字级 API，而非仅直接操作帧的示例。

## PHY 访问与链路管理

PHY 访问通常使用 MDC/MDIO 管理寄存器或辅助函数：

- `ETH_ReadPHYRegister(phy_addr, reg)`
- `ETH_WritePHYRegister(phy_addr, reg, value)`
- CH561/CH563 的寄存器级等效方式使用 `R32_ETH_MII_ADDR` 和 `R32_ETH_MII_DATA`。

PHY 处理规则：

- 当前 CH569/CH32H417 EVT 中固定 `PHY_ADDRESS` 为 1，这只是配套开发板示例值，不是芯片固定属性。
- PHY 地址应以原理图和 PHY strap 为准；扫描地址 0 至 31 是未知板卡或链路故障时的调试策略，不应替代产品配置。
- 若读取返回 `0xFFFF` 或 `0x0000`，检查 PHY 电源、复位、MDC/MDIO 引脚映射及参考时钟。
- 开发板有要求时，在上电后复位 PHY。
- 等待自动协商完成，或显式配置速率/双工模式。
- 发送帧前及 PHY 变化中断后重新检查链路状态。

来源笔记中的常用寄存器：

| 寄存器 | 用途 |
|---|---|
| `PHY_BCR` | 基本控制寄存器，包含复位。 |
| `PHY_BSR` | 基本状态寄存器，包含链路及自动协商状态。 |
| `PHY_PHYIDR1` / `PHY_PHYIDR2` | PHY 标识。 |
| `PHY_ANAR` | 自动协商通告。 |

## DMA 描述符与缓冲区放置

常用描述符字段：

| 字段 | 用途 |
|---|---|
| `Status` | 所有权位、帧状态、首/末段及长度字段。 |
| `ControlBufferSize` | 缓冲区大小及描述符控制。 |
| `Buffer1Addr` | 帧缓冲区指针。 |
| `Buffer2NextDescAddr` | 下一描述符指针或第二缓冲区。 |

规则：

- TX/RX 描述符必须至少按字边界对齐。
- 以太网帧缓冲区必须足以容纳完整以太网帧，通常约为 1518 至 1536 字节。
- 描述符环的最后一个描述符应回绕至第一个描述符。
- 写入 TX 描述符或读取 RX 描述符前，必须遵循 DMA 所有权位。
- 将 RX 描述符归还 DMA 后，若硬件有要求，应恢复 RX 轮询/请求。

系列专用缓冲区放置规则：

| 系列 | 缓冲区放置规则 |
|---|---|
| CH569 / CH56x | 来源笔记要求通过 `.dmadata` 将以太网 DMA 缓冲区及描述符放入 RAMX；ETH DMA 无法访问普通 RAM。 |
| CH561/CH563 | 来源笔记将对齐缓冲区放入 DATA SRAM，而非 `.dmadata`。 |
| CH32V / CH32H | 来源笔记要求对齐；应在 EVT 中验证缓存/TCM/外部 RAM 约束。 |
| CH32F20x WCHNET | 套接字接收缓冲区通过 WCHNET API 分配；应检查库约束。 |

## PTP 状态

当前仓库材料尚不足以确认 CH569 或 CH32H417 的完整 PTP 硬件能力、时间戳路径、时钟源和网络栈集成。相关元数据和 HAL API 一律标为“待核”，必须经目标 RM/DS、头文件和可运行 EVT 示例共同验证后才能宣称支持。

## 帧级 API 结构

底层以太网 HAL 应区分：

- 使用 DMA 描述符或 `ETH_HandleTxPkt` / `ETH_HandleRxPkt` 收发原始帧。
- 链路及 PHY 管理。
- MAC 地址配置。
- DMA 描述符环配置。
- WCHNET 或类 lwIP 层等高层协议栈集成。

不得将原始 MAC/PHY API 与套接字 API 合并为一层。应使用以下独立层级：

```text
Ethernet MAC 驱动         # 描述符、帧、链路、PHY
网络栈适配器              # WCHNET、lwIP、DHCP、DNS、TCP/UDP socket
应用模板                  # TCP 客户端/服务器、UDP、MQTT 等
```

## WCHNET 笔记

CH32F20x 来源笔记描述了 WCHNET 套接字 API。

重要规则：

- 使用 IP、网关、掩码及 MAC 调用 `WCHNET_Init(...)`。
- 周期性调用 `WCHNET_MainTask()`，通常从定时器 ISR 或周期任务调用。
- 处理 PHY 变化、套接字事件、不可达及 IP 冲突等全局中断。
- 使用各套接字的中断处理接收、连接、断开及超时。
- 需要时显式配置套接字接收缓冲区。

WCHNET 位于原始 MAC 层之上。后续 HAL 元数据应将 WCHNET 作为网络栈配置，而不是强制性的以太网驱动 API。

## 中断与轮询

中断风格因架构而异：

- CH56x RISC-V 示例使用 WCH 快速中断属性。
- CH561/CH563 使用 `__irq` 及寄存器级中断标志。
- CH32V/CH32H 根据系列使用 StdPeriph/PFIC/类 NVIC API。
- 即使存在以太网中断，WCHNET 仍可能依赖定时器驱动的周期任务处理。

规则：

- 按系列 API 或寄存器写入语义清除 DMA RX/TX 中断标志。
- 避免在底层 DMA ISR 内直接执行繁重的协议处理；可行时延后到任务/主循环执行。
- 若系列提供致命总线错误及异常中断汇总标志，必须进行处理。

## 常见陷阱

| 陷阱 | 受影响系列 | 规则 |
|---|---|---|
| 未使能以太网时钟 | 全部 | 访问寄存器前使能 ETH MAC/DMA/外设时钟。 |
| PHY 地址错误 | 外部 PHY 设计 | EVT 中地址 1 仅为示例；先检查原理图/strap，扫描 0-31 仅作为调试策略。 |
| 未配置 MDC/MDIO 或 MII/RMII 引脚 | 所有板级 ETH | 访问 PHY 前配置引脚复用/重映射/AF。 |
| PHY 上电后未复位 | 许多开发板 | 使用复位 GPIO 或 PHY 软件复位并等待。 |
| DMA 缓冲区位于不可访问的内存中 | 尤其是 CH569 / CH56x | 将缓冲区放入要求的 RAM 区域及段。 |
| DMA 缓冲区未对齐 | 所有 DMA ETH | 将描述符及帧缓冲区至少按 4 字节对齐。 |
| 大型帧缓冲区位于栈中 | 全部 | 使用静态/全局缓冲区；避免 1500 字节的局部数组。 |
| 忽略链路状态 | 全部 | TX/RX 前检查链路并处理 PHY 变化。 |
| 将 CH561/CH563 当作 CH569 | CH561/CH563 | 使用 ARM7 寄存器级流程，而非 CH569 RISC-V API。 |
| 假设 CH32V103/CH32V20x 有 ETH | CH32V 通用系列 | 验证确切芯片能力；来源笔记标明 CH32V307/V407 支持 ETH。 |
| MAC 地址重复 | 所有联网产品 | 使用 WCH 提供的 MAC、由芯片 UID 派生的本地 MAC 或分配的本地管理 MAC。 |

## 当前示例索引

| 系列 | 当前仓库示例路径 |
|---|---|
| CH569 | `CH569EVT/EXAM/ETH/` |
| CH32V307 | `CH32V307EVT/EXAM/ETH/` |
| CH32V407 | `CH32V407EVT/EXAM/ETH/` |
| CH32H417 | `CH32H47TEVT/EXAM/ETH/` |

CH561/CH563 和 CH32F20x 在当前仓库没有对应 EVT 示例根，本文中的相关说明仅作来源笔记索引，不能当作当前仓库路径。

## 建议的以太网元数据

后续 CubeX/HAL 元数据应包括：

- `has_eth_mac`：是/否。
- `phy_type`：内部、外部或由开发板定义。
- `phy_interface`：MII、RMII、RGMII 或固定内部接口。
- `phy_address`：默认、扫描或开发板专用。
- `phy_reset_pin`：可选端口/引脚及时序。
- `mac_address_source`：固定、由 UID 派生、WCH 提供或用户提供。
- `dma_buffer_region`：普通 RAM、RAMX、DTCM、外部 RAM 或禁止使用栈。
- `descriptor_alignment`：字节对齐要求。
- `frame_buffer_size`：默认 RX/TX 缓冲区大小。
- `driver_level`：原始 MAC、WCHNET 套接字栈、lwIP 适配器或开发板 BSP。
- `requires_periodic_task`：WCHNET 等网络栈配置是否需要周期任务。
- `ptp_support`：当前对 CH569/CH32H417 标为待核，不能由来源笔记直接置为支持。

## 验证状态

- 仅提取自 `wch-dev-skill` Markdown。
- 必须根据仓库 EVT 示例及开发板原理图检查确切的 PHY 引脚映射、内存段名称、DMA 可访问性及网络栈 API。
- 下一轮验证应检查 CH569、CH561/CH563、CH32V307/407、CH32F20x 及 CH32H417 已导入的 `ETH` / `NET` 示例和当前头文件。
