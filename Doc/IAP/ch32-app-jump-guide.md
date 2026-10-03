# CH32 Bootloader 跳转 APP 开发指导书

## 1. 文档目的

本文说明 CH32 MCU 的 Bootloader 如何安全跳转到位于片内 Flash 非零偏移处的 APP，覆盖：

- Bootloader/APP 的 Flash 分区；
- APP 链接地址修改；
- CH32 RISC-V（QingKe 内核）的推荐跳转方式；
- QingKe 专用 CSR、机器模式、硬件压栈和中断嵌套配置；
- FreeRTOS/RT-Thread 的中断栈和上下文切换约束；
- 跳转前的镜像检查、外设清理和中断交接；
- APP 返回 Bootloader 的方式；
- Cortex-M 型号的差异；
- 调试步骤和常见故障。

本文以 WCH EVT 工程和 GCC/MounRiver 工程结构为基准。寄存器数量、Flash/RAM 容量、擦除页大小、启动文件名和中断名称必须以具体芯片的 EVT、数据手册和参考手册为准。

> 重要：CH32V、CH32X、CH32L、CH32M、部分 CH32H 使用 QingKe RISC-V 内核，不能直接照搬 Cortex-M 的“读取 MSP 和 Reset_Handler”跳转代码。
>
> 同样重要：QingKe 也不能只按标准 RISC-V 处理。WCH 启动文件会设置专用 CSR、中断入口模式、硬件压栈和机器模式，RTOS port 又会接管软件中断、`mscratch`、`mepc` 和任务上下文。必须使用目标芯片与目标 RTOS 配套的 startup、port 和中断声明。

## 2. 跳转原理

### 2.1 CH32 RISC-V APP 映像

CH32 RISC-V APP 的链接起始位置通常是启动代码 `_start`，随后是中断向量段和程序正文。它不像 Cortex-M 映像那样要求 APP 起始位置的前两个 32 位字依次保存 MSP 和复位向量。

因此，RISC-V Bootloader 应跳到 APP 的链接起始地址，让 APP 自己的启动代码完成以下工作：

1. 初始化全局指针和栈指针；
2. 配置 `mtvec`；
3. 复制 `.data`；
4. 清零 `.bss`；
5. 初始化 C/C++ 运行时；
6. 调用 `SystemInit()` 和 `main()`。

不要直接跳到 APP 的 `main()`，否则 APP 的栈、全局变量、中断入口和运行时环境可能仍属于 Bootloader。

### 2.2 QingKe 启动状态不是标准 RISC-V 默认状态

仓库中的 CH32 QingKe 启动文件除初始化 `sp`、`.data` 和 `.bss` 外，还会配置以下核心状态：

| 状态 | 作用 | 对 APP/RTOS 的影响 |
|---|---|---|
| `mstatus` | MIE/MPIE、MPP、FPU 状态和全局中断状态 | 决定 `mret` 后的特权级、中断状态及 FPU 是否可用 |
| `mtvec` | 中断向量基址和 WCH 入口模式 | 低位可能使用 WCH 扩展模式，不应按标准 direct/vectored 两种模式简单解释 |
| `mepc` | `mret` 的返回目标 | startup 通常写入 `main` 后执行 `mret` |
| `mscratch` | RTOS 常用作任务栈与中断栈交换寄存器 | 调度器启动前后含义不同，热跳转不能继承旧值 |
| CSR `0x804` | WCH 中断系统控制，涉及硬件压栈、嵌套及溢出处理 | 必须与 ISR 编译属性和 RTOS port 一致 |
| CSR `0xBC0` | QingKe 内核流水线、分支预测等配置 | 值随内核变化，只能由匹配的 startup 设置 |

不同工程即使是同一芯片，startup 配置也可能因裸机或 RTOS 而不同。例如仓库 CH32V307 D8：

- 裸机 startup 写 `CSR 0x804 = 0x0B`、`mstatus = 0x6088`；
- FreeRTOS startup 写 `CSR 0x804 = 0x1F`、`mstatus = 0x7800`；
- 两者都将 `_vector_base | 3` 写入 `mtvec`，但其硬件压栈、嵌套和 FPU/中断初始状态并不相同。

CH32X035 也存在区别：仓库裸机 startup 使用 `CSR 0x804 = 0x03`、`mstatus = 0x88`，FreeRTOS startup 使用 `CSR 0x804 = 0x02`、`mstatus = 0x1800`。

这些值只是当前 EVT 的事实示例，不是跨 QingKe 内核的常量。Bootloader 不应提前替 APP 猜测并写入它们；正确做法是跳转到 APP 自己的 `_start`，由 APP 配套 startup 重建状态。

### 2.3 CH32 地址别名

