# WCH 芯片系列路由

本文档是本仓库将 WCH 芯片路由到其源参考资料、EVT 目录及未来 HAL 系列说明的入口。

## 源文件

- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/AGENTS.md`
- 工作区根目录下的仓库 EVT 目录
- `Doc/DS/` 和 `Doc/RM/` 下的官方参考资料

## 路由表

| 芯片或系列 | Skill 来源系列 | 仓库 EVT/源目录 | 架构 | 主要关注点 |
|---|---|---|---|---|
| CH57x, CH572, CH573, CH579 | `Doc/Ref/wch-dev-skill/chips/ch57x/` | `CH572EVT/`, `CH573EVT/` | RISC-V | BLE 5.x、RF、USB、LCD/NET 变体 |
| CH58x, CH59x, CH583, CH585, CH592, CH595 | `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/` | `CH583EVT/`, `CH585EVT/`, `CH592EVT/`, `CH595EVT/` | RISC-V | BLE、USB、LCD、NFCA、编码器/键盘扫描变体 |
| CH32V103, CH32V20x, CH32V307, CH32V407 | `Doc/Ref/wch-dev-skill/chips/ch32v-general/` | `CH32V103EVT/`, `CH32V20xEVT/`, `CH32V307EVT/`, `CH32V407EVT/` | RISC-V | StdPeriphDriver、USB、ETH；CAN 与高速外设按子系列验证 |
| CH32V003, CH32V006, CH32L103 | `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/` | `Doc/DS/`、`Doc/RM/` 和相关 EVT 源中的低成本资料 | RISC-V | 小资源 HAL 子集、OPA/CMP、低成本 Flash/RAM 约束 |
| CH32M030 | 当前 `CH32M030EVT/`（可独立路由或并入低资源 StdPeriph 组） | `CH32M030EVT/` | QingKe RISC-V V3B | `ch32m030.h`、64 KB Flash/12 KB RAM、电机控制、USB-PD |
| CH32F103, CH32F20x | `Doc/Ref/wch-dev-skill/chips/ch32f-arm/` | 当前 SDK 未导入对应 EVT，来源未验证 | ARM Cortex-M3 | 仅作来源路由；导入官方 SDK 后再确认功能、内存和工程结构 |
| CH32X035, CH32X315, CH641, CH643 | `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/` | `CH32X035EVT/`, `CH32X315EVT/` | RISC-V | USB-PD、PIOC、USB、触摸/按键外设变体 |
| CH569 | `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/` | `CH569EVT/` | RISC-V | 以太网、USB3、eMMC、ECDC、HSPI、DVP |
| CH32H415/416/417 | `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/` | `CH32H47TEVT/` | RISC-V V3F/V5F | 共享 `ch32h417.h` 与启动文件、IPC/HSEM、双 SysTick、内核专用链接布局 |
| CH561, CH563 | `Doc/Ref/wch-dev-skill/chips/ch561-ch563/` | 当前 SDK 未导入对应 EVT，来源未验证 | ARM7TDMI | 仅作来源路由；寄存器级和工程结论需待官方 SDK 验证 |
| CH543 至 CH559 | `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/` | CH5xx 相关官方文档及任何已导入的 EVT 资料 | 8051 | USB、TouchKey、Type-C、8051 内存限定符和安全模式 |

## 仓库规则

- 此表仅用于路由和源发现。
- 以官方 `Doc/DS/`、`Doc/RM/` 和 EVT 源代码为最终依据。
- 将 `Doc/Ref/wch-dev-skill` 视为提取输入，而非最终 HAL 规范。
- 不要混用不同路由系列的头文件、启动代码文件、链接脚本或中断属性。

## 提取目标

| 系列 | 首个提取目标 | 后续目标 |
|---|---|---|
| `ch57x` | `Doc/BLE/` | `Doc/Core/`, `Doc/USB/`, `Doc/HAL/` |
| `ch58x-ch59x` | `Doc/BLE/` | `Doc/USB/`, `Doc/HAL/`, `Doc/Family/` |
| `ch32v-general` | `Doc/HAL/` | `Doc/Core/`, `Doc/ETH/`, `Doc/USB/`, `Doc/Family/` |
| `ch32v-lowcost` | `Doc/HAL/` | `Doc/Core/`, `Doc/USBPD/`, `Doc/Family/` |
| `ch32f-arm` | `Doc/HAL/` | `Doc/Core/`, `Doc/ETH/`, `Doc/USB/`, `Doc/Family/` |
| `ch32x-usbpd` | `Doc/USBPD/` | `Doc/USB/`, `Doc/HAL/`, `Doc/Family/` |
| `ch56x-ethernet` | `Doc/ETH/` | `Doc/USB/`, `Doc/Core/`, `Doc/Family/` |
| `ch32h-highperf` | `Doc/HAL/` | `Doc/USB/`, `Doc/ETH/`, `Doc/Core/`, `Doc/Family/` |
| `ch561-ch563` | `Doc/ETH/` | `Doc/Core/`, `Doc/Family/` |
| `ch5xx-8051` | `Doc/Core/` | `Doc/USB/`, `Doc/Family/` |

## 待处理事项

- 验证本仓库对 CH32V003、CH32V006、CH32L103、CH32F103、CH32F20x、CH561、CH563 和 CH5xx 的确切 EVT 覆盖范围；当前未导入者不得写成已验证支持。
- 仅在主动归一化某个系列时创建该系列的说明。
- 未来主题文档提取系列特定行为时，应交叉链接本文档。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` 顶层路由文档和仓库目录布局提取。
- 本轮尚未针对每个 EVT 树、RM、DS、封装变体或芯片专用启动代码/链接文件进行完整验证。
- 将本文档视为路由索引。实现前必须根据官方 EVT、RM、DS 和开发板资料核查确切的芯片能力、封装可用性、寄存器细节和示例覆盖范围。
