#include "tetris.h"
#include "ssd1306.h"
#include "mpu6050.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ─────────────────────────────────────────────────────────────────────────────
 *  SysTick millisecond counter
 * ───────────────────────────────────────────────────────────────────────────*/
volatile uint32_t g_tick = 0;

void SysTick_Handler(void) { g_tick++; }

static uint32_t millis(void) { return g_tick; }

static void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) < ms);
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Tetromino definitions
 *  Each piece has 4 rotations, each rotation is 4 (row,col) offsets from origin
 * ───────────────────────────────────────────────────────────────────────────*/
typedef struct { int8_t r, c; } Cell_t;

/* I  O  T  S  Z  J  L */
static const Cell_t pieces[NUM_TETROMINOES][4][4] = {
    /* I */
    { {{0,0},{0,1},{0,2},{0,3}}, {{0,2},{1,2},{2,2},{3,2}},
      {{2,0},{2,1},{2,2},{2,3}}, {{0,1},{1,1},{2,1},{3,1}} },
    /* O */
    { {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}},
      {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}} },
    /* T */
    { {{0,0},{0,1},{0,2},{1,1}}, {{0,1},{1,1},{2,1},{1,2}},
      {{1,0},{1,1},{1,2},{0,1}}, {{0,0},{1,0},{2,0},{1,1}} },  /* fixed: symmetrical */
    /* S */
    { {{0,1},{0,2},{1,0},{1,1}}, {{0,0},{1,0},{1,1},{2,1}},
      {{0,1},{0,2},{1,0},{1,1}}, {{0,0},{1,0},{1,1},{2,1}} },
    /* Z */
    { {{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,0},{1,1},{2,0}},
      {{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,0},{1,1},{2,0}} },
    /* J */
    { {{0,0},{1,0},{1,1},{1,2}}, {{0,0},{0,1},{1,0},{2,0}},
      {{0,0},{0,1},{0,2},{1,2}}, {{0,1},{1,1},{2,0},{2,1}} },
    /* L */
    { {{0,2},{1,0},{1,1},{1,2}}, {{0,0},{1,0},{2,0},{2,1}},
      {{0,0},{0,1},{0,2},{1,0}}, {{0,0},{0,1},{1,1},{2,1}} },
};

/* ─────────────────────────────────────────────────────────────────────────────
 *  Game state
 * ───────────────────────────────────────────────────────────────────────────*/
static uint8_t     board[BOARD_ROWS][BOARD_COLS];  /* 0=empty, 1=filled */
static int8_t      cur_type, cur_rot;
static int8_t      cur_row, cur_col;
static int8_t      next_type;
static uint32_t    score;
static uint8_t     level;
static GameState_t gameState;
static uint32_t    last_fall_time;
static uint32_t    fall_interval;  /* ms between automatic drops */

/* ─────────────────────────────────────────────────────────────────────────────
 *  Simple LCG pseudo-random (seeded from SysTick)
 * ───────────────────────────────────────────────────────────────────────────*/
