# QSPI Demo 学习笔记

> 目标平台: **CH32H417** (双核 MCU: V3F + V5F)
> 样例目录: `CH32H417TEVT/EXAM/QSPI/`
> 硬件: Winbond **W25Qxx** SPI Flash (兼容 QSPI)

---

## 1. 目录结构总览

QSPI 目录下提供 **3 个示例**, 都同时包含 V3F 和 V5F 两个核心的工程:

```
QSPI/
├── QSPI_FLASH/           # 软件轮询方式驱动 W25Qxx (CPU 轮询 FIFO)
│   ├── Common/hardware.{c,h}
│   ├── V3F/              # 在 V3F 核运行的工程
│   └── V5F/              # 在 V5F 核运行的工程
│
├── QSPI_FLASH_DMA/       # 通过 DMA 搬运数据, 释放 CPU
│   ├── Common/hardware.{c,h}
│   ├── V3F/
│   └── V5F/
│
└── QSPI_MemoryMap/       # 内存映射模式 (Memory-Mapped)
    ├── Common/hardware.{c,h}
    ├── V3F/
    └── V5F/
```

> 三个例子的差异在于**数据搬运方式**:
> - `QSPI_FLASH`: CPU 轮询 FIFO 收发 (`QSPI_Send` / `QSPI_Receive`)
> - `QSPI_FLASH_DMA`: DMA 自动收发 (DMA1 Channel1)
> - `QSPI_MemoryMap`: 把外部 Flash 映射到地址 `0x90000000`, 直接指针访问

---

## 2. 硬件连接 (从 `main.c` 注释 + `GPIO_Config` 整理)

