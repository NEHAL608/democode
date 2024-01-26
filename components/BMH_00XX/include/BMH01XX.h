
#define SLAVE_ADDRESS 0x50
#define MAX 2000
#define UART_PORT UART_NUM_1 // Change to the appropriate UART port
#define uart_enable 1


uint16_t Read_Adc(void) ;
void Write_Zero() ;
uint16_t Read_Weight();
uint8_t readCalibration();
void Write_Calibration(uint16_t value);
void weightsensor_init(void);

uint8_t readCalibrationStatus() ;
bool calibrate(uint16_t calWeight) ;
bool readTemperatureValue(uint16_t *tempValue);
bool readADCValue(uint16_t *adcValue) ;

bool readWeightValue(uint16_t *weightValue);