static uint32_t rng_state = 12345;
static int8_t rand_piece(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (int8_t)((rng_state >> 16) % NUM_TETROMINOES);
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Board helpers
 * ───────────────────────────────────────────────────────────────────────────*/
static int piece_fits(int8_t type, int8_t rot, int8_t row, int8_t col)
{
    for (int i = 0; i < 4; i++) {
        int8_t r = row + pieces[type][rot][i].r;
        int8_t c = col + pieces[type][rot][i].c;
        if (r < 0 || r >= BOARD_ROWS) return 0;
        if (c < 0 || c >= BOARD_COLS) return 0;
        if (board[r][c])              return 0;
    }
    return 1;
}

static void lock_piece(void)
{
    for (int i = 0; i < 4; i++) {
        int8_t r = cur_row + pieces[cur_type][cur_rot][i].r;
        int8_t c = cur_col + pieces[cur_type][cur_rot][i].c;
        if (r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS)
            board[r][c] = 1;
    }
}

static uint8_t clear_lines(void)
{
    uint8_t cleared = 0;
    for (int r = BOARD_ROWS - 1; r >= 0; r--) {
        int full = 1;
        for (int c = 0; c < BOARD_COLS; c++)
            if (!board[r][c]) { full = 0; break; }
        if (full) {
            /* Shift everything above down */
            for (int rr = r; rr > 0; rr--)
                memcpy(board[rr], board[rr-1], BOARD_COLS);
            memset(board[0], 0, BOARD_COLS);
            r++;   /* re-check same row */
            cleared++;
        }
    }
    return cleared;
}

static void spawn_piece(void)
{
    cur_type = next_type;
    cur_rot  = 0;
    cur_row  = 0;
    cur_col  = BOARD_COLS / 2 - 2;
    next_type = rand_piece();

    if (!piece_fits(cur_type, cur_rot, cur_row, cur_col))
        gameState = GAME_OVER;
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Rendering
 * ───────────────────────────────────────────────────────────────────────────*/
static void draw_board(void)
{
    /* Board border */
    SSD1306_DrawRect(BOARD_X_OFFSET - 1,
                     BOARD_Y_OFFSET - 1,
                     BOARD_COLS * CELL_SIZE + 2,
                     BOARD_ROWS * CELL_SIZE + 2,
                     SSD1306_COLOR_WHITE);

    /* Locked cells */
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            int px = BOARD_X_OFFSET + c * CELL_SIZE;
            int py = BOARD_Y_OFFSET + r * CELL_SIZE;
            if (board[r][c])
                SSD1306_FillRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, SSD1306_COLOR_WHITE);
            else
                SSD1306_FillRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, SSD1306_COLOR_BLACK);
        }
    }

    /* Active piece */
    for (int i = 0; i < 4; i++) {
        int8_t r = cur_row + pieces[cur_type][cur_rot][i].r;
        int8_t c = cur_col + pieces[cur_type][cur_rot][i].c;
        if (r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS) {
            int px = BOARD_X_OFFSET + c * CELL_SIZE;
            int py = BOARD_Y_OFFSET + r * CELL_SIZE;
            SSD1306_FillRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, SSD1306_COLOR_WHITE);
        }
    }
}

static void draw_panel(void)
{
    /* Score label + value */
    SSD1306_DrawString(PANEL_X, 0, "SCR", SSD1306_COLOR_WHITE);
    char buf[10];
    snprintf(buf, sizeof(buf), "%5lu", (unsigned long)score);
    SSD1306_DrawString(PANEL_X, 8, buf, SSD1306_COLOR_WHITE);

    /* Level */
    SSD1306_DrawString(PANEL_X, 18, "LVL", SSD1306_COLOR_WHITE);
    snprintf(buf, sizeof(buf), "%3u", level);
    SSD1306_DrawString(PANEL_X, 26, buf, SSD1306_COLOR_WHITE);

    /* Next piece preview */
    SSD1306_DrawString(PANEL_X, 36, "NXT", SSD1306_COLOR_WHITE);
    for (int i = 0; i < 4; i++) {
        int8_t r = pieces[next_type][0][i].r;
        int8_t c = pieces[next_type][0][i].c;
        int px = PANEL_X + c * 4 + 2;
        int py = 44     + r * 4;
        if (py < SSD1306_HEIGHT)
            SSD1306_FillRect(px, py, 3, 3, SSD1306_COLOR_WHITE);
    }
}