部分 CH32 工程存在两种指向片内 Flash 的地址：

- Flash 擦写或读取地址，例如 `0x08005000`；
- CPU 启动和执行别名地址，例如 `0x00005000`。

仓库中的 CH32V20x USB/UART IAP 示例即使用 `0x08005000` 编程 APP，并在软件中断处理程序中跳到 `0x00005000`。是否存在该别名以及具体范围必须以目标芯片的存储器映射和官方 EVT 为准。

建议始终使用两个含义明确的宏，禁止用一个 `APP_ADDR` 同时承担擦写和执行用途：

```c
#define APP_FLASH_ADDR    0x08005000UL /* Flash 擦写/校验地址 */
#define APP_EXEC_ADDR     0x00005000UL /* CPU 跳转执行地址 */
#define APP_OFFSET        0x00005000UL
```

如果目标芯片的 Flash 只映射在 `0x00000000`，则 `APP_FLASH_ADDR` 与 `APP_EXEC_ADDR` 可以相同。

## 3. Flash 分区设计

以下仅为示例，不是所有 CH32 的固定布局：

| 区域 | 示例范围 | 用途 |
|---|---:|---|
| Bootloader | `0x00000000` 至 `0x00004FFF` | 启动判断、通信升级、Flash 写入和镜像校验 |
| APP | `0x00005000` 至 APP 分区末尾 | APP 启动代码、向量表、代码和只读数据 |
| Metadata | 独立 Flash 页 | APP 长度、版本、CRC/哈希、有效标志和升级状态 |
| 参数区 | 独立 Flash 页 | 用户配置或设备参数 |

分区必须满足：

- APP 起始地址符合启动代码和向量表要求，建议至少 4 字节对齐，并采用官方示例的对齐值；
- Bootloader 结束位置不得超过 APP 起始地址；
- APP 最大长度不得覆盖 Metadata、参数区、备份映像或芯片保留区域；
- Metadata 和状态标志各自占用可安全擦除的页，不能与 APP 正文共页；
- 擦除页大小和最小编程单位来自目标芯片参考手册，不能跨系列照搬；
- 链接脚本中的 `LENGTH` 应是 APP 实际可用容量，而不是芯片 Flash 总容量。

推荐在 Bootloader 和 APP 共用的头文件中集中定义分区，或者由构建系统同时生成 C 头文件和 linker 参数，避免两个工程的地址手工漂移。

## 4. APP 工程配置

### 4.1 修改链接脚本

以预留 20 KB Bootloader、APP 从偏移 `0x5000` 开始为例：

```ld
ENTRY(_start)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00005000, LENGTH = 44K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}
```

实际工程需保留原 EVT 链接脚本中的 `.init`、`.vector`、`.text`、`.data`、`.bss`、栈、堆及芯片专用段，仅修改经过计算的 `FLASH ORIGIN` 和 `LENGTH`。不要从不同系列复制整份链接脚本。

检查生成的 `.map` 文件：

- `_start` 等于 `APP_EXEC_ADDR`；
- `.init` 和 `.vector` 都位于 APP 分区内；
- `_eusrstack` 或等效栈顶位于有效 RAM；
- 映像末地址没有超过 APP 分区；
- 没有异常段仍链接到地址 `0x00000000`。

### 4.2 生成升级文件

Bootloader 通常接收纯二进制 `.bin`。应从已经按 APP 地址链接的 ELF 生成：

```sh
riscv-none-embed-objcopy -O binary app.elf app.bin
```

工具链前缀可能是 `riscv-wch-elf-` 或工程自带前缀，以 MounRiver 构建日志为准。

不要将 Bootloader 和 APP 的合并 HEX 当作 APP 升级包。写入时 `app.bin` 的第 0 字节应写到 `APP_FLASH_ADDR`。

### 4.3 APP 独立运行要求

APP 必须能在 Bootloader 已经运行过的硬件状态下启动：

- APP 启动代码必须重新建立 `sp`、`gp`、`mtvec`、`.data` 和 `.bss`；
- APP 的 `SystemInit()` 应重新配置自己需要的时钟；
- APP 不应假定所有外设都处于上电复位值，除非 Bootloader 在跳转前执行了完整系统复位；
- APP 应清除自己所用外设的状态标志后再打开中断；
- APP 使用 RTOS 时必须从 APP 的 `_start` 进入，不能直接调用调度器或 `main()`。

### 4.4 startup、工具链、RTOS port 必须成套

以下文件构成一个不可随意拆分的启动与上下文契约：

