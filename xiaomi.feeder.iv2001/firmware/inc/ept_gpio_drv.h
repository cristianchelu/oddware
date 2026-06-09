/*
 * IV2001 GPIO pinmux — spec/10-hardware/pinmap.md
 *
 * UART0 console on GPIO21/22 (module flash pads). Do not use mt7682_hdk EVK
 * pinmux: EVK drives JTAG, UART1, BT/WiFi antenna pins that conflict with
 * feeder peripherals (CS1270, AW9523B, TM1637, etc.).
 */

#ifndef _EPT_GPIO_DRV_H
#define _EPT_GPIO_DRV_H

#define MODE_0  0
#define MODE_1  1
#define MODE_2  2
#define MODE_3  3
#define MODE_4  4
#define MODE_5  5
#define MODE_6  6
#define MODE_7  7
#define MODE_8  8
#define MODE_9  9
#define MODE_10 10
#define MODE_NC 0

#define PULL_ENABLE   1
#define PULL_DISABLE  0
#define DIR_INPUT     0
#define DIR_OUTPUT    1
#define GPIO_PORTNULL_MODE           0
#define GPIO_PORTNULL_DIR            0
#define GPIO_PORTNULL_OUTPUT_LEVEL   0
#define GPIO_PORTNULL_PU             0
#define GPIO_PORTNULL_PD             0
#define GPIO_PORTNULL_R0             0
#define GPIO_PORTNULL_R1             0
#define GPIO_PORTNULL_PUPD           0
#define GPIO_PORTNULL_DOUT           0

/* Pin modes — IV2001 application wiring */
#define GPIO_PORT0_MODE   MODE_0   /* hopper low-fill IR (GPIO until PWM) */
#define GPIO_PORT1_MODE   MODE_0   /* TM1637 DIO */
#define GPIO_PORT2_MODE   MODE_0   /* UART1 unused (factory validation only) */
#define GPIO_PORT3_MODE   MODE_0
#define GPIO_PORT4_MODE   MODE_3   /* EINT4 — AW9523B INT */
#define GPIO_PORT5_MODE   MODE_0
#define GPIO_PORT6_MODE   MODE_0
#define GPIO_PORT7_MODE   MODE_0
#define GPIO_PORT8_MODE   MODE_0
#define GPIO_PORT9_MODE   MODE_0
#define GPIO_PORT10_MODE  MODE_0
#define GPIO_PORT11_MODE  MODE_0   /* CS1270 UART2 — mux deferred to weighing bring-up */
#define GPIO_PORT12_MODE  MODE_0
#define GPIO_PORT13_MODE  MODE_0   /* TM1637 CLK */
#define GPIO_PORT14_MODE  MODE_0   /* AW9523B RST (active-low, held deasserted) */
#define GPIO_PORT15_MODE  MODE_0   /* AW9523B I2C SCL — I2C1 mux in i2c_bus_adapter */
#define GPIO_PORT16_MODE  MODE_0   /* AW9523B I2C SDA — I2C1 mux in i2c_bus_adapter */
#define GPIO_PORT17_MODE  MODE_6   /* AUXADC0 — battery/motor sense mux */
#define GPIO_PORT18_MODE  MODE_NC
#define GPIO_PORT19_MODE  MODE_NC  /* EVK UART0 — not used on IV2001 */
#define GPIO_PORT20_MODE  MODE_NC
#define GPIO_PORT21_MODE  MODE_1   /* UTXD0 — UART0 TX (console) */
#define GPIO_PORT22_MODE  MODE_1   /* URXD0 — UART0 RX (console) */

#define GPIO_PORT0_DIR   DIR_INPUT
#define GPIO_PORT1_DIR   DIR_OUTPUT
#define GPIO_PORT2_DIR   DIR_INPUT
#define GPIO_PORT3_DIR   DIR_INPUT
#define GPIO_PORT4_DIR   DIR_INPUT
#define GPIO_PORT5_DIR   DIR_INPUT
#define GPIO_PORT6_DIR   DIR_INPUT
#define GPIO_PORT7_DIR   DIR_INPUT
#define GPIO_PORT8_DIR   DIR_INPUT
#define GPIO_PORT9_DIR   DIR_INPUT
#define GPIO_PORT10_DIR  DIR_INPUT
#define GPIO_PORT11_DIR  DIR_INPUT
#define GPIO_PORT12_DIR  DIR_INPUT
#define GPIO_PORT13_DIR  DIR_OUTPUT
#define GPIO_PORT14_DIR  DIR_OUTPUT
#define GPIO_PORT15_DIR  DIR_INPUT
#define GPIO_PORT16_DIR  DIR_INPUT
#define GPIO_PORT17_DIR  DIR_INPUT
#define GPIO_PORT18_DIR  DIR_INPUT
#define GPIO_PORT19_DIR  DIR_INPUT
#define GPIO_PORT20_DIR  DIR_INPUT
#define GPIO_PORT21_DIR  DIR_INPUT
#define GPIO_PORT22_DIR  DIR_INPUT

