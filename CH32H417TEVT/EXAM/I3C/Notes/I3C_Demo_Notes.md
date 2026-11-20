# CH32H417 I3C Demo 学习笔记

> 来源:`CH32H417TEVT/EXAM/I3C/` 下三个示例
> 参考 HAL:`CH32H417TEVT/EXAM/SRC/Peripheral/inc/ch32h417_i3c.h` 与 `src/ch32h417_i3c.c`

---

## 1. 示例总览

| 示例 | 角色 | 重点 |
| --- | --- | --- |
| `I3C_DMA_Controller` | Controller(主控) | DMA + CCC 命令(RSTDAA / ENTDAA)、私有消息读写 |
| `I3C_DMA_Target` | Target(从机) | Target 模式初始化、预加载 TX、DMA 收发 |
| `I3C_Controller_Check_ZeroIBI` | Controller | 中断驱动,监听 IBI / CR / HJ / FrameComplete(ZeroIBI) |

硬件接线(三个 demo 完全一致):

```
PE14 -- I3C_SCL   (AF3)
PE15 -- I3C_SDA   (AF3)
```

---

## 2. 工程结构与双核启动

CH32H417 内部集成 V3F(RISC-V)与 V5F(Cortex-M33)两个核。每个 demo 给出 V3F / V5F 两个工程目录,业务代码放在 `Common/` 下,被 V3F 端编译使用。`Run_Core` 选项(`debug.h`):

```c
#define Run_Core_V3F        0   // 只 V3F 跑
#define Run_Core_V5F        1   // 只 V5F 跑
#define Run_Core_V3FandV5F  2   // 双核,V3F 唤醒 V5F
```

`I3C_DMA_Controller/V3F/User/main.c` 的启动模板(三个 demo 一样):

```c
#if (Run_Core == Run_Core_V3FandV5F)
    NVIC_WakeUp_V5F(Core_V5F_StartAddr);  // 唤醒 V5F
    HSEM_ITConfig(HSEM_ID0, ENABLE);
    NVIC->SCTLR |= 1<<4;                  // 进入低功耗后允许被唤醒
    PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFE);
    HSEM_ClearFlag(HSEM_ID0);
    Hardware();   // V3F 自己执行硬件示例
#elif (Run_Core == Run_Core_V3F)
    Hardware();
#elif (Run_Core == Run_Core_V5F)
    NVIC_WakeUp_V5F(Core_V5F_StartAddr);
    PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFE);
#endif
```

关键点:
- **I3C 示例的硬件代码只跑在 V3F(V5F/User 中没有 `hardware.c`)**。
- `Run_Core_V3FandV5F` 模式下,V3F 是主核,负责跑 `Hardware()`,V5F 在 V3F 唤醒后通过 `HSEM_FastTake`/`HSEM_ReleaseOneSem` 同步,然后空转。
- `Run_Core_V5F` 模式下,V3F 进入 STOP,V5F 唤醒后执行 `Hardware()`(但三个示例的 `Common/hardware.c` 仍归 V3F 编译,实际需要在 V5F 工程中另外包含)。

---

## 3. I3C 协议要点(从 demo 看到的)

### 3.1 角色
- **Controller**:主导总线、生成 SCL、发起 CCC/读写。私有自己的动态地址(`DynamicAddr`,demo 中设为 `0x22`)。
- **Target**:被动响应;有 48-bit 静态标识符 `Provider ID`(Vendor 12b + Part 16b + Instance 4b + TypeSelector 1b)。

### 3.2 关键 CCC 命令(Common Command Codes)
| 命令 | 值 | 含义 |
| --- | --- | --- |
| `RSTDAA` | `0x06` | 重置所有 Target 的动态地址 |
| `ENTDAA` | `0x07` | 控制器发起动态地址分配 |
| `SETMWL` / `SETMRL` | — | 设置最大读写长度 |
| `GETMWL` / `GETMRL` | — | 读最大读写长度 |
| `GETPID` / `GETBCR` / `GETDCR` | — | 读 Target 能力 |
| `ENEC` / `DISEC` | — | 使能/禁用事件(IBI / CR / HJ) |
| `ENTASx` | — | 进入活动状态 |

