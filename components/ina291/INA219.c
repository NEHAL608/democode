/*
 * INA219.c
 *
 *  Created on: 25-Jan-2020
 *      Author: NEHAL
 */

#include "INA219.h"

#define HAL_OK 0
/*!
 *  @brief  Sends a single command byte over I2C
 *  @param  reg
 *          register address
 *  @param  value
 *          value to write
 */
int8_t wireWriteRegister(uint8_t addr, uint8_t reg, uint16_t value)
{
	int8_t result = HAL_OK;

	uint8_t data[2];
	data[0] = ( value >> 8 ) & 0xFF;
	data[1] = value & 0xFF;

	if (i2c_master_Write(INA219_ADDRESS,reg,data,2))
		result = 1;

#if DEBUGPRINT
  printf("Function			: wireWriteRegister\r\n");
  printf("Device Address	: %d\r\n",addr);
  printf("Write Register	: %d\r\n",reg);
  printf("Value to write	: %d\r\n",value);
#endif
	return result;
}

/*!
 *  @brief  Reads a 16 bit values over I2C
 *  @param  reg
 *          register address
 *  @param  *value
 *          read value
 */
int8_t wireReadRegister(uint8_t addr, uint8_t reg, uint16_t *value)
{
	int8_t result;
	uint8_t data[2];
	result=i2c_master_Read(INA219_ADDRESS,reg,data,2);
  if(result !=ESP_OK)
    *value=0;
  else  
    *value = ( data[0] << 8 ) + data[1];
	return result;
}

/*!
 *  @brief  Gets the raw bus voltage (16-bit signed integer, so +-32767)
 *  @return the raw bus voltage reading
 */
int16_t getBusVoltage_raw(float *bat_voltage) 
{
  uint16_t value;
  esp_err_t result = ESP_OK;
  result = wireReadRegister(INA219_ADDRESS, INA219_REG_BUSVOLTAGE, &value);
	if(result!=ESP_OK)
	  return 0;
	else
   {
    *bat_voltage=((int16_t)((value >> 3) * 4))* 0.001;
    return 1 ;
   }
  // Shift to the right 3 to drop CNVR and OVF and multiply by LSB
}

/*!
 *  @brief  Gets the raw shunt voltage (16-bit signed integer, so +-32767)
 *  @return the raw shunt voltage reading
 */
int16_t getShuntVoltage_raw(INA219 *ptemp) {
  uint16_t value;
  int8_t result = HAL_OK;

  result = wireReadRegister(INA219_ADDRESS, INA219_REG_SHUNTVOLTAGE, &value);
	if(result)
	{
		value = 0;
	}
#if DEBUGPRINT
  printf("Function			: getShuntVoltage_raw\r\n");
  printf("Device Address	: %d\r\n",ptemp->addr);
  printf("Read Register		: %d\r\n",INA219_REG_SHUNTVOLTAGE);
  printf("ShuntVoltage_raw	: %d\r\n",value);
#endif
  return (int16_t)value;
}

/*!
 *  @brief  Gets the raw current value (16-bit signed integer, so +-32767)
 *  @return the raw current reading
 */
int16_t getCurrent_raw(INA219 *ptemp) {
  uint16_t value;
  int8_t result = HAL_OK;
  // Sometimes a sharp load will reset the INA219, which will
  // reset the cal register, meaning CURRENT and POWER will
  // not be available ... avoid this by always setting a cal
  // value even if it's an unfortunate extra step
  result = wireWriteRegister(ptemp->addr, INA219_REG_CALIBRATION, ptemp->ina219_calValue);

  // Now we can safely read the CURRENT register!
  result = wireReadRegister(ptemp->addr, INA219_REG_CURRENT, &value);
	if(result)
	{
		value = 0;
	}
#if DEBUGPRINT
  printf("Function			: getCurrent_raw\r\n");
  printf("Device Address	: %d\r\n",ptemp->addr);
  printf("Write Register	: %d\r\n",INA219_REG_CALIBRATION);
  printf("Cal_Value			: %d\r\n",ptemp->ina219_calValue);
  printf("Read Register		: %d\r\n",INA219_REG_CURRENT);
  printf("Current_value		: %d\r\n",value);
#endif
  return (int16_t)value;
}

/*!
 *  @brief  Gets the raw power value (16-bit signed integer, so +-32767)
 *  @return raw power reading
 */
