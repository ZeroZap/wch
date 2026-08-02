# WCH IAP/OTA 与应用跳转分析

## 1. 分析范围

本报告重新分析仓库中的实际 EVT 源码，不再以旧版 `Doc/Ref/wch-dev-skill` 摘要作为主要依据。

原分析的窄范围为：

```text
*EVT/EXAM/IAP/**
*EVT/EVT/EXAM/IAP/**
```

该窄范围共覆盖 16 个 EVT、487 个 `EXAM/IAP` 路径内文件：

| 分组 | EVT |
|---|---|
| CH32 通用 IAP | CH32V103、CH32V20x、CH32V307、CH32V407、CH32X035、CH32X315、CH32L103、CH32M030 |
| CH32H 双核 | CH32H417/CH32H47T |
| CH56x | CH569 |
| CH57/58/59 | CH572、CH573、CH583、CH585、CH592、CH595 |

CH573 的目录比其他 EVT 多一层 `EVT`：

```text
CH573EVT/EVT/EXAM/IAP
```

但仓库中的 IAP/OTA 不只位于上述目录。为避免把“`EXAM/IAP` 示例”误写成“全仓库 IAP”，本文同时扩展检索以下形态：

```text
*EVT/EXAM/USB*/**/HOST_IAP/**
*EVT/EXAM/ETH/ETH_IAP/**
*EVT/EXAM/BLE/**/*IAP*/**
```

按升级入口和传输角色可分为：

| 形态 | 说明 | 已核对入口 |
|---|---|---|
| 原 `EXAM/IAP` | USB device/UART 接收镜像，或其 APP 配套工程 | 见第 4 节原窄范围总表 |
| Host IAP | MCU 作为 USB Host，从 U 盘等介质读取镜像 | X035 `0x6000`；X315/M030/L103/V407 `0x8000`；H417 V3F/V5F `0xC000`/`0x1C000` |
| ETH IAP | WCHNET/Ethernet 接收并写入镜像 | V20x/V307 `0xA000` |
| BLE OTA/IAP | BLE、Mesh 或备份升级工程，含 `OnlyUpdateApp`、`BackupUpgrade`、`RemoteController`、`adv_vendor_self_provision` 等 IAP/JumpIAP 组合 | 入口和镜像布局随具体 BLE 工程而定，不能套用 `EXAM/IAP` 的 `0x1000` |

因此，后文标注“当前 16 个 EVT”或“当前示例”时，如未特别说明，仍指原 `EXAM/IAP` 窄范围；涉及全仓库结论时会明确写出 Host IAP、ETH IAP 或 BLE OTA。

本报告重点回答：

1. APP 是否可以像常见 Cortex-M3/M4 bootloader 那样直接跳转。
2. 是否存在 Cortex-M0 式限制。
3. 每组示例实际如何处理 APP 入口、栈和中断向量。
4. 示例代码距离可投入产品的可靠 IAP/OTA 还缺什么。

## 2. 核心结论

### 2.1 当前已选目标都是 RISC-V，但源码保留 ARM 条件分支

当前实际选择的 `EXAM/IAP` 目标以及本轮核对的 Host IAP、ETH IAP、BLE OTA 目标都使用 WCH QingKe RISC-V 32 位内核，不存在当前按 Cortex-M0、M3 或 M4 构建的目标。

直接证据包括：

- 工程配置选择 `RISC-V`、`rv32i` 或 QingKe 内核。
- IAP 源码包含 `core_riscv.h`，例如 `CH32M030EVT/EXAM/IAP/UART_USB_IAP/CH32M030_IAP/User/iap.c:12-15`。
- startup 使用 RISC-V 的 `gp`、`sp`、`mstatus`、`mtvec`、`mepc` 和 `mret`。
- 跳转汇编使用 `li` 和 `jr`，而不是 ARM 的 `MSR MSP`、`BX` 或 `SCB->VTOR`。