### 3.3 中断事件(Controller 收到)
| 中断 | 含义 |
| --- | --- |
| `IBIIE` | Target 发起 IBI(In-Band Interrupt) |
| `CRIE` | Target 请求成为 Controller |
| `HJIE` | Target 发起 Hot-Join(未分配地址情况下) |
| `FCIE` | Frame Complete(传输完成) |

### 3.4 消息类型
- `I3C_CONTROLLER_MTYPE_PRIVATE` — 私有 SDR 读写
- `I3C_CONTROLLER_MTYPE_LEGACY_I2C` — 兼容 I2C 消息
- `I3C_CONTROLLER_MTYPE_CCC` — CCC 命令
- `I3C_CONTROLLER_MTYPE_DIRECT` — 直接 CCC(带 Target 地址)
- `I3C_TARGET_MTYPE_IBI` / `CR` / `HJ` — Target 主动消息

### 3.5 结束方式
- `I3C_GENERATE_STOP` — 当前帧以 STOP 结束
- `I3C_GENERATE_RESTART` — 当前帧以 Repeated-START 结束(继续占用总线)

---

## 4. HAL 关键 API(摘选)

`ch32h417_i3c.h` 提供以下分组,记住即可覆盖日常开发:

### 4.1 初始化
```c
void I3C_DeInit(void);
void I3C_Ctrl_Init(I3C_Ctrl_BusTypeDef *);   // 时序: SCL PP/I3C/I2C、Bus Free、Bus Idle…
void I3C_Tgt_Init (uint8_t BusAvailableDuration); // Target 模式下总线空闲可用延时
void I3C_Ctrl_Config(I3C_CtrlConfTypeDef *);  // DA、Stall、HotJoinAllowed、CCC/Tx/Rx Stall
void I3C_Tgt_Config(I3C_TgtConfTypeDef *);    // PID/MAXRLR/BCR/DCR
void I3C_Cmd(FunctionalState);                // CFGR.EN
void I3C_SetModeConfig(uint32_t PeripheralMode); // CFGR.CRINIT 切换主/从角色
```

### 4.2 总线设备描述(Controller 端)
```c
void I3C_Ctrl_ConfigBusDevices(I3C_DeviceConfTypeDef *pDesc);
// 配置项: DeviceIndex(1~4), TargetDynamicAddr, IBIAck, IBIPayload, CtrlRoleReqAck, CtrlStopTransfer
```

### 4.3 收发
```c
void I3C_ControllerHandleMessage(uint32_t TargetAddr, uint32_t TransferSize,
                                 uint32_t Direction, uint32_t MessageType, uint32_t EndMode);
void I3C_ControllerHandleCCC(uint32_t CCCValue, uint32_t AddByteSize, uint32_t EndMode);
void I3C_TargetHandleMessage(uint32_t MessageType, uint32_t IBISize);
uint8_t  I3C_ReadByte(void);
uint32_t I3C_ReadWord(void);
void     I3C_WriteByte(uint8_t);
void     I3C_WriteWord(uint32_t);
uint16_t I3C_GetXferDataCount(void);   // 剩余字节数
uint8_t  I3C_GetIBITargetAddr(void);   // IBI 发起方地址
uint8_t  I3C_GetNbIBIAddData(void);    // IBI 附加字节数
```

### 4.4 DMA / 中断
```c
void I3C_DMAReq_TXCmd(FunctionalState);   // CFGR.TXDMAEN
void I3C_DMAReq_RXCmd(FunctionalState);   // CFGR.RXDMAEN
void I3C_TxPreloadConfig(uint16_t TxDataCount); // TGTTDR.PRELOAD + TGTTDCNT,Target 预加载
void I3C_ITConfig(uint16_t I3C_IT, FunctionalState);
ITStatus   I3C_GetITStatus(uint16_t);
void       I3C_ClearITPendingBit(uint16_t);
FlagStatus I3C_GetFlagStatus(uint16_t);
void       I3C_ClearFlag(uint16_t);
```

