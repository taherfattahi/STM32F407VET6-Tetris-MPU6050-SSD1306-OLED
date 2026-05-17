# STM32F407VET6 Tetris — MPU6050 + SSD1306 OLED

A bare-metal Tetris game running on the **STM32F407VET6 Black Board**, controlled by tilting the board using the **MPU-6050 IMU**, and displayed on a **0.96" SSD1306 128×64 OLED** over I2C.

---

## Table of Contents

- [STM32F407VET6 Tetris — MPU6050 + SSD1306 OLED](#stm32f407vet6-tetris--mpu6050--ssd1306-oled)
  - [Table of Contents](#table-of-contents)
  - [Hardware Required](#hardware-required)
  - [Wiring](#wiring)
    - [I2C Address Summary](#i2c-address-summary)
    - [Wiring Diagram](#wiring-diagram)
  - [Project Structure](#project-structure)
  - [Module Descriptions](#module-descriptions)
    - [`main.c` — Clock Setup \& Entry Point](#mainc--clock-setup--entry-point)
      - [Clock Tree](#clock-tree)
      - [Flow](#flow)
    - [`ssd1306.h` / `ssd1306.c` — OLED Driver](#ssd1306h--ssd1306c--oled-driver)
      - [Architecture](#architecture)
      - [Frame Buffer Layout](#frame-buffer-layout)
      - [I2C Protocol](#i2c-protocol)
      - [Public API](#public-api)
    - [`mpu6050.h` / `mpu6050.c` — IMU Driver](#mpu6050h--mpu6050c--imu-driver)
      - [Initialisation Registers](#initialisation-registers)
      - [Burst Read](#burst-read)
      - [Physical Axis Orientation](#physical-axis-orientation)
      - [Public API](#public-api-1)
    - [`tetris.h` / `tetris.c` — Game Engine](#tetrish--tetrisc--game-engine)
      - [Key Constants (`tetris.h`)](#key-constants-tetrish)
      - [Tetromino Data Structure](#tetromino-data-structure)
      - [Game Loop Flow](#game-loop-flow)
      - [Collision Detection](#collision-detection)
      - [Line Clear \& Scoring](#line-clear--scoring)
      - [Wall Kick](#wall-kick)
      - [Spawn \& Game Over](#spawn--game-over)
  - [Game Controls](#game-controls)
  - [Screen Layout](#screen-layout)
  - [Contributors](#contributors)
  - [License](#license)

---

## Hardware Required

| Component | Description |
|---|---|
| STM32F407VET6 Black Board | Main microcontroller (168 MHz Cortex-M4) |
| SSD1306 0.96" OLED (I2C) | 128×64 monochrome display |
| MPU-6050 | 6-axis accelerometer + gyroscope (I2C) |
| Jumper wires | 4 wires per module (VCC, GND, SCL, SDA) |

---

## Wiring

Both the OLED and MPU-6050 share the same **I2C1 bus** (PB6 = SCL, PB7 = SDA). They have different I2C addresses so they coexist without conflict.

```
STM32F407VET6          SSD1306 OLED (I2C)
─────────────          ──────────────────
PB6  (SCL) ────────────── SCL
PB7  (SDA) ────────────── SDA
3.3V       ────────────── VCC
GND        ────────────── GND


STM32F407VET6          MPU-6050
─────────────          ────────
PB6  (SCL) ────────────── SCL
PB7  (SDA) ────────────── SDA
3.3V       ────────────── VCC
GND        ────────────── GND
GND        ────────────── AD0  ← sets I2C address to 0x68
```

### I2C Address Summary

| Device | AD0/SA0 Pin | I2C Address |
|---|---|---|
| SSD1306 OLED | SA0 → GND | 0x3C |
| MPU-6050 | AD0 → GND | 0x68 |

### Wiring Diagram

```
                    ┌─────────────────────────┐
                    │   STM32F407VET6 Black   │
                    │                         │
                    │  PB6 ──────────┬────────┼──── SCL (OLED)
                    │                │        │
                    │  PB7 ──────────┼──┬─────┼──── SDA (OLED)
                    │                │  │     │
                    │  3.3V ─────────┼──┼──┬──┼──── VCC (OLED)
                    │                │  │  │  │
                    │  GND ──────────┼──┼──┼──┼──── GND (OLED)
                    │                │  │  │  │
                    │                └──┼──┼──┼──── SCL (MPU)
                    │                   └──┼──┼──── SDA (MPU)
                    │                      └──┼──── VCC (MPU)
                    │  GND ─────────────────┬─┼──── GND (MPU)
                    │                       └─┼──── AD0 (MPU)
                    └─────────────────────────┘
```

> **Pull-up resistors:** The STM32 GPIO is configured with internal pull-ups (enabled in software), so no external resistors are needed for this short-wire setup. For longer wires add 4.7kΩ pull-ups from SDA and SCL to 3.3V.

---

## Project Structure

```
gpio-example/
│
├── Inc/
│   ├── ssd1306.h          ← OLED display driver API
│   ├── mpu6050.h          ← IMU driver API
│   └── tetris.h           ← Game constants and public API
│
├── Src/
│   ├── main.c             ← Clock config (168 MHz) + main loop
│   ├── ssd1306.c          ← SSD1306 I2C driver + frame buffer + font
│   ├── mpu6050.c          ← MPU-6050 I2C driver + burst read
│   ├── tetris.c           ← Full game engine (board, pieces, render, tilt)
│   ├── system_stm32f4xx.c ← CMSIS system file (defines SystemCoreClock)
│   ├── syscalls.c         ← Standard library stubs
│   └── sysmem.c           ← Heap memory stubs
│
├── Startup/
│   └── startup_stm32f407vetx.s  ← Reset handler, vector table
│
├── STM32F407VETX_FLASH.ld       ← Linker script (flash placement)
└── STM32F407VETX_RAM.ld         ← Linker script (RAM placement)
```

---

## Module Descriptions

---

### `main.c` — Clock Setup & Entry Point

**Responsibility:** Configure the STM32F407 to run at 168 MHz and start the game.

#### Clock Tree

```
HSE Crystal (8 MHz)
        │
        ▼
   PLL Input (/M=8) → 1 MHz
        │
        ▼
   PLL VCO (×N=336) → 336 MHz
        │
        ├──(/P=2)──► SYSCLK = 168 MHz
        │                │
        │         AHB  (/1) = 168 MHz  (AHB bus, GPIO, DMA)
        │         APB1 (/4) =  42 MHz  ← I2C1 lives here
        │         APB2 (/2) =  84 MHz
        │
        └──(/Q=7)──► USB clock = 48 MHz
```

#### Flow

```
main()
  │
  ├── SystemClock_Config()    set PLL → 168 MHz
  │
  ├── Tetris_Init()           init display, IMU, game state, SysTick
  │
  └── while(1)
        └── Tetris_Run()      one full game tick (move + gravity + render)
```

---

### `ssd1306.h` / `ssd1306.c` — OLED Driver

**Responsibility:** Drive the SSD1306 128×64 OLED over I2C using a software frame buffer.

#### Architecture

```
Application code calls:
  SSD1306_DrawPixel()
  SSD1306_FillRect()
  SSD1306_DrawString()
          │
          ▼
  ┌───────────────────┐
  │   Frame Buffer    │  1024 bytes in RAM
  │  (128×64 / 8)     │  each byte = 8 vertical pixels (one page-column)
  └───────────────────┘
          │
    SSD1306_UpdateScreen()   ← call this to push buffer to display
          │
          ▼
  I2C1 → SSD1306 OLED (address 0x3C)
```

#### Frame Buffer Layout

The SSD1306 organises its RAM into 8 "pages" of 128 bytes each. Each byte encodes 8 vertical pixels:

```
Page 0: rows 0-7    (byte bit 0 = row 0, bit 7 = row 7)
Page 1: rows 8-15
Page 2: rows 16-23
...
Page 7: rows 56-63

Byte index = x + (y/8) * 128
Bit  index = y % 8
```

#### I2C Protocol

```
START → [0x3C | W] → 0x00 (command mode) → CMD_BYTE → STOP
START → [0x3C | W] → 0x40 (data mode)    → DATA...  → STOP
```

#### Public API

| Function | Description |
|---|---|
| `SSD1306_Init()` | Initialise I2C1 GPIO, configure SSD1306 registers, clear screen |
| `SSD1306_Clear()` | Zero the frame buffer (does not send to display) |
| `SSD1306_UpdateScreen()` | Flush entire frame buffer to display over I2C |
| `SSD1306_DrawPixel(x,y,color)` | Set or clear one pixel in the buffer |
| `SSD1306_FillRect(x,y,w,h,color)` | Filled rectangle |
| `SSD1306_DrawRect(x,y,w,h,color)` | Outline rectangle |
| `SSD1306_DrawChar(x,y,c,color)` | Draw one ASCII character (5×7 bitmap font) |
| `SSD1306_DrawString(x,y,str,color)` | Draw a null-terminated string |

---

### `mpu6050.h` / `mpu6050.c` — IMU Driver

**Responsibility:** Read raw accelerometer and gyroscope data from the MPU-6050 over the shared I2C1 bus.

#### Initialisation Registers

| Register | Value | Meaning |
|---|---|---|
| PWR_MGMT_1 (0x6B) | 0x00 | Wake up from sleep |
| SMPLRT_DIV (0x19) | 0x07 | Sample rate = 1 kHz / (1+7) = 125 Hz |
| CONFIG (0x1A) | 0x03 | Digital low-pass filter ~44 Hz |
| GYRO_CONFIG (0x1B) | 0x00 | Gyro range ±250 °/s |
| ACCEL_CONFIG (0x1C) | 0x00 | Accel range ±2 g (16384 LSB/g) |

#### Burst Read

All 14 bytes (accel XYZ + temp + gyro XYZ) are read in one I2C transaction starting at register `0x3B`:

```
Bytes 0-1:   AX high, AX low
Bytes 2-3:   AY high, AY low
Bytes 4-5:   AZ high, AZ low
Bytes 6-7:   TEMP (unused)
Bytes 8-9:   GX high, GX low
Bytes 10-11: GY high, GY low
Bytes 12-13: GZ high, GZ low
```

#### Physical Axis Orientation

```
          +Y (forward — top of board away from you)
           ↑
           │
  -X ──────┼────── +X (right)
           │
           ↓
          -Y (backward — top of board toward you)

  +Z = up (out of the chip surface)
```

#### Public API

| Function | Description |
|---|---|
| `MPU6050_Init()` | Wake chip, configure ranges, verify WHO_AM_I. Returns 0 on success |
| `MPU6050_Read(d)` | Burst-read all 6 axes into `MPU6050_Data_t` struct |

---

### `tetris.h` / `tetris.c` — Game Engine

**Responsibility:** Everything game-related — pieces, board, collision, scoring, tilt input, and rendering.

#### Key Constants (`tetris.h`)

| Constant | Value | Meaning |
|---|---|---|
| `BOARD_COLS` | 10 | Tetris standard width |
| `BOARD_ROWS` | 12 | Rows visible on 64px OLED (12×5=60px) |
| `CELL_SIZE` | 5 | Pixels per cell |
| `BOARD_X_OFFSET` | 2 | Left edge of board on display |
| `BOARD_Y_OFFSET` | 2 | Top edge of board on display |
| `PANEL_X` | 56 | X start of score/next panel |

#### Tetromino Data Structure

All 7 pieces × 4 rotations × 4 cells are stored as `(row, col)` offsets:

```c
pieces[type][rotation][cell] = { row_offset, col_offset }
```

Example — the I piece:

```
Rotation 0:   [ ][ ][ ][ ]     offsets: (0,0)(0,1)(0,2)(0,3)
Rotation 1:   [ ]
              [ ]
              [ ]               offsets: (0,2)(1,2)(2,2)(3,2)
              [ ]
```

#### Game Loop Flow

```
Tetris_Run()  called every ~33ms
      │
      ├─► read_tilt()
      │       │
      │       ├── ax/ay compared against TILT_THRESHOLD (5000 raw units ≈ 0.3g)
      │       └── 200ms repeat guard prevents multiple fires per tilt
      │
      ├─► Apply gesture:
      │       ├── TILT_LEFT   → cur_col-- (if piece_fits)
      │       ├── TILT_RIGHT  → cur_col++ (if piece_fits)
      │       ├── TILT_ROTATE → cur_rot = (cur_rot+1)&3  + wall-kick ±1 col
      │       └── TILT_DROP   → while(piece_fits down) cur_row++; lock; spawn
      │
      ├─► Gravity tick (every fall_interval ms):
      │       ├── piece_fits(row+1) → cur_row++
      │       └── else → lock_piece() → clear_lines() → update_score() → spawn_piece()
      │
      └─► Render:
              ├── SSD1306_Clear()
              ├── draw_board()   ← board[][] cells + active piece
              ├── draw_panel()   ← score, level, next piece preview
              └── SSD1306_UpdateScreen()
```

#### Collision Detection

```c
piece_fits(type, rotation, row, col):
  for each of 4 cells:
    r = row + offset.r
    c = col + offset.c
    if r or c out of bounds  → FAIL
    if board[r][c] != 0      → FAIL
  return OK
```

#### Line Clear & Scoring

```
After piece locks → scan every row bottom to top:
  if all 10 cols filled → shift everything above down by 1 → repeat

Lines cleared → score += line_scores[lines] × (level + 1)

Line scores:
  1 line  →   40 pts
  2 lines →  100 pts
  3 lines →  300 pts
  4 lines → 1200 pts  (Tetris!)

Level = score / 500  (capped at 15)
Fall interval = 800ms - (level × 46ms)
  Level  0 → 800 ms/row
  Level 15 → ~100 ms/row
```

#### Wall Kick

When rotating, if the rotated piece overlaps a wall or existing block, the engine tries shifting it ±1 column before giving up:

```
Try rotate at col   → fail
Try rotate at col+1 → success → accept
Try rotate at col-1 → success → accept
All fail            → rotation blocked
```

#### Spawn & Game Over

```
spawn_piece():
  cur_type  = next_type
  cur_row   = 0
  cur_col   = BOARD_COLS/2 - 2   (near top-centre)
  next_type = rand_piece()

  if NOT piece_fits(cur_type, 0, cur_row, cur_col):
      gameState = GAME_OVER       ← board is full
```

---

## Game Controls

| Physical Action | Axis Change | Game Action |
|---|---|---|
| Tilt board left | ay negative | Move piece LEFT |
| Tilt board right | ay positive | Move piece RIGHT |
| Tilt board backward | ax negative | **Rotate** piece |
| Tilt board forward | ax positive | **Hard drop** (instant) |

> **Tip:** The tilt threshold is `5000` raw units (~0.3g). Adjust `TILT_THRESHOLD` in `tetris.c` if the controls feel too sensitive or too sluggish.

---

## Screen Layout

```
 col→  0        49  55 56         127
        ┌──────────┬───┬───────────┐  row 0
        │          │   │  SCR      │
        │  Play    │   │  00000    │
        │  Field   │   │  LVL      │
        │          │   │    0      │
        │  10×12   │   │  NXT      │
        │  cells   │   │   [][]    │
        │          │   │   [][]    │
        │██████████│   │           │
        └──────────┴───┴───────────┘  row 63

        ◄──50px──►     ◄───72px───►
```

- Play field: 10 cols × 12 rows × 5px = 50×60 pixels, offset 2px from top-left
- Right panel: score, level number, and next piece 4-cell preview

---

## Contributors
Contributions are welcome! Feel free to fork the repo and submit pull requests. 🚀

---

## License
This project is licensed under the MIT License.
