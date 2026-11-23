# N1_Console 玩法与调参

主屏为 16×16 逻辑像素，WS2812 蛇形走线由 `panel_gfx.c` 自动映射。
所有游戏共用 60 Hz 帧循环、6 键输入和蜂鸣器音符队列。

## 菜单

| 操作             | 作用                                              |
| ---------------- | ------------------------------------------------- |
| UP / DOWN        | 上一个 / 下一个游戏，长按约 0.43 s 后连发         |
| A                | 进入当前游戏                                      |
| B                | 声音开 / 关                                       |
| 按住 UP+DOWN 2 s | 写 UPGRADE flag，复位进入 USB boot（`1A86:FE0D`） |
| 菜单底部条       | 当前游戏位置；右下角绿/红点表示声音开/关          |

菜单显示：

- 上方 16×10 区域：游戏图标
- 中间 3×5 字库：3 字母代号（`SNK` / `BRK` / `INV` / `FLY` / `SIM`）
- 最后一行：5 段位置条，当前段用游戏主题色

## 通用规则

| 项       | 规则                                                |
| -------- | --------------------------------------------------- |
| 返回菜单 | 任意游戏内按住 B 约 0.75 s                          |
| 游戏结束 | 显示 `GAME` / `OVER` + 3 位分数，2.5 s 后自动回菜单 |
| 重新开始 | 结束画面按 A 立即重开                               |
| 最高分   | 存在外挂 NOR，掉电保留；无 NOR 时只存 RAM           |
| 音效     | 菜单移动、选择、得分、碰撞、结束、胜利              |
| 亮度     | 默认 64/255，可用 USB `--bright` 调整并保存         |

## 1. 贪吃蛇（SNK）

**目标**：吃红色食物，越长分数越高，撞墙或撞自己结束。

| 操作                     | 作用                     |
| ------------------------ | ------------------------ |
| UP / DOWN / LEFT / RIGHT | 改变方向，不能 180° 回头 |

规则：

- 初始长度 3，食物随机出现在空格。
- 每吃一个食物 +10 分，移动间隔从 10 帧逐步缩短到 4 帧。
- 蛇头白色，前 3 节亮绿，其余暗绿；食物红色闪烁。
- 撞墙/撞自己进入结束画面。

调参（`User/games/game_snake.c`）：

```c
move_period = 10;      // 初始每 10 帧走一格（约 6 格/秒）
if (move_period > 4) move_period--;   // 每吃一个加速
```

## 2. 打砖块（BRK）

**目标**：用 4 像素挡板反弹球，打掉 4×8 块砖。

| 操作         | 作用     |
| ------------ | -------- |
| LEFT / RIGHT | 移动挡板 |

规则：

- 砖块 4 行 × 8 列，每块 2×1 像素，按行红/橙/黄/绿。
- 球初始速度约 0.55–0.65 像素/帧；碰挡板后根据落点改变水平速度。
- 3 条命；球落底扣命，命尽结束。
- 打光所有砖块显示 `YOU WIN`。
- 右上角青色短条表示剩余命数。

调参：

```c
#define START_LIVES   3u
ball_vx = ±0.55f;
ball_vy = -0.65f;
```

## 3. 太空入侵者（INV）

**目标**：击毁 3×5 个外星人，别被它们压到底线。

| 操作         | 作用                     |
| ------------ | ------------------------ |
| LEFT / RIGHT | 移动飞船                 |
| A            | 发射子弹（同时只有一发） |

规则：

- 外星人整体左右移动，撞到边缘下移一格并反向。
- 外星人随机发射子弹；被击中扣 1 命，短暂无敌闪烁。
- 击毁一个 +20 分；全灭显示 `YOU WIN`。
- 外星人到达飞船高度或命尽结束。
- 右下角青色短条表示剩余命数。

调参：

```c
move_period = 18;     // 数字越小，外星人移动越快
fire_timer  = 80;     // 初始开火间隔
```

