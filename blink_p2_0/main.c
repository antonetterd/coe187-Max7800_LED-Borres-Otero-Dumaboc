#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "gpio.h"
#include "board.h"

/* ------------ LED pin definitions ------------ */
/* First 4 LEDs on Port 2 */
#define LED_PORT_A      MXC_GPIO2
#define LED0_PIN        MXC_GPIO_PIN_3   // P2_3
#define LED1_PIN        MXC_GPIO_PIN_4   // P2_4
#define LED2_PIN        MXC_GPIO_PIN_6   // P2_6
#define LED3_PIN        MXC_GPIO_PIN_7   // P2_7
#define LED_MASK_A      (LED0_PIN | LED1_PIN | LED2_PIN | LED3_PIN)

/* Next 3 LEDs on Port 0 */
#define LED_PORT_B      MXC_GPIO0
#define LED4_PIN        MXC_GPIO_PIN_7   // P0_7
#define LED5_PIN        MXC_GPIO_PIN_5   // P0_5
#define LED6_PIN        MXC_GPIO_PIN_6   // P0_6
#define LED_MASK_B      (LED4_PIN | LED5_PIN | LED6_PIN)

/* Last LED on Port 1 */
#define LED_PORT_C      MXC_GPIO1
#define LED7_PIN        MXC_GPIO_PIN_6   // P1_6
#define LED_MASK_C      (LED7_PIN)

/* ------------ Button1 (built-in) ------------ */
#define BTN_PORT        MXC_GPIO0
#define BTN_PIN         MXC_GPIO_PIN_2   // P0_2 = BUTTON1

/* Helper: set LED pattern using lower 8 bits of 'pat'
   bit0 -> LED0 (P2_3)
   bit1 -> LED1 (P2_4)
   bit2 -> LED2 (P2_6)
   bit3 -> LED3 (P2_7)
   bit4 -> LED4 (P0_7)
   bit5 -> LED5 (P0_5)
   bit6 -> LED6 (P0_6)
   bit7 -> LED7 (P1_6)
*/
static void set_led_pattern(uint8_t pat)
{
    uint32_t maskA = 0;
    uint32_t maskB = 0;
    uint32_t maskC = 0;

    if (pat & 0x01) maskA |= LED0_PIN;  // P2_3
    if (pat & 0x02) maskA |= LED1_PIN;  // P2_4
    if (pat & 0x04) maskA |= LED2_PIN;  // P2_6
    if (pat & 0x08) maskA |= LED3_PIN;  // P2_7

    if (pat & 0x10) maskB |= LED4_PIN;  // P0_7
    if (pat & 0x20) maskB |= LED5_PIN;  // P0_5
    if (pat & 0x40) maskB |= LED6_PIN;  // P0_6

    if (pat & 0x80) maskC |= LED7_PIN;  // P1_6

    // Clear all LEDs on all ports first
    MXC_GPIO_OutClr(LED_PORT_A, LED_MASK_A);
    MXC_GPIO_OutClr(LED_PORT_B, LED_MASK_B);
    MXC_GPIO_OutClr(LED_PORT_C, LED_MASK_C);

    // Now turn on only the selected ones
    MXC_GPIO_OutSet(LED_PORT_A, maskA);
    MXC_GPIO_OutSet(LED_PORT_B, maskB);
    MXC_GPIO_OutSet(LED_PORT_C, maskC);
}

