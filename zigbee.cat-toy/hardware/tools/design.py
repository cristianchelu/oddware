"""Zigbee cat toy controller, rev B: component placement and nets.

Coordinates are sheet millimetres (A3, Y down).
"""

R0603 = 'Resistor_SMD:R_0603_1608Metric'
C0603 = 'Capacitor_SMD:C_0603_1608Metric'
C0805 = 'Capacitor_SMD:C_0805_2012Metric'
JST_PH_SIDE = 'Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal'


# (ref, net, back-silk label): grouped on the back, see placement.py
TEST_POINTS = [
    ('TP1', 'VBAT', 'VBAT'), ('TP2', '3V3', '3V3'), ('TP3', 'VBUS', 'VBUS'),
    ('TP4', 'GND', 'GND'), ('TP5', 'CHG_STAT', 'nCHG'), ('TP6', 'CHG_PROG', 'PROG'),
    ('TP7', 'MOT_PWM', 'PWM'), ('TP8', 'MOT_DIR', 'DIR'), ('TP9', 'MOT_IPROPI', 'ISNS'),
    ('TP10', 'MOT_GAINSEL', 'GAIN'), ('TP11', 'SPK_PWM', 'SPK'), ('TP12', 'GND', 'GND'),
    ('TP13', 'ACC_INT1', 'INT1'), ('TP15', 'I2C_SDA', 'SDA'),
    ('TP16', 'I2C_SCL', 'SCL'), ('TP17', 'BTN', 'BOOT'), ('TP18', 'LED_STATUS', 'LED'),
    ('TP19', 'DBG_TX', 'TX'), ('TP20', 'DBG_RX', 'RX'), ('TP21', 'EN', 'EN'), ('TP22', 'GND', 'GND'),
]