1. 目标内核对应的 startup，例如 D6、D8、D8W、V3F、V5F；
2. 对应链接脚本和中断向量段；
3. WCH GCC/MounRiver 工具链及其 `interrupt("WCH-Interrupt-fast")` 实现；
4. RTOS 的 `portASM.S`、`port.c`、`portmacro.h`；
5. `freertos_risc_v_chip_specific_extensions.h` 或 RTOS 等效芯片扩展文件；
6. FPU ABI、`ARCH_FPU` 等构建选项；
7. 应用 ISR 的函数属性和中断栈切换宏。

禁止以下组合：

- 从裸机 EVT 复制 startup，却使用另一个 EVT 的 FreeRTOS port；
- 只根据 RV32I/RV32IMAC 名称换用通用 RISC-V port；
- CH32V20x D6/D8/D8W 之间混用 startup；
- CH32H V3F/V5F 之间混用 startup、链接脚本或 FPU 上下文；
- 启用硬件压栈，却将 fast ISR 改成普通 RISC-V ISR，或反向混用；
- APP 使用硬浮点 ABI，但 RTOS 上下文未保存 FPU 寄存器。

## 5. 推荐方案：RISC-V 软件中断跳转

本章的软件中断方案只推荐用于尚未启动 RTOS 调度器的 Bootloader。Bootloader 一旦启动 FreeRTOS、RT-Thread、TencentOS 或 LiteOS，必须先阅读第 6 章。

### 5.1 为什么使用软件中断

WCH 的 CH32 RISC-V IAP 示例通常采用以下流程：

1. Bootloader 停止升级外设；
2. 使能 PFIC 的 `Software_IRQn`；
3. 将软件中断置为 pending；
4. CPU 进入 Bootloader 的 `SW_Handler`；
5. `SW_Handler` 以裸跳转方式进入 APP `_start`。

这种方式符合 WCH EVT 的 QingKe/PFIC 路径，并让编译器按 WCH 中断约定进入处理程序。不要把 `SW_Handler` 写成普通 C 函数后直接调用。

### 5.2 Bootloader 示例代码

下面代码是可移植模板。`boot_deinit_peripherals()` 和 `boot_app_is_valid()` 必须按产品实现；中断寄存器组数量也必须按具体芯片核对。

```c
#include "debug.h"

#define APP_FLASH_ADDR    0x08005000UL
#define APP_EXEC_ADDR     0x00005000UL
#define APP_MAX_SIZE      (44UL * 1024UL)

typedef struct {
    uint32_t magic;
    uint32_t image_size;
    uint32_t image_version;
    uint32_t crc32;
} app_metadata_t;

static int boot_app_is_valid(void)
{
    const app_metadata_t *meta = (const app_metadata_t *)APP_META_ADDR;

    if (meta->magic != APP_META_MAGIC) {
        return 0;
    }
    if ((meta->image_size == 0U) || (meta->image_size > APP_MAX_SIZE)) {
        return 0;
    }
    if ((APP_META_ADDR <= APP_FLASH_ADDR) ||
        (meta->image_size > (APP_META_ADDR - APP_FLASH_ADDR))) {
        return 0;
    }
    if (crc32((const void *)APP_FLASH_ADDR, meta->image_size) != meta->crc32) {
        return 0;
    }

    return 1;
}

static void boot_deinit_peripherals(void)
{
    /* 顺序：停止数据流 -> 关外设中断 -> 清状态 -> 关外设 -> 关时钟。 */
    boot_transport_stop();
    boot_transport_irq_disable();
    boot_transport_flags_clear();
    boot_transport_deinit();

    /* 停止 Bootloader 使用的定时器、DMA 和 SysTick，并清 pending 状态。 */
    boot_timebase_stop();
    boot_dma_stop();
    boot_irq_pending_clear();
}

__attribute__((noreturn)) void boot_jump_to_app(void)
{
    if (!boot_app_is_valid()) {
        boot_error_stay_in_loader();
    }

    boot_deinit_peripherals();

    /* 软件中断需要全局中断可响应，不能在这里调用 __disable_irq()。 */
    NVIC_EnableIRQ(Software_IRQn);
    __enable_irq();
    NVIC_SetPendingIRQ(Software_IRQn);

    for (;;) {
        __asm volatile("nop");
    }
}
```

上例中的 `APP_META_ADDR`、CRC 和外设清理函数是产品接口，不是 WCH 标准库 API。Metadata 的有效标志、字段布局和 CRC 覆盖规则应由 Bootloader 与升级工具共同定义。

### 5.3 软件中断处理程序

若 APP 地址是编译期常量，可参考 EVT 的最小写法：

```c
void SW_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void SW_Handler(void)
{
    __asm volatile(
        "li a6, 0x5000\n"
        "jr a6\n"
    );
    __builtin_unreachable();
}
```