不能据此写成“仓库没有 MSP 代码”。部分 Host IAP 复用了跨内核源码并保留 `DEF_CORE_CM3` 条件分支，例如 `CH32V103EVT/EXAM/USB/USBFS/HOST_IAP/HOST_IAP/User/Host_IAP/usb_host_iap.c`、L103/V20x/H417 的同名文件。该分支会读取 `APP_BASE + 4` 的入口，并读取 APP 首字作为 MSP；这是为 Cortex-M3 镜像格式保留的条件编译实现，不代表当前工程选择了 CM3，更不能据此把当前构建架构判定为 ARM。

尤其要注意：

- `CH32M030` 名称中的 `M030` 不表示 Cortex-M0。
- CH32H417 的 V3F/V5F 也是 WCH RISC-V 双核，不是 Cortex-M3/M4/M5。
- CH572/573/583/585/592/595 也不是 Cortex-M，它们同样使用 QingKe RISC-V 和 PFIC。

### 2.2 可以直接跳，但不是 Cortex-M4 的跳法

这些示例都能把控制权直接交给 APP，但准确说法是：

> 跳到 APP 镜像的 RISC-V `_start` 指令入口，由 APP startup 自行重新建立运行环境。

不是：

> 从 APP 向量表读取首字 MSP、次字 Reset_Handler，设置 MSP/VTOR 后跳转。

两种镜像格式完全不同。

RISC-V APP 起始位置是可执行指令：

```asm
_start:
    j handle_reset
```

而常见 Cortex-M APP 起始位置是数据表：

```text
APP_BASE + 0x00 = initial MSP
APP_BASE + 0x04 = Reset_Handler address
```

因此当前已选 RISC-V 路径不会读取 APP 首字作为栈指针，也不会设置 MSP。APP startup 会执行 `la sp, _eusrstack`，再设置自己的 `mtvec`。Host IAP 中未被当前目标选中的 `DEF_CORE_CM3` 分支不改变这一核心结论。

### 2.3 Cortex-M0 并非不能跳转

“Cortex-M0 不能像 M4 那样直接跳转”不准确。

真正的 Cortex-M0：

- 有 MSP，可以读取 APP 首字并设置 MSP。
- 可以读取 APP 第二字并跳到 Reset_Handler。
- 主要限制通常是部分 Cortex-M0 没有可编程 VTOR，导致 APP 中断向量不能简单改基址。
- 没有 VTOR 时，需要芯片提供 Flash/SRAM remap、固定向量代理，或由 bootloader 转发中断。

所以 M0 的难点是**向量重定位和中断接管**，不是架构上完全不能跳 APP。

但这套 Cortex 讨论不适用于仓库当前已选 IAP 构建目标，因为它们都是 RISC-V；它只解释了 Host IAP 公共源码中保留的 `DEF_CORE_CM3` 分支。

## 3. Cortex 与当前 RISC-V 跳转模型对比

### 3.1 常见 Cortex-M3/M4 模型

典型 Cortex-M 跳转流程：

```c
uint32_t app_sp = *(uint32_t *)(APP_BASE + 0);
uint32_t app_pc = *(uint32_t *)(APP_BASE + 4);

__disable_irq();
SCB->VTOR = APP_BASE;
__set_MSP(app_sp);
((void (*)(void))app_pc)();
```

这个模型成立的前提是 APP 镜像开头为 Cortex 向量表。

### 3.2 当前 WCH RISC-V 模型

当前 EVT 示例的等价过程是：

```text
IAP 跳 APP_BASE
  -> 执行 APP 的 _start
  -> j handle_reset
  -> 设置 gp
  -> 设置 sp
  -> 复制 highcode/data
  -> 清 bss
  -> 配置 mstatus 和厂商 CSR
  -> mtvec = APP 自己的 _vector_base
  -> mepc = main
  -> mret
```

以 CH583 APP 为例：

- `handle_reset`：`CH583EVT/EXAM/IAP/APP/Startup/startup_CH583.S:121-124`
- 设置 `gp/sp`：同文件 `125-130`
- 复制 highcode：`132-143`
- 复制 `.data`：`145-156`
- 清 `.bss`：`158-165`
- 写 `mstatus`：`174-175`
- 写 APP 的 `mtvec`：`176-180`
- `mepc=main`、`mret`：`197-201`

