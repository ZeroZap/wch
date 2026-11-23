# N1_Console_Pixels — WS2812B 卡带掌机（五合一，纯灯板）

> **本工程是纯 WS2812B 产品**：菜单和游戏全部画在 16×16 蛇形灯板，
> **无 OLED**（`N1_BOARD_HAS_OLED = 0`，OLED 代码编译成空操作）。
> OLED 版本见 [`../N1_Console`](../N1_Console/)。

把 `WS2812_Panel_IAP` 的 16×16 灯板、`Strip_Games` 的游戏框架和
`IAP_USB_SIGNED` 的签名 boot 合成一个**可插卡、可升级、带 OLED 菜单**的掌机：

- **主屏**：16×16 WS2812 蛇形面板（现有 LQFP48 开发板）
- **副屏**：0.96" SSD1306 OLED（PA10/PA11，软件 I2C，可选）
- **游戏**：贪吃蛇 / 打砖块 / 太空入侵者 / 像素鸟 / 西蒙记忆
- **卡带**：5 款游戏是独立的 30 KB 镜像，打包进外挂 W25Q32；菜单按需把选中游戏
  搬到片内游戏槽再跳转
- **菜单**：WS2812 图标 + OLED 游戏列表/最高分
- **升级**：现有 10 KB IAP boot 保留，签名校验只覆盖 20 KB 启动器；换游戏不擦
  启动器，换启动器不擦游戏
- **音频**：PA3/TIM2_CH4 无源蜂鸣器；同一脚预留 38 kHz 红外发射
- **封装**：LQFP48 全功能 / TSSOP20 精简，一套源码编译期切换

> 硬件原理图、引脚表、BOM、电源树见 [`HARDWARE.md`](HARDWARE.md)。
> 玩法与卡带制作见 [`GAMES.md`](GAMES.md)。
> KiCad 工程见 [`hardware/kicad/`](hardware/kicad/)。

## 两级 boot + 卡带

```text
第一级  IAP_USB_SIGNED boot (0x0000, 10 KB)
        签名校验 / USB CDC 升级 / 上电按住 UP 停留 / 刷坏救回
              ↓ 校验通过，SW_Handler 跳到 0x2800
第二级  N1_Console 启动器 (0x2800, 20 KB)
        OLED/WS2812 菜单 + NOR 卡带目录 + 游戏加载器 + USB 卡带上载
              ↓ 从 NOR 复制到片内 0x7800，CRC32 校验
第三级  游戏镜像 (0x7800, 30 KB)
        独立 .bin，自己初始化驱动，B 长按复位回启动器
```

片内 62 KB 分区：

```text
0x0000..0x27FF   boot        10 KB
0x2800..0x77FF   launcher    20 KB   签名镜像
0x7800..0xEFFF   game slot   30 KB   从 NOR 复制
0xF000 / 0xF100  镜像头 / flag 页
0xF200..0xF7FF   预留
```

IAP boot 正常升级只按签名头里的 `img_len` 擦除/编程；`IAP_CMD_ERASE` 也改成
“有有效头时只擦启动器长度”，所以 **升级启动器不会擦掉游戏槽**。游戏槽本身不在
签名范围内，由启动器在加载前做 CRC32 校验。

## 目录