该常量是 `APP_EXEC_ADDR`，不是 `APP_FLASH_ADDR`。更换 APP 分区时必须同步修改。为避免 C 编译器在跳转前后生成不可控的函数尾声，生产工程可将跳转处理程序放到独立汇编文件：

```asm
    .section .text.SW_Handler, "ax", @progbits
    .align 2
    .global SW_Handler
    .type SW_Handler, @function
SW_Handler:
    li a6, 0x5000
    jr a6
```

如果需要运行时选择 A/B 槽，不要假定任意 C 全局变量在 WCH 快速中断入口中都能安全访问。优先为两个槽提供两个固定汇编入口，或按目标工具链 ABI 编写并反汇编确认从固定 RAM 地址取目标地址的汇编实现。

### 5.4 跳转时的中断规则

触发软件中断前：

- 关闭 Bootloader 所使用外设的中断源；
- 停止 DMA，等待或强制终止当前传输；
- 清除对应外设状态标志；
- 清除 PFIC 中除软件中断外的 pending 位；
- 保持机器全局中断使能，使 `Software_IRQn` 能够进入；
- 不要在 ISR 内发起跳转，先退出当前 ISR，再由主循环执行跳转；
- 不要在跳转前调用 `__disable_irq()` 后仍期待软件中断触发。

不同 CH32 的 PFIC 中断寄存器组数量和有效 IRQ 范围不同。应优先逐个调用 `NVIC_DisableIRQ()`、`NVIC_ClearPendingIRQ()` 清理由 Bootloader 实际使用的 IRQ，或者按照目标芯片头文件中的 PFIC 定义实现统一清理，不能硬编码固定组数。

## 6. RTOS、硬件压栈与上下文切换约束

### 6.1 为什么硬件压栈不能直接当作 RTOS 任务上下文

QingKe 的硬件压栈用于加速中断入口和返回，但 RTOS 上下文切换需要保存一个可长期驻留、可由调度器任意恢复的完整任务上下文，包括：

- `mepc` 和 `mstatus`；
- ABI 要求的通用寄存器；
- 任务栈指针；
- 必要时的 FPU 寄存器和 FPU 状态；
- RTOS 自己的临界区与中断栈状态。

硬件压栈深度有限，并且属于当前中断嵌套链，不等同于 TCB 中保存的任务栈帧。仓库 CH32V307 中断嵌套示例明确说明：8 级嵌套时硬件只保存较低的 3 级，更高层需要软件压栈；若只用硬件压栈，应限制嵌套并配置溢出处理；若不用硬件压栈，则需清除 `CSR 0x804` 对应使能位并移除 `WCH-Interrupt-fast` 属性。

因此不能因为内核具有 HPE/硬件压栈，就删除 RTOS port 的软件上下文保存；也不能仅修改 `CSR 0x804` 就假定现有 RTOS port 仍正确。

### 6.2 FreeRTOS 的实际依赖

仓库中的 WCH FreeRTOS port 具有以下关键行为：

- `Software_IRQn` 和 `SW_Handler` 是 `portYIELD()` 的任务切换入口；
- `SW_Handler` 保存通用寄存器、`mstatus` 和 `mepc`，更新当前 TCB 后用 `mret` 恢复任务；
- `xPortStartFirstTask()` 将中断栈地址写入 `mscratch`；
- C ISR 通过 `GET_INT_SP()`/`FREE_INT_SP()` 执行 `csrrw sp, mscratch, sp`，在任务栈和中断栈之间交换；
- 某些 port 在软件中断入口设置 `CSR 0x804` 的 HPE 相关位；该行为在不同内核 port 中并不完全相同；
- `xPortStartScheduler()` 检查 `mtvec` 低两位是否为 `0b11`，即 WCH 的表模式；
- FPU 上下文是否保存由芯片扩展头和构建选项共同决定。

这意味着：运行 FreeRTOS 的 Bootloader 不能把 `SW_Handler` 替换为 APP 跳转处理程序，也不能通过 `NVIC_SetPendingIRQ(Software_IRQn)` 跳 APP。这样做会触发任务切换，或者破坏当前 TCB/任务栈。

### 6.3 RT-Thread 的实际依赖

仓库 WCH RT-Thread port 同样将 `SW_Handler` 用作线程切换入口，并且：

- 完整保存线程通用寄存器，必要时保存 FPU 寄存器；
- 保存/恢复 `mepc` 和机器模式相关的 `mstatus`；
- 使用 `mscratch` 在任务栈与独立中断栈之间交换；
- 首次切换时初始化 `mscratch`；
- 中断退出时根据 `rt_thread_switch_interrupt_flag` 选择下一线程上下文。

因此 RT-Thread Bootloader 也不能复用软件中断作为 APP 跳转通道。

### 6.4 RTOS Bootloader 的推荐跳转流程

