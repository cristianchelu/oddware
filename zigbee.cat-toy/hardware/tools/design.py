"""Zigbee cat toy controller, rev A: component placement and nets.

Coordinates are sheet millimetres (A3, Y down). See docs/hardware-design.md.
"""

R0603 = 'Resistor_SMD:R_0603_1608Metric'
C0603 = 'Capacitor_SMD:C_0603_1608Metric'
C0805 = 'Capacitor_SMD:C_0805_2012Metric'
LED0603 = 'LED_SMD:LED_0603_1608Metric'
SOT23 = 'Package_TO_SOT_SMD:SOT-23'
JST_PH_SIDE = 'Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal'


# (ref, net, back-silk label): grouped on the back, see placement.py
TEST_POINTS = [
    ('TP1', 'VBAT', 'VBAT'), ('TP2', 'VDD', 'VDD'), ('TP3', 'VBUS', 'VBUS'),
    ('TP4', 'GND', 'GND'), ('TP5', 'CHG_STAT', 'nCHG'), ('TP6', 'CHG_PROG', 'PROG'),
    ('TP7', 'MOT_PWM', 'PWM'), ('TP8', 'MOT_DIR', 'DIR'), ('TP9', 'MOT_IPROPI', 'ISNS'),
    ('TP10', 'MOT_GAINSEL', 'GAIN'), ('TP11', 'SPK_PWM', 'SPK'), ('TP12', 'GND', 'GND'),
    ('TP13', 'ACC_INT1', 'INT1'), ('TP14', 'ACC_INT2', 'INT2'), ('TP15', 'I2C_SDA', 'SDA'),
    ('TP16', 'I2C_SCL', 'SCL'), ('TP17', 'BTN', 'BTN'), ('TP18', 'LED_STATUS', 'LED'),
    ('TP19', 'DBG_TX', 'TX'), ('TP20', 'DBG_RX', 'RX'),          # UART ground: J2 pin 2, right beside them
]