| 信号     | 引脚 | GPIO       | AF  | 备注                          |
| -------- | ---- | ---------- | --- | ----------------------------- |
| QSPI_SCK | PB2  | `GPIOB_2`  | AF9 | 时钟                          |
| QSPI_CS  | PB6  | `GPIOB_6`  | AF10 | 片选                           |
| QSPI_SIO0| PF8  | `GPIOF_8`  | AF10 | IO0 / MOSI (1 线模式主出从入) |
| QSPI_SIO1| PF9  | `GPIOF_9`  | AF10 | IO1 / MISO (1 线模式主入从出) |
| QSPI_SIO2| PF7  | `GPIOF_7`  | AF10 | IO2 (Quad 模式 WP#)           |
| QSPI_SIO3| PF6  | `GPIOF_6`  | AF10 | IO3 (Quad 模式 HOLD#)         |

要点:

- **CS / SCK** 走 AF9/AF10, **SIO0~SIO3** 全部走 AF10。
- GPIO 全部配置为 `GPIO_Mode_AF_PP` (复用推挽), 速率 `GPIO_Speed_Very_High`。
- 需要打开的时钟: `AFIO`, `GPIOB`, `GPIOF`, `GPIOC`, `GPIOE` (其中 GPIOC/E 可能为示例外设预留)。

---

## 3. QSPI 外设初始化 (`QSPI1_Config`)

```c
RCC_HB1PeriphClockCmd(RCC_HB1Periph_QSPI1, ENABLE);   // 打开 QSPI1 时钟

QSPI_InitStructure.QSPI_Prescaler = 3;                // SCK = HCLK / (Prescaler+1)
QSPI_InitStructure.QSPI_CKMode    = QSPI_CKMode_Mode3;// CPOL=1, CPHA=2 (空闲高, 第二边沿采样)
QSPI_InitStructure.QSPI_CSHTime   = QSPI_CSHTime_8Cycle; // CS 片选高电平至少 8 个时钟周期
QSPI_InitStructure.QSPI_FSize     = 22;               // Flash 大小 2^(FSize+1) = 2^23 = 8 MB
QSPI_InitStructure.QSPI_FSelect   = QSPI_FSelect_1;   // 选择 Flash 1
QSPI_InitStructure.QSPI_DFlash    = QSPI_DFlash_Disable; // 不启用双 Flash 模式

QSPI_Init(QSPI1, &QSPI_InitStructure);
QSPI_SetFIFOThreshold(QSPI1, 10);                    // FIFO 阈值 = 10 字节
```

> **DMA 例程的差异**: Prescaler = 1 (更快的 SCK); CKMode = Mode0; FIFOThreshold = 0; 立即设置数据长度 = 1; 额外打开 `RCC_HBPeriphClockCmd(RCC_HBPeriph_DMA1, ENABLE)`。

之后必须 `QSPI_Cmd(QSPI1, ENABLE)` 使能控制器。

---

## 4. W25Qxx 指令码宏定义

```c
#define FLASH_CMD_EnableReset             0x66
#define FLASH_CMD_ResetDevice             0x99
#define FLASH_CMD_JedecID                 0x9F
#define FLASH_CMD_WriteEnable             0x06
#define FLASH_CMD_WriteDisable            0x04
#define FLASH_CMD_ManufactDeviceID        0x90
#define FLASH_CMD_ManufactDeviceID_DualIO 0x92
#define FLASH_CMD_ManufactDeviceID_QuadIO 0x94
#define FLASH_CMD_SectorErase             0x20
#define FLASH_CMD_Read                    0x03
#define FLASH_CMD_FastRead                0x0B     // 仅 DMA 例程使用 (带 dummy cycle)
#define FLASH_CMD_FastReadQuadIO          0xEB
#define FLASH_CMD_FastReadDualIO          0xBB
#define FLASH_CMD_PageProgram             0x02
#define FLASH_CMD_PageProgramQuad         0x32
#define FLASH_CMD_ReadStatus_REG1         0x05
#define FLASH_CMD_ReadStatus_REG2         0x35
```

> 注意: `0xEB` (Quad I/O Fast Read) 通常需要 6 个 dummy cycles; `0xBB` (Dual I/O Fast Read) 需要 4 个; `0x0B` (Fast Read) 需要 8 个。

---

## 5. 通用封装函数 (3 个例程几乎共用)

| 函数                          | 说明                                                          |
| ----------------------------- | ------------------------------------------------------------- |
| `QSPI_Send(buf, len)`         | 轮询 `QSPI_FLAG_FT` (FIFO 阈值), `QSPI_SendData8` 写入数据     |
| `QSPI_Receive(buf, len)`      | 轮询 `QSPI_FLAG_FT`, `QSPI_ReceiveData8` 读取数据              |
| `QSPI_WriteCmd(cmd)`          | 仅发一字节指令, 不带数据/地址 (写使能、复位、读状态寄存器前)   |
| `QSPI_CmdWithAddress(cmd,addr)`| 发指令 + 24 位地址, 不带数据 (扇区擦除等)                      |
| `QSPI_CmdWithData(cmd, data)` | 发指令 + 1 字节数据 (如写状态寄存器)                           |
| `QSPI_ReadSR(cmd)`            | 读 1 字节状态寄存器 (1 线)                                     |
| `QSPI_WaitForBsy()`           | 用自动轮询模式 (Auto Polling) 等 BUSY=0 (SR1[0])                |
| `QSPI_WriteEnable()`          | 发 `0x06` (内联)                                              |
| `QSPI_WriteDisable()`         | 发 `0x04` (内联)                                              |
| `QSPI_EraseSector(addr)`      | 写使能 + 扇区擦除 + 等待 BUSY                                  |
| `QSPI_WritePage(addr,...)`    | 单线页编程, **未写使能, 未等 BUSY** (供 `ProgramPage` 调用)    |
| `QSPI_WritePageQuad(addr,...)`| 四线页编程, 通过 `QSPI_EnableQuad(ENABLE)` 进入 Quad           |
| `QSPI_ProgramPage(...)`       | 写使能 + 单线页编程 + 等待 BUSY                                 |
| `QSPI_ProgramPageQuad(...)`   | 写使能 + 四线页编程 + 等待 BUSY                                 |
| `QSPI_GetManufacturerID()`    | 1 线读 Manufacturer/Device ID (2 字节)                         |
| `QSPI_GetManufacturerID_DualIO()` | 2 线读 ID, DummyCycles=4, 启用 Quad                       |
| `QSPI_GetManufacturerID_QuadIO()`  | 4 线读 ID, DummyCycles=6, 启用 Quad                       |
| `QSPI_GetData(addr,buf,len)`  | 1 线读数据 (普通读)                                            |
| `QSPI_GetData_DualIO(...)`    | 2 线读, DummyCycles=4                                          |
| `QSPI_GetData_QuadIO(...)`    | 4 线读, DummyCycles=6, 启用 Quad                               |
| `Buffer_Cmp / Print_Buffer`   | 内存比较 / 16 字节一行打印到串口                               |

### 5.1 关键: QSPI 命令序列 (`QSPI_ComConfig_InitTypeDef`)

每次访问 Flash 都要先填好这套结构体, 它对应 QSPI 的 `CCR` 寄存器:

| 字段                     | 典型值                              |
| ------------------------ | --------------------------------- |
| `QSPI_ComConfig_IMode`   | 指令线数: `1Line` / `2Line` / `4Line` / `NoInstruction` |
| `QSPI_ComConfig_ADMode`  | 地址线数: `1Line` / `2Line` / `4Line` / `NoAddress` |
| `QSPI_ComConfig_DMode`    | 数据线数: `1Line` / `2Line` / `4Line` / `NoData` |
| `QSPI_ComConfig_ABMode`  | 交替字节模式 (一般不用)            |
| `QSPI_ComConfig_FMode`   | 功能模式: `Indirect_Read` / `Indirect_Write` / `Auto_Polling` / `Memory_Mapped` |
| `QSPI_ComConfig_SIOOMode`| 仅发一次指令(用于 XIP 连续读)       |
| `QSPI_ComConfig_ADSize`  | 地址宽度: `8/16/24/32bit`          |
| `QSPI_ComConfig_ABSize`  | 交替字节宽度                       |
| `QSPI_ComConfig_DummyCycles` | Dummy 时钟数 (Fast Read 时使用) |
| `QSPI_ComConfig_Ins`     | 指令值                             |

调用顺序: `QSPI_ComConfig_Init()` → 可选 `QSPI_SetAddress()` → `QSPI_SetDataLength()` → `QSPI_Start()` → 轮询 `QSPI_FLAG_TC` → 清 `FT/TC` 标志。

### 5.2 QSPI_WaitForBsy 实现

```c
QSPI_AutoPollingMode_Config(QSPI1, 0x0000, 0x01, QSPI_PMM_AND);  // Match=0, Mask=1, AND
QSPI_AutoPollingModeStopCmd(QSPI1, ENABLE);
QSPI_AutoPollingMode_SetInterval(QSPI1, 2500);                  // 匹配间隔

QSPI_ComConfig_InitStructure.QSPI_ComConfig_FMode = QSPI_ComConfig_FMode_Auto_Polling;
QSPI_ComConfig_InitStructure.QSPI_ComConfig_DMode = QSPI_ComConfig_DMode_1Line;
QSPI_ComConfig_InitStructure.QSPI_ComConfig_Ins   = FLASH_CMD_ReadStatus_REG1;  // 0x05

QSPI_Start(QSPI1);
while (QSPI_GetFlagStatus(QSPI1, QSPI_FLAG_SM) == RESET) {}     // 等待 Status Match
```

> **逻辑**: 反复读 SR1, 当 `SR1 & Mask == Match` (即 BUSY=0) 时硬件自动停止轮询, 置位 `SM` 标志。

---

## 6. QSPI_FLASH 示例 (CPU 轮询方式)

### 6.1 main 流程 (`Hardware()`)

```
1. printf("QSPI TEST\n")
2. GPIO_Config() / QSPI1_Config() / QSPI_Cmd(ENABLE)
3. QSPI_WriteCmd(0x66) + QSPI_WriteCmd(0x99)   // 使能复位 + 复位设备
   Delay_Us(100)
4. QSPI_WaitForBsy()
5. 读 3 个 Manufacturer ID (1线 / DualIO / QuadIO)
6. 读 SR1 / SR2
7. 读 256 字节:
     - QSPI_GetData()       (1 线)
     - QSPI_GetData_DualIO() (2 线)
     - QSPI_GetData_QuadIO() (4 线)
   每次读出后用 Buffer_Cmp 与第一次比较, 不一致则报错。
8. while(1) 死循环
```

### 6.2 测试数据生成 (`Write_Test_Data`)

```c
// 仅在调用方使用, 本例 Hardware() 没有调用, 是给用户参考用的工具函数。
// 流程: 按 4KB 边界擦除, 然后以 256 字节(1 页)为单位写。
for (i=0; i<write_page_count*256; i+=256) {
    fill writeBuffer with i/4, i/4+1, ...
    if (addr % 4096 == 0) QSPI_EraseSector(addr);
    QSPI_ProgramPageQuad(addr, writeBuffer, 256);
}
```

---

## 7. QSPI_FLASH_DMA 示例 (DMA 搬运方式)

### 7.1 DMA 通道分配

| DMA 通道              | 请求源 (Mux) | 用途                |
| --------------------- | ------------- | ------------------- |
| `DMA1_Channel1`       | Mux Channel 1, 值 71 | QSPI1 收发 (RX/TX) |
| (MemoryMap 例程另外用) `DMA1_Channel2` | -             | 内存到内存 `Mem_Copy` |

> 请求号 71 = QSPI1 (具体查 `DMA_MuxChannelConfig` 章节)。

### 7.2 DMA 配置模板

```c
// 接收
DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(QSPI1->DR);
DMA_InitStructure.DMA_Memory0BaseAddr    = (u32)buffer;
DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;     // 外设→内存
DMA_InitStructure.DMA_BufferSize         = len / 4;                  // Word 单位
DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word;
DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
DMA_InitStructure.DMA_Priority           = DMA_Priority_VeryHigh;
DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
DMA_MuxChannelConfig(DMA_MuxChannel1, 71);

// 发送 (仅 DIR 不同)
DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  // 内存→外设
```

### 7.3 QSPI_Start + DMA 配合模板

```c
// 1. 配置 QSPI ComConfig + 地址 + 数据长度
QSPI_ComConfig_Init(QSPI1, &cfg);
QSPI_SetAddress(QSPI1, addr);
QSPI_SetDataLength(QSPI1, len);

// 2. 配置 DMA
QSPI_Rx_DMA(buf, len);     // 或 QSPI_Tx_DMA(buf, len);
QSPI_DMACmd(QSPI1, ENABLE);
DMA_Cmd(DMA1_Channel1, ENABLE);

// 3. 启动 QSPI 传输
QSPI_Start(QSPI1);

// 4. 等待 DMA 完成
while (DMA_GetFlagStatus(DMA1, DMA1_FLAG_TC1) == RESET) {}

// 5. 等待 QSPI TC
while (QSPI_GetFlagStatus(QSPI1, QSPI_FLAG_TC) == RESET) {}
QSPI_ClearFlag(QSPI1, QSPI_FLAG_FT);
QSPI_ClearFlag(QSPI1, QSPI_FLAG_TC);
QSPI_DMACmd(QSPI1, DISABLE);
```

### 7.4 与 QSPI_FLASH 的区别

| 项目            | QSPI_FLASH                       | QSPI_FLASH_DMA                          |
| --------------- | -------------------------------- | --------------------------------------- |
| 数据 IO          | `QSPI_Send / QSPI_Receive` 轮询 FIFO | DMA 自动搬运                            |
| 预分频 / 时钟模式 | Prescaler=3, Mode3            | Prescaler=1, Mode0 (按需调整)           |
| FIFO 阈值       | 10                              | 0                                       |
| 进入 QSPI 操作前 | 不等待 IDLE                     | `while (QSPI_GetFlagStatus(...IDLE)==RESET)` |
| DMA 通道        | -                               | `DMA1_Channel1`, Mux Channel=71        |
| 提供额外工具    | `Write_Test_Data`              | `write_data(addr,data,len)` (按页写 + 4K 自动擦) |

---

## 8. QSPI_MemoryMap 示例 (内存映射模式)

### 8.1 核心思想

把外部 SPI Flash 映射到 MCU 的地址空间 **0x90000000**, 之后可以直接用 C 指针 `*(volatile uint8_t *)0x90000000` 像访问内部 Flash 一样读取数据 — **XIP (Execute-In-Place)**。

### 8.2 进入 Memory-Mapped 模式

```c
void QSPI_MemoryMap() {
    cfg.c. QSPI_ComConfig_FMode       = QSPI_ComConfig_FMode_Memory_Mapped;
    cfg.c. QSPI_ComConfig_DMode       = QSPI_ComConfig_DMode_1Line;
    cfg.c. QSPI_ComConfig_ADMode      = QSPI_ComConfig_ADMode_1Line;
    cfg.c. QSPI_ComConfig_Ins         = FLASH_CMD_FastRead;  // 0x0B
    cfg.c. QSPI_ComConfig_DummyCycles = 8;
    QSPI_ComConfig_Init(QSPI1, &cfg.c);
    QSPI_Start(QSPI1);   // 注意: Mapped 模式下 QSPI 持续响应读访问
}

// Quad 版本类似, 只是 ADMode/DMode = 4Line, Ins=0xEB, Dummy=6, 启用 QSPI_EnableQuad
```

> 一旦调用 `QSPI_Start()`, 控制器会**持续监听 SCK** 上的读指令, 任何对 `0x90000000+` 的读访问都会自动通过 QSPI 拉取数据。

### 8.3 演示代码 (Hardware 末尾)

```c
uint8_t*  QSPI_MapAddr8  = (void*)0x90000000;
uint16_t* QSPI_MapAddr16 = (void*)0x90000000;
uint32_t* QSPI_MapAddr32 = (void*)0x90000000;
for (size_t i = 0; i < 10; i++) {
    printf("u8 [%x] = %x\n", &QSPI_MapAddr8[i],  QSPI_MapAddr8[i]);
    printf("u16[%x] = %x\n", &QSPI_MapAddr16[i], QSPI_MapAddr16[i]);
    printf("u32[%x] = %x\n", &QSPI_MapAddr32[i], QSPI_MapAddr32[i]);
}
```

> 必须保证地址 `0x90000000` 在分散加载脚本 / Keil Options 中映射到了 QSPI 外设区域 (CH32H417 参考手册里是固定的, 一般不需要改链接脚本)。

### 8.4 退出 Mapped 模式

调用 `QSPI_AbortRequest(QSPI1)` (在 `QSPI_WritePage` 等函数中可见), 然后等待 `IDLE` 标志。

### 8.5 额外提供: `Mem_Copy_DMA` (内存到内存)

```c
void Mem_Copy_DMA(uint8_t* src, uint8_t* dst, uint32_t len) {
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)dst;
    DMA_InitStructure.DMA_Memory0BaseAddr    = (u32)src;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize         = len / 4;     // len 必须 4 字节对齐
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Enable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Enable;
    DMA_Init(DMA1_Channel2, &DMA_InitStructure);
    DMA_ClearFlag(DMA1, DMA1_FLAG_TC2);
    DMA_Cmd(DMA1_Channel2, ENABLE);
    while (DMA_GetFlagStatus(DMA1, DMA1_FLAG_TC2) == RESET) {}
```

> 这个工具函数可以把某个区域的数据通过 DMA 拷到 QSPI MemoryMap 区, 但 Hardware() 里没有演示调用, 仅作为模板参考。

---

## 9. 工程通用结构 (V3F / V5F 双核)

每个示例下都有 `V3F/`、`V5F/` 两个独立工程, 通过 `ch32h417_conf.h` 中的 `Run_Core` 选择运行哪个核:

| `Run_Core` 值              | 行为                                                                |
| ------------------------ | ------------------------------------------------------------------- |
| `Run_Core_V3FandV5F`     | V3F 唤醒 V5F, 通过 HSEM 同步, 两边都跑                              |
| `Run_Core_V3F`           | 只跑 V3F                                                            |
| `Run_Core_V5F`           | 只跑 V5F                                                            |

`Hardware()` 实际只在被选中的核里调用一次 (使用 `HSEM_FastTake` 防止两边都进入)。

主要差异:

- **V3F 工程**:
  - `main.c` 调用 `SystemInit()` (V3F 是主启动核)
  - 多核模式下要先 `NVIC_WakeUp_V5F(Core_V5F_StartAddr)` 唤醒 V5F
  - 进入 `Hardware()` 的逻辑由 V3F 主导

- **V5F 工程**:
  - `main.c` 没有 `SystemInit()` (被 V3F 启动)
  - 直接 `Hardware()` (通过 HSEM 抢占)

---

## 10. 关键流程总结

### 10.1 通用 QSPI 操作流程

```
1. 初始化 GPIO + QSPI 外设
2. QSPI_Cmd(ENABLE)
3. (可选) 软件复位 Flash: 0x66 → 0x99
4. (可选) 启用 Quad 模式 (写状态寄存器 0x31, 数据 0x02)
5. 根据操作填 QSPI_ComConfig_InitTypeDef:
     IMode / ADMode / DMode / ABMode / FMode / SIOO / ADSize / Dummy / Ins
6. (读/写) QSPI_SetAddress()
8. QSPI_SetDataLength()
9. (读 Quad/写 Quad) QSPI_EnableQuad(ENABLE)
10. (DMA 方式) QSPI_DMACmd + DMA 配置 + DMA_Cmd
11. QSPI_Start()
12. (DMA 方式) 等 DMA TC; (轮询方式) 等 FT 然后读 FIFO
13. 等 QSPI_FLAG_TC, 清 FT/TC
14. (Quad) QSPI_EnableQuad(DISABLE)
```

### 10.2 读 Manufacturer ID (1 线)

```c
cfg.IMode = 1Line; cfg.ADMode = 1Line; cfg.DMode = 1Line;
cfg.FMode = Indirect_Read; cfg.ADSize = 24bit; cfg.Dummy = 0; cfg.Ins = 0x90;
QSPI_SetAddress(0);          // 24 位地址占位
QSPI_SetDataLength(2);       // 返回 2 字节 (Manuf + Dev)
QSPI_Start();
读 2 字节到 res
```

### 10.3 读 Manufacturer ID (4 线)

```c
cfg.ADMode = 4Line; cfg.DMode = 4Line;
cfg.Dummy = 6;                // 0x94 协议需要 6 个 dummy
cfg.Ins = 0x94;
QSPI_EnableQuad(ENABLE);
QSPI_SetDataLength(2);
QSPI_Start();
读 2 字节; QSPI_EnableQuad(DISABLE);
```

### 10.4 扇区擦除 (4 KB)

```c
QSPI_WriteEnable();            // 0x06
QSPI_CmdWithAddress(0x20, addr); // 0x20 + 24-bit 地址
QSPI_WaitForBsy();             // Auto-Polling 等 SR1[0]==0
```

### 10.5 Quad 页面写入 (256 字节)

```c
QSPI_WriteEnable();                              // 0x06
cfg.IMode=1Line; cfg.ADMode=1Line; cfg.DMode=4Line;
cfg.FMode=Indirect_Write; cfg.Ins=0x32; cfg.Dummy=0;
QSPI_SetAddress(addr);
QSPI_SetDataLength(len);
QSPI_EnableQuad(ENABLE);
QSPI_Start();
QSPI_Send(data, len);    // 或 DMA
QSPI_EnableQuad(DISABLE);
QSPI_WaitForBsy();
```

### 10.6 快速读 (1 线 / 带 dummy)

```c
cfg.IMode=1Line; cfg.ADMode=1Line; cfg.DMode=1Line;
cfg.FMode=Indirect_Read; cfg.Ins=0x0B; cfg.Dummy=8;
QSPI_SetAddress(addr); QSPI_SetDataLength(len);
QSPI_Start();
QSPI_Receive(buf, len);  // 或 DMA
```

### 10.7 Quad I/O 快速读

```c
cfg.ADMode=4Line; cfg.DMode=4Line;
cfg.Ins=0xEB; cfg.Dummy=6;
QSPI_EnableQuad(ENABLE);
QSPI_SetAddress(addr); QSPI_SetDataLength(len);
QSPI_Start();
QSPI_Receive(buf, len);
QSPI_EnableQuad(DISABLE);
```

### 10.8 内存映射模式 (XIP)

```c
cfg.IMode=1Line; cfg.ADMode=1Line; cfg.DMode=1Line;
cfg.FMode=Memory_Mapped; cfg.Ins=0x0B; cfg.Dummy=8;
QSPI_ComConfig_Init(QSPI1, &cfg);
QSPI_Start();
// 之后访问 *(uint8_t*)0x90000000 即透明落到外部 Flash
```

---

## 11. 重要注意点 & 经验

1. **FIFO 阈值** 决定 `FT` 标志何时拉高, 设为 0 时**每写入/读取 1 个字**就拉高一次, 适合 DMA; 设为较大值时适合 CPU 轮询。

2. **Dummy Cycles**: 一定要按 Flash 数据手册配, W25Q 的 `0xEB` (Quad I/O) 需要 6 个; `0xBB` (Dual I/O) 需要 4 个; `0x0B` (Fast Read) 需要 8 个。

3. **进入 Quad 模式**: W25Q 默认单线模式, 需要先发 `0x06` (写使能) → `0x31` (写状态寄存器 2) + 数据 `0x02` (置 QE 位) 才能真正使用 Quad I/O。代码里有 `#if 0` 注释的参考:

   ```c
   #if 0
   QSPI_WriteEnable();
   QSPI_WriteCmd(0x50);            // 写状态寄存器 1/2/3 的解锁
   QSPI_CmdWithData(0x31, 0x02);   // 写 SR2, 0x02 = QE=1
   #endif
   ```

4. **CH32H417 QSPI 的 SIO2/SIO3** 在 Quad 模式下被用作双向数据线 (WP# / HOLD# 被复用为 IO2/IO3)。使用 Quad 模式时**必须**把 PB2/PF6-9 都接好。

5. **DMA 例程里写之前要 `QSPI_AbortRequest`**: 因为 DMA 占用了总线状态, 重新配置前要安全退出, 然后等待 `TC` 与 `IDLE`。

6. **Mapped 模式下不能直接 `QSPI_WriteEnable()`**: 必须先 `QSPI_AbortRequest` 退出 Mapped, 再走写流程。

7. **三个例程的 Common 代码高度相似**, 区别只是 IO 方式 (CPU / DMA / MD), 学习时**先掌握 QSPI_FLASH**, 后续示例只是替换了 `QSPI_GetData*` / `QSPI_WritePage*` 的内部实现。

---

## 12. QSPI 库 API 速查 (来自 `ch32h417_qspi.h`)

| API                          | 作用                                                       |
| ---------------------------- | ---------------------------------------------------------- |
| `QSPI_Init`                  | 初始化 prescaler / CK mode / CSH time / FSize / FSelect / DFlash |
| `QSPI_ComConfig_Init`        | 设入 `CCR`: FMode / IMode / ADMode / DMode / AB / Dummy / Ins |
| `QSPI_Cmd(ENABLE/DISABLE)`  | 控制器使能                                                   |
| `QSPI_AutoPollingMode_Config` | 配置 auto-polling 匹配值 / 掩码 / 模式                       |
| `QSPI_SetAddress`           | 设置地址寄存器                                               |
| `QSPI_SetAlternateByte`      | 设置旁路字节                                                  |
| `QSPI_SetFIFOThreshold`     | 设置 FIFO 阈值                                                |
| `QSPI_SetDataLength`        | 设置数据长度                                                  |
| `QSPI_EnableQuad`             | 启用 Quad 模式 (单线 ↔ 四线切换)                              |
| `QSPI_Start`                | 启动一次传输                                                 |
| `QSPI_AbortRequest`         | 中止当前传输 (Mapped 模式下退出)                              |
| `QSPI_SendData8/16/32`     | 写 FIFO                                                       |
| `QSPI_ReceiveData8/16/32`  | 读 FIFO                                                       |
| `QSPI_DMACmd(ENABLE)`       | 让 QSPI 触发 DMA 请求                                        |
| `QSPI_GetFlagStatus / ClearFlag` | 查询/清 BUSY/TC/FT/SM/TO/TE/IDLE                       |
| `QSPI_GetFMode`             | 读当前 FMode                                                  |

---

## 13. 思考与扩展

- 想做 XIP (代码就地执行), 应当:
  1. 进 Mapped 模式 (`QSPI_ComConfig_FMode_Memory_Mapped`);
  2. 在链接脚本里把 `0x90000000` 映射成某段只读 region;
  3. 启用 `QSPI_ComConfig_SIOOMode_Enable` (仅发一次指令) 可进一步减少 SCK 上的开销, 适合持续读。
- 想做 DMA + 中断收数据: 可启用 `QSPI_ITConfig(QSPI1, QSPI_IT_TC, ENABLE)`, 在中断里清标志继续下次传输。
- 想做 Quad Page Program + Memory-Mapped 切换: 必须先 `QSPI_AbortRequest` 退出 Mapped, 再 `QSPI_EnableQuad` 配写配置, 写完重新进 Mapped。
- 对 CH32H417 这种双核 MCU, QSPI 操作应放在**单一核** (通常 V5F) 上, 避免两核同时操作 QSPI 寄存器。`HSEM` 就是用来同步的。