### 4.5 寄存器(底层)
- `I3C->CFGR` — 主控配置(EN, CRINIT, TXDMAEN, RXDMAEN, HJACK, RSTPTRN, EXITPTRN…)
- `I3C->RESET` — 软复位(`I3C_RESET_HST_SIE_RST` / `I3C_RESET_TGT_SIE_RST`)
- `I3C->TIMINGR1` / `TIMINGR2` — 时序
- `I3C->TDBR` / `RDBR` — TX/RX 数据寄存器(DMA 源/目的)
- `I3C->IBIDR` — IBI 数据寄存器
- `I3C->CTLR` — 命令寄存器(Address | Size | Direction | MType | EndMode)
- `I3C->TGTTDR` — Target TX 预加载

---

## 5. 示例一:`I3C_DMA_Controller`

`Common/hardware.c` 关键步骤:

```c
GPIO_Config();                  // PE14/PE15 → AF_PP, GPIO_AF3
I3C_Config();                   // 见下
I3C_RSTDAA();                   // CCC 0x06
I3C_ENTDAA(target_daa1 /*0x32*/);  // CCC 0x07
I3C_DMA_Transmit(target_daa1, trasmit_buf, 10);  // 私有写
I3C_DMA_Recive (target_daa1, recive_buf , 10);   // 私有读
```

### 5.1 `I3C_Config()`
```c
I3C_Ctrl_BusInit.SDAHoldTime        = I3C_SDAHoldTime_1_5;  // tHD_DAT
I3C_Ctrl_BusInit.WaitTime           = I3C_WaitTime_State_2;  // tCAS
I3C_Ctrl_BusInit.SCLPPLowDuration   = 0x89;                 // Push-Pull 低电平
I3C_Ctrl_BusInit.SCLI3CHighDuration = 0x88;                 // I3C Open-Drain 高电平
I3C_Ctrl_BusInit.SCLODLowDuration   = 0x88;                 // OD 低电平
I3C_Ctrl_BusInit.SCLI2CHighDuration = 0x88;                 // I2C 兼容 OD 高
I3C_Ctrl_BusInit.BusFreeDuration    = 0x77;                 // tFREE
I3C_Ctrl_BusInit.BusIdleDuration    = 0x8e;                 // tIDLE

I3C_CtrlConfInit.DynamicAddr       = 0x22;            // 控制器动态地址(可选)
I3C_CtrlConfInit.StallTime         = 0x10;            // Stall 时间
I3C_CtrlConfInit.HotJoinAllowed_EN = DISABLE;
// ACK/CCC/Tx/Rx Stall 都关闭

I3C_Ctrl_Init(&I3C_Ctrl_BusInit);
I3C_Ctrl_Config(&I3C_CtrlConfInit);
I3C_SetConfigFifo(&...);          // Rx/Tx FIFO 阈值 = 1/4, Control/Status FIFO 关闭
I3C_Cmd(ENABLE);
```

注意:这些时序参数是 demo 的示例值,实际项目中要根据总线速率(典型 12.5 MHz SDR)、上拉电阻、PCB 寄生电容重新计算。

### 5.2 `I3C_ENTDAA()` 实现要点
```c
I3C_ControllerHandleCCC(0x07, 0, I3C_GENERATE_STOP);   // ENTDAA + STOP
do {
    if (I3C_GetFlagStatus(I3C_EVR_TXFNFF)) {           // RX FIFO 非满?
        for (int i = 0; i < 8; i++) {                  // 收 8 字节 PID
            while (I3C_GetFlagStatus(I3C_EVR_RXFNEF) == RESET);
            payload |= (uint64_t)I3C_ReadByte() << (i * 8);
        }
        I3C_WriteByte(tgt_daa++);                      // 写入分配给该 Target 的动态地址
        ...
    }
} while (FCF==0 && ERRF==0);
```
- ENTDAA 一次只能给一个 Target 分配地址,所以循环处理最多 4 个 Target(`target_payload[4]`)。
- 48-bit PID 是按"字节序小端、整体大端"组合的(每个字节按发送顺序移位):低 32 位 + 高 16 位。
- ENTDAA 完成后,该 demo 直接进入 DMA 收发,而 **`Check_ZeroIBI` 示例**在此调用 `I3C_Ctrl_ConfigBusDevices` 把 Target 写进"总线设备表",从而允许 IBI/CR 等中断。