推荐采用“RTOS 内请求复位，复位后最小路径跳转”：

1. 升级任务验证 APP，并停止创建新的 I/O 请求；
2. 通知其他任务进入停机状态，停止 DMA、USB、Ethernet、Timer 和回调；
3. 将一次性的 `BOOT_REQUEST_APP` 写入备份寄存器或 `.noinit` RAM；
4. 必要时等待日志发送完成并提交 Metadata；
5. 调用 `NVIC_SystemReset()`，不要在任务上下文中裸跳；
6. 复位后 Bootloader 在调度器启动之前读取并清除请求；
7. 完成最小镜像校验；
8. 使用 Bootloader 的裸机软件中断入口跳到 APP `_start`。

如果产品必须从运行中的 RTOS 热跳转，需要为目标内核和目标 RTOS 编写专用汇编“调度器退出/核心状态复位”路径，至少处理：

- 当前是否处于任务、ISR 或嵌套 ISR；
- `sp` 是否为任务栈、主栈或中断栈；
- `mscratch` 中保存的是哪一个栈指针；
- PFIC active/pending 状态；
- `mstatus` 的 MIE/MPIE/MPP/FS；
- `mepc`、`mcause` 和 WCH 专用 CSR；
- 硬件压栈层级是否已经完全退出；
- FPU、Cache、多核和 RTOS tick 状态。

这种路径不能做成跨 CH32 系列的通用 C 函数，验证成本通常高于一次系统复位，不作为本文推荐实现。

### 6.5 ISR 声明必须匹配 CSR 0x804

| `CSR 0x804`/port 模式 | ISR 入口 | 要求 |
|---|---|---|
| 启用 WCH 硬件压栈 | `interrupt("WCH-Interrupt-fast")` | 使用匹配工具链，遵循可用硬件压栈深度和嵌套限制 |
| 禁用硬件压栈 | 普通 WCH/RISC-V interrupt 入口或专用汇编入口 | 必须由软件保存完整所需上下文，不能保留 fast 属性 |
| RTOS tick 的 C fast ISR | fast 属性加 port 的 `GET_INT_SP()`/`FREE_INT_SP()` | 中断栈交换必须成对且在 port 规定位置执行 |
| RTOS 软件调度中断 | RTOS 自带 `portASM.S` 的 `SW_Handler` | 禁止由应用或 Bootloader 重定义 |

表中只是设计关系，具体位值和属性字符串仍以目标 EVT 为准。

## 7. 镜像有效性检查

仅检查 APP 首字不是 `0xFFFFFFFF` 不足以证明映像可启动。生产 Bootloader 至少检查：

1. Metadata magic 正确；
2. 镜像长度非零且不超过 APP 分区；
3. 起止地址计算没有整数溢出；
4. CRC32 或哈希覆盖完整镜像且匹配；
5. 升级完成标志已提交；
6. 可选：产品 ID、硬件版本、芯片系列和最低 Bootloader 版本匹配；
7. 可选：版本满足防回滚规则；
8. 有安全启动要求时，验证数字签名，CRC 不能代替真实性校验。

推荐提交顺序：

1. 将 Metadata 标记为“更新中/无效”；
2. 擦除并写入 APP；
3. 回读并校验完整 APP；
4. 写入长度、版本和 CRC/哈希；
5. 最后以一次对齐写操作提交“有效”标志；
6. 复位或执行 APP 跳转。

断电发生在第 5 步之前时，Bootloader 下次启动必须判定 APP 无效并继续等待升级。若升级过程先擦除唯一 APP，则系统不能保证断电恢复；高可靠产品需要 A/B 映像或外部备份区。

## 8. 跳转前硬件交接

### 8.1 建议清理项

| 模块 | 跳转前处理 |
|---|---|
| UART/SPI/I2C | 等待必要数据发送完成，关闭中断和 DMA，清状态并反初始化 |
| USB | 断开设备或停止 Host，关闭端点/通道和中断，再关闭模块时钟 |
| Ethernet | 停止 MAC/DMA，释放描述符，关闭中断 |
| Timer/SysTick | 停止计数，关闭更新中断，清 pending |
| DMA | 禁止通道，确认通道停止，清所有相关标志 |
| GPIO | 只在产品需要时恢复安全状态；避免造成电源、马达或片选毛刺 |
| Flash | 等待 BUSY 清零，锁定 Flash，确认没有挂起的擦写操作 |
| Cache/预取 | 高性能型号按 RM 清理或失效，特别是刚完成 APP 编程时 |
| Watchdog | 见下一节，不能假定可关闭 |