所以 IAP 不需要提前知道 APP 的栈顶值，也不需要模拟 Cortex 的向量表读取。

## 4. IAP 工程与地址布局

### 4.1 总表

下表区分：

- **执行地址**：CPU 跳转及 APP linker 使用的零基别名。
- **编程地址**：部分 CH32 Flash 控制器访问使用的 `0x08000000` 别名。

| EVT | IAP 传输 | APP 执行地址 | Flash 编程地址 | 直接切换机制 |
|---|---|---:|---:|---|
| CH572 | USB、UART | `0x00001000` | 同执行地址/ROM API | 函数指针调用 APP `_start` |
| CH573 | USB、UART | `0x00001000` | 同执行地址/ROM API | 函数指针调用 APP `_start` |
| CH583 | USB、UART | `0x00001000` | 同执行地址/ROM API | 函数指针调用 APP `_start` |
| CH585 | USB、USBHS、UART | `0x00001000` | 同执行地址/ROM API | 函数指针调用 APP `_start` |
| CH592 | USB、UART | `0x00001000` | 同执行地址/ROM API | 函数指针调用 APP `_start` |
| CH595 | USB、UART | `0x00001000` | 同执行地址/Flash API | 函数指针调用 APP `_start` |
| CH569 | USBHS/UART 组合 | `0x00004000` | `0x00004000` | PFIC SWI handler 中 `jr 0x4000` |
| CH32V103 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32V20x | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32V307 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32V407 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32X035 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32X315 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32L103 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32M030 | USB/UART | `0x00005000` | `0x08005000` | PFIC SWI handler 中 `jr 0x5000` |
| CH32H417 V3F | USB/UART | `0x00006000` | `0x08006000` | PFIC SWI handler 中 `jr 0x6000` |
| CH32H417 V5F | V3F 唤醒 | `0x00010000` | 镜像布局专用 | `NVIC_WakeUp_V5F(0x10000)` |

原表只描述 `EXAM/IAP` 窄范围。全仓库其他 IAP 形态的已核对布局为：

| 形态/目标 | APP 执行入口 | 备注 |
|---|---:|---|
| CH32V103/V20x Host IAP | `0x00005000` | USBFS Host IAP 配套 APP，与各自 USB/UART IAP 入口相同 |
| CH32V307 USBFS/USBHS Host IAP | `0x00006000` | 两类 Host IAP 配套 APP；不能沿用其 USB/UART IAP 的 `0x5000` |
| CH32X035 Host IAP | `0x00006000` | USB Host IAP 配套 APP |
| CH32X315/M030/L103/V407 Host IAP | `0x00008000` | USB Host IAP 配套 APP |
| CH32V20x/V307 ETH IAP | `0x0000A000` | 编程别名为 `0x0800A000`，SWI 中 `jr 0xA000` |
| CH32H417 Host IAP V3F | `0x0000C000` | Host IAP 双核布局 |
| CH32H417 Host IAP V5F | `0x0001C000` | Host IAP 双核布局，由 V3F 唤醒 |

### 4.2 地址不是统一的 `0x1000`

仅原 `EXAM/IAP` 窄范围就存在五种 APP 入口：

| APP 入口 | 使用者 |
|---:|---|
| `0x1000` | CH572/573/583/585/592/595 |
| `0x4000` | CH569 |
| `0x5000` | 普通 CH32 IAP 示例 |
| `0x6000` | CH32H417 V3F APP |
| `0x10000` | CH32H417 V5F APP |

扩展到全仓库后，Host/ETH IAP 还使用 `0x5000`、`0x6000`、`0x8000`、`0xA000`、`0xC000`、`0x1C000`；其中 `0x5000` 和 `0x6000` 都同时出现在不同形态中。不能把 APP 起址固化为统一常量，也不能只凭入口值推断升级形态。

## 5. CH57/58/59 IAP 深入分析

### 5.1 工程枚举