### 5.3 DMA 收发
固定 DMA1 Channel4 = TX,Channel5 = RX,请求号 `DMA_MuxChannel4/5 = 83/84`。
```c
void DMA_Tx_Init(... DMA_CHx, I3C->TDBR, send_buf, len) {
    DIR = PeripheralDST;      // 内存 → 外设
    PeripheralInc = Disable;  // TDBR 固定
    MemoryInc     = Enable;
    Priority     = VeryHigh;
}
void DMA_Rx_Init(... DMA_CHx, I3C->RDBR, recv_buf, len) {
    DIR = PeripheralSRC;      // 外设 → 内存
    Priority     = High;
}
```
发送流程:`DMA_Cmd → I3C_DMAReq_TXCmd(ENABLE) → I3C_ControllerHandleMessage(addr, len, WR, PRIVATE, STOP) → DMA_Cmd(ENABLE)`,等 `DMA_FLAG_TC4`,再等 `I3C_EVR_FCF`(帧完成)。
接收流程对称(`RX`, `RDBR`, `DMA1_Channel5`, `TC5`)。

> ⚠️ 顺序细节:`I3C_ControllerHandleMessage` 必须先于 `DMA_Cmd(ENABLE)` 调用(否则 FIFO 启动得太早数据会被丢)。demo 中在 `I3C_DMAReq_TXCmd` 后立刻 `I3C_ControllerHandleMessage`,但 `DMA_Cmd` 在 `HandleMessage` 之后才打开。

### 5.4 等待与错误恢复
- `I3C_EVR_FCF` — Frame Complete(成功)。
- `I3C_EVR_ERRF` — Error(查 `I3C_CEVR_CERRF` 细分)。
- 超时机制:轮询计数 > 5000 视为失败(5000 个 NOP ≈ 50 µs @ 100 MHz,实际工程中需要根据总线速率放大)。

---

## 6. 示例二:`I3C_DMA_Target`

```c
I3C_TgtConfInit.Identifier          = 0xC6;     // 48-bit 静态地址示例
I3C_TgtConfInit.MIPIVendorID        = 0x0;
I3C_TgtConfInit.MIPIIdentifier      = 0xa;
I3C_TgtConfInit.CtrlRoleRequest     = DISABLE;
I3C_TgtConfInit.HotJoinRequest      = DISABLE;
I3C_TgtConfInit.IBIRequest          = DISABLE;  // 不主动 IBI
I3C_TgtConfInit.IBIPayload_EN       = DISABLE;
I3C_TgtConfInit.IBIPayloadSize      = I3C_IBIPayloadSize_None;
I3C_TgtConfInit.MaxReadDataSize     = 0xFF;
I3C_TgtConfInit.MaxWriteDataSize    = 0xFF;
I3C_TgtConfInit.CtrlCapability      = DISABLE;  // 不能成为 Controller
I3C_TgtConfInit.GroupAddrCapability = DISABLE;
I3C_TgtConfInit.DataTurnAroundDuration = I3C_DataTurnAroundDuration_Mode0;
I3C_TgtConfInit.MaxReadTurnAround   = 0;
I3C_TgtConfInit.MaxDataSpeed        = I3C_MaxDataSpeed_Format_Mode1;
I3C_TgtConfInit.MaxSpeedLimitation  = DISABLE;
I3C_TgtConfInit.HandOffActivityState= I3C_HandOffActivityState_0;
I3C_TgtConfInit.HandOffDelay        = DISABLE;
I3C_TgtConfInit.PendingReadMDB      = DISABLE;

I3C_Tgt_Init(0xfe);    // 设定 Bus Available Duration = 0xfe
I3C_Tgt_Config(&I3C_TgtConfInit);
I3C_SetConfigFifo(&...);
I3C_Cmd(ENABLE);
```

### 6.1 Target 启动后的工作
```c
I3C_TxPreloadConfig(10);   // 告诉硬件:我即将发出 10 字节
I3C_DMA_Transmit(trasmit_buf, 10);   // 配置 DMA1_Ch4 + 83,打开 TX DMA
I3C_DMA_Recive (recive_buf, 10);     // 配置 DMA1_Ch5 + 84,打开 RX DMA
// 然后等 DMA1_FLAG_TC5
```
- `I3C_TxPreloadConfig(N)` 是 Target 端特有的"预加载"机制,硬件会等待 SCL/SDA 上有 Controller 发起匹配地址的 SDR 读,然后自动用 DMA 把 N 字节送出去。
- Target 端没有 `ControllerHandleMessage`,数据流完全由 Controller 触发;Target 只在硬件请求时(通过 DMA + FIFO)把数据送出。
- 默认开启 RX + TX 同时进行,这样 Controller 一次私有写、一次私有读都能被同步处理。
- **没有显式 ENTDAA / RSTDAA**:Target 端只需要保持配置,等待 Controller 通过广播 ENTDAA 给自己分配地址。