def build(sh):
    # ------------------------------------------------------------ battery
    sh.box(12.7, 12.7, 135, 92, 'Battery, protection, reverse polarity')
    sh.place('Connector_Generic:Conn_01x02', 'J3', 'LiPo 1S (JST-PH)', 45.72, 38.1,
             {'1': 'CELL_P', '2': 'CELL_N'},
             fp=JST_PH_SIDE, lcsc='C173752',
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
    sh.box(12.7, 97.79, 135, 195.58, 'USB-C (sink), LiPo charger, USB detect')
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
    sh.passive('C', 'C4', '4.7u', 93.98, 171.45, 'VBUS', 'GND', C0603, 'C19666')
    sh.passive('R', 'R14', '68k', 114.3, 144.78, 'VBUS', 'VBUS_SENSE', R0603, 'C23231')
    sh.passive('R', 'R15', '100k', 127, 144.78, 'VBUS_SENSE', 'GND', R0603, 'C25803')
    sh.flag('VBUS', 110.49, 167.64)
    sh.text('I_chg = 1000 V / R3 = ~300 mA.  CHRG (open drain) -> MCU, internal pull-up.\n'
            'VBUS_SENSE = 0.6 x VBUS (3.0 V at 5 V): plugged in, charging or full.', 15.24, 184.15)

    # ------------------------------------------------------------ 3.3 V
    sh.box(139.7, 12.7, 279.4, 59.69, '3.3 V regulator')
    sh.place('zigbee_toy:RT9080', 'U6', 'RT9080-33GJ5', 190.5, 33.02,
             {'1': 'VBAT', '3': 'VBAT', '5': '3V3', '4': 'NC', '2': 'GND'}, lcsc='C841192')
    sh.passive('C', 'C6', '4.7u', 162.56, 38.1, 'VBAT', 'GND', C0603, 'C19666')
    sh.passive('C', 'C8', '22u', 223.52, 38.1, '3V3', 'GND', C0805, 'C45783')
    sh.passive('C', 'C9', '10u', 236.22, 38.1, '3V3', 'GND', C0603, 'C19702')
    sh.text('2 uA quiescent, 600 mA, 0.31 V dropout at 600 mA. C8 bulk at the regulator, C9 at module pin 3.', 142.24, 53.34)

    # ------------------------------------------------------------ MCU
    sh.box(139.7, 64.77, 279.4, 195.58, 'ESP32-C6 module (Zigbee / Thread / BLE / Wi-Fi, USB)')
    sh.place('zigbee_toy:ESP32-C6-MINI-1U', 'U1', 'ESP32-C6-MINI-1U-H4', 209.55, 127, {
        '3': '3V3', '8': 'EN',
        '12': 'XTAL_P', '13': 'XTAL_N', '17': 'USB_DN', '18': 'USB_DP',
        '30': 'DBG_RX', '31': 'DBG_TX',
        '5': 'MOT_IPROPI', '6': 'MOT_DIR', '9': 'MOT_PWM', '10': 'SPK_PWM',
        '15': 'VBAT_SENSE', '16': 'ACC_INT1', '22': 'I2C_SDA', '23': 'BTN',
        '19': 'MOT_GAINSEL', '20': 'NC', '24': 'I2C_SCL',
        '25': 'CHG_STAT', '26': 'VBUS_SENSE', '27': 'NC', '28': 'NC', '29': 'LED_STATUS',
        **{str(n): 'NC' for n in (4, 7, 21, 32, 33, 34, 35)},
        **{str(n): 'GND' for n in [1, 2, 11, 14] + list(range(36, 54))}}, lcsc='C20627095')
    sh.passive('R', 'R9', '10k', 152.4, 81.28, '3V3', 'EN', R0603, 'C25804')
    sh.passive('C', 'C5', '1u', 162.56, 81.28, 'EN', 'GND', C0603, 'C15849')
    sh.place('Device:Crystal', 'Y1', '32.768k 12.5pF', 160.02, 160.02, {'1': 'XTAL_P', '2': 'XTAL_N'},
             fp='Crystal:Crystal_SMD_3215-2Pin_3.2x1.5mm', lcsc='C32346')
    sh.passive('C', 'C12', '20p C0G', 152.4, 172.72, 'XTAL_P', 'GND', C0603, 'C1648')
    sh.passive('C', 'C13', '20p C0G', 167.64, 172.72, 'XTAL_N', 'GND', C0603, 'C1648')
    sh.text('EN: 10k / 1u power-on delay (Espressif).  IO9 = BOOT (SW1).  IO8 strap high via R16.\n'
            'IO0/IO1: 32.768 kHz crystal for the light-sleep RTC.  IO12/13: USB Serial/JTAG on the charging port.\n'
            'ADC1: IO2 MOT_IPROPI, IO6 VBAT_SENSE.  LP (deep-sleep wake): IO7 ACC_INT1.',
            142.24, 182.88)

    # ------------------------------------------------------------ UI / battery sense
    sh.box(139.7, 200.66, 220.98, 284.48, 'Button, status LED, battery sense')
    sh.place('Switch:SW_Push', 'SW1', 'PAIR / BOOT', 160.02, 220.98, {'1': 'BTN', '2': 'GND'},
             fp='Button_Switch_SMD:SW_Push_1P1T-MP_NO_Horizontal_Alps_SKRTLAE010', lcsc='C110293')
    sh.passive('R', 'R4', '1k', 185.42, 220.98, 'LED_STATUS', 'LED_A', R0603, 'C21190', horizontal=True)
    sh.place('Device:LED', 'D2', 'RED (status)', 213.36, 220.98, {'1': 'GND', '2': 'LED_A'}, rot=180,
             fp='LED_SMD:LED_0603_1608Metric', lcsc='C2286')
    sh.passive('R', 'R12', '1M', 180.34, 251.46, 'VBAT', 'VBAT_SENSE', R0603, 'C22935')
    sh.passive('R', 'R13', '1M', 195.58, 251.46, 'VBAT_SENSE', 'GND', R0603, 'C22935')
    sh.passive('C', 'C16', '100n', 208.28, 251.46, 'VBAT_SENSE', 'GND', C0603, 'C14663')
    sh.text('D2: GPIO high = on, 1.3 mA.\n'
            'VBAT_SENSE = VBAT / 2, 2.1 uA. C16 holds it for the ADC sample.', 142.24, 271.78)

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
    sh.place('zigbee_toy:LIS2DW12TR-MS', 'U3', 'LIS2DW12TR-MS', 340.36, 182.88,
             {'7': '3V3', '3': '3V3', '10': 'NC', '1': 'NC', '4': 'NC', '11': 'NC',
              '2': 'I2C_SDA', '12': 'I2C_SCL', '5': 'ACC_INT1', '6': 'NC',
              '9': 'GND', '8': 'GND'}, lcsc='C50385411')
    sh.passive('C', 'C10', '100n', 299.72, 213.36, '3V3', 'GND', C0603, 'C14663')
    sh.passive('C', 'C11', '100n', 312.42, 213.36, '3V3', 'GND', C0603, 'C14663')
    sh.passive('R', 'R16', '4.7k', 370.84, 213.36, '3V3', 'I2C_SDA', R0603, 'C23162')
    sh.passive('R', 'R17', '4.7k', 383.54, 213.36, '3V3', 'I2C_SCL', R0603, 'C23162')
    sh.text('MSKSEMI part, not ST: WHO_AM_I 0x11, LIS2DH-style register map. INT2 unused: route wake and tap to INT1.\n'
            'CS and SDO/SA0 open (internal pull-ups): I2C, address 0x19.  C10 at VDD, C11 at VDDIO.',
            287.02, 224.79)

    # ------------------------------------------------------------ test points
    sh.box(223.52, 200.66, 279.4, 284.48, 'Test points (back)')
    for i, (ref, net, label) in enumerate(TEST_POINTS):
        x, y = 242.57 + i // 11 * 27.94, 210.82 + i % 11 * 6.35
        sh.place('Connector:TestPoint', ref, label, x, y, {'1': net}, rot=270,
                 fp='TestPoint:TestPoint_Pad_D1.0mm', in_bom=False,
                 prop_pos={'Reference': (x + 3.81, y - 1.27, 'left'), 'Value': (x + 3.81, y + 1.27, 'left')})

    # ------------------------------------------------------------ mechanical
    for i in range(2):
        sh.place('Mechanical:MountingHole', f'H{i + 1}', 'M2', 104.14 + i * 7.62, 274.32, {},
                 fp='MountingHole:MountingHole_2.2mm_M2_ISO7380', in_bom=False)

    # ------------------------------------------------------------ notes
    sh.box(12.7, 200.66, 135, 284.48, 'Notes')
    sh.text('One board for all three toys (bird / fish / ball). 1S LiPo, protected or not.\n'
            '\n'
            'Battery voltage: VBAT_SENSE (IO6, ADC1_CH6), VBAT / 2.\n'
            'On charger: VBUS_SENSE (IO20). Charging: CHG_STAT (IO19), low = charging.\n'
            'Motor current: MOT_IPROPI (IO2, ADC1_CH2), 0 dB attenuation.\n'
            'GAINSEL: drive low / drive high / input-disconnected = Hi-Z.\n'
            'Motor: DIR (IN1, static) + PWM (IN2): fwd DIR=0 PWM=duty, rev DIR=1 PWM=1-duty.\n'
            'Motor: DIR IO3, PWM IO4, GAINSEL IO14.  Speaker: SPK_PWM IO5.  Status LED: LED_STATUS IO23.\n'
            '\n'
            'Test points: value = back-silk label. TX/RX = UART0, 3.3 V. BOOT low + EN pulse = download mode.\n'
            'Hand-soldered, not JLC-assembled: J3 battery, J4 motor, J5 speaker.\n'
            'Antenna: 2.4 GHz FPC antenna on the module u.FL, plugged in by hand.\n'
            'H1, H2: M2 mounting holes (NPTH), diagonal corners; ISO 7380 button head (3.5 mm).',
            15.24, 208.28)