| EVT | IAP 子工程 |
|---|---|
| CH572 | `USB_IAP`、`UART_IAP`、`APP` |
| CH573 | `USB_IAP`、`UART_IAP`、`APP` |
| CH583 | `USB_IAP`、`UART_IAP`、`APP` |
| CH585 | `USB_IAP`、`USBHS_IAP`、`UART_IAP`、`APP` |
| CH592 | `USB_IAP`、`UART_IAP`、`APP` |
| CH595 | `USB_IAP`、`UART_IAP`、`APP` |

### 5.2 跳转实现

这 6 个型号都定义类似：

```c
#define APP_CODE_START_ADDR 0x00001000
#define jumpApp ((void (*)(void))((int *)APP_CODE_START_ADDR))
```

CH583 证据为 `CH583EVT/EXAM/IAP/USB_IAP/src/iap.h:17-24`。

这个宏虽然类型转换写法不够整洁，但语义是把数值 `0x1000` 当函数入口调用。它不是解引用 `0x1000` 得到另一个入口，也不是读取 Cortex 向量表。

APP linker 将 `.init` 放在 `0x1000`，而 startup 的 `_start` 位于 `.init` 并执行 `j handle_reset`。因此直接调用 `0x1000` 能进入完整 startup。

### 5.3 向量表分组

CH572、CH583、CH585、CH592、CH595 使用 WCH 绝对地址向量模式：

```asm
la t0, _vector_base
ori t0, t0, 3
csrw mtvec, t0
```

证据：

- CH572：`CH572EVT/EXAM/IAP/APP/Startup/startup_CH572.S:171-175`
- CH583：`CH583EVT/EXAM/IAP/APP/Startup/startup_CH583.S:176-180`
- CH585：`CH585EVT/EXAM/IAP/APP/Startup/startup_CH585.S:216-220`
- CH592：`CH592EVT/EXAM/IAP/APP/Startup/startup_CH592.S:176-180`
- CH595：`CH595EVT/EXAM/IAP/APP/Startup/startup_CH595.S:189-193`

CH573 使用较早的指令槽向量模式：

- 向量槽内容为 `j Handler`：`CH573EVT/EVT/EXAM/IAP/APP/Startup/startup_CH573.S:19-59`
- 设置 `mtvec | 1`：同文件 `156-160`

无论是哪种模式，APP startup 都会覆盖 IAP 的 `mtvec`，后续中断才进入 APP 向量表。

### 5.4 CH585 startup 特殊性

CH585 在常规初始化前先复制 `.highcode_init` 到 RAM，再 `jr` 到 RAM 中执行早期初始化：

- 设置 `sp`：`CH585EVT/EXAM/IAP/APP/Startup/startup_CH585.S:139-145`
- 复制 `.highcode_init`：`147-158`
- 跳 RAM：`161-162`
- 后续 highcode/data/bss：`169-202`
- 设置 `mtvec`：`216-220`

因此 CH585 虽然也能直接跳 `0x1000`，但必须使用其专用 startup，不能用其他 CH58x 的简化入口替代。

### 5.5 Flash API 分组

| 型号 | Flash 更新 API | Flag 存储 |
|---|---|---|
| CH572 | `FLASH_ROM_ERASE/WRITE/VERIFY` | Code Flash `0x3B000` |
| CH573/583/585/592 | `FLASH_ROM_ERASE/WRITE/VERIFY` | EEPROM/DataFlash |
| CH595 | `FLASH_ERASE/WRITE/READ` | Code Flash 末端保留区 |

CH573/583/585/592 的 ROM API 通常要求缓冲区在 RAM、4 字节对齐，并推荐按 256 字节页处理。CH595 已改用新的显式 Flash API，verify 为读回后 `memcmp`。

### 5.6 USB 协议

常规 USB IAP 命令：

```text
0x80 PROM
0x81 ERASE
0x82 VERIFY
0x83 END
```

典型定义见 `CH583EVT/EXAM/IAP/USB_IAP/src/iap.h:40-47`。

特征：