```text
N1_Console/
├── User/
│   ├── main.c             菜单、卡带加载、USB 命令调度
│   ├── cart.c/.h          NOR 目录解析、游戏复制/CRC、跳转、卡带上载
│   ├── game_flash.c/.h    片内游戏槽擦写（0x08007800）
│   ├── game_runtime.c/.h  游戏镜像运行时（驱动初始化 + OLED HUD + 复位）
│   ├── oled.c/.h          SSD1306 128x64 软件 I2C
│   ├── panel_gfx.c/.h     16x16 帧缓冲、3x5 字库、调色板
│   ├── input.c/.h         6 键消抖与边沿
│   ├── audio.c/.h         TIM2_CH4 蜂鸣器音符队列
│   ├── nor_store.c/.h     SPI NOR + 设置/最高分双槽 + 原始读写
│   ├── n1_usb.c/.h        USB CDC 命令（含 CART_*）
│   ├── game.h             游戏模块接口（id/code/name/init/tick/score）
│   ├── game_util.c        游戏结束画面
│   ├── games/
│   │   ├── game_snake.c ... game_simon.c   游戏逻辑
│   │   └── main_snake.c ... main_simon.c   每个镜像的 main()
│   ├── ws2812.c/.h        ← 复用 WS2812_Panel_IAP
│   ├── RGB1W.c/.h         ← PIOC 机器码
│   ├── wsp_gamma.c/.h     ← 调色表（本工程关闭 gamma）
│   ├── wsp_tick.c/.h      ← TIM3 1 ms
│   ├── usb_cdc.c/.h       ← CDC 端点
│   ├── usb_desc.c/.h      ← 描述符，PID 1A86:FE0F
│   ├── app_iap.c/.h       ← 写 0xF100 UPGRADE flag 并复位
│   └── system_* / ch32x035_* 标准库胶水
├── Ld/Link.ld             启动器 @0x2800 / 20 KB
├── Ld/Game.ld             游戏 @0x7800 / 30 KB
├── tools/build.sh         编译启动器 + 签名 + factory.bin
├── tools/build_games.sh   编译 5 个游戏 .bin
├── tools/n1pack.py        生成/上传 cart.bin
├── tools/games.json       卡带元数据（名称/代号/主题色/图标）
├── tools/n1ctl.py         主机侧 USB 工具
├── tools/gen_kicad.py     从 pinmap.csv 生成 KiCad 工程
├── tools/check_kicad.py   KiCad S-expression 结构自检
├── hardware/              pinmap.csv / BOM.csv / kicad/
├── HARDWARE.md            综合原理图 / 共用外设
├── GAMES.md               玩法、卡带制作、调参
└── README.md
```

## 硬件速查