### 6.2 Target 端 DMA 注意
- Target 的 `I3C_DMA_Transmit/Recive` 不等待 DMA 完成,只配置并使能。Controller 来读时,Target 端 DMA 自然完成。
- `I3C_TxPreloadConfig` 的预加载长度必须 ≥ 实际要发送的长度;否则最后几个字节需要 `I3C_RequestTransfer()` 触发。

---

## 7. 示例三:`I3C_Controller_Check_ZeroIBI`

这是 demo 系列里**唯一使用中断**的例子,演示 Controller 如何监测 Target 的"零长度 IBI"事件。

### 7.1 额外动作
```c
NVIC_EnableIRQ(I3C_ER_IRQn);     // 使能事件中断
I3C_ITConfig(I3C_IT_IBIIE, ENABLE);   // IBI 中断
I3C_ITConfig(I3C_IT_CRIE , ENABLE);   // Controller Role Request
I3C_ITConfig(I3C_IT_HJIE , ENABLE);   // Hot-Join
// ENTDAA 成功后打开帧完成中断
I3C_ITConfig(I3C_IT_FCIE , ENABLE);
```

### 7.2 ENTDAA 后注册总线设备(关键差异)
```c
I3C_DeviceConfTypeDef I3C_DeviceConf = {
    .DeviceIndex        = I3C_DeviceIndex_1,
    .TargetDynamicAddr  = target_daa1,
    .IBIAck             = ENABLE,    // 允许控制器 ACK IBI
    .IBIPayload         = DISABLE,   // ZeroIBI:不带 payload
    .CtrlRoleReqAck     = ENABLE,    // 允许控制器接管角色
    .CtrlStopTransfer   = ENABLE,
};
I3C_Ctrl_ConfigBusDevices(&I3C_DeviceConf);
```
"ZeroIBI" = Target 主动发起 IBI 但 **不** 携带 payload(只有地址)。开启后,Target 拉低 SDA(START),Controller 收到地址并 ACK,完成一次中断通知。

### 7.3 中断处理
```c
void I3C_EV_IRQHandler(void) {
    if (I3C_GetITStatus(I3C_IT_IBIIE)) {
        printf("addr 0x%x cnt %d\n", I3C_GetIBITargetAddr(), I3C_GetNbIBIAddData());
        printf("data: %x\n", I3C->IBIDR);
        I3C_ClearITPendingBit(I3C_IT_IBIIE);
    } else if (I3C_GetITStatus(I3C_IT_CRIE))  { ... }
      else if (I3C_GetITStatus(I3C_IT_HJIE))  { ... }
      else if (I3C_GetITStatus(I3C_IT_FCIE))  {
        printf("data: %x\n", I3C->IBIDR);
        I3C_ClearITPendingBit(I3C_IT_FCIE);
      }
}
```
- IBIDR(IBI Data Register)在 FC 中再次读取,可能是为了观察紧随 IBI 后的数据(若 payload 非零)。
- 注意 **没清除 IBIEND / ERR 中断**,实际工程中应补全。

### 7.4 对应的 Target 端配置
要让 ZeroIBI 工作,Target 必须设置 `IBIRequest = ENABLE` 且 `IBIPayload_EN = DISABLE`、`IBIPayloadSize = None`。demo 系列没有提供对应的 Target 工程,但 `I3C_DMA_Target` 关闭了 IBI,只在另一个 demo 中提供(在仓库里未找到,可能要自己改 `I3C_TgtConfInit`)。

---

## 8. 跨 demo 共同的注意点

