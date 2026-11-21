# Strip_Games — WS2812 灯带街机（三合一）

把 [`CH32X033_WS2812_Arcade_Games.md`](CH32X033_WS2812_Arcade_Games.md) 的设计落地
成一个可编译、可上板、可 USB 升级的工程：

- **驱动**：`WS2812_Panel_IAP/User/ws2812.c` 原样复用（PIOC 硬件单线、限流、抖动）
- **框架**：`IAP_USB_SIGNED` 的应用侧（链接在 `0x2800`，10 KB boot + 50 KB app）
- **游戏**：能量对波 / 拆弹轮盘 / 1D 跑酷，逻辑移植自设计文档

`WS2812_Panel_IAP` 和 `IAP_USB_SIGNED` **没有任何改动**，本目录是独立工程。

## 目录

```
Strip_Games/
├── User/
│   ├── main.c            三个游戏 + 蜂鸣器音效 + 帧循环
│   ├── strip_usb.c/.h    USB CDC 命令（INFO/PING/SET_GAME/SET_BRIGHT/UPGRADE）
│   ├── board_config.h    封装/引脚表（LQFP48 / TSSOP20）
│   ├── game_config.h     LED 数、帧率、亮度、驱动宏
│   ├── wsp_config.h      驱动宏的转 include（ws2812.c 依赖）
│   ├── ws2812.c/.h       ← 拷贝自 WS2812_Panel_IAP
│   ├── RGB1W.c/.h        ← PIOC 机器码
│   ├── wsp_gamma.c/.h    ← gamma/占空比表
│   ├── wsp_tick.c/.h     ← TIM3 1 ms tick
│   ├── usb_cdc.c/.h      ← CDC 端点（PID 1A86:FE0E）
│   ├── usb_desc.c/.h     ← 描述符，产品名 "Strip Games"
│   ├── app_iap.c/.h      ← 写 0xF100 UPGRADE flag + 复位
│   └── system_* / ch32x035_* ← 标准外设库胶水
├── Ld/Link.ld            app @0x2800 / 50 KB，RAM 16 KB（高 4 KB 归 PIOC）
├── tools/build.sh        编译 + 签名打包 + factory.bin
├── tools/stripctl.py     主机侧 USB 调试工具
├── GAMEPLAY.md           玩法规则、时序、颜色/音效对照
└── .gitignore            build/
```

## 硬件接线

| 功能       | LQFP48（参考板，当前开发目标） | CH32X033 TSSOP20 | 说明                                  |
| ---------- | ------------------------------ | ---------------- | ------------------------------------- |
| WS2812 DIN | **PC7**                        | **PC18**         | PIOC IO0；PC7 走重映射，保留 SDI 调试 |
| P1 按键    | PA0                            | PA0              | 内部上拉，按下接地                    |
| P2 按键    | PA1                            | PA1              | 内部上拉，按下接地                    |
| 无源蜂鸣器 | **PA2 / TIM2_CH3**             | PA2 / TIM2_CH3   | 50% 方波，不需要外部驱动              |
| USB        | PC16 / PC17                    | PC16 / PC17      | CDC，`1A86:FE0E`                      |

> 设计文档里蜂鸣器写的是 `PA8 / TIM1_CH1`，但 `PA8` 在 TSSOP20 上不存在，
> 这里统一改成 `PA2 / TIM2_CH3`（PinMux 默认复用里就有 `T2C3`）。

供电建议（来自设计文档）：灯带 `VDD` 串一颗 1N4007 降到约 4.3 V，让 3.3 V 逻辑
高电平稳定过阈值；灯带电源并 470~1000 µF 电解电容防爆闪拉低 USB 5 V。
软件已把整体亮度限制在 25%、帧限流 350 mA，普通 USB 口可以带 48 颗。

## 玩法与按键

完整规则（判定条件、时序、颜色/音效含义、调参）见 [`GAMEPLAY.md`](GAMEPLAY.md)，速查：

| 模式 | 游戏     | 操作                                                                                   |
| ---- | -------- | -------------------------------------------------------------------------------------- |
| 0    | 能量对波 | P1 在左、P2 在右；短按发射，长按 ≥0.5 s 松开打蓄力重击；击穿对面扣 1 格护盾（各 5 格） |
| 1    | 拆弹轮盘 | 红点来回反弹，进入中央绿区时按任意键拆弹；成功一次后加速、绿区缩小，失手重置           |
| 2    | 1D 跑酷  | P1 跳跃（玩家变青色，0.37 s 滞空），P2 发射黄色激光击毁红色障碍                        |

**同时按住 P1+P2 5 秒**（`GAME_SWITCH_MS`）切下一款游戏。按住期间游戏画面和操作完全
不受影响（双键仍按蓄力/射击正常参与），只有满 5 秒才切换；切完需松开双键才能再切。
游戏内任何时候都可以用 USB `SET_GAME` / `stripctl.py --game N` 切换（`--games` 查看）。

## 构建

```sh
sh tools/build.sh                 # LQFP48，默认 48 颗灯
sh tools/build.sh -DGAME_SWITCH_MS=8000    # 切游戏双键长按改成 8 秒
sh tools/build.sh --tssop20       # CH32X033 TSSOP20（仅编译期验证，暂无板）
sh tools/build.sh -DGAME_LEDS=30  # 换灯带长度（24..120 体验最好）
sh tools/build.sh --no-pack       # 只出 elf/bin
sh tools/build.sh --standalone    # 链接在 0x0，纯调试用（不经过 boot）
```

`build/` 产物：