> 现有 `WS2812_Panel_IAP` LQFP48 板只有 1 个用户键（PA0）。要直接试玩，
> 需按 [`HARDWARE.md` 11.0 节](HARDWARE.md#110-在现有-ws2812_panel_iap-板上先玩起来)
> 飞 5 根线到轻触开关（另一端 GND，内部上拉），蜂鸣器、OLED 可选。

| 功能                 | LQFP48（默认）        | TSSOP20              |
| -------------------- | --------------------- | -------------------- |
| WS2812 DIN           | **PC7**               | **PC18**（SDI 失效） |
| NOR CS/SCK/MISO/MOSI | PA2 / PA5 / PA6 / PA7 | 同左                 |
| 卡座检测             | PB12                  | PB12                 |
| USB                  | PC16 / PC17           | PC16 / PC17          |
| 蜂鸣器 / IR TX       | PA3 / TIM2_CH4        | PA3 / TIM2_CH4       |
| IR RX                | PB1                   | 无                   |
| UP / DOWN            | PA0 / PA1             | PA0 / PA1            |
| LEFT / RIGHT         | PA4 / PA8             | PC14 / PC15          |
| A / B                | PA9 / PB0             | PC1 / PB1            |
| OLED（可选）         | PA10 / PA11           | 无                   |
| 调试串口             | PB10 / PB11           | 无                   |
| SWD                  | PC18 / PC19           | PC18 被 WS2812 占用  |

## 构建

```sh
# 1) 启动器（20 KB，链接在 0x2800）
sh tools/build.sh
sh tools/build.sh --tssop20       # TSSOP20 引脚表
sh tools/build.sh --standalone    # 0x0 调试版

# 2) 5 个游戏镜像（30 KB，链接在 0x7800）
sh tools/build_games.sh
sh tools/build_games.sh --tssop20

# 3) 卡带镜像（目录 + 5 个游戏，约 76 KB）
python tools/n1pack.py
```

`build/` 产物：

| 文件                       | 用途                                                        |
| -------------------------- | ----------------------------------------------------------- |
| `n1_console.elf/.bin/.map` | 启动器，链接在 `0x2800`                                     |
| `n1_console.img`           | 启动器签名镜像，给 USB 升级用                               |
| `factory.bin`              | **boot + 启动器 + 头页 + flag 页**，61952 B，调试器一次刷完 |
| `games/snake.bin ...`      | 5 个游戏镜像，链接在 `0x7800`                               |
| `cart.bin`                 | NOR 卡带镜像（目录 4 KB + 游戏数据，4 KB 对齐）             |

### 尺寸

| 版本           | flash           | RAM（data+bss） | 说明                 |
| -------------- | --------------- | --------------- | -------------------- |
| 启动器 LQFP48  | 18012 B / 20 KB | 7764 B / 16 KB  | 含 OLED/USB/卡带加载 |
| 启动器 TSSOP20 | 16280 B / 20 KB | 6740 B / 16 KB  | OLED/USB 调试裁剪    |
| 游戏 snake     | 11440 B         | 5496 B          |                      |
| 游戏 breakout  | 15372 B         | 5456 B          |                      |
| 游戏 invaders  | 14704 B         | 5472 B          |                      |
| 游戏 flappy    | 13868 B         | 5440 B          |                      |
| 游戏 simon     | 11612 B         | 5480 B          |                      |
| `cart.bin`     | 77824 B         | —               | 5 款游戏             |

## 烧录、卡带上载与升级

```sh
# 1) 调试器一次刷完 boot + 启动器（生产/换板）
sh ../IAP_USB_SIGNED/tools/flash.sh factory build/factory.bin

# 2) 第一次使用：进入菜单后，通过 USB 上传卡带
python tools/n1pack.py --upload --port COM62
#    或先 python tools/n1pack.py，再
python tools/n1ctl.py --port COM62 --cart-upload build/cart.bin

# 3) 现场升级启动器
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM62 --reboot-to-boot
#    等 1A86:FE0F 变成 1A86:FE0D
python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM61 --image build/n1_console.img

# 4) 只想单步调某个游戏：
sh tools/build_games.sh --standalone
#    烧 build/games/<name>_standalone.elf 到 0x0（会覆盖 boot，调完刷回 factory.bin）
#    正常玩法不需要这一步
```

> 调试器侧不能“只刷启动器”：WCH OpenOCD 会整片擦除。调试器永远用
> `factory.bin`；换启动器走 USB，换游戏走卡带。

### 进入 boot 的三个入口

1. **上电按住 UP（PA0）**：boot 读到低电平，停在 `FE0D`。
2. **菜单里按住 UP+DOWN 2 秒**：启动器写 `0xF100` flag 并复位。
3. **USB 命令 `--upgrade`**：效果同上。

## USB 协议

帧格式与 boot 完全一致：

```text
A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8      CRC8 poly 0x07, init 0
回复：CMD | 0x80，其余字段相同
```

| CMD    | 名称         | payload | 说明                                                           |
| ------ | ------------ | ------- | -------------------------------------------------------------- |
| `0x01` | INFO         | —       | `ver,w,h,count,current,state,bright,sound,nor,cart_valid,oled` |
| `0x02` | PING         | ≤56 B   | 原样回显                                                       |
| `0x10` | SELECT_GAME  | 1 B     | 从卡带加载第 N 款并跳转；`0xFF` 回菜单                         |
| `0x11` | SET_BRIGHT   | 1 B     | 全局亮度 8..255                                                |
| `0x12` | SET_SOUND    | 1 B     | 0/1                                                            |
| `0x20` | UPGRADE      | —       | 先 ACK，再写 flag 并复位进 boot                                |
| `0x21` | MENU         | —       | 回菜单（启动器内）                                             |
| `0x30` | SCORES       | —       | 回最多 8×uint16 最高分                                         |
| `0x31` | RESET_SCORES | —       | 清零最高分                                                     |
| `0x40` | CART_INFO    | —       | `valid,count,total u32,jedec u32,max_game u16`                 |
| `0x41` | CART_BEGIN   | 8 B     | `total u32, crc32 u32`，擦除卡带区                             |
| `0x42` | CART_DATA    | ≤56 B   | 顺序写入，返回状态字节                                         |
| `0x43` | CART_END     | —       | 校验 CRC、重读目录，返回状态                                   |
| `0x44` | CART_ABORT   | —       | 放弃上传                                                       |
| `0x45` | CART_ERASE   | —       | 擦除目录 + 当前卡带数据区                                      |
| `0x46` | CART_LIST    | 1 B     | 返回 `name[12],code[4],theme,flags,size u32,crc32 u32`         |
| `0x47` | OLED_SCAN    | —       | 扫 I2C 总线，返回 `count,addr7...`（确认 OLED 是否在线）       |
| `0x48` | OLED_INIT    | —       | 重新初始化 OLED（热插拔用），返回 1 字节 present               |

主机工具：

```sh
python tools/n1ctl.py --port COM62 --info
python tools/n1ctl.py --port COM62 --games
python tools/n1ctl.py --port COM62 --game 2
python tools/n1ctl.py --port COM62 --bright 64
python tools/n1ctl.py --port COM62 --sound 0
python tools/n1ctl.py --port COM62 --scores
python tools/n1ctl.py --port COM62 --cart-info
python tools/n1ctl.py --port COM62 --cart-erase
python tools/n1ctl.py --port COM62 --cart-upload build/cart.bin
python tools/n1ctl.py --port COM62 --upgrade
python tools/n1ctl.py --port COM62 --oled-scan
python tools/n1ctl.py --port COM62 --oled-init
```

`n1ctl.py` 复用 `IAP_USB_SIGNED/tools/iap_flash.py` 的 `Link`（同一套帧/CRC），
只需要 `pyserial`。

## 操作

| 场景   | 按键                      | 作用                   |
| ------ | ------------------------- | ---------------------- |
| 菜单   | UP / DOWN                 | 选择游戏（长按连发）   |
| 菜单   | A                         | 从 NOR 加载并启动      |
| 菜单   | B                         | 声音开/关              |
| 菜单   | 按住 UP+DOWN 2 s          | 进入 USB boot          |
| 无卡带 | A                         | 重新扫描卡带           |
| 游戏内 | 按住 B 0.75 s             | 复位回启动器           |
| 游戏内 | 按住 UP+DOWN 0.75 s       | 同上（B 没接时的备用） |
| 游戏内 | 见 [`GAMES.md`](GAMES.md) |                        |

## 存储与卡带

片内：

- 启动器 20 KB（签名）
- 游戏槽 30 KB（每次从 NOR 复制，CRC32 校验，掉电不会影响启动器）
- 设置/最高分在 NOR `0x000000/0x001000` 双槽

外挂 W25Q32：

| 地址                    | 用途                                |
| ----------------------- | ----------------------------------- |
| `0x000000` / `0x001000` | 设置/最高分双槽（A/B 交替）         |
| `0x010000`              | 卡带目录 4 KB（`N1CA`，最多 32 条） |
| `0x011000` 起           | 游戏数据，4 KB 对齐；每款 ≤30 KB    |

卡带目录条目包含名称、3 字母代号、主题色、NOR 绝对偏移、大小、CRC32 和
16×10 单色图标。启动器只读目录，不执行 NOR 中的代码；选中游戏后复制到片内并
校验 CRC，再跳转。

## 与现有工程的关系

| 项          | `WS2812_Panel_IAP` | `Strip_Games`   | 本工程                      |
| ----------- | ------------------ | --------------- | --------------------------- |
| 用途        | 蛇形灯带特效/图片  | 48 灯 1D 小游戏 | 16×16 卡带掌机 + OLED       |
| WS2812 驱动 | PIOC               | 同一份拷贝      | 同一份拷贝                  |
| 链接        | `0x2800` / 50 KB   | 相同            | 启动器 `0x2800` / 20 KB     |
| boot        | `IAP_USB_SIGNED`   | 相同            | **相同 + ERASE 只擦启动器** |
| USB PID     | `0xFE0E`           | `0xFE0E`        | **`0xFE0F`**                |
| 游戏形态    | 2D 场景/特效       | 一维灯带        | 独立 30 KB 卡带镜像         |

## 注意

- **`-flto` 不可用**：会让 weak 向量覆盖 strong handler，上板后卡在
  `mcause=0x8000000e`；两个构建脚本都已去掉。
- **PIOC 占 RAM 高 4 KB**（`0x20004000..0x20004FFF`），C 运行时只有 16 KB；
  `Ld/*.ld` 有 `ASSERT`，溢出会编译失败。
- **跳转必须走 `SW_Handler`**：启动器跳游戏用软件中断，游戏复位回启动器；
  不要用普通函数指针跳 `0x7800`。
- **TSSOP20 上 WS2812 用 PC18**，会关闭 SDI；调试请用 LQFP48 + `--standalone`。
- **游戏 ID 顺序**：`SNAKE=0, BREAKOUT=1, INVADERS=2, FLAPPY=3, SIMON=4`，
  与 `tools/games.json` 的顺序一致；最高分按这个下标存。
- **`WS2812_PIOC_PIN` 必须由 `wsp_config.h` 从 `WSP_WS2812_PIOC_PIN` 派生**
  （`#define WS2812_PIOC_PIN WSP_WS2812_PIOC_PIN`）。漏掉会落到 `ws2812.c` 的
  `#else` PC18 分支：执行 `GPIO_Remap_SWJ_Disable` 关掉 **SDI**（WCH-Link 连不上），
  且 WS2812 输出错到 PC18（灯板不亮）。LQFP48 必须是 PC7 + `GPIO_Remap_PIOC`。
- **OLED 地址**：SSD1306 `0x3C`（8 位写 `0x78`），PA10/PA11 需 4.7k 上拉。
  驱动上电会先做总线恢复，并自动探测 `0x3C` / `0x3D`，初始化失败会重试一次。

## OLED 不亮排查

OLED 只有在**启动器运行**且**地址被 ACK** 时才点亮（IAP boot 阶段屏幕是黑的）。

1. 先看 16×16 灯板有没有菜单/图标：
   - 有 → 启动器在跑，问题在 OLED；
   - 没有 → 固件没跑起来，先用调试器刷 `build/factory.bin`。
2. USB 连上后读在线状态：

   ```sh
   python tools/n1ctl.py --port COMxx --info
   python tools/n1ctl.py --port COMxx --oled-scan
   ```

   `--oled-scan` 会扫整条 I2C 总线：
   - 列出 `0x3C`/`0x3D` → MCU 已经看到屏，接线/供电 OK，问题在初始化/对比度；
   - `no device answered` → 接线/供电/地址问题，按下面逐项查。

   `--info` 的 `OLED` 字段：
   - `not found` → 接线/地址/模块类型问题：
     - **CH32X035 EVT 板**：SCL→PA10（**P2 第 14 脚**）、SDA→PA11（**P1 第 12 脚**）；
       VCC→**P2 第 2 脚**（3.3V），GND→任一 GND。注意 **P1 第 1/2 脚是 LED，不是电源**；
       P4 的 SCL/SDA 是 USB-PD 控制器的 I2C，不是 OLED 座；
     - SCL=PA10、SDA=PA11，别接反；
     - SCL/SDA 各 4.7k 上拉到 **3.3V**（模块自带的可省）；
     - 模块电源用 **3.3V**（5V 裸屏可能不工作）；
     - 4 脚 I2C 版；7 脚 SPI 版不支持；
     - 地址可能是 `0x3D`（驱动已自动试）；
   - `present` 但黑屏 → 多为 SH1106/兼容屏（132×64，需列偏移），
     或对比度/复位差异，可换标准 SSD1306 模块验证。

3. 万用表静态量 SCL/SDA 应约为 3.3V；刷新时 SCL 会快速翻转。

- key 变了要重新打包：`iap_pack.py` 与 boot 共用
  `IAP_USB_SIGNED/boot/User/img_key.c`；`-D` 覆盖时两边必须一致。
