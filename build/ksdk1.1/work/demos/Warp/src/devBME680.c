/*
	Authored 2016-2018. Phillip Stanley-Marbell, Youchao Wang, James Meech.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceBME680State;
extern volatile uint8_t			deviceBME680CalibrationValues[];
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;
extern volatile	int32_t			pressure_comp;
extern volatile	int32_t			calc_hum;
extern volatile	int16_t			calc_temp;
extern volatile uint32_t        	calc_gas_res;


void
initBME680(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (kWarpTypeMaskPressure | kWarpTypeMaskTemperature);

	return;
}

WarpStatus
writeSensorRegisterBME680(uint8_t deviceRegister, uint8_t payload, uint16_t menuI2cPullupValue)
{
	uint8_t		payloadByte[1], commandByte[1];
	i2c_status_t	status;

	if (deviceRegister > 0xFF)
	{
		return kWarpStatusBadDeviceCommand;
	}

	i2c_device_t slave =
	{
		.address = deviceBME680State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	commandByte[0] = deviceRegister;
	payloadByte[0] = payload;
	status = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							1,
							gWarpI2cTimeoutMilliseconds);
	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

WarpStatus
readSensorRegisterBME680(uint8_t deviceRegister, int numberOfBytes)
{
	uint8_t		cmdBuf[1] = {0xFF};
	i2c_status_t	status;


	USED(numberOfBytes);

	/*
	 *	We only check to see if it is past the config registers.
	 *
	 *	TODO: We should eventually numerate all the valid register addresses
	 *	(configuration, control, and calibration) here.
	 */
	if (deviceRegister > kWarpSensorConfigurationRegisterBME680CalibrationRegion2End)
	{
		return kWarpStatusBadDeviceCommand;
	}

	i2c_device_t slave =
	{
		.address = deviceBME680State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	cmdBuf[0] = deviceRegister;

	status = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceBME680State.i2cBuffer,
							1,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}


