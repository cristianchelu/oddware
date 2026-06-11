/*
 * AW9523B bootstrap bitmaps — spec/10-hardware/pinmap.md
 */

#ifndef BOARD_GPIO_IV2001_H
#define BOARD_GPIO_IV2001_H

#include <stdint.h>

#define BOARD_GPIO_AW9523B_ADDR  0x58u

/* HAL I2C1 — GPIO15=SCL, GPIO16=SDA (pinmap.md, HAL_GPIO_15_SCL1 / _16_SDA1). */
#define BOARD_GPIO_I2C_SCL_PIN  15u
#define BOARD_GPIO_I2C_SDA_PIN  16u

/* Port P0 pin masks (bits 0–7). */
#define BOARD_GPIO_MOTOR_PH_MASK     0x01u
#define BOARD_GPIO_MOTOR_PH_PORT     0u
#define BOARD_GPIO_MOTOR_PH_PIN      0u
#define BOARD_GPIO_MOTOR_EN_MASK     0x02u
#define BOARD_GPIO_MOTOR_EN_PORT     0u
#define BOARD_GPIO_MOTOR_EN_PIN      1u
#define BOARD_GPIO_CS1270_PWR_MASK   0x04u
#define BOARD_GPIO_CS1270_PWR_PORT   0u
#define BOARD_GPIO_CS1270_PWR_PIN    2u
#define BOARD_GPIO_DISPLAY_RAIL_MASK 0x20u
#define BOARD_GPIO_INDEX_LED_MASK    0x40u
#define BOARD_GPIO_INDEX_LED_PORT    0u
#define BOARD_GPIO_INDEX_LED_PIN     6u
#define BOARD_GPIO_INDEX_DET_MASK    0x80u   /* P0.7 */
#define BOARD_GPIO_INDEX_BEAM_OPEN_HIGH  1u  /* [probe] */

#define BOARD_GPIO_ADC_MUX_PORT             1u
#define BOARD_GPIO_ADC_MUX_PIN              7u
#define BOARD_GPIO_ADC_MUX_MASK             0x80u   /* P1.7 — NC7SB3157 S */

#define BOARD_GPIO_HOPPER_SENSE_MASK        0x10u   /* P1.4 */
#define BOARD_GPIO_HOPPER_BEAM_BLOCKED_HIGH 1u      /* [probe] */
#define BOARD_GPIO_HOPPER_IR_PULSE_MS       1u      /* [tune] */

#define BOARD_GPIO_DISPLAY_RAIL_PORT  0u
#define BOARD_GPIO_DISPLAY_RAIL_PIN   5u

/*
 * Direction: 0 = output, 1 = input (AW9523B).
 * P0 outputs: 0,1,2,5,6. P0 inputs: 3,4,7.
 */
#define BOARD_GPIO_BOOT_DIR_P0  0x98u
/* P1 outputs: 7. P1 inputs: 0–6. */
#define BOARD_GPIO_BOOT_DIR_P1  0x7Fu

/*
 * Safe boot outputs: motor off, scale off, index LED off; display rail off
 * until display_rail_on() (display_hello order).
 */
#define BOARD_GPIO_BOOT_OUT_P0  0x00u
#define BOARD_GPIO_BOOT_OUT_P1  0x00u

/* Button input masks. See pinmap.md — P0.3/P0.4 active-low; P1.0 active-high. */
#define BOARD_GPIO_BTN_POWER_MASK       0x08u   /* P0.3 */
#define BOARD_GPIO_BTN_RESET_MASK       0x10u   /* P0.4 */
#define BOARD_GPIO_BTN_DISPENSE_MASK    0x01u   /* P1.0 */
#define BOARD_GPIO_BTN_DISPENSE_ACTIVE_LOW  0u

/* AW9523B IRQ mask defaults (0 = enabled). See gpio-expander-aw9523b.md */
#define BOARD_GPIO_AW9523_INT_MASK_P0  0x57u
#define BOARD_GPIO_AW9523_INT_MASK_P1  0xECu

#endif /* BOARD_GPIO_IV2001_H */