- USB 数据包最大 64 字节。
- program 数据积累到约 256 字节后写 Flash。
- erase 要求地址等于 APP 起址。
- verify 是按块校验，不是全镜像 CRC。
- end 后可直接 `jumpApp()`。
- 包内没有额外 CRC 字段，主要依赖 USB 链路 CRC 和 verify 命令。

### 5.7 UART 协议

CH572/573/583/585/592/595 UART IAP 基本使用：

```text
SOP: AA 55
CMD
LEN
ADDR/DATA
16-bit additive checksum, little-endian
EOP: 55 AA
```

这不是 CRC，只是从命令开始逐字节累加的 16 位和。UART 状态机通常要求先成功执行 `ERASE(APP_START)`，之后才允许 program。

### 5.8 链接边界问题

CH572 的更新边界与 APP linker 不一致：

- IAP 定义 APP 结束地址/flag 附近为 `0x3B000`。
- APP linker 为 `ORIGIN=0x1000, LENGTH=235K`，见 `CH572EVT/EXAM/IAP/APP/Ld/Link.ld:5`。
- `0x1000 + 235 KiB = 0x3BC00`，理论上可链接进入 `0x3B000` 起的 flag 区域。

产品化前必须收紧 APP linker 长度。

## 6. 普通 CH32 IAP 深入分析

### 6.1 地址别名

普通 CH32 示例存在两套地址：

- Flash 编程使用 `FLASH_Base=0x08005000`。
- APP linker 和 CPU 执行入口使用 `0x00005000`。

例如 CH32M030：

- `FLASH_Base=0x08005000`：`CH32M030EVT/EXAM/IAP/UART_USB_IAP/CH32M030_IAP/User/iap.h:20`
- `User_APP_Addr_offset=0x5000`：`.../User/iap.c:18-22`
- APP linker `ORIGIN=0x00005000`：`.../CH32M030_APP/Ld/Link.ld:1`

统一库必须分别记录 `program_address` 和 `execute_address`，不能只存一个 `app_base`。

### 6.2 跳转实现

普通 CH32 不是从 C 代码直接调用 APP 函数指针，而是：

1. 启用 PFIC Software IRQ。
2. 设置 Software IRQ pending。
3. 进入 `SW_Handler`。
4. handler 中执行固定地址 `jr`。

CH32M030 的实际跳转桩：

```c
void SW_Handler(void) {
    __asm("li  a6, 0x5000");
    __asm("jr  a6");
    while(1);
}
```

见 `CH32M030EVT/EXAM/IAP/UART_USB_IAP/CH32M030_IAP/User/iap.c:445-459`。

其他型号对应位置：

| EVT | 跳转源码 |
|---|---|
| CH32V103 | `CH32V103EVT/EXAM/IAP/UART_USB_IAP/CH32V103_IAP/User/ch32v10x_it.c:54-59` |
| CH32V20x | `CH32V20xEVT/EXAM/IAP/USB_UART/CH32V20x_IAP/User/ch32v20x_it.c:52-56` |
| CH32V307 | `CH32V307EVT/EXAM/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_it.c:51-53` |
| CH32V407 | `CH32V407EVT/EXAM/IAP/USB_UART/CH32V407_IAP/User/iap.c:452-454` |
| CH32X035 | `CH32X035EVT/EXAM/IAP/USB_UART/CH32X035_IAP/User/iap.c:454-456` |
| CH32X315 | `CH32X315EVT/EXAM/IAP/USB_UART/CH32X315_IAP/User/iap.c:455-457` |
| CH32L103 | `CH32L103EVT/EXAM/IAP/USB_UART/CH32L103_IAP/User/iap.c:450-452` |
| CH32M030 | `CH32M030EVT/EXAM/IAP/UART_USB_IAP/CH32M030_IAP/User/iap.c:454-456` |

### 6.3 为什么 SWI 中跳转仍能工作

这个模式依赖 APP startup 完成上下文重建：

- 重设 RISC-V `sp`。
- 重新初始化 `.data/.bss`。
- 重写 `mstatus`。
- 把 APP `_vector_base` 写入 `mtvec`。
- 设置 `mepc=main` 后 `mret`。