1. **FIFO 阈值**:三个示例都用 `1/4`,即尽早通知应用层(降低延迟,但增加中断频率)。可调到 `4/4` 提升吞吐。
2. **DMA Mux 通道**:`I3C_TX_mux=83`,`I3C_RX_mux=84`。HAL 没有提供宏,需要在用户层定义;若改其它 DMA 通道必须同步更新 mux。
3. **轮询 vs 中断**:DMA Demo 用轮询(`while (FCF == RESET)`),ZeroIBI 用中断。生产代码推荐 **DMA + 关键中断(IBI/CR/HJ)**,轮询只保留超时兜底。
4. **ENAS 失败处理**:每次 `I3C_ControllerHandleCCC/Message` 后必须清 `FCF` 和 `ERRF`,否则下一次传输会被卡住。
5. **多 Target 场景**:ENTDAA 演示只处理 4 个 Target(`target_payload[4]`)。更多 Target 需要循环 + 总线设备表空间,`I3C_Ctrl_ConfigBusDevices` 一次配置 1 个 slot,共 4 个 slot(`I3C_DeviceIndex_1..4`)。
6. **Controller 复位**:`I3C_CONTROLLER_ResetCmd` 只复位 SIE(串行接口引擎),不会复位寄存器。所以初始上电要先 `I3C_DeInit()`。
7. **双核访问**:CH32H417 双核共享 I3C 外设,如果 V3F / V5F 都要用,需要 HSEM 互斥;示例中只让 V3F 操作,规避了这个问题。

---

## 9. 我自己的总结/疑问

- **ENC/Bus 初始化顺序**:`I3C_Ctrl_Init` → `I3C_Ctrl_Config` → `I3C_SetConfigFifo` → `I3C_Cmd` 是固定套路,顺序不能错(尤其 `Init` 写入 `CRINIT`)。
- **Target 的动态地址**:`I3C_Tgt_Init(0xfe)` 中的 0xfe 是 **Bus Available Duration**(总线空闲延时阈值),不是动态地址。Target 在 ENTDAA 时被 Controller 写入动态地址,自身不需要预知。
- **ZeroIBI 与 IBI 区别**:ZeroIBI = 仅地址(1 byte),普通 IBI 可附 payload(1~4 字节,由 `IBIPayloadSize` 决定)。
- **示例的局限性**:
  - 三个示例都依赖 `printf`,没有 release/deinit 流程。
  - `Check ZeroIBI` 缺 Target 端示例,要测试 IBI 需要在 Target 上开启 `IBIRequest`。
  - 错误处理只有"轮询超时返回 1",没有细分的错误类型日志。
  - 时序参数全是 demo 默认(IOF line = `0x88~0x89` 等),对应较保守的速率(约 1 MHz 左右)。高速 PNR/HDR 模式不在示例中。
- **可以拓展的功能**(自我练习方向):
  - CCC `SETMWL` / `SETMRL` 设置,`GETMWL` / `GETMRL` 读取,完整演示 GETBCR/GETDCR。
  - Target 端使能 IBI,结合 `I3C_TargetHandleMessage(I3C_TARGET_MTYPE_IBI, 1)`。
  - 加入 HJ(Hot-Join)演示——Target 给芯片设置 `HotJoinRequest = ENABLE`,断电再上电(或模拟)看 HJ 流程。
  - 把示例改成基于中断 + 双 buffer 的轮询收发(配合 FIFO 阈值调高)。

---

## 10. 关键文件路径速查

| 文件 | 作用 |
| --- | --- |
| `CH32H417TEVT/EXAM/I3C/I3C_DMA_Controller/Common/hardware.c` | Controller + DMA 主流程 |
| `CH32H417TEVT/EXAM/I3C/I3C_DMA_Target/Common/hardware.c` | Target + DMA 主流程 |
| `CH32H417TEVT/EXAM/I3C/I3C_Controller_Check_ZeroIBI/Common/hardware.c` | Controller ZeroIBI 中断示例 |
| `CH32H417TEVT/EXAM/I3C/*/V3F/User/main.c` | V3F 启动入口(三种 Run_Core) |
| `CH32H417TEVT/EXAM/I3C/*/V5F/User/main.c` | V5F 启动入口(默认空转) |
| `CH32H417TEVT/EXAM/SRC/Peripheral/inc/ch32h417_i3c.h` | HAL 头文件(类型/枚举/API 声明) |
| `CH32H417TEVT/EXAM/SRC/Peripheral/src/ch32h417_i3c.c` | HAL 实现 |
