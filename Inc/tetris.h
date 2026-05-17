#ifndef TETRIS_H
#define TETRIS_H

#include <stdint.h>

/* ── Play-field dimensions (each cell = CELL_SIZE pixels) ── */
#define BOARD_COLS      10
#define BOARD_ROWS      12
#define CELL_SIZE        5     /* pixels per cell */

/* ── Board pixel origin on the 128×64 OLED ── */
#define BOARD_X_OFFSET   2     /* left edge of the board */
#define BOARD_Y_OFFSET   2     /* top edge of the board  */

/* ── Right-panel origin (score / next piece) ── */
#define PANEL_X         56     /* start of info column  */

/* ── Number of tetromino shapes ── */
#define NUM_TETROMINOES  7

/* ── Game states ── */
typedef enum {
    GAME_PLAYING,
    GAME_OVER,
    GAME_PAUSED
} GameState_t;

/* ── Public API ── */
void Tetris_Init(void);
void Tetris_Run(void);       /* call repeatedly from main loop */

#endif /* TETRIS_H */