int16_t getPower_raw(INA219 *ptemp) {
  uint16_t value;
  int8_t result = HAL_OK;
  // Sometimes a sharp load will reset the INA219, which will
  // reset the cal register, meaning CURRENT and POWER will
  // not be available ... avoid this by always setting a cal
  // value even if it's an unfortunate extra step
  wireWriteRegister(ptemp->addr, INA219_REG_CALIBRATION, ptemp->ina219_calValue);

  // Now we can safely read the POWER register!
  result = wireReadRegister(ptemp->addr, INA219_REG_POWER, &value);
	if(result)
	{
		value = 0;
	}
	#if DEBUGPRINT
  printf("Function			: getPower_raw\r\n");
  printf("Device Address	: %d\r\n",ptemp->addr);
  printf("Write Register	: %d\r\n",INA219_REG_CALIBRATION);
  printf("Cal_Value			: %d\r\n",ptemp->ina219_calValue);
  printf("Read Register		: %d\r\n",INA219_REG_POWER);
  printf("Power_value		: %d\r\n",value);
#endif
  return (int16_t)value;
}


/*!
 *  @brief  Set power save mode according to parameters
 *  @param  on
 *          boolean value
 */
int8_t powerSave(INA219 *ptemp, _Bool on) {
  uint16_t current;
  int8_t result = HAL_OK;
  result = wireReadRegister(ptemp->addr, INA219_REG_CONFIG, &current);
  uint8_t next;
  if (on) {
    next = current | INA219_CONFIG_MODE_POWERDOWN;
  } else {
    next = current & ~INA219_CONFIG_MODE_POWERDOWN;
  }
  result = wireWriteRegister(ptemp->addr, INA219_REG_CONFIG, next);

#if DEBUGPRINT
  printf("Function			: powerSave\r\n");
  printf("Device Address	: %d\r\n",ptemp->addr);
  printf("Read Register		: %d\r\n",INA219_REG_CONFIG);
  printf("Current			: %d\r\n",current);
  printf("Write Register	: %d\r\n",INA219_REG_CONFIG);
  printf("next				: %d\r\n",next);
#endif
  return result;
}

/*!
 *  @brief  Configures to INA219 to be able to measure up to 32V and 2A
 *          of current.  Each unit of current corresponds to 100uA, and
 *          each unit of power corresponds to 2mW. Counter overflow
 *          occurs at 3.2A.
 *  @note   These calculations assume a 0.1 ohm resistor is present
 */