所以 APP startup 最后的 `mret` 会建立新的 machine-mode 返回状态。不能把 SWI handler 中的 `jr` 简化为适用于任意 RISC-V bootloader 的通用范式。

更稳健的通用实现应优先从普通线程上下文完成清理后跳 `_start`，而不是依赖中断 handler 上下文。

### 6.4 APP 有效性检查

普通 CH32 示例的启动校验较弱：

| 分组 | 检查方式 |
|---|---|
| V20x/V307/V407/X315/H417 | APP 首字不等于 `0xe339e339`，再检查更新 flag |
| X035/L103/M030 | APP 首字不等于 `0xFFFFFFFF`，再检查更新 flag |
| V103 | 主要依赖 PA0，没有可靠 APP 镜像有效性校验 |

`0xe339e339` 是对 RISC-V 入口指令内容的启发式判断，不是 RAM 栈地址。它也不能证明镜像完整。

这些检查均不包含：

- 镜像长度。
- 整镜像 CRC/哈希。
- 硬件型号和版本。
- 签名。
- 防回滚计数。

协议中的逐块 verify 也没有和持久化“完整镜像有效”状态强绑定。

## 7. CH569 特殊实现

CH569 与其他两组都不同。

### 7.1 地址和跳转

- APP 起址：`0x4000`。
- APP linker：`CH569EVT/EXAM/IAP/APP/.ld:10`。
- IAP 编程地址：`CH569EVT/EXAM/IAP/IAP/User/iap.c:21-26`。
- `IAP_2_APP()` 触发 PFIC SWI：`CH569EVT/EXAM/IAP/IAP/User/Main.c:27-34`。
- `SW_Handler` 执行 `li a6,0x4000; jr a6`：同文件 `117-129`。

APP startup 位于 `CH569EVT/EXAM/SRC/Startup/startup_CH56x.S`：

- `_start -> j handle_reset`：`13-17`
- 设置 `sp`：`121-127`
- 初始化 data/bss/dmadata：`129-157`
- 设置 `mtvec | 1`：`159-165`
- `mepc=main; mret`：`167-169`

### 7.2 更新和复位流程

CH569 使用 `FLASH_ROMA_ERASE/WRITE/VERIFY`。`END` 后不是立即从协议处理函数跳 APP，而是：

1. 设置结束标记。
2. 写 `R8_GLOB_RESET_KEEP=0xaa`。
3. 启动 watchdog 复位。
4. 复位后 IAP 检查 keep 值。
5. 触发 SWI，再跳 `0x4000`。

这比其他型号多了一层 reset-keep 握手。

### 7.3 风险

- IAP linker 声明整个 448 KiB Flash，没有把 bootloader 强制限制在 `0x4000` 以下。
- APP 启动前没有有效镜像检查。
- `ERASE` 命令本身没有执行典型的整区擦除，实际擦除与 program 流程耦合。
- UART 协议使用 `57 AB` 帧头和 8 位加和，与其他型号不兼容。

## 8. CH32H417 双核特殊流程

CH32H417 不能作为普通单核 IAP 处理。

### 8.1 地址布局

| 镜像 | 起址 | 证据 |
|---|---:|---|
| V3F IAP | `0x00000000`，24 KiB | `CH32H417_IAP/Common/Ld/V3F/Link_v3f.ld:12` |
| V3F APP | `0x00006000` | `CH32H417_APP/Common/Ld/V3F/Link_v3f.ld:12` |
| V5F APP | `0x00010000` | `CH32H417_APP/Common/Ld/V5F/Link_v5f.ld:12` |

完整路径前缀为：

```text
CH32H47TEVT/EXAM/IAP/USB_UART/
```

### 8.2 启动顺序

1. V3F IAP 的 SWI handler `jr 0x6000`。
2. V3F APP 执行自己的 startup，设置 V3F 的 `sp/mtvec`。
3. V3F APP `main` 调用 `NVIC_WakeUp_V5F(Core_V5F_StartAddr)`。
4. `Core_V5F_StartAddr=0x10000`，见 `CH32H417_APP/Common/Debug/debug.h:57-58`。
5. V5F 从自己的 startup 启动，并建立独立栈和向量入口。

