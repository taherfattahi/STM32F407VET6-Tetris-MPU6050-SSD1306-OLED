#include "stm32f4xx.h"
#include "tetris.h"

/* ─────────────────────────────────────────────────────────────────────────────
 *  System clock  – 168 MHz via HSE (8 MHz) + PLL
 *  Adjust HSE_VALUE and PLL parameters to match your board's crystal.
 * ───────────────────────────────────────────────────────────────────────────*/
static void SystemClock_Config(void)
{
    /* Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* Enable power interface clock; scale voltage for 168 MHz */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR      |= PWR_CR_VOS;   /* VOS = 1 (scale 1, max performance) */

    /* Flash latency for 168 MHz @ 3.3 V: 5 wait states */
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN
               | FLASH_ACR_LATENCY_5WS;

    /*
     * PLL: SYSCLK = HSE(8) / M(8) * N(336) / P(2) = 168 MHz
     *      USB FS = HSE(8) / M(8) * N(336) / Q(7)  =  48 MHz
     */
    RCC->PLLCFGR = (8u  << RCC_PLLCFGR_PLLM_Pos)   /* M = 8   */
                 | (336u << RCC_PLLCFGR_PLLN_Pos)   /* N = 336 */
                 | (0u  << RCC_PLLCFGR_PLLP_Pos)    /* P = 2   */
                 | RCC_PLLCFGR_PLLSRC_HSE
                 | (7u  << RCC_PLLCFGR_PLLQ_Pos);   /* Q = 7   */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* Bus prescalers: AHB=/1, APB1=/4 (42 MHz), APB2=/2 (84 MHz) */
    RCC->CFGR = RCC_CFGR_HPRE_DIV1
              | RCC_CFGR_PPRE1_DIV4
              | RCC_CFGR_PPRE2_DIV2;

    /* Switch to PLL */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* Update CMSIS SystemCoreClock variable */
    SystemCoreClock = 168000000UL;
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Main
 * ───────────────────────────────────────────────────────────────────────────*/
int main(void)
{
    SystemClock_Config();   /* 168 MHz */
    Tetris_Init();          /* init display, IMU, game state, SysTick */

    while (1) {
        Tetris_Run();       /* one game-loop tick (~30 fps) */
    }
}