WarpStatus
configureSensorBME680(uint8_t payloadCtrl_Hum, uint8_t payloadCtrl_Meas, uint8_t payloadGas_0, uint16_t menuI2cPullupValue)
{
	uint8_t		reg, index = 0;
	WarpStatus	status1, status2, status3, status4 = 0, gasReadStatus, gasWriteStatus = 0;

	status1 = writeSensorRegisterBME680(kWarpSensorConfigurationRegisterBME680Ctrl_Hum,
							payloadCtrl_Hum,
							menuI2cPullupValue);

	status2 = writeSensorRegisterBME680(kWarpSensorConfigurationRegisterBME680Ctrl_Meas,
							payloadCtrl_Meas,
							menuI2cPullupValue);

	status3 = writeSensorRegisterBME680(kWarpSensorConfigurationRegisterBME680Ctrl_Gas_0,
							payloadGas_0,
							menuI2cPullupValue);

	/*
	 *	Read the calibration registers
	 */
	for (	reg = kWarpSensorConfigurationRegisterBME680CalibrationRegion1Start;
		reg < kWarpSensorConfigurationRegisterBME680CalibrationRegion1End;
		reg++)
	{
		status4 |= readSensorRegisterBME680(reg, 1 /* numberOfBytes */);
		deviceBME680CalibrationValues[index++] = deviceBME680State.i2cBuffer[0];
	}

	for (	reg = kWarpSensorConfigurationRegisterBME680CalibrationRegion2Start;
		reg < kWarpSensorConfigurationRegisterBME680CalibrationRegion2End;
		reg++)
	{
		status4 |= readSensorRegisterBME680(reg, 1 /* numberOfBytes */);
		deviceBME680CalibrationValues[index++] = deviceBME680State.i2cBuffer[0];
	}
	
	// Here we perform the calculations to write the correct bytes to the gas sensor resistance configuration registers

	int8_t          par_gh1;
	int16_t         par_gh2;
	int8_t          par_gh3;

	uint8_t         res_heat_range;
	uint8_t         res_heat_val;

	// Read calibration constants from appropriate registers on the BME680

	par_gh1 =       (int8_t)        deviceBME680CalibrationValues[BME680_GH1_REG];
	par_gh2 =       (int16_t)       ((deviceBME680CalibrationValues[BME680_GH2_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_GH2_LSB_REG]);
	par_gh3 =       (int8_t)        deviceBME680CalibrationValues[BME680_GH3_REG];

	// Read more calibration parameters

	gasReadStatus = readSensorRegisterBME680(0x02, 1);
	res_heat_range = deviceBME680State.i2cBuffer[0];
	gasReadStatus = readSensorRegisterBME680(0x00, 1);
	res_heat_val = deviceBME680State.i2cBuffer[0];


	// Calculate the byte that needs to be written to the device in order to set the heater temperature, by giving it a target resistance. Here 320 C was chosen

	uint8_t         heatr_res;
	int32_t         var_g1;
	int32_t         var_g2;
	int32_t         var_g3;
	int32_t         var_g4;
	int32_t         var_g5;
	int32_t         heatr_res_x100;
	uint16_t        temp;

	temp = 320;

	var_g1 = (((int32_t) 20 * par_gh3) / 1000) * 256;
	var_g2 = (par_gh1 + 784) * (((((par_gh2 + 154009) * temp * 5) / 100) + 3276800) / 10);
	var_g3 = var_g1 + (var_g2 / 2);
	var_g4 = (var_g3 / (res_heat_range + 4));
	var_g5 = (131 * res_heat_val) + 65536;
	heatr_res_x100 = (int32_t) (((var_g4 / var_g5) - 250) * 34);
	heatr_res = (uint8_t) ((heatr_res_x100 + 50) / 100);


	// Calculate the byte to be written to the device to set the correct heating duration (100ms chosen here)

	uint8_t         factor = 0;
	uint8_t         durval;
	uint16_t        dur;

	dur = 100;

	if (dur >= 0xFC0)
	{
		durval = 0xFF;
	}
	else
	{
		while (dur > 0x3F)
		{
			dur = dur / 4;
			factor += 1;
		}
		durval = (uint8_t) (dur + (factor * 64));
	}

	//SEGGER_RTT_printf(0, "\n These are the heater calibration coefficients 0x%02x 0x%02x \n", heatr_res, durval);
	
	// Write the configuration values and enable gas readings

	gasWriteStatus |= writeSensorRegisterBME680(0x71, 0b00010000, menuI2cPullupValue);
	gasWriteStatus |= writeSensorRegisterBME680(0x64, durval, menuI2cPullupValue);
	gasWriteStatus |= writeSensorRegisterBME680(0x5A, heatr_res, menuI2cPullupValue);

	return (status1 | status2 | status3 | status4 | gasReadStatus | gasWriteStatus);
}


void
printSensorDataBME680(bool hexModeFlag, uint16_t menuI2cPullupValue)
{
	uint16_t	readSensorRegisterValueLSB;
	uint16_t	readSensorRegisterValueMSB;
	uint16_t	readSensorRegisterValueXLSB;
	uint32_t	unsignedRawAdcValue;

	uint16_t	par_t1;
	int16_t		par_t2;
	int8_t		par_t3;
	int32_t		t_fine;

	uint16_t	par_p1;
	int16_t		par_p2;
	int8_t		par_p3;
	int16_t		par_p4;
	int16_t		par_p5;
	int8_t		par_p6;
	int8_t		par_p7;
	int16_t		par_p8;
	int16_t		par_p9;
	uint8_t		par_p10;

	uint16_t	par_h1;
	uint16_t	par_h2;
	int8_t		par_h3;
	int8_t		par_h4;
	int8_t		par_h5;
	uint8_t		par_h6;
	int8_t		par_h7;	
	
	int64_t 	var1;
	int64_t 	var2;
	int64_t 	var3;

	WarpStatus	triggerStatus, i2cReadStatusMSB, i2cReadStatusLSB, i2cReadStatusXLSB;

	par_t1 = 	(uint16_t) 	((deviceBME680CalibrationValues[BME680_T1_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_T1_LSB_REG]);
	par_t2 = 	(int16_t) 	((deviceBME680CalibrationValues[BME680_T2_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_T2_LSB_REG]);
	par_t3 = 	(int8_t) 	(deviceBME680CalibrationValues[BME680_T3_REG]);
	
	par_p1 = 	(uint16_t) 	((deviceBME680CalibrationValues[BME680_P1_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_P1_LSB_REG]);
	par_p2 = 	(int16_t) 	((deviceBME680CalibrationValues[BME680_P2_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_P2_LSB_REG]);
	par_p3 = 	(int8_t) 	deviceBME680CalibrationValues[BME680_P3_REG];
	par_p4 = 	(int16_t) 	((deviceBME680CalibrationValues[BME680_P4_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_P4_LSB_REG]);
	par_p5 = 	(int16_t)	((deviceBME680CalibrationValues[BME680_P5_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_P5_LSB_REG]);
	par_p6 =	(int8_t)	deviceBME680CalibrationValues[BME680_P6_REG];
	par_p7 = 	(int8_t)	deviceBME680CalibrationValues[BME680_P7_REG];
	par_p8 = 	(int16_t)	((deviceBME680CalibrationValues[BME680_P8_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_P8_LSB_REG]);
	par_p9 = 	(int16_t)	((deviceBME680CalibrationValues[BME680_P9_MSB_REG] << 8) | deviceBME680CalibrationValues[BME680_P9_LSB_REG]);
	par_p10 = 	(uint8_t)	deviceBME680CalibrationValues[BME680_P10_REG];

	par_h1 = 	(uint16_t)	((deviceBME680CalibrationValues[BME680_H1_MSB_REG] << 4) | (deviceBME680CalibrationValues[BME680_H1_LSB_REG] & 0x0F));
	par_h2 = 	(uint16_t)	((deviceBME680CalibrationValues[BME680_H2_MSB_REG] << 4) | (deviceBME680CalibrationValues[BME680_H2_LSB_REG] >> 4));
	par_h3 = 	(int8_t)	deviceBME680CalibrationValues[BME680_H3_REG];
	par_h4 = 	(int8_t)	deviceBME680CalibrationValues[BME680_H4_REG];
	par_h5 = 	(int8_t)	deviceBME680CalibrationValues[BME680_H5_REG];
	par_h6 = 	(uint8_t)	deviceBME680CalibrationValues[BME680_H6_REG];
	par_h7 = 	(int8_t)	deviceBME680CalibrationValues[BME680_H7_REG];	

	/*
	 *	First, trigger a measurement
	 */
	triggerStatus = writeSensorRegisterBME680(kWarpSensorConfigurationRegisterBME680Ctrl_Meas,
							0b00100101,
							menuI2cPullupValue);


	// Moving the pressure measurements down to below the temperature measurement so that we can use t_fine
	
	/*i2cReadStatusMSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680press_msb, 1);
	readSensorRegisterValueMSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680press_lsb, 1);
	readSensorRegisterValueLSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusXLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680press_xlsb, 1);
	readSensorRegisterValueXLSB = deviceBME680State.i2cBuffer[0];
	unsignedRawAdcValue =
			((readSensorRegisterValueMSB & 0xFF)  << 12) |
			((readSensorRegisterValueLSB & 0xFF)  << 4)  |
			((readSensorRegisterValueXLSB & 0xF0) >> 4);

	int32_t 	var_p1;
	int32_t		var_p2;
	int32_t		var_p3;
	int32_t		pressure_comp;

	var_p1 = (((int32_t) t_fine) >> 1) -64000;
	var_p2 = ((((var_p1 >> 2) * (var_p1 >> 2)) >> 11) * (int32_t) par_p6) >> 2;
	var_p2 = var_p2 + ((var_p1 * (int32_t) par_p5) << 1);
	var_p2 = (var_p2 >> 2) + ((int32_t) par_p4 << 16);
	var_p1 = (((((var_p1 >> 2) * (var_p1 >> 2)) >> 13) *
			((int32_t)par_p3 << 5)) >> 3) +
			(((int32_t)par_p2 * var_p1) >> 1);
	var_p1 = var_p1 >> 18;
	var_p1 = ((32768 + var_p1) * (int32_t)par_p1) >> 15;
	pressure_comp = 1048576 - unsignedRawAdcValue;
	pressure_comp = (int32_t)((pressure_comp - (var_p2 >> 12)) * ((uint32_t)3125));
	if (pressure_comp >= 0x40000000)
		pressure_comp = ((pressure_comp / var_p1) << 1);
	else
		pressure_comp = ((pressure_comp << 1) / var_p1);
	var_p1 = ((int32_t)par_p9 * (int32_t(((pressure_comp >> 3) * 
			(pressure_comp >> 3)) >> 13)) >> 12;
	var_p2 = ((int32_t)(pressure_comp >> 2) * 
			(int32_t)par_p8) >> 13;
	var_p3 = ((int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) * 
		(int32_t)(pressure_comp >> 8) * 
		(int32_t)par_p10) >> 17;

	pressure_comp = (int32_t)(pressure_comp) + ((var_p1 + var_p2 + var_p3 + 
			((int32_t)par_p7 << 7)) >> 4;


	if ((triggerStatus != kWarpStatusOK) || (i2cReadStatusMSB != kWarpStatusOK) || (i2cReadStatusLSB != kWarpStatusOK) || (i2cReadStatusXLSB != kWarpStatusOK))
	{
		SEGGER_RTT_WriteString(0, " ----,");
	}
	else
	{
		if (hexModeFlag)
		{
			SEGGER_RTT_printf(0, " 0x%02x 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB, readSensorRegisterValueXLSB);
		}
		else
		{
			SEGGER_RTT_printf(0, " %u,", unsignedRawAdcValue);
		}
	}*/


	i2cReadStatusMSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680temp_msb, 1);
	readSensorRegisterValueMSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680temp_lsb, 1);
	readSensorRegisterValueLSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusXLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680temp_xlsb, 1);
	readSensorRegisterValueXLSB = deviceBME680State.i2cBuffer[0];
	unsignedRawAdcValue =
			((readSensorRegisterValueMSB & 0xFF)  << 12) |
			((readSensorRegisterValueLSB & 0xFF)  << 4)  |
			((readSensorRegisterValueXLSB & 0xF0) >> 4);

	// Temperature ADC value compensation to yield valid and accurate temperature values

	var1 = ((int32_t) unsignedRawAdcValue >> 3) - ((int32_t) par_t1 << 1);
	var2 = (var1 * (int32_t) par_t2) >> 11;
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
	var3 = ((var3) * ((int32_t) par_t3 << 4)) >> 14;
	t_fine = (int32_t) (var2 + var3);
	calc_temp = (int16_t) (((t_fine * 5) + 128) >> 8);

	double 	temp_value;
	temp_value = (double) calc_temp / 100;
	int intPart = (int) temp_value;
	int decPart = (temp_value - (double) intPart)*100;

	if ((triggerStatus != kWarpStatusOK) || (i2cReadStatusMSB != kWarpStatusOK) || (i2cReadStatusLSB != kWarpStatusOK) || (i2cReadStatusXLSB != kWarpStatusOK))
	{
		SEGGER_RTT_WriteString(0, " ----,");
	}
	else
	{
		if (hexModeFlag)
		{
			SEGGER_RTT_printf(0, " 0x%02x 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB, readSensorRegisterValueXLSB);
		}
		else
		{
			SEGGER_RTT_printf(0, "Temperature: %d.%d degC \n", intPart, decPart  /*unsignedRawAdcValue*/);
		}
	}


	i2cReadStatusMSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680hum_msb, 1);
	readSensorRegisterValueMSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680hum_lsb, 1);
	readSensorRegisterValueLSB = deviceBME680State.i2cBuffer[0];
	unsignedRawAdcValue = ((readSensorRegisterValueMSB & 0xFF) << 8) | (readSensorRegisterValueLSB & 0xFF);
	
	int32_t		var_h1;
	int32_t		var_h2;
	int32_t		var_h3;
	int32_t		var_h4;
	int32_t		var_h5;
	int32_t		var_h6;
	int32_t		temp_scaled;
	//int32_t		calc_hum;

	// Humidity ADC value compensation, as for the temperature

	temp_scaled = (((int32_t) t_fine * 5) + 128) >> 8;
	var_h1 = (int32_t) (unsignedRawAdcValue - ((int32_t) ((int32_t)par_h1 * 16))) - 
			(((temp_scaled * (int32_t) par_h3) / ((int32_t) 100)) >> 1);
	var_h2 = ((int32_t)par_h2 * 
			(((temp_scaled * (int32_t) par_h4) / ((int32_t) 100)) + 
			 (((temp_scaled * ((temp_scaled * (int32_t) par_h5) / ((int32_t) 100))) >> 6)
			  / ((int32_t) 100)) + (int32_t) (1 << 14))) >> 10;
	var_h3 = var_h1 * var_h2;
	var_h4 = (int32_t) par_h6 << 7;
	var_h4 = ((var_h4) + ((temp_scaled * (int32_t) par_h7) / ((int32_t) 100))) >> 4;
	var_h5 = ((var_h3 >> 14) * (var_h3 >> 14)) >> 10;
	var_h6 = (var_h4 * var_h5) >> 1;
	calc_hum = (((var_h3 + var_h6) >> 10) * ((int32_t) 1000)) >> 12;

	if (calc_hum > 100000) /* Cap at 100% RH*/
		calc_hum = 100000;
	else if (calc_hum < 0)
		calc_hum = 0;

	double humidity_value;
	humidity_value = (double) calc_hum/1000;
	intPart = (int) humidity_value;
	decPart = (humidity_value - (double) intPart)*1000;

	if ((triggerStatus != kWarpStatusOK) || (i2cReadStatusMSB != kWarpStatusOK) || (i2cReadStatusLSB != kWarpStatusOK))
	{
		SEGGER_RTT_WriteString(0, " ----,");
	}
	else
	{
		if (hexModeFlag)
		{
			SEGGER_RTT_printf(0, " 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
		}
		else
		{
			SEGGER_RTT_printf(0, "Relative Humidity: %d.%d Percent \n", intPart, decPart);
		}
	}

	i2cReadStatusMSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680press_msb, 1);
	readSensorRegisterValueMSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680press_lsb, 1);
	readSensorRegisterValueLSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusXLSB = readSensorRegisterBME680(kWarpSensorOutputRegisterBME680press_xlsb, 1);
	readSensorRegisterValueXLSB = deviceBME680State.i2cBuffer[0];
	unsignedRawAdcValue = 
			((readSensorRegisterValueMSB & 0xFF) << 12) |
			((readSensorRegisterValueLSB & 0xFF) << 4 ) |
			((readSensorRegisterValueXLSB & 0xF0) >> 4);

	int32_t		var_p1;
	int32_t		var_p2;
	int32_t		var_p3;
	//int32_t		pressure_comp;
	
	// Pressure ADC value compensation

	var_p1 = (((int32_t) t_fine) >> 1) - 64000;
	var_p2 = ((((var_p1 >> 2) * (var_p1 >> 2)) >> 11) * (int32_t) par_p6) >> 2;
	var_p2 = var_p2 + ((var_p1 * (int32_t) par_p5) << 1);
	var_p2 = (var_p2 >> 2) + ((int32_t) par_p4 << 16);
	var_p1 = (((((var_p1 >> 2) * (var_p1 >> 2)) >> 13) * 
				((int32_t)par_p3 << 5)) >> 3) + 
				(((int32_t)par_p2 * var_p1) >> 1);
	var_p1 = var_p1 >> 18;
	var_p1 = ((32768 + var_p1) * (int32_t)par_p1 >> 15);
	pressure_comp = 1048576 - unsignedRawAdcValue;
	pressure_comp = (int32_t)((pressure_comp - (var_p2 >> 12)) * ((uint32_t)3125));
	if (pressure_comp >= 0x40000000)
		pressure_comp = ((pressure_comp / var_p1) << 1);
	else
		pressure_comp = ((pressure_comp << 1) / var_p1);
	var_p1 = ((int32_t)(par_p9 * (int32_t)(((pressure_comp >> 3) * 
					(pressure_comp >> 3)) >> 13)) >> 12);
	var_p2 = ((int32_t)(pressure_comp >> 2) * 
			(int32_t)par_p8) >> 13;
	var_p3 = ((int32_t)(pressure_comp >> 8) * (int32_t)(pressure_comp >> 8) * 
			(int32_t)(pressure_comp >> 8) * 
			(int32_t)par_p10) >> 17;

	pressure_comp = (int32_t)(pressure_comp) + ((var_p1 + var_p2 + var_p3 + 
			((int32_t)par_p7 << 7)) >> 4);

	double pressure_value;
	pressure_value = (double) pressure_comp / 1000;
	intPart = (int) pressure_value;
	decPart = (pressure_value - (double) intPart)*1000;

	if ((triggerStatus != kWarpStatusOK) || (i2cReadStatusMSB != kWarpStatusOK) || (i2cReadStatusLSB != kWarpStatusOK) || (i2cReadStatusXLSB != kWarpStatusOK))
	{
		SEGGER_RTT_WriteString(0, " ----,");
	}
	else 
	{
		if (hexModeFlag)
		{
			SEGGER_RTT_printf(0, " 0x%02x 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB, readSensorRegisterValueXLSB);
		}
		else
		{
			SEGGER_RTT_printf(0, "Pressure: %d.%d kPa \n", intPart, decPart);
		}
	}

	// Gas resistance measurement
	

	uint16_t	gas_res_adc;
	uint8_t		gas_range;
	
	i2cReadStatusMSB = readSensorRegisterBME680(0x2A, 1);
	readSensorRegisterValueMSB = deviceBME680State.i2cBuffer[0];
	i2cReadStatusLSB = readSensorRegisterBME680(0x2B, 1);
	readSensorRegisterValueLSB = deviceBME680State.i2cBuffer[0];

	gas_res_adc = ((readSensorRegisterValueMSB << 2) | (readSensorRegisterValueLSB >> 6));
	gas_range   = (readSensorRegisterValueLSB & 0x0F);

	int8_t		range_sw_err;

	i2cReadStatusXLSB = readSensorRegisterBME680(0x04, 1);
	range_sw_err = deviceBME680State.i2cBuffer[0];

	int64_t		var_g1;
	uint64_t	var_g2;
	int64_t 	var_g3;
	//uint32_t 	calc_gas_res;
	
	// Gas resistance ADC compensation

	uint32_t lookupTable1[16] = { UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2147483647),
		UINT32_C(2147483647), UINT32_C(2126008810), UINT32_C(2147483647), UINT32_C(2130303777),
		UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2143188679), UINT32_C(2136746228),
		UINT32_C(2147483647), UINT32_C(2126008810), UINT32_C(2147483647), UINT32_C(2147483647) };

	uint32_t lookupTable2[16] = { UINT32_C(4096000000), UINT32_C(2048000000), UINT32_C(1024000000), UINT32_C(512000000),
		UINT32_C(255744255), UINT32_C(127110228), UINT32_C(64000000), UINT32_C(32258064), UINT32_C(16016016),
		UINT32_C(8000000), UINT32_C(4000000), UINT32_C(2000000), UINT32_C(1000000), UINT32_C(500000),
		UINT32_C(250000), UINT32_C(125000) };

	var_g1 = (int64_t) ((1340 + (5 * (int64_t) range_sw_err)) *
			((int64_t) lookupTable1[gas_range])) >> 16;
	var_g2 = (((int64_t) ((int64_t) gas_res_adc << 15) - (int64_t) (16777216)) + var_g1);
	var_g3 = ((((int64_t) lookupTable2[gas_range]) * (int64_t) var_g1) >> 9);
	//var_g3 = (((int64_t) 8000000 * (int64_t) 41779199) >> 9);
	calc_gas_res = (uint32_t) ((var_g3 + ((int64_t) var_g2 >> 1)) / (int64_t) var_g2);

	if (i2cReadStatusMSB != kWarpStatusOK)
	{
		SEGGER_RTT_WriteString(0, "We can't read from the gas status register for some reason");
	}
	else
	{
		SEGGER_RTT_printf(0, "Gas resistance (ohms):  %u \n", calc_gas_res );
	}
}