static void draw_game_over(void)
{
    SSD1306_Clear();
    SSD1306_DrawString(18, 20, "GAME", SSD1306_COLOR_WHITE);
    SSD1306_DrawString(18, 30, "OVER", SSD1306_COLOR_WHITE);
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)score);
    SSD1306_DrawString(18, 44, buf, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  MPU-6050 gesture detection
 * ───────────────────────────────────────────────────────────────────────────*/
#define TILT_THRESHOLD   5000   /* ~0.3 g, raw units */
#define TILT_REPEAT_MS   200    /* minimum ms between tilt actions */

typedef enum {
    TILT_NONE,
    TILT_LEFT,
    TILT_RIGHT,
    TILT_ROTATE,   /* tilt backward */
    TILT_DROP      /* tilt forward  */
} Tilt_t;

static uint32_t last_tilt_time = 0;

static Tilt_t read_tilt(void)
{
    MPU6050_Data_t d;
    MPU6050_Read(&d);

    if ((millis() - last_tilt_time) < TILT_REPEAT_MS)
        return TILT_NONE;

    if (d.ay > TILT_THRESHOLD) {
        last_tilt_time = millis();
        return TILT_RIGHT;
    }
    if (d.ay < -TILT_THRESHOLD) {
        last_tilt_time = millis();
        return TILT_LEFT;
    }
    if (d.ax < -TILT_THRESHOLD) {
        last_tilt_time = millis();
        return TILT_ROTATE;
    }
    if (d.ax > TILT_THRESHOLD) {
        last_tilt_time = millis();
        return TILT_DROP;
    }
    return TILT_NONE;
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Score / level logic
 * ───────────────────────────────────────────────────────────────────────────*/
static const uint16_t line_scores[5] = {0, 40, 100, 300, 1200};

static void update_score(uint8_t lines)
{
    if (lines > 4) lines = 4;
    score += (uint32_t)line_scores[lines] * (level + 1);
    level  = (uint8_t)(score / 500);
    if (level > 15) level = 15;
    /* Speed: from 800 ms at level 0 to 100 ms at level 15 */
    fall_interval = 800u - (uint32_t)level * 46u;
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Public API
 * ───────────────────────────────────────────────────────────────────────────*/
void Tetris_Init(void)
{
    /* SysTick at 1 ms */
    SysTick_Config(SystemCoreClock / 1000);

    SSD1306_Init();
    MPU6050_Init();

    rng_state = millis() + 42;   /* seed with startup jitter */

    memset(board, 0, sizeof(board));
    score         = 0;
    level         = 0;
    fall_interval = 800;
    next_type     = rand_piece();
    spawn_piece();
    gameState     = GAME_PLAYING;
    last_fall_time = millis();
}

void Tetris_Run(void)
{
    if (gameState == GAME_OVER) {
        draw_game_over();
        delay_ms(3000);
        /* Restart */
        Tetris_Init();
        return;
    }

    /* ── Read tilt gesture ── */
    Tilt_t tilt = read_tilt();

    if (tilt == TILT_LEFT) {
        if (piece_fits(cur_type, cur_rot, cur_row, cur_col - 1))
            cur_col--;
    }
    else if (tilt == TILT_RIGHT) {
        if (piece_fits(cur_type, cur_rot, cur_row, cur_col + 1))
            cur_col++;
    }
    else if (tilt == TILT_ROTATE) {
        int8_t new_rot = (cur_rot + 1) & 3;
        /* Wall-kick: try original col, then ±1 */
        if      (piece_fits(cur_type, new_rot, cur_row, cur_col))     cur_rot = new_rot;
        else if (piece_fits(cur_type, new_rot, cur_row, cur_col + 1)) { cur_rot = new_rot; cur_col++; }
        else if (piece_fits(cur_type, new_rot, cur_row, cur_col - 1)) { cur_rot = new_rot; cur_col--; }
    }
    else if (tilt == TILT_DROP) {
        /* Hard drop: slam to bottom */
        while (piece_fits(cur_type, cur_rot, cur_row + 1, cur_col))
            cur_row++;
        lock_piece();
        uint8_t lines = clear_lines();
        update_score(lines);
        spawn_piece();
        last_fall_time = millis();
    }

    /* ── Gravity ── */
    if ((millis() - last_fall_time) >= fall_interval) {
        last_fall_time = millis();
        if (piece_fits(cur_type, cur_rot, cur_row + 1, cur_col)) {
            cur_row++;
        } else {
            /* Lock piece, render it into the board BEFORE spawning next */
            lock_piece();

            /* Flash the locked piece on screen so player sees it land */
            SSD1306_Clear();
            draw_board();
            draw_panel();
            SSD1306_UpdateScreen();
            delay_ms(80);   /* brief pause so the landing is visible */

            uint8_t lines = clear_lines();
            update_score(lines);
            spawn_piece();
            last_fall_time = millis(); /* reset timer for new piece */
        }
    }

    /* ── Render ── */
    SSD1306_Clear();
    draw_board();
    draw_panel();
    SSD1306_UpdateScreen();
}
