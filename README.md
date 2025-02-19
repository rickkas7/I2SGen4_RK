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
| 36 | TX / D9 | Serial TX, PWM, GPIO, SPI1 MOSI, I2S MCLK | PA[12] | |
| 17 | D21 | D21 GPIO, I2S RX | PA[0] | &check; |
| 19 | D20 | D20 GPIO, I2S TX | PA[1] | &check; |
| 59 | D26 | D26 GPIO, I2S WS | PA[4] | &check; |
| 68 | D5 | D5 GPIO, PWM, I2S TX | PB[19] | |
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