V3F 唤醒 V5F 的调用见 `CH32H417_APP/V3F/User/main.c:40-46`。

因此 H417 的镜像元数据必须按核记录，不能只定义一个 APP base。

## 9. 跳转前状态清理

### 9.1 示例现状

不同工程清理程度差异很大：

- CH32V307 会关闭 USBHS/USBFS、IRQ、GPIO、USART 和相关时钟，处理较完整。
- CH32V20x 也会关闭 USB、GPIO、USART 和部分时钟。
- CH32H417 会处理外设并调用 `RCC_DeInit()`。
- X035/M030 等只清理部分 USB、GPIO 和时钟。
- CH569 基本只触发 SWI，没有完整停止 USBHS、UART、DMA 和所有 pending IRQ。
- CH57/58/59 USB IAP 通常会断开 USB 或关闭相关中断，但 UART 路径的全局清理较弱。

### 9.2 产品代码建议

直接跳 APP 前至少应：

1. 禁止全局中断。
2. 停止 SysTick/系统定时器。
3. 停止所有 DMA。
4. 关闭更新传输外设及其中断。
5. 清 PFIC enable/pending 状态，或只保留架构明确需要的状态。
6. 处理 watchdog，避免在 APP startup 尚未接管前复位。
7. 复位时钟到 APP startup 能正确识别的状态，或明确规定 APP startup 必须全量重配。
8. 执行必要的 `fence.i`，尤其在刚写入可执行 Flash 后。
9. 跳到 APP `_start`，不能跳 `main`。

当前示例能够运行，不代表它们已经实现了完整的无状态切换。

## 10. 镜像完整性与安全性

### 10.1 当前示例提供的保护

- UART 包有简单 additive checksum。
- USB 依赖链路 CRC，并提供按块 verify。
- 多数示例检查升级 flag 或按键。
- 部分 CH32 检查 APP 首字不是擦除态或特定 opcode。
- Flash 写入后可通过 verify/readback 检查局部数据。

### 10.2 当前示例缺失的保护

- 固定镜像头。
- 镜像总长度和 slot 边界强校验。
- 全镜像 CRC32 或哈希。
- 芯片型号、板卡型号和镜像角色。
- 版本和防回滚计数。
- 数字签名和公钥验证。
- 原子提交状态。
- A/B 镜像和失败回滚。
- 启动尝试计数与健康确认。

### 10.3 推荐镜像头

```c
typedef struct {
    uint32_t magic;
    uint32_t header_version;
    uint32_t target_id;
    uint32_t image_version;
    uint32_t image_size;
    uint32_t entry_address;
    uint32_t crc32;
    uint32_t flags;
} image_header_t;
```

对当前 RISC-V 方案，`entry_address` 应指向 APP `_start`，而不是 Cortex Reset_Handler。启动前至少验证：

- magic 和 header version。
- target ID。
- image size 不越界。
- entry 位于当前 slot 且满足指令对齐。
- 整镜像 CRC/hash。
- 提交状态完整。

## 11. 统一 HAL/Bootloader 设计

### 11.1 不要只保存 `app_base`

建议目标元数据至少包含：

```rust
struct BootTarget {
    arch: Architecture,
    program_base: usize,
    execute_base: usize,
    slot_end: usize,
    entry_kind: EntryKind,
    vector_kind: VectorKind,
    flash: FlashLayout,
}
```

其中：

```rust
enum EntryKind {
    RiscvStartup,
    CortexVectorTable,
    SecondaryCoreWakeup,
}

enum VectorKind {
    WchMtvecJumpTable,
    WchMtvecAbsoluteTable,
    CortexVtor,
    CortexRemapOrProxy,
}
```

### 11.2 跳转实现按架构分离

RISC-V 当前 EVT：

```text
validate image
quiesce hardware
disable/clear interrupts
fence.i
jump execute_base (_start)
APP startup sets sp and mtvec
```