不要无条件对所有 GPIO 执行 `GPIO_DeInit()`。Bootloader 若控制电源保持、外部 Flash 片选、马达使能或通信方向，无条件复位 GPIO 可能导致硬件危险状态或瞬时毛刺。应由板级函数定义交接状态。

### 8.2 时钟策略

有两种可行策略：

- Bootloader 将 RCC 恢复到接近复位状态，APP 完整重新配置；
- Bootloader 保留时钟，APP 的 `SystemInit()` 无条件重配为 APP 所需状态。

推荐第二种情况下也确保 APP 不依赖 Bootloader 的频率。切换系统时钟前必须遵循目标芯片 RM 的时钟源稳定和分频顺序，不能在 USB、Flash 编程或高速总线仍工作时直接关闭其时钟源。

### 8.3 看门狗

多数 MCU 的独立看门狗启动后不能由软件关闭。若 Bootloader 已启动 IWDG：

- 将看门狗超时纳入 Bootloader/APP 接口契约；
- 校验大映像和清理外设期间持续喂狗；
- APP 启动代码和早期初始化必须在超时前喂狗；
- APP 必须按已运行的看门狗重新配置允许的参数，或直接接管喂狗；
- 不要假设跳转会像芯片复位一样停止看门狗。

如果无法保证平滑交接，推荐升级完成后写入启动请求并调用 `NVIC_SystemReset()`，让 Bootloader 在下一次复位后快速验证并跳转 APP。

## 9. APP 返回 Bootloader

推荐使用“请求标志 + 系统复位”，而不是 APP 反向裸跳到 Bootloader：

```c
void app_request_bootloader(void)
{
    boot_request_write(BOOT_REQUEST_MAGIC); /* 备份寄存器或约定 RAM/Flash */
    __asm volatile("fence rw, rw" ::: "memory");
    NVIC_SystemReset();
    for (;;) {
    }
}
```

Bootloader 启动后读取并立即清除一次性请求标志，再进入升级模式。标志位置可选：

- 复位后保留的备份寄存器；
- 双方约定且启动代码不清零的 `.noinit` RAM；
- 独立 DataFlash/配置页。

使用 RAM 标志时必须验证该复位类型下 RAM 是否保留，并在链接脚本中明确 `NOLOAD`；使用 Flash 标志时要考虑擦除寿命和断电一致性。不要使用普通 `.bss` 变量，因为 APP 或 Bootloader 启动时会将其清零。

## 10. 复位后跳转

对于外设复杂、RTOS 已运行、存在多核/Cache 或难以彻底清理硬件状态的产品，必须优先考虑：

1. Bootloader 写入“APP 已验证”状态；
2. 调用 `NVIC_SystemReset()`；
3. 复位后 Bootloader 只完成最小初始化；
4. 检查有效 APP 后立即按软件中断方式跳转。

这比在 USB/Ethernet/RTOS 活跃状态下直接热跳转更可靠。注意系统复位仍可能不复位备份域或始终运行的看门狗，具体行为以 RM 为准。

## 11. Cortex-M 型号附录

若目标明确是 CH32F 等 Cortex-M 型号，APP 映像开头通常是向量表：

- `[APP_ADDR + 0]`：初始 MSP；
- `[APP_ADDR + 4]`：Reset_Handler 地址。

典型流程如下，但 CMSIS 名称和向量重定位能力必须按具体型号确认：

```c
__attribute__((naked, noreturn))
static void start_app_cm(uint32_t app_msp, uint32_t app_reset)
{
    __asm volatile(
        "msr msp, r0\n"
        "cpsie i\n"
        "bx r1\n"
    );
}

__attribute__((noreturn)) void boot_jump_to_app_cm(uint32_t app_addr)
{
    uint32_t app_msp = *(volatile uint32_t *)(app_addr + 0U);
    uint32_t app_reset = *(volatile uint32_t *)(app_addr + 4U);

    if (!address_is_in_sram(app_msp) ||
        !address_is_in_app_flash(app_reset & ~1UL) ||
        ((app_reset & 1UL) == 0U)) {
        boot_error_stay_in_loader();
    }

    boot_deinit_peripherals();
    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    clear_all_nvic_enable_and_pending_bits();

    SCB->VTOR = app_addr;
    __set_CONTROL(0U);
    __DSB();
    __ISB();
    start_app_cm(app_msp, app_reset);

    for (;;) {
    }
}
```

Cortex-M 注意事项：

- 先验证 MSP 在目标芯片的真实 SRAM 范围内；
- Reset_Handler 最低位必须为 1，表示 Thumb 状态；
- 设置 MSP 后必须立即以不再访问当前 C 栈的汇编入口跳转，避免编译器使用 Bootloader 栈帧；
- `SCB->VTOR` 的存在和对齐限制取决于内核；
- 若器件不支持 VTOR，必须使用厂商的向量重映射机制；
- 此模板不适用于 CH32V/CH32X QingKe RISC-V。

