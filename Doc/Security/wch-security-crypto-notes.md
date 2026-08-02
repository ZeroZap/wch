# WCH 安全、密码、CRC 与 RNG 笔记

本文从 `Doc/Ref/wch-dev-skill` 提取硬件密码、CRC 及随机数相关指导，整理为适用于本仓库的规则，供后续 HAL 元数据、启动/更新验证、安全传输及高速加密传输模板使用。

最终应以官方 EVT 示例、RM、DS、安全评审、外部协议规范、开发板原理图及当前仓库源码为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/ecdc_crypto.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/ecdc_crypto.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/ecdc_crypto.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/crc_calculation.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/rng_random.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/peripheral_api.md`
- `CH32H47TEVT/EXAM/SRC/Peripheral/inc/ch32h417_ecdc.h`
- `CH32V20xEVT/EXAM/CRC/CRC_Calculation/User/main.c`

## 能力矩阵

| 能力 | 来源笔记涉及的系列 | 主要约束 |
|---|---|---|
| CH56x ECDC | CH569 / CH56x RISC-V | 支持 AES-128/192/256 和 SM4、ECB/CTR、单寄存器、自 DMA 及 HSPI 联动 DMA；必须将数据放置在 RAMX 并初始化密钥。 |
| CH32H ECDC | CH32H417 | 当前 EVT 头文件枚举 AES、SM4 两种算法，CTR、ECB 两种模式；AES 密钥长度枚举为 128/192/256 位，没有 CBC 枚举。支持单次及 RAM 块加解密；缓冲区对齐和长度单位仍须按具体执行路径核对。 |
| CH561/CH563 密码运算 | CH561/CH563 ARM7TDMI | 来源笔记中没有 ECDC 引擎；需要硬件加密时，应使用软件密码算法或选择 CH569。 |
| 硬件 CRC | CH32V20x、CH32H417；CH561/CH563 笔记提及有待验证的 CRC 能力 | 当前仓库已有 V20x 硬件 CRC-32 EVT 示例；用于数据完整性、固件验证及协议检查时，必须明确多项式、种子、字序及复位行为。 |
| 硬件 RNG | CH32F20x 和 CH32H417 来源笔记 | 必须等待数据就绪标志；原始硬件输出用于密码学用途时仍需安全评审及调理。 |

## 密码功能边界规则

- 将加密、仅用于完整性的 CRC、随机数生成、密钥存储及协议认证视为不同概念。
- 硬件 ECDC 用于加速分组密码；其本身不提供认证、安全密钥存储、防回滚或传输安全能力。
- CRC 不是密码学签名或 MAC。它用于检测意外损坏，不能用于抵御攻击者。
- RNG 输出应视为熵输入。用于密码学时需要进行调理、健康检查及安全评审。
- 不得仅依据 Markdown 提取内容宣称安全属性；必须结合数据手册、威胁模型及产品要求进行验证。

## ECDC 规则

CH56x ECDC 规则：

- 加解密前，始终使用 `ECDC_Init()` 初始化密钥及模式，或使用 `ECDC_SetKey()` 设置密钥。来源中的陷阱说明密钥寄存器在启动时包含随机值。
- 显式选择算法、分组模式、密钥长度、时钟、执行模式及字节序。
- 根据来源笔记，将 `ECDC_SelfDMA()` 的长度视为 32 位字数，而非字节数。
- 将自 DMA 缓冲区放置在 RAMX 中（来源方案中为 `0x20020000+`）；ECDC DMA 可能无法访问普通 RAM。
- ECB 输入长度必须为 16 字节的整数倍。CTR 模式还需要明确的计数器/随机数管理及重载规则。
- 对于 HSPI 联动 DMA，记录数据路径方向：从 RAM 到外设的加解密，或从外设到 RAM 的加解密。
- ECDC 时钟配置应与系统时钟分开。来源笔记提到 240 MHz 或 160 MHz ECDC 时钟，以及对最低系统时钟的要求。

CH32H ECDC 规则：

- 配置传输前，先使能 ECDC 总线/外设时钟及硬件时钟。
- 使用对齐的缓冲区；来源笔记采用 32 字节对齐。
- 在元数据中存储源地址、目标地址、块数、算法、分组模式、密钥长度、IV、密钥、字节序模式及执行模式。
- 读取目标缓冲区前，等待 RAM 到 RAM 结束等完成标志，并在使用后清除标志。
- 当前 EVT 头文件 `CH32H47TEVT/EXAM/SRC/Peripheral/inc/ch32h417_ecdc.h` 明确枚举 `ECDCAlgorithm_SM4`、`ECDCAlgorithm_AES`，`ECDCBlockCipherMode_CTR`、`ECDCBlockCipherMode_ECB`，以及 `ECDCKeyLen_128b/192b/256b`。其中 192/256 位是 AES 密钥长度能力；SM4 使用固定的 128 位密钥。
- 该头文件没有 CBC 枚举，因此不得把来源笔记中的 CBC 描述当作 H417 当前 EVT API 能力，也不得推断 GCM 或其他认证模式。

CH561/CH563 规则：

- 将 ECDC 标记为不可用。不得为 CH561/CH563 生成 CH569 ECDC 调用。
- 可以使用软件 AES/SM4，但必须考虑性能及内存影响。
- 若需要加密的以太网或 eMMC 传输，来源笔记指出 CH569 是具备硬件加速的系列。

## CRC 规则

仅当双方使用相同配置时，CRC 才适用于启动、存储及通信完整性检查。

规则：

- 存储完整的 CRC 配置：宽度、多项式、初始值、输入反射、输出反射、最终异或、馈入宽度、字节/字顺序及复位行为。
- 开始新的独立计算前复位 CRC 状态。
- 将固件镜像 CRC、数据包 CRC、存储块 CRC 及协议专用 CRC 分别配置。
- 若多个模块共用 CRC 外设，应定义所有权或锁；不相关的代码可能破坏唯一的硬件 CRC 状态。
- 对于引导加载程序验证，应同时检查 CRC、镜像长度、目标范围、栈/复位向量，并按 `Doc/IAP/wch-iap-ota-notes.md` 采用可选签名策略。
- 不得使用 CRC 证明真实性。

CH32V20x 当前 EVT 证据：

- 准确示例路径为 `CH32V20xEVT/EXAM/CRC/CRC_Calculation/User/main.c`。
- 示例使能 `RCC_AHBPeriph_CRC`，调用 `CRC_CalcBlockCRC()` 对 32 个 `u32` 计算硬件 CRC-32；注释给出的多项式为 `0x04C11DB7`，示例期望值为 `0x199AC3CA`。
- 该示例证明当前 V20x EVT 提供硬件 CRC 路径，但不能据此省略对初始值、输入顺序、反射和最终异或等协议参数的核对。

## RNG 规则

RNG 涉及安全性，不应将其当作普通计数器或 PRNG 辅助函数。

规则：

- 启用 RNG 前，先使能正确的总线/外设时钟。
- 读取前等待 `RNG_FLAG_DRDY` 等数据就绪标志。
- 在目标提供相关能力时，处理启动延迟及错误/状态标志。
- 对安全敏感的有界随机值不得使用取模缩减，因为这会引入偏差；要求均匀分布时应使用拒绝采样。
- 未经调理、健康检查及安全评审，不得直接使用原始 RNG 输出生成长期密码密钥。
- 按芯片记录可用性。当前仓库未导入 CH32F EVT，因此 CH32F10x/20x 的 RNG 结论仍只是来源笔记；CH32M030 是独立的 QingKe RISC-V V3B 系列，应只依据 `CH32M030EVT` 的头文件和示例判断，不能与 CH32F 合并推断。

## 与其他主题的集成

- `Doc/Storage/wch-storage-notes.md`：加密 eMMC、HSPI ECDC 及外部闪存完整性需要密码/存储交叉元数据。
- `Doc/IO/wch-io-media-notes.md`：高吞吐 DMA 数据流在增加加密前，需要定义缓冲区所有权及对齐规则。
- `Doc/IAP/wch-iap-ota-notes.md`：固件更新验证可以使用 CRC 检测意外损坏，并使用签名/MAC 验证真实性。
- `Doc/System/wch-system-analog-power-notes.md`：RNG、CRC 及电源/时钟门控规则应与系统时钟和低功耗策略一致。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| ECDC 密钥未初始化 | 产生随机或无法恢复的密文 | 使用前始终初始化密钥/计数器。 |
| ECDC 缓冲区位于错误的 RAM 中 | DMA 传输失败或数据损坏 | 使用系列要求的 DMA 可访问 RAM，例如 CH56x RAMX。 |
| 误读 ECDC 长度单位 | 传输不完整或溢出 | 记录长度单位是字、字节还是 128 位块。 |
| 重用 CTR 计数器 | 机密性失效 | 将计数器/随机数唯一性作为协议元数据的一部分。 |
| 对结构化数据使用 ECB | 泄露数据模式 | 优先使用经过评审的模式/填充/协议；产品安全设计不得默认使用 ECB。 |
| 使用 CRC 进行认证 | 攻击者可以修改数据并重新计算 CRC | 使用 MAC/签名验证真实性。 |
| CRC 配置不完整 | 系统间计算结果不一致 | 存储多项式、种子、位顺序、最终异或及馈入顺序。 |
| RNG 就绪前读取 | 得到未定义或重复的值 | 等待就绪并处理错误。 |
| 对机密随机数使用 RNG 取模缩减 | 有界值存在偏差 | 使用拒绝采样获得均匀的安全范围。 |

## 元数据检查清单

后续安全/密码模板应显式表示以下字段：

- 密码能力：无、仅软件、CH56x ECDC、CH32H ECDC 或其他已验证的硬件模块。
- 算法、分组模式、密钥长度、IV/计数器/随机数策略、字节序、填充、认证策略及密钥生命周期。
- 执行路径：单寄存器、RAM 到 RAM DMA、自 DMA、外设联动 DMA 或软件。
- 缓冲区要求：地址范围、对齐、长度单位、缓存/一致性、所有权及原地/异地处理行为。
- CRC 配置：宽度、多项式、种子、反射、最终异或、馈入宽度/顺序、复位及所有者/锁策略。
- RNG 配置：可用性、时钟、就绪/错误标志、调理、健康检查及获准的密码学用途。
- 验证来源：准确的 EVT 示例、RM/DS 章节、头文件定义、测试向量及安全评审状态。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮已抽查当前仓库 H417 EVT ECDC 头文件和 V20x CRC 示例，修正了 H417 算法、模式和密钥长度枚举，并确认 V20x 硬件 CRC 示例路径；尚未在芯片实物、RM、DS、官方安全文档、已知答案测试向量、侧信道行为或产品威胁模型上验证。
- H417 的当前 EVT API 枚举和 V20x 示例存在性可视为已核对；长度单位、时钟限制、RAM 要求、RNG 质量、CRC 完整参数及安全声明仍须针对目标芯片和产品要求验证。