int main(void)
{
    Board_Init();
    printf("8-LED sequences with Button1 toggle\r\n");

    /* ---------------- Configure LED pins as outputs (3.3V domain) ---------------- */
    mxc_gpio_cfg_t ledsA_cfg;
    ledsA_cfg.port   = LED_PORT_A;
    ledsA_cfg.mask   = LED_MASK_A;
    ledsA_cfg.func   = MXC_GPIO_FUNC_OUT;
    ledsA_cfg.pad    = MXC_GPIO_PAD_NONE;
    ledsA_cfg.vssel  = MXC_GPIO_VSSEL_VDDIOH;   // 3.3V I/O
    MXC_GPIO_Config(&ledsA_cfg);

    mxc_gpio_cfg_t ledsB_cfg;
    ledsB_cfg.port   = LED_PORT_B;
    ledsB_cfg.mask   = LED_MASK_B;
    ledsB_cfg.func   = MXC_GPIO_FUNC_OUT;
    ledsB_cfg.pad    = MXC_GPIO_PAD_NONE;
    ledsB_cfg.vssel  = MXC_GPIO_VSSEL_VDDIOH;   // 3.3V I/O
    MXC_GPIO_Config(&ledsB_cfg);

    mxc_gpio_cfg_t ledsC_cfg;
    ledsC_cfg.port   = LED_PORT_C;
    ledsC_cfg.mask   = LED_MASK_C;
    ledsC_cfg.func   = MXC_GPIO_FUNC_OUT;
    ledsC_cfg.pad    = MXC_GPIO_PAD_NONE;
    ledsC_cfg.vssel  = MXC_GPIO_VSSEL_VDDIOH;   // 3.3V I/O
    MXC_GPIO_Config(&ledsC_cfg);

    // Start with all LEDs off
    MXC_GPIO_OutClr(LED_PORT_A, LED_MASK_A);
    MXC_GPIO_OutClr(LED_PORT_B, LED_MASK_B);
    MXC_GPIO_OutClr(LED_PORT_C, LED_MASK_C);

    /* ---------------- Configure Button1 as input with pull-up ---------------- */
    mxc_gpio_cfg_t btn_cfg;
    btn_cfg.port   = BTN_PORT;
    btn_cfg.mask   = BTN_PIN;
    btn_cfg.func   = MXC_GPIO_FUNC_IN;
    btn_cfg.pad    = MXC_GPIO_PAD_PULL_UP;     // released = HIGH, pressed = LOW
    btn_cfg.vssel  = MXC_GPIO_VSSEL_VDDIOH;
    MXC_GPIO_Config(&btn_cfg);

    int last_btn = 1;   // last sampled state (1 = released)
    int mode     = 0;   // 0 = Sequence A, 1 = Sequence B

    /* ---------------- Define the two sequences ---------------- */

    /*
     * SEQUENCE A (mode 0)
     * Order (for turning ON, 0.3 s each, latching):
     *   1) P2_4
     *   2) P2_3 + P0_7
     *   3) P1_6 + P0_5
     *   4) P2_7 + P0_6
     *   5) P2_6
     *
     * Once all are ON, they turn OFF in reverse groups, then repeat.
     *
     * Bit mapping:
     *   P2_3 -> bit0 -> 0x01
     *   P2_4 -> bit1 -> 0x02
     *   P2_6 -> bit2 -> 0x04
     *   P2_7 -> bit3 -> 0x08
     *   P0_7 -> bit4 -> 0x10
     *   P0_5 -> bit5 -> 0x20
     *   P0_6 -> bit6 -> 0x40
     *   P1_6 -> bit7 -> 0x80
     */
    const uint8_t seqA[] = {
        0x02, // step 1: P2_4
        0x13, // step 2: + P2_3 + P0_7  (P2_4 + P2_3 + P0_7)
        0xB3, // step 3: + P1_6 + P0_5  (prev + P1_6 + P0_5)
        0xFB, // step 4: + P2_7 + P0_6  (prev + P2_7 + P0_6)
        0xFF, // step 5: + P2_6         (all ON)
        0xFB, // OFF P2_6
        0xB3, // OFF P2_7 + P0_6
        0x13, // OFF P1_6 + P0_5
        0x02, // OFF P2_3 + P0_7
        0x00  // OFF P2_4 (all OFF)
    };
    const int lenA = sizeof(seqA) / sizeof(seqA[0]);
    int idxA = 0;

    /*
     * SEQUENCE B (mode 1)
     *
     * Order for single-LED blink:
     *   P1_6, P2_7, P2_6, P0_6, P0_5, P0_7, P2_4, P2_3
     *
     * Behavior:
     *   - Step 0: ALL LEDs ON for 0.3 s
     *   - Steps 1–8: only ONE LED ON at a time (others OFF),
     *                following the order above, each for 0.3 s
     *   - Then loop back to step 0.
     */
    const uint8_t seqB[] = {
        0xFF, // all ON (first 0.3 s)

        0x80, // P1_6 only
        0x08, // P2_7 only
        0x04, // P2_6 only
        0x40, // P0_6 only
        0x20, // P0_5 only
        0x10, // P0_7 only
        0x02, // P2_4 only
        0x01  // P2_3 only
    };
    const int lenB = sizeof(seqB) / sizeof(seqB[0]);
    int idxB = 0;

    while (1) {
        /* ----- Read button and detect a press (falling edge) ----- */
        int btn_now = MXC_GPIO_InGet(BTN_PORT, BTN_PIN) ? 1 : 0;

        // falling edge: released(1) -> pressed(0)
        if (last_btn == 1 && btn_now == 0) {
            mode ^= 1;  // toggle between 0 and 1
            printf("Mode changed to %d\r\n", mode);
            // Simple debounce so one physical press = one toggle
            MXC_Delay(MXC_DELAY_MSEC(150));
        }
        last_btn = btn_now;

        /* ----- Output next step of the selected sequence ----- */
        if (mode == 0) {
            // Sequence A
            set_led_pattern(seqA[idxA]);
            idxA = (idxA + 1) % lenA;
        } else {
            // Sequence B
            set_led_pattern(seqB[idxB]);
            idxB = (idxB + 1) % lenB;
        }

        // Step speed: 0.3 seconds
        MXC_Delay(MXC_DELAY_MSEC(300));
    }
}