## 12. 调试与验收步骤

### 12.1 静态检查

1. 查看 APP `.map`，确认 `_start == APP_EXEC_ADDR`。
2. 查看 `objdump -h app.elf`，确认所有可加载段位于 APP 分区。
3. 查看 `objdump -d app.elf`，确认 `_start` 是 APP 的完整启动入口。
4. 查看 `app.bin` 大小，确认不超过 `APP_MAX_SIZE`。
5. 对升级包独立计算 CRC/哈希，确认与 Metadata 相同。
6. 反汇编 Bootloader，确认 `SW_Handler` 最终跳到 `APP_EXEC_ADDR`。

示例命令：

```sh
riscv-none-embed-objdump -h app.elf
riscv-none-embed-objdump -d app.elf > app.dis
riscv-none-embed-nm -n app.elf
```

### 12.2 单步调试

建议设置断点：

1. `boot_jump_to_app()`；
2. `SW_Handler`；
3. APP `_start`；
4. APP `SystemInit()`；
5. APP `main()`。

观察：

- `Software_IRQn` 是否成功 pending 并进入处理程序；
- `jr` 的目标是否为 APP 链接起始地址；
- APP 启动代码是否重新设置 `sp`、`gp`、`mstatus`、`mtvec` 和 WCH 专用 CSR；
- `.data` 和 `.bss` 是否正确；
- 是否有 Bootloader 遗留中断在 APP 初始化前触发；
- 看门狗是否在 APP 接管前复位系统。

RTOS 工程还应观察：

- `Software_IRQn` 是否进入 RTOS 自带的 `SW_Handler`，而不是 Bootloader 跳转入口；
- 首次任务启动时 `mscratch` 是否指向有效且对齐的中断栈；
- tick ISR 的 `GET_INT_SP()`/`FREE_INT_SP()` 是否成对执行；
- yield 前后的 `mepc`、`mstatus`、任务 `sp` 和当前 TCB 是否一致；
- 启用 FPU 时，浮点上下文是否随任务切换保存和恢复。

### 12.3 必测场景

- APP 区全为 `0xFF`；
- APP 长度为 0、超长或 Metadata 损坏；
- APP 正文任意一位损坏导致 CRC 不匹配；
- 正常上电直接进入 APP；
- 按键/命令强制停留 Bootloader；
- 升级接收中断电；
- 擦除中断电；
- 写入中断电；
- Metadata 最终提交前后分别断电；
- USB/UART/Ethernet 正在传输时请求跳转；
- RTOS 任务中请求 APP，确认走“请求标志 + 系统复位”而不是软件中断裸跳；
- RTOS 启动后反复执行 tick、主动 yield 和 ISR 唤醒高优先级任务；
- 启用 FPU 时由两个任务交替使用不同浮点寄存器数据；
- 看门狗已经运行时跳转；
- APP 请求返回 Bootloader；
- 连续复位和异常 APP 的启动失败回退。

## 13. 常见故障

| 现象 | 常见原因 | 排查方向 |
|---|---|---|
| 设置软件中断后停在死循环 | 跳转前执行了 `__disable_irq()`；软件 IRQ 未使能；当前仍在 ISR | 检查 `mstatus.MIE`、PFIC enable/pending 和调用上下文 |
| 进入 `SW_Handler` 后 HardFault/异常 | 跳到了 `0x080xxxxx` 而芯片执行别名应为 `0x000xxxxx`；地址未对齐；APP 不存在 | 对照存储器映射、ELF `_start` 和 Flash 内容 |
| 到达 APP 后全局变量异常 | 直接跳到 `main()`；启动代码未运行；APP 链接地址错误 | 必须跳 APP `_start`，检查 `.data/.bss` |
| APP 一开中断就异常 | `mtvec` 仍指向 Bootloader；遗留 pending；外设标志未清 | 检查 APP 启动代码和跳转前清理 |
| APP 串口/USB 时钟异常 | APP 假定复位时钟状态；Bootloader 留下 PLL/分频配置 | 让 APP 无条件重建时钟配置 |
| 跳转后立即复位 | IWDG 已启动但 APP 未及时喂狗；APP 异常；电源问题 | 读取复位原因，缩短 APP 接管时间 |
| 单独烧 APP 可运行，Bootloader 跳转不运行 | APP 实际仍链接在 `0x00000000`；烧录工具做了地址偏移但 ELF 未重定位 | 检查 `.map` 和 `_start`，不要只看烧录地址 |
| 升级后偶发旧代码 | Flash 写入未完成；Cache/预取未处理；校验覆盖范围错误 | 等待 BUSY、回读校验，按 RM 处理 Cache |
| 改了 APP 地址但仍跳旧地址 | `SW_Handler` 汇编常量、链接脚本和写入地址未同步 | 分开检查 `APP_FLASH_ADDR`、`APP_EXEC_ADDR` 和 `FLASH ORIGIN` |
| RTOS 中触发软件 IRQ 却没有进入 APP | `Software_IRQn/SW_Handler` 已被调度器占用 | RTOS 任务写请求标志后系统复位，在调度器启动前跳转 |
| RTOS 首次 tick 或 yield 后崩溃 | startup、`CSR 0x804`、ISR fast 属性、`mscratch` 或 port 不匹配 | 使用同一 EVT 的 startup/port/链接脚本，检查中断栈和反汇编 |
| 浮点任务切换后数据异常 | 硬浮点 ABI/FPU 已启用，但 port 未保存 FPU 上下文 | 核对 `mstatus.FS`、`ARCH_FPU`、芯片扩展头和编译 ABI |

