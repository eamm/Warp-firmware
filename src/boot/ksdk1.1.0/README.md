## 4B25 Air Quality Monitoring System
Ewan Morrin, Churchill College, em702
github repository link: https://github.com/eamm/Warp-firmware/tree/master/src/boot/ksdk1.1.0

This repository contains all the files necessary for running this air quality monitoring system using an adafruit Bosch BME680 sensor breakout board, OLED display and FRDM-KL03Z. This system is based on the Warp Firmware software made by Phillip Stanley Marbell and Martin Rinard (ArXiv e-prints (2018). arXiv:1804.09241).

The main functional changes made to files can be found in the files devBME680.c, devBME680.h (the BME680 driver files), devSSD1331.c, devSSD1331.h (the OLED display driver files) and warp-kl03-ksdk1.1-boot.c (the Warp Firmware implementation file). The main changes to the .c files are explained below.

# devBME680.c
The original file provided in the Warp Firmware was edited to configure resistance readings from the BME680 MOx sensor. The original file contained only raw ADC values from the sensor without compensation, so the Bosch BME680 API (https://github.com/BoschSensortec/BME680_driver/blob/master/bme680.c) was used to make these raw ADC values interpretable. The ADC value compensation functions found in this driver were used to produce the accurate compensated values in the final file. The file was also modified so that reads from the gas resistance register were possible.

# devSSD1331.c
The file was modified so that text drawing commands are possible to the OLED screen. This included all the characters necessary for displaying temperature and humidity values, as well as some green and red fill commands.

# warp-kl03-ksdk1.1-boot.c
This file was modified to include an environment evaluation function, which evaluates whether the environment the BME680 is in is comfortable and safe for peope to spend extended periods of time in. It does this using functions from devBME680 to read temperature, pressure and humidity values and then decides whether these values lie in an acceptable range as (temperature between 20-26 degrees celcius, humidity 40-70%). It then uses the OLED display to show the temperature and humidity and also indicates whether the environment is safe with a simple red or green background. This was also extended to incorporate the gas sensor resistance values. The gas sensor is sensitive to changes in the VOC (volatile organic compound) level in the environment the sensor is lying in. If the VOC concentration rises then the sensor resistance will fall due to interactions between the metal oxide and the VOCs. With this in mind, resistance readings are taken for 50 measurements in an unchanging environment and then the average is taken to form a baseline. If the resistance drops too far below this baseline then a warning message is displayed, indicating that there is a dangerous VOC level in the environment. 