| 文件                                       | 用途                                                     |
| ------------------------------------------ | -------------------------------------------------------- |
| `strip_games.elf/.bin/.map`                | 应用本体，链接在 `0x2800`                                |
| `strip_games.img`                          | 签名镜像，给 USB 升级用                                  |
| `factory.bin`                              | **boot + app + 头页 + flag 页**，61952 B，调试器一次刷完 |
| `factory_payload.bin` / `factory_meta.bin` | 需要分区写入的烧录器用                                   |
| `strip_games_standalone.elf/.bin`          | `--standalone` 时生成，链接在 `0x0`                      |

`factory.bin` 需要 `IAP_USB_SIGNED/build/boot/boot.bin`，没有就先编译 boot：

```sh
cd ../IAP_USB_SIGNED && sh tools/build.sh boot
```

## 烧录与升级

```sh
# 1) 调试器一次刷完（生产/换板）
sh ../IAP_USB_SIGNED/tools/flash.sh factory build/factory.bin

# 2) 现场升级：app 已经在跑时，先回 boot
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM62 --reboot-to-boot
#   等 1A86:FE0E 变成 1A86:FE0D（COM 号通常会变）
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM61 --image build/strip_games.img

# 3) 调游戏逻辑：不经过 boot，0x0 直接跑，可单步/断点
sh tools/build.sh --standalone
#   烧 build/strip_games_standalone.elf（会覆盖 boot，调完按 1) 刷回来）
```

> 调试器侧不能"只刷 app"：WCH OpenOCD 的 `program` 会整片擦除，会冲掉 `0xF000`
> 头页和 `0xF100` flag 页。调试器永远用 `factory.bin`，换应用走 USB。

无按键升级手势：升级入口只有 USB 命令，按键只负责玩和切游戏。

## USB 协议

帧格式与 boot 完全一致：

```
A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8        CRC8 poly 0x07, init 0
回复：CMD | 0x80，其余字段相同
```

| CMD    | 名称       | payload | 说明                                             |
| ------ | ---------- | ------- | ------------------------------------------------ |
| `0x01` | INFO       | —       | 回 `ver, leds, fps, game, bright, 0,0,0`         |
| `0x02` | PING       | ≤56 B   | 原样回显                                         |
| `0x10` | SET_GAME   | 1 B     | `0` 能量对波 / `1` 拆弹 / `2` 跑酷               |
| `0x11` | SET_BRIGHT | 1 B     | 驱动全局亮度 0..255（RAM，掉电恢复默认）         |
| `0x20` | UPGRADE    | —       | 先 ACK，再写 `0xF100` UPGRADE flag 并复位进 boot |

主机侧工具：

```sh
python tools/stripctl.py --port COM62 --info
python tools/stripctl.py --port COM62 --ping 00112233
python tools/stripctl.py --port COM62 --game 2
python tools/stripctl.py --port COM62 --bright 64
python tools/stripctl.py --port COM62 --upgrade
```

`stripctl.py` 复用 `IAP_USB_SIGNED/tools/iap_flash.py` 的 `Link`（同一套帧/CRC
实现），只需要 `pyserial`。

## 尺寸与余量

| 封装    | flash（`.bin`） | 占 50 KB 槽 | RAM（data+bss，另加 2 KB 栈） | 占 16 KB |
| ------- | --------------- | ----------- | ----------------------------- | -------- |
| LQFP48  | 20280 B         | 40%         | 3496 B                        | 21%      |
| TSSOP20 | 15424 B         | 30%         | 3480 B                        | 21%      |

LQFP48 比 TSSOP20 多约 4.9 KB，差在调试串口的 `printf`（`%u/%s` 走 libc）。

`Ld/Link.ld` 有 `ASSERT(_ebss <= ...)`，RAM 溢出直接编译失败；FLASH 由
`LENGTH = 50K` 兜底。

## 和 `WS2812_Panel_IAP` 的关系

| 项          | `WS2812_Panel_IAP`        | 这里                                   |
| ----------- | ------------------------- | -------------------------------------- |
| 用途        | 蛇形灯带 + 图片/特效/字库 | 48 颗一维灯带的小游戏                  |
| WS2812 驱动 | PIOC                      | **同一份拷贝，未改**                   |
| 链接        | `0x2800` / 50 KB          | 相同                                   |
| USB PID     | `0xFE0E`                  | 相同                                   |
| gamma       | 开                        | **关**（游戏是加色混合，gamma 会失真） |
| 限流        | 按面板几何算              | 350 mA 固定                            |

## 注意

- **TSSOP20 尚未实测**（手头没有板子），只保证编译通过、引脚表与 PinMux CSV 一致。
  该封装上 WS2812 用 PC18，会关掉 SDI，调试只能 `--standalone` + LQFP48。
- **boot 只剩 <400 B 余量**（9964 / 10240），不要往 boot 加功能；需要更多能力请
  另开 boot 版本。
- **`-flto` 不可用**：会让 weak 向量覆盖 strong handler，上板后卡在
  `mcause=0x8000000e`。`tools/build.sh` 里已经去掉。
- **PIOC 占用 RAM 高 4 KB**（`0x20004000..0x20004FFF`），所以 C 运行时只给 16 KB。
- **跳转必须走 `SW_Handler`**：直接函数指针跳 `0x2800` 会因 `csrw 0xbc0` 非法指令卡死。
- key 变了要重新打包：`iap_pack.py` 与 boot 共用
  `IAP_USB_SIGNED/boot/User/img_key.c`；`-D` 覆盖时两边必须一致。