## 4. 像素鸟（FLY）

**目标**：穿过绿色管道缝隙，每过一根 +1 分。

| 操作   | 作用             |
| ------ | ---------------- |
| A / UP | 拍翅膀（向上冲） |

规则：

- 重力 0.28 像素/帧²，拍翅速度 -2.0，最大下落 2.4。
- 管道宽 2 像素，缝隙高 5 像素，从右向左滚动。
- 撞管道、撞天花板/地面结束。
- 左上角 1–3 像素青色条表示分数个位。

调参：

```c
#define GAP_HEIGHT 5
bird_vy = -2.0f;
bird_vy += 0.28f;
pipe_x -= 0.55f;
```

## 5. 西蒙记忆（SIM）

**目标**：记住四色/四方向的闪烁顺序，并重复出来。

| 按键  | 对应象限      |
| ----- | ------------- |
| UP    | 左上（红）    |
| RIGHT | 右上（绿）    |
| LEFT  | 左下（蓝）    |
| DOWN  | 右下（灰/黄） |

规则：

- 每轮先播放序列，再进入输入阶段。
- 输入正确，序列加长 1 位；输入错误结束。
- 每完成一轮分数 +1，最高 32 位。
- 播放时每个象限亮起并发出不同音高（440/554/659/880 Hz）。
- 每个象限内画有方向箭头，方便盲记。

调参：

```c
#define SEQ_MAX 32u
timer = 22;   // 点亮时长（帧）
timer = 8;    // 间隔
```

## 调色板

`User/panel_gfx.h` 固定 16 色，游戏只写索引：

| 索引 | 名称      | RGB         | 用途               |
| ---- | --------- | ----------- | ------------------ |
| 0    | P_BLACK   | 0,0,0       | 背景               |
| 1    | P_WHITE   | 255,255,255 | 蛇头、挡板、箭头   |
| 2    | P_RED     | 255,0,0     | 食物、敌人、砖块   |
| 3    | P_GREEN   | 0,255,0     | 蛇身、管道         |
| 4    | P_BLUE    | 0,0,255     | 西蒙蓝             |
| 5    | P_YELLOW  | 255,255,0   | 球、鸟、分数       |
| 6    | P_CYAN    | 0,255,255   | 飞船、命数         |
| 7    | P_MAGENTA | 255,0,255   | 外星人第三行       |
| 8    | P_ORANGE  | 255,96,0    | 砖块、结束文字     |
| 9    | P_GRAY    | 96,96,96    | 未选中/暗色        |
| 10   | P_DGREEN  | 0,96,0      | 蛇身暗色           |
| 11   | P_DBLUE   | 0,0,96      | 西蒙暗蓝           |
| 12   | P_DRED    | 96,0,0      | 食物暗色、西蒙暗红 |
| 13   | P_PURPLE  | 128,0,255   | 预留               |
| 14   | P_TEAL    | 0,128,128   | 预留               |
| 15   | P_PINK    | 255,96,160  | 预留               |

全局亮度由 `ws2812_set_brightness()` 统一缩放（默认 64），游戏内不再乘 0.25，
所以颜色表可以保持满幅，换亮度时不会改变色相。

## 音效表

| SFX          | 音符                      | 场景              |
| ------------ | ------------------------- | ----------------- |
| `SFX_MOVE`   | 1200 Hz / 2 帧            | 菜单移动          |
| `SFX_SELECT` | 880 → 1320 Hz             | 进入游戏 / 开声音 |
| `SFX_BACK`   | 700 → 460 Hz              | 返回菜单          |
| `SFX_POINT`  | 1568 → 2093 Hz            | 得分/吃食物       |
| `SFX_HIT`    | 180 → 120 Hz              | 碰撞/扣命         |
| `SFX_OVER`   | 440 → 330 → 220 Hz        | 游戏结束          |
| `SFX_WIN`    | 523 → 659 → 784 → 1046 Hz | 通关              |
| `SFX_FLAP`   | 620 → 880 Hz              | 拍翅              |
| `SFX_SHOOT`  | 1400 Hz                   | 发射              |
| `SFX_BUMP`   | 260 Hz                    | 撞墙/挡板         |