int8_t  setCalibration_32V_2A(INA219 *ptemp) {
	int8_t result = HAL_OK;
  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 32V             (Assumes 32V, can also be set to 16V)
  // VSHUNT_MAX = 0.32          (Assumes Gain 8, 320mV, can also be 0.16, 0.08, 0.04)
  // RSHUNT = 0.1               (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 3.2A

  // 2. Determine max expected current
  // MaxExpected_I = 2.0A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.000061              (61uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0,000488              (488uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0001 (100uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 4096 (0x1000)

	ptemp->ina219_calValue = 4096;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.002 (2mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 3.2767A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.32V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 3.2 * 32V
  // MaximumPower = 102.4W

  // Set multipliers to convert raw current/power values
  ptemp->ina219_currentDivider_mA = 10; // Current LSB = 100uA per bit (1000/100 = 10)
  ptemp->ina219_powerMultiplier_mW = 2; // Power LSB = 1mW per bit (2/1)

  // Set Calibration register to 'Cal' calculated above
  result = wireWriteRegister(ptemp->addr, INA219_REG_CALIBRATION, ptemp->ina219_calValue);

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  result = wireWriteRegister(ptemp->addr, INA219_REG_CONFIG, config);

#if DEBUGPRINT
  printf("Function : setCalibration_32V_2A\r\n");
#endif
  return result;
}

/*!
 *  @brief  begin I2C and set up the hardware
 */
int8_t init_INA219(uint8_t Addr, INA219 *ptemp)
{
	int8_t result = HAL_OK;
//	hi2c = phi2c;
	// Set chip address
	ptemp->addr = Addr;
	// Set chip to large range config values to start
	result = setCalibration_16V_400mA(ptemp);
 // result = setCalibration_32V_1A(ptemp);
#if DEBUGPRINT
  printf("Function : init_INA219\r\n");
#endif
  return result;
}

/*!
 *  @brief  Configures to INA219 to be able to measure up to 32V and 1A
 *          of current.  Each unit of current corresponds to 40uA, and each
 *          unit of power corresponds to 800�W. Counter overflow occurs at
 *          1.3A.
 *  @note   These calculations assume a 0.1 ohm resistor is present
 */
int8_t setCalibration_32V_1A(INA219 *ptemp) {
	int8_t result = HAL_OK;
  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 32V		(Assumes 32V, can also be set to 16V)
  // VSHUNT_MAX = 0.32	(Assumes Gain 8, 320mV, can also be 0.16, 0.08, 0.04)
  // RSHUNT = 0.1			(Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 3.2A

  // 2. Determine max expected current
  // MaxExpected_I = 1.0A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000305             (30.5�A per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.000244              (244�A per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0000400 (40�A per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 10240 (0x2800)

  ptemp->ina219_calValue = 10240;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.0008 (800�W per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 1.31068A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // ... In this case, we're good though since Max_Current is less than
  // MaxPossible_I
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.131068V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 1.31068 * 32V
  // MaximumPower = 41.94176W

  // Set multipliers to convert raw current/power values
  ptemp->ina219_currentDivider_mA = 25;    // Current LSB = 40uA per bit (1000/40 = 25)
  ptemp->ina219_powerMultiplier_mW = 0.8f; // Power LSB = 800uW per bit

  // Set Calibration register to 'Cal' calculated above
  result = wireWriteRegister(ptemp->addr, INA219_REG_CALIBRATION, ptemp->ina219_calValue);

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  result = wireWriteRegister(ptemp->addr, INA219_REG_CONFIG, config);
#if DEBUGPRINT
  printf("Function : setCalibration_32V_1A\r\n");
#endif
  return result;
}


/*!
 *  @brief set device to alibration which uses the highest precision for
 *     current measurement (0.1mA), at the expense of
 *     only supporting 16V at 400mA max.
 */
int8_t setCalibration_16V_400mA(INA219 *ptemp) {
	int8_t result = HAL_OK;
  // Calibration which uses the highest precision for
  // current measurement (0.1mA), at the expense of
  // only supporting 16V at 400mA max.

  // VBUS_MAX = 16V
  // VSHUNT_MAX = 0.04          (Assumes Gain 1, 40mV)
  // RSHUNT = 0.1               (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 0.4A

  // 2. Determine max expected current
  // MaxExpected_I = 0.4A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000122              (12uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.0000977              (98uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.00005 (50uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 8192 (0x2000)

  ptemp->ina219_calValue = 8192;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.001 (1mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 1.63835A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_Current_Before_Overflow = MaxPossible_I
  // Max_Current_Before_Overflow = 0.4
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.04V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If
  //
  // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Max_ShuntVoltage_Before_Overflow = 0.04V

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 0.4 * 16V
  // MaximumPower = 6.4W

  // Set multipliers to convert raw current/power values
  ptemp->ina219_currentDivider_mA = 20;    // Current LSB = 50uA per bit (1000/50 = 20)
  ptemp->ina219_powerMultiplier_mW = 1.0f; // Power LSB = 1mW per bit

  // Set Calibration register to 'Cal' calculated above
  result = wireWriteRegister(ptemp->addr, INA219_REG_CALIBRATION, ptemp->ina219_calValue);

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  result = wireWriteRegister(ptemp->addr, INA219_REG_CONFIG, config);

#if DEBUGPRINT
  printf("Function : setCalibration_16V_400mA\r\n");
#endif

  return result;
}

/*!
 *  @brief  Gets the shunt voltage in mV (so +-327mV)
 *  @return the shunt voltage converted to millivolts
 */
float getShuntVoltage_mV(INA219 *ptemp) 
{
  int16_t value;
  value = getShuntVoltage_raw(ptemp);

#if DEBUGPRINT
  printf("Function : getShuntVoltage_mV\r\n");
#endif

  return value * 0.01;
}

/*!
 *  @brief  Gets the shunt voltage in volts
 *  @return the bus voltage converted to volts
 */
uint8_t getBusVoltage_V(float *bat_voltage) {
  return getBusVoltage_raw(bat_voltage);
}

/*!
 *  @brief  Gets the current value in mA, taking into account the
 *          config settings and current LSB
 *  @return the current reading convereted to milliamps
 */
float getCurrent_mA(INA219 *ptemp) {
  float valueDec = getCurrent_raw(ptemp);
	if(valueDec == 0.0)
		return 0.0;
  valueDec /= ptemp->ina219_currentDivider_mA;
#if DEBUGPRINT
  printf("Function : getCurrent_mA\r\n");
#endif
  return valueDec;
}

/*!
 *  @brief  Gets the power value in mW, taking into account the
 *          config settings and current LSB
 *  @return power reading converted to milliwatts
 */
float getPower_mW(INA219 *ptemp) {
  float valueDec = getPower_raw(ptemp);
  valueDec *= ptemp->ina219_powerMultiplier_mW;
#if DEBUGPRINT
  printf("Function : getPower_mW\r\n");
#endif
  return valueDec;
}