## 14. 仓库内参考工程

本仓库可参考以下 WCH EVT 实现，但复制前必须保持芯片系列一致：

- CH32V20x USB/UART IAP 跳转：`CH32V20xEVT/EXAM/IAP/USB_UART/CH32V20x_IAP/User/main.c`；
- CH32V20x 软件中断入口：`CH32V20xEVT/EXAM/IAP/USB_UART/CH32V20x_IAP/User/ch32v20x_it.c`；
- CH32V20x APP 链接脚本：`CH32V20xEVT/EXAM/IAP/USB_UART/CH32V20x_APP/Ld/Link.ld`；
- CH32V103 Host IAP 的 RISC-V/Cortex-M 分支：`CH32V103EVT/EXAM/USB/USBFS/HOST_IAP/HOST_IAP/User/Host_IAP/usb_host_iap.c`；
- CH32X035 USB/UART IAP：`CH32X035EVT/EXAM/IAP/USB_UART/CH32X035_IAP/User/main.c`；
- CH32X315 USB/UART IAP：`CH32X315EVT/EXAM/IAP/USB_UART/CH32X315_IAP/User/main.c`；
- CH32V307 裸机/FreeRTOS startup 差异：`CH32V307EVT/EXAM/SRC/Startup/startup_ch32v30x_D8.S` 与 `CH32V307EVT/EXAM/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8.S`；
- CH32V307 FreeRTOS 上下文切换：`CH32V307EVT/EXAM/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/portASM.S`；
- CH32V20x RT-Thread 上下文与软件中断：`CH32V20xEVT/EXAM/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/context_gcc.S`、`interrupt_gcc.S`；
- CH32V307 硬件压栈/嵌套说明：`CH32V307EVT/EXAM/INT/Interrupt_Nest/User/main.c`；
- 内核和 IAP 偏移汇总：`Doc/Core/wch-core-notes.md`。

官方 EVT 示例主要演示跳转机制。量产产品仍需补齐完整镜像校验、断电安全、看门狗交接、失败回退和安全启动策略。

## 15. 发布检查清单

- [ ] Bootloader 与 APP 使用完全一致的分区定义；
- [ ] APP `_start` 与 `APP_EXEC_ADDR` 相同；
- [ ] APP 写入位置与 `APP_FLASH_ADDR` 相同；
- [ ] APP 分区长度不覆盖 Metadata/参数区；
- [ ] 已确认目标芯片 Flash 擦除页和编程粒度；
- [ ] 已确认目标芯片 Flash 的执行别名；
- [ ] RISC-V 跳转进入 APP `_start`，未读取 MSP/Reset_Handler；
- [ ] startup、链接脚本、工具链、RTOS port 和 ISR 属性来自同一内核配置；
- [ ] 已核对 APP startup 对 `mstatus`、`mtvec`、`CSR 0x804`、`CSR 0xBC0` 的设置；
- [ ] RTOS 已启动时不复用 `Software_IRQn/SW_Handler` 作为 APP 跳转入口；
- [ ] RTOS 的 `mscratch`、中断栈和 FPU 上下文配置已验证；
- [ ] 软件中断触发前保持全局中断可响应；
- [ ] Bootloader 使用的 IRQ、DMA、Timer 和通信外设已安全停止；
- [ ] APP 启动后重建时钟和中断环境；
- [ ] 镜像长度、边界和 CRC/哈希均已验证；
- [ ] 有安全要求时已验证签名和防回滚；
- [ ] IWDG 交接经过最坏启动时间测试；
- [ ] 升级各阶段断电测试通过；
- [ ] APP 启动失败可以回到 Bootloader；
- [ ] 已在真实芯片而非仅仿真环境完成验证。