Cortex-M3/M4：

```text
validate image
read MSP and Reset_Handler
quiesce hardware
disable/clear interrupts
set VTOR
set MSP
jump Reset_Handler
```

Cortex-M0：

```text
validate image
read MSP and Reset_Handler
quiesce hardware
disable/clear interrupts
configure chip-specific vector remap/proxy
set MSP
jump Reset_Handler
```

H417 V5F：

```text
validate per-core image
V3F starts first
V3F wakes V5F at configured V5F startup address
```

## 12. 已发现的源码风险

| 严重度 | 问题 | 影响 |
|---|---|---|
| 高 | 多数示例没有整镜像 CRC/hash 或签名 | 损坏或非目标固件可能被执行 |
| 高 | CH572 APP linker 可进入 flag 保留区 | APP 与升级 flag 可能互相覆盖 |
| 高 | CH569 IAP linker 未限制在 APP `0x4000` 之前 | bootloader 增长后可能覆盖 APP |
| 高 | CH569 启动前不检查 APP 有效性 | 空 Flash/损坏入口可直接执行 |
| 中 | 普通 CH32 只检查首字/opcode 和 flag | 无法证明完整镜像有效 |
| 中 | 多数跳转路径未完整清所有 IRQ、DMA、外设 | APP startup 前可能发生旧中断或总线活动 |
| 中 | SWI handler 内 `jr` 依赖 WCH startup 的 `mret` 流程 | 不适合作为通用 RISC-V 跳转模板直接复制 |
| 中 | USB/UART verify 未与持久化完整镜像状态强绑定 | 断电或漏发 verify 后仍可能启动半成品 |
| 低 | `jumpApp` 的函数指针转换写法不规范 | 可读性差，易被误解为解引用 APP 首字 |

## 13. 对问题的最终回答

### 是否都可以类似 Cortex-M4 直接跳转？

**可以直接把执行流交给 APP，但不能使用 Cortex-M4 的 MSP+Reset_Handler 模板。**

当前 EVT 中：

- CH572/573/583/585/592/595 直接调用 APP 固定地址的 `_start`。
- 普通 CH32、CH569、H417 V3F 通过 PFIC 软件中断 handler 中的 `jr` 跳 APP `_start`。
- H417 V5F 由 V3F 使用专用 wakeup API 启动。

这些 APP 都依赖自己的 startup 设置 `sp`、初始化 RAM 段并重写 `mtvec`。

### 是否需要像 Cortex-M0 那样特殊处理？

**不需要，因为这些工程不是 Cortex-M0。**

当前代码中不存在 Cortex-M0 的 VTOR 缺失问题。需要处理的是 WCH RISC-V 的 `mtvec/PFIC`、APP `_start`、Flash 执行别名，以及 H417 双核启动顺序。

如果未来仓库加入真正 Cortex-M0 IAP，则仍可以跳 APP，但需要额外解决其芯片特定的向量 remap/代理机制。

### 推荐结论

统一库不要提供一个含糊的 `jump_to_app(base)`。建议提供：

```text
jump_to_riscv_startup(execute_base)
jump_to_cortex_vector_table(vector_base)
wakeup_secondary_core(core, execute_base)
```

并将镜像验证、硬件清理、Flash 地址映射和跳转原语分开实现。

## 14. 验证状态

- 已扫描全部 16 个实际 EVT 的 `EXAM/IAP` 源码目录。
- 已核对 IAP/APP linker、startup、`iap.c/h`、main、软件中断 handler 和关键工程配置。
- 已确认当前范围无 Cortex-M IAP 工程。
- 已扩展核对 USB Host IAP、ETH IAP 和 BLE OTA/IAP 工程；所列当前目标均为 RISC-V。
- 已确认部分 Host IAP 公共源码保留 `DEF_CORE_CM3` 的 MSP/`APP+4` 分支，但该分支不是当前已选构建架构。
- PDF/Windows 主机工具的私有实现未作为跳转结论依据。
- 本报告描述的是源码实现，不代表所有路径都已在硬件上验证。