音符队列长度 10，`audio_update()` 每帧推进；队列满时丢弃最新音符，不阻塞游戏。

## 卡带制作与加载

```sh
# 1) 编译 5 个游戏镜像（链接在 0x7800，30 KB 上限）
sh tools/build_games.sh

# 2) 按 tools/games.json 打包目录 + 游戏数据
python tools/n1pack.py                 # -> build/cart.bin

# 3) 菜单状态下通过 USB 上传（约 76 KB，2~4 秒）
python tools/n1pack.py --upload --port COM62
# 或
python tools/n1ctl.py --port COM62 --cart-upload build/cart.bin
```

卡带目录在 NOR `0x010000`，游戏数据从 `0x011000` 起 4 KB 对齐。启动器：

1. 上电读目录，OLED 列出名称/最高分，WS2812 显示图标 + 代号。
2. 按 A 选中游戏，启动器把该 `.bin` 从 NOR 复制到片内 `0x7800`。
3. 复制过程逐页计算 CRC32，写完后回读再校验一次。
4. 校验通过后用 `SW_Handler` 跳到 `0x7800`；失败则显示 `LOAD ERR` 留在菜单。
5. 游戏内按住 B 0.75 s 或游戏结束返回，走 `NVIC_SystemReset()` 回到启动器。

> 启动器不会执行 NOR 里的代码，也不会让游戏覆盖菜单；游戏槽与启动器物理分离。

## OLED 菜单与 HUD

- 菜单：`N1 CONSOLE` / `CART n GAMES` / 最多 5 条游戏列表 / `A PLAY B SOUND`。
- 无卡带：显示 `NO CARTRIDGE` 和 `n1ctl --cart-upload` 提示。
- 游戏内：顶部游戏名，`HI` 和 `SCORE` 每 60 帧刷新一页；底部 `HOLD B: MENU`。
- OLED 不存在时自动降级：所有 `oled_*` 变成空操作，WS2812 菜单照常工作。

## 新增游戏

1. 在 `User/games/` 新建 `game_xxx.c`，实现 `init` / `tick` / `icon` / `score`，
   并定义 `const game_desc_t game_xxx`。
2. 新建 `User/games/main_xxx.c`：

   ```c
   #include "game_runtime.h"
   extern const game_desc_t game_xxx;
   int main(void) { game_runtime_run(&game_xxx); return 0; }
   ```

3. 在 `User/game.h` 增加 `GAME_ID_XXX`（最高分下标，必须与卡带顺序一致）。
4. 在 `tools/build_games.sh` 的循环里加入 `xxx`。
5. 在 `tools/games.json` 增加名称、3 字母代号、主题色和 16×10 图标。
6. 重新执行 `build_games.sh` + `n1pack.py`，卡带目录会自动多一条。

每个游戏镜像上限 30 KB；当前最大的是 breakout 15.0 KB，余量约 15 KB。

最小骨架：

```c
#include "game.h"
#include "panel_gfx.h"

static uint16_t my_score;

static void my_init(void) { my_score = 0u; }
static uint8_t my_tick(const input_state_t *in)
{
    panel_clear();
    panel_text3x5_center(5, "HI", P_WHITE);
    if (in->pressed & BTN_A) my_score++;
    if (in->pressed & BTN_B) return 1u;
    return 0u;
}
static void my_icon(void) { panel_rect(2, 2, 12, 6, P_CYAN); }
static uint16_t my_get_score(void) { return my_score; }

const game_desc_t game_my = {
    GAME_ID_MY, "MYG", "my game", P_CYAN,
    my_init, my_tick, my_icon, my_get_score
};
```