#define GPIO_PORT0_OUTPUT_LEVEL   0
#define GPIO_PORT1_OUTPUT_LEVEL   0
#define GPIO_PORT2_OUTPUT_LEVEL   0
#define GPIO_PORT3_OUTPUT_LEVEL   0
#define GPIO_PORT4_OUTPUT_LEVEL   0
#define GPIO_PORT5_OUTPUT_LEVEL   0
#define GPIO_PORT6_OUTPUT_LEVEL   0
#define GPIO_PORT7_OUTPUT_LEVEL   0
#define GPIO_PORT8_OUTPUT_LEVEL   0
#define GPIO_PORT9_OUTPUT_LEVEL   0
#define GPIO_PORT10_OUTPUT_LEVEL  0
#define GPIO_PORT11_OUTPUT_LEVEL  0
#define GPIO_PORT12_OUTPUT_LEVEL  0
#define GPIO_PORT13_OUTPUT_LEVEL  0
#define GPIO_PORT14_OUTPUT_LEVEL  1   /* AW9523B reset inactive */
#define GPIO_PORT15_OUTPUT_LEVEL  0
#define GPIO_PORT16_OUTPUT_LEVEL  0
#define GPIO_PORT17_OUTPUT_LEVEL  0
#define GPIO_PORT18_OUTPUT_LEVEL  0
#define GPIO_PORT19_OUTPUT_LEVEL  0
#define GPIO_PORT20_OUTPUT_LEVEL  0
#define GPIO_PORT21_OUTPUT_LEVEL  0
#define GPIO_PORT22_OUTPUT_LEVEL  0

#define GPIO_PORT0_PU   0
#define GPIO_PORT1_PU   1   /* TM1637 DIO pull-up */
#define GPIO_PORT2_PU   0
#define GPIO_PORT3_PU   0
#define GPIO_PORT4_PU   1
#define GPIO_PORT5_PU   0
#define GPIO_PORT6_PU   0
#define GPIO_PORT7_PU   0
#define GPIO_PORT8_PU   0
#define GPIO_PORT9_PU   0
#define GPIO_PORT10_PU  0
#define GPIO_PORT11_PU  0
#define GPIO_PORT12_PU  0
#define GPIO_PORT13_PU  1   /* TM1637 CLK pull-up */
#define GPIO_PORT14_PU  0
#define GPIO_PORT15_PU  1
#define GPIO_PORT16_PU  1
#define GPIO_PORT17_PU  0
#define GPIO_PORT18_PU  0
#define GPIO_PORT19_PU  0
#define GPIO_PORT20_PU  0
#define GPIO_PORT21_PU  0
#define GPIO_PORT22_PU  1   /* UART0 RX */
#define GPIO_PORT0_PD   0
#define GPIO_PORT1_PD   0
#define GPIO_PORT2_PD   0
#define GPIO_PORT3_PD   0
#define GPIO_PORT4_PD   0
#define GPIO_PORT5_PD   0
#define GPIO_PORT6_PD   0
#define GPIO_PORT7_PD   0
#define GPIO_PORT8_PD   0
#define GPIO_PORT9_PD   0
#define GPIO_PORT10_PD  0
#define GPIO_PORT11_PD  0
#define GPIO_PORT12_PD  0
#define GPIO_PORT13_PD  0
#define GPIO_PORT14_PD  0
#define GPIO_PORT15_PD  0
#define GPIO_PORT16_PD  0
#define GPIO_PORT17_PD  0
#define GPIO_PORT18_PD  0
#define GPIO_PORT19_PD  0
#define GPIO_PORT20_PD  0
#define GPIO_PORT21_PD  0
#define GPIO_PORT22_PD  0

#define GPIO_PORT11_PUPD  0
#define GPIO_PORT11_R1    0
#define GPIO_PORT11_R0    0
#define GPIO_PORT12_PUPD  0
#define GPIO_PORT12_R1    0
#define GPIO_PORT12_R0    0
#define GPIO_PORT13_PUPD  0
#define GPIO_PORT13_R1    0
#define GPIO_PORT13_R0    0
#define GPIO_PORT14_PUPD  0
#define GPIO_PORT14_R1    0
#define GPIO_PORT14_R0    0
#define GPIO_PORT15_PUPD  0
#define GPIO_PORT15_R1    0
#define GPIO_PORT15_R0    0
#define GPIO_PORT16_PUPD  0
#define GPIO_PORT16_R1    0
#define GPIO_PORT16_R0    0

#define EPT_GPIO_PIN_MASK_0   0x7fffff

#endif /* _EPT_GPIO_DRV_H */
