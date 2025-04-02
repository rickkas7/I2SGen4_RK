# I2SGen4_RK
I2S (sound) support for Particle Gen 4 (RTL872x) devices


## Hardware support

| Device     | I2S     | PDM     |
| :--------- | :-----: | :-----: |
| P2         | &check; | &check; |
| Photon 2   | &nbsp;  | &check; |
| M-SoM      | &check; | &check; |
| Muon       | &check; | &nbsp;  |


### P2

The following pins are used for I2S:

| Pin | Pin Name | Description | MCU |
| :---: | :--- | :--- | :--- |
| 40 | S0 / D15 | S0 GPIO, PWM, SPI MOSI, Serial3 TX, I2S MCLK. (Was P1S0 on P1.) | PA[12] |
| 33 | S6 / D21 | S6 GPIO, I2S WS. (Was P1S6/TESTMODE on P1.)| PB[31] |
| 47 | S4 / D19 | S4 GPIO, I2S RX. (Was P1S4 on P1.) | PA[0] |
| 44 | S3 / D18 | S3 GPIO, I2S TX. (Was P1S3 on P1.), SPI SS | PB[26] |
| 48 | S5 / D20 | S5 GPIO, I2S CLK. (Was P1S5 on P1.) | PB[29] |


The following pins are used for PDM:

| Pin | Pin Name | Description | MCU |
| :---: | :--- | :--- | :--- |
| 43 | A1 / D12 | A1 Analog in, PDM DAT, GPIO | PB[2] |
| 50 | A0 / D11 | A0 Analog in, PDM CLK, GPIO | PB[1] |


### Photon 2

The Photon 2 does not support I2S as the I2S CLK and I2S WS are not mapped from the P2 to Photon 2 pins.

PDM (digital microphone, DMIC) is supported on the Photon 2 on pins A0 and A1.

| Pin Name | Description | MCU |
| :--- | :--- | :--- |
| A0 / D11 | A0 Analog in, PDM CLK, GPIO | PB[1] |
| A1 / D12 | A1 Analog in, PDM DAT, GPIO | PB[2] |



### M-SoM

The M-SoM supports I2S on the following pins. Some functions can be used on multiple pins; the pins used on the Muon
have a checkmark in the rightmost column.

| Pin | Pin Name | Description | MCU | Used on Muon |
| :---: | :--- | :--- | :--- | :---: |
| 36 | TX / D9 | Serial TX, PWM, GPIO, SPI1 MOSI, I2S MCLK | PA[12] | &check; |
| 17 | D21 | D21 GPIO, I2S RX | PA[0] | &check; |
| 19 | D20 | D20 GPIO, I2S TX | PA[1] | &check; |
| 59 | D26 | D26 GPIO, I2S WS | PA[4] | &check; |
| 68 | D5 | D5 GPIO, PWM, I2S TX | PB[19] | &check; |
| 70 | D6 | D6 GPIO, PWM, I2S CLK | PB[20] | &check; |
| 72 | D7 | D7 GPIO, PWM, I2S WS | PB[21] | |


The M-SoM supports PDM (DMIC) on the following pins:

| Pin | Pin Name | Description | MCU |
| :---: | :--- | :--- | :--- |
| 37 | A3 / D16 | A3 Analog in, PDM CLK, GPIO | PB[1] |
| 41 | A4 / D15 | A4 Analog in, PDM DAT, GPIO | PB[2] |


### Muon

The Muon does not support PDM as the PDM pins (A2, A3) are used for internal peripherals and not available on the 40-pin expansion connector. You can, however use an I2S microphone instead of PDM.

The following pins are used for I2S.

| Pin | Pin Name | Description | M2 Pin | MCU | Raspberry Pi |
| :---: | :--- | :--- | :--- | :--- | :--- |
| 12 | D6 | D6 GPIO, PWM, I2S CLK | 70 | PB[20] | GPIO18 (PCM_CLK) |
| 35 | D26 | D26 GPIO, I2S WS | 59 | PA[4] | GPIO19 (PCM_FS) |
| 38 | D21 | D21 GPIO, I2S RX | 17 | PA[0] | GPIO20 (PCM_DIN) |
| 40 | D20 | D20 GPIO, I2S TX | 19 | PA[1] | GPIO21 (PCM_DOUT) |


The Muon is only compatible with Raspberry Pi expansion cards that support I2S on the PCM pins, not cards that use raw PCM frames.


These are non-standard pins for I2S for the Raspberry Pi 40-pin HAT connector, though the RTL8722DM does support these as alternate pins for I2s:

| Pin | Pin Name | Description | M2 Pin | MCU | Raspberry Pi |
| :---: | :--- | :--- | :--- | :--- | :--- |
|  8 | TX | Serial TX, PWM, GPIO, SPI1 MOSI, I2S MCLK | PA[12] | GPIO14 (TX) |
| 32 | D5 | D5 GPIO, PWM, I2S TX | PB[19] | GPIO12 (PWM0) |

Muon Test 1:

| Color  | Pin  | Function | MCU    | Pi              | Channel | 
| :----- | :--- | :------- | :----- | :-------------- | :--- |
| Gray   |  6   | GND      | GND    | GND             |      | 
| White  | 12   | CLK      | PB[20] | PCM_CLK GPIO18  | 0    |
| Purple | 35   | WS       | PA[4]  | PCM_FS GPIO19   | 1    |
| Blue   | 40   | TX       | PA[1]  | PCM_DOUT GPIO21 | 2    |

[Adafruit UDA1334 I2S breakout](https://www.adafruit.com/product/3678)

| Side  | Pin  | Label | Connection | Description | Color |
| :---- | ---: | :---- | :--- | :--- | :--- |
| Short |  1   | SCLK  | NC   | Optional 27 MHz system clock in, not used | |
| Short |  2   | SF1   | NC  | SF0 & SF1 LOW or NC for I2S mode | |
| Short |  3   | MUTE  | NC  | NC or LOW for normal operation, HIGH for mute | |
| Short |  4   | SF0   | NC  | SF0 & SF1 LOW or NC for I2S mode | |
| Short |  5   | PLL   | NC  | LOW or NC for I2S audio mode | |
| Short |  6   | DEEM  | NC  | De-emphasis mode, not sure how this works | |
| Long  |  1   | VIN   | 3V3 | Voltage in (3.3V to 5V) | Red |
| Long  |  2   | 3VO   | NC  | 3.3V output | |
| Long  |  3   | GND   | GND | Ground | Black |
| Long  |  4   | WSEL  | WS  | I2S WS/FS (word select) | White |
| Long  |  5   | DIN   | TX  | I2S DOUT/TX | Gray |
| Long  |  6   | BCLK  | CLK | Bit clock | Brown |
| Long  |  7   | LOUT  | NC  | Audio output, left (also on 3.5mm jack) | |
| Long  |  8   | AGND  | NC  | Audio output, ground (also on 3.5mm jack) | |
| Long  |  1   | ROUT  | NC  | Audio output, right (also on 3.5mm jack) | |

Muon test 2:

| Color  | Pin  | Function | MCU    | Pi              |
| :----- | :--- | :------- | :----- | :-------------- |
| Red    |  1   | 3V3      | 3V3    | 3V3             |      
| Black  |  6   | GND      | GND    | GND             |
| Brown  | 12   | CLK      | PB[20] | PCM_CLK GPIO18  |
| White  | 35   | WS       | PA[4]  | PCM_FS GPIO19   |
| Gray   | 40   | TX       | PA[1]  | PCM_DOUT GPIO21 |