def build(sh):
    # ------------------------------------------------------------ battery
    sh.box(12.7, 12.7, 135, 92, 'Battery, protection, reverse polarity')
    sh.place('Connector_Generic:Conn_01x02', 'J3', 'LiPo 1S (JST-PH)', 45.72, 38.1,
             {'1': 'CELL_P', '2': 'CELL_N'},
             fp='Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal', lcsc='C173752',
             props={'Note': 'Side-entry JST-PH, hand-solder (THT). Pin 1 = +'}, in_bom=False)
    sh.place('zigbee_toy:XB5353A', 'U5', 'XB5353A', 76.2, 43.18,
             {'3': 'CELL_P', '1': 'NC', '4': 'GND', '5': 'GND', '2': 'CELL_N'}, lcsc='C80628')
    sh.passive('C', 'C15', '100n', 63.5, 66.04, 'CELL_P', 'CELL_N', C0603, 'C14663')
    sh.place('Transistor_FET:AO3401A', 'Q1', 'AO3401A', 115.57, 40.64,
             {'1': 'GND', '2': 'VBAT', '3': 'CELL_P'}, lcsc='C15127')
    sh.passive('C', 'C1', '22u', 127, 43.18, 'VBAT', 'GND', C0805, 'C45783')
    for i, net in enumerate(['CELL_P', 'CELL_N', 'VBAT', 'GND']):
        sh.flag(net, 22.86 + i * 10.16, 63.5)
    sh.text('U5: VM (pins 4,5) is pack minus = board GND. Keep CELL_N -> U5 -> GND short and wide.\n'
            'Q1: body diode conducts until U5 closes, then the channel turns fully on (Vgs = -Vcell).\n'
            'Reversed cell: Q1 blocks the high side, U5 blocks the low side.', 15.24, 81.28)

    # ------------------------------------------------------------ USB + charger
    sh.box(12.7, 97.79, 135, 195.58, 'USB-C (sink) and LiPo charger')
    sh.place('Connector:USB_C_Receptacle_USB2.0_16P', 'J1', 'USB-C 16P', 33.02, 140.97,
             {'A1': 'GND', 'A12': 'GND', 'B1': 'GND', 'B12': 'GND', 'SH': 'GND',
              'A4': 'VBUS', 'A9': 'VBUS', 'B4': 'VBUS', 'B9': 'VBUS',
              'A5': 'CC1', 'B5': 'CC2', 'A6': 'USB_DP', 'B6': 'USB_DP',
              'A7': 'USB_DN', 'B7': 'USB_DN', 'A8': 'NC', 'B8': 'NC'},
             fp='Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12', lcsc='C165948')
    sh.passive('R', 'R1', '5.1k', 71.12, 171.45, 'CC1', 'GND', R0603, 'C23186')
    sh.passive('R', 'R2', '5.1k', 81.28, 171.45, 'CC2', 'GND', R0603, 'C23186')
    sh.place('zigbee_toy:TP4054', 'U4', 'TP4054-42-SOT25R', 104.14, 120.65,
             {'4': 'VBUS', '5': 'CHG_PROG', '3': 'VBAT', '1': 'CHG_STAT', '2': 'GND'},
             lcsc='C32574')
    sh.passive('R', 'R3', '3.3k', 73.66, 144.78, 'CHG_PROG', 'GND', R0603, 'C22978')
    sh.place('Device:LED', 'D2', 'RED (charging)', 114.3, 144.78,
             {'1': 'CHG_STAT', '2': 'CHG_LED'}, fp=LED0603, lcsc='C2286')
    sh.passive('R', 'R4', '1k', 127, 144.78, 'VBUS', 'CHG_LED', R0603, 'C21190')
    sh.passive('C', 'C4', '4.7u', 93.98, 171.45, 'VBUS', 'GND', C0603, 'C19666')
    sh.flag('VBUS', 110.49, 167.64)
    sh.text('I_chg = 1000 V / R3 = ~300 mA.  C4 sits between J1, U4 VCC and U1 VBUS.', 15.24, 186.69)

    # ------------------------------------------------------------ MCU
    sh.box(139.7, 12.7, 279.4, 195.58, 'nRF52840 module (Zigbee / BLE / USB)')
    # Pin map chosen for the two-sided floorplan (module rotated 180 deg, antenna on top):
    # left pads -> motor driver / LED / button, bottom pads -> crystal, supplies and the two
    # PWM outputs, right pads -> USB, SWD. Inner bottom row (via escapes) -> accelerometer, LED, button.
    sh.place('zigbee_toy:E73-2G4M08S1C', 'U1', 'E73-2G4M08S1C', 209.55, 101.6, {
        '19': 'VDD', '23': 'VBAT', '25': 'NC', '27': 'VBUS', '31': 'USB_DP', '29': 'USB_DN',
        '11': 'XL1', '13': 'XL2', '26': 'nRESET', '37': 'SWDIO', '39': 'SWCLK',
        '5': 'GND', '21': 'GND', '24': 'GND',
        '15': 'MOT_PWM', '17': 'SPK_PWM',
        '3': 'MOT_IPROPI', '4': 'MOT_GAINSEL', '6': 'MOT_DIR', '7': 'NC', '8': 'NC',
        '1': 'NC', '2': 'NC', '9': 'NC', '10': 'NC',
        '35': 'NC', '33': 'NC', '41': 'NC', '43': 'NC',
        '36': 'NC', '34': 'NC',
        '12': 'ACC_INT1', '14': 'I2C_SDA', '16': 'I2C_SCL', '18': 'ACC_INT2', '20': 'LED_STATUS', '22': 'BTN',
        '28': 'DBG_TX', '30': 'DBG_RX', '32': 'NC', '38': 'NC', '40': 'NC', '42': 'NC'}, lcsc='C356849')
    sh.place('Device:Crystal', 'Y1', '32.768k 12.5pF', 160.02, 27.94, {'1': 'XL1', '2': 'XL2'},
             fp='Crystal:Crystal_SMD_3215-2Pin_3.2x1.5mm', lcsc='C32346')
    sh.passive('C', 'C12', '20p C0G', 152.4, 44.45, 'XL1', 'GND', C0603, 'C1648')
    sh.passive('C', 'C13', '20p C0G', 167.64, 44.45, 'XL2', 'GND', C0603, 'C1648')
    sh.passive('C', 'C6', '4.7u', 147.32, 171.45, 'VBAT', 'GND', C0603, 'C19666')
    sh.passive('C', 'C7', '100n', 157.48, 171.45, 'VBAT', 'GND', C0603, 'C14663')
    sh.passive('C', 'C8', '4.7u', 167.64, 171.45, 'VDD', 'GND', C0603, 'C19666')
    sh.passive('C', 'C9', '100n', 177.8, 171.45, 'VDD', 'GND', C0603, 'C14663')
    sh.flag('VDD', 205.74, 160.02)
    sh.text('C6/C7 at VDDH, C8/C9 at VCC (VDD = REG0 output, 3.0 V via UICR.REGOUT0).\n'
            'REG0 runs as LDO (DCCH unconnected).  No 32 kHz crystal inside the module: Y1 is required.\n'
            'PWM nets avoid the low-frequency pins near the radio (P0.02-P0.05, P0.28-P0.31, P1.1x).',
            142.24, 182.88)

    # ------------------------------------------------------------ UI / debug
    sh.box(139.7, 200.66, 279.4, 284.48, 'Button, status LED, SWD')
    sh.place('Switch:SW_Push', 'SW1', 'PAIR', 160.02, 220.98, {'1': 'BTN', '2': 'GND'},
             fp='Button_Switch_SMD:SW_Push_1P1T-MP_NO_Horizontal_Alps_SKRTLAE010', lcsc='C110293')
    sh.passive('R', 'R5', '1k', 199.39, 220.98, 'LED_STATUS', 'LED_A', R0603, 'C21190', horizontal=True)
    sh.place('Device:LED', 'D3', 'RED (status)', 233.68, 220.98, {'1': 'GND', '2': 'LED_A'},
             fp=LED0603, lcsc='C2286')
    sh.place('Connector_Generic:Conn_01x05', 'J2', 'SWD', 172.72, 254,
             {'1': 'VDD', '2': 'GND', '3': 'nRESET', '4': 'SWCLK', '5': 'SWDIO'},
             fp='Connector_PinHeader_1.27mm:PinHeader_1x05_P1.27mm_Vertical', in_bom=False,
             props={'Note': 'Unpopulated: pogo clip or tacked wires for the first flash'})

    # ------------------------------------------------------------ motor
    sh.box(284.48, 12.7, 406.4, 87.63, 'Motor driver (DRV8213, current sense)')
    sh.place('zigbee_toy:DRV8213DSG', 'U2', 'DRV8213DSGR', 335.28, 40.64,
             {'1': 'VBAT', '6': 'MOT_DIR', '5': 'MOT_PWM', '7': 'MOT_GAINSEL',
              '2': 'MOT_A', '3': 'MOT_B', '8': 'MOT_IPROPI', '4': 'GND', '9': 'GND'},
             lcsc='C22407186')
    sh.passive('C', 'C2', '10u 25V', 299.72, 66.04, 'VBAT', 'GND', C0805, 'C15850')
    sh.passive('C', 'C3', '100n', 312.42, 66.04, 'VBAT', 'GND', C0603, 'C14663')
    sh.passive('R', 'R6', '1.5k', 368.3, 66.04, 'MOT_IPROPI', 'GND', R0603, 'C22843')
    sh.place('Connector_Generic:Conn_01x02', 'J4', 'MOTOR', 391.16, 33.02,
             {'1': 'MOT_A', '2': 'MOT_B'}, fp=JST_PH_SIDE, lcsc='C173752', in_bom=False,
             props={'Note': 'Side-entry JST-PH, hand-solder (THT)'})
    sh.text('I_limit = 0.51 V / (R6 x A_IPROPI):  GAINSEL low 1.66 A, Hi-Z 324 mA, high 69 mA.\n'
            'Auto-sleep (20 nA) when IN1 = IN2 = 0.  Keep the 100 nF cap on the motor tabs.',
            287.02, 79.5)

    # ------------------------------------------------------------ speaker
    sh.box(284.48, 92.71, 406.4, 151.13, 'Speaker (optional, PWM)')
    sh.passive('R', 'R11', '0R', 297.18, 109.22, 'VBAT', 'SPK_P', R0603, 'C21189')
    sh.place('Connector_Generic:Conn_01x02', 'J5', 'SPEAKER', 327.66, 106.68,
             {'1': 'SPK_P', '2': 'SPK_N'}, fp=JST_PH_SIDE, lcsc='C173752', in_bom=False,
             props={'Note': 'Side-entry JST-PH, hand-solder (THT)'})
    sh.place('Device:D_Schottky', 'D1', 'B5819W', 358.14, 106.68, {'1': 'SPK_P', '2': 'SPK_N'},
             fp='Diode_SMD:D_SOD-123', lcsc='C8598')
    sh.place('Transistor_FET:AO3400A', 'Q2', 'AO3400A', 384.81, 129.54,
             {'1': 'SPK_G', '2': 'GND', '3': 'SPK_N'}, lcsc='C20917')
    sh.passive('R', 'R7', '100R', 345.44, 129.54, 'SPK_PWM', 'SPK_G', R0603, 'C22775', horizontal=True)
    sh.passive('R', 'R8', '100k', 365.76, 137.16, 'SPK_G', 'GND', R0603, 'C25803')
    sh.text('R11: 0R link (1 A). 8 ohm speaker: cap PWM duty in firmware.', 287.02, 144.78)

    # ------------------------------------------------------------ accelerometer
    sh.box(284.48, 156.21, 406.4, 233.68, 'Accelerometer (wake-on-motion)')
    sh.place('zigbee_toy:LIS2DH12', 'U3', 'LIS2DH12TR', 340.36, 182.88,
             {'9': 'VDD', '10': 'VDD', '2': 'VDD', '3': 'VDD', '5': 'GND',
              '4': 'I2C_SDA', '1': 'I2C_SCL', '12': 'ACC_INT1', '11': 'ACC_INT2',
              '6': 'GND', '7': 'GND', '8': 'GND'}, lcsc='C110926')
    sh.passive('C', 'C10', '100n', 299.72, 213.36, 'VDD', 'GND', C0603, 'C14663')
    sh.text('CS = VDD (I2C).  SA0 = VDD -> address 0x19.  I2C uses the nRF internal pull-ups (~13k, 100 kHz).',
            287.02, 226.06)

    # ------------------------------------------------------------ test points
    sh.box(186.69, 231.14, 279.4, 284.48, 'Test points (back, 1 mm pads, not assembled)')
    for i, (ref, net, label) in enumerate(TEST_POINTS):
        x, y = 207.01 + i // 7 * 30.48, 240.03 + i % 7 * 5.08
        sh.place('Connector:TestPoint', ref, label, x, y, {'1': net}, rot=270,
                 fp='TestPoint:TestPoint_Pad_D1.0mm', in_bom=False,
                 prop_pos={'Reference': (x + 3.81, y - 1.27, 'left'), 'Value': (x + 3.81, y + 1.27, 'left')})
    sh.text('Value = back-silk label.  DBG_TX/RX: spare GPIOs, 3 V UART log port (P0.15 / P0.17).\n'
            'PROG: I_chg = V x 300 mA.  ISNS: I_motor = V / (A_IPROPI x 1.5k).',
            189.23, 278.13)

    # ------------------------------------------------------------ mechanical
    for i in range(2):
        sh.place('Mechanical:MountingHole', f'H{i + 1}', 'M2', 104.14 + i * 7.62, 274.32, {},
                 fp='MountingHole:MountingHole_2.2mm_M2_ISO7380', in_bom=False)

    # ------------------------------------------------------------ notes
    sh.box(12.7, 200.66, 135, 284.48, 'Notes')
    sh.text('One board for all three toys (bird / fish / ball). 1S LiPo, protected or not.\n'
            '\n'
            'Battery voltage: SAADC VDDHDIV5 (internal), no divider.\n'
            'Charging detect: nRF USBDETECTED (VBUS), CHRG drives D2 only.\n'
            'Motor current: SAADC AIN1 (P0.03) on MOT_IPROPI, 0.6 V ref, gain 1.\n'
            'GAINSEL: drive low / drive high / input-disconnected = Hi-Z.\n'
            'Motor: DIR (IN1, static) + PWM (IN2): fwd DIR=0 PWM=duty, rev DIR=1 PWM=1-duty.\n'
            'Speaker: PWM sample playback on SPK_PWM (~62.5 kHz carrier).\n'
            '\n'
            'Hand-soldered, not JLC-assembled: J3 battery, J4 motor, J5 speaker.\n'
            'J2 SWD: 1.27 mm pad row, unpopulated (first flash only, then USB DFU).\n'
            'H1, H2: M2 mounting holes (NPTH), diagonal corners; ISO 7380 button head (3.5 mm).',
            15.24, 208.28)
