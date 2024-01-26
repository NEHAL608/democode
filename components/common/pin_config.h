/* * pin configuration of device as per schemetic 50002_02 */

/* * ======== UART1 ======== */
 #define TXD1_LCD 1 // TX (Transmit) pin for the LCD communication
 #define RXD1_LCD 2 // RX (Receive) pin for the LCD communication


/* * ======== UART2 ======== */
 #define TXD2_BMH 41 // TX BMH_001220
 #define RXD2_BMH 42 // RX BMH_00120

/* *  ======== I2C0 ======== */
#define EXT0_SCL                11     /*!< GPIO number used for I2C master clock */
#define EXT0_SDA                10     /*!< GPIO number used for I2C master data  */
#define I2C0_NUM_0              0       /*!< I2C master i2c port number */
#define I2C_MASTER_FREQ_HZ      100000  /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS   1000



/* *  ======== sd card SPI ======== */
#define SD_CARD_CS              12   //20
#define SD_CARD_MOSI            13   //21sdi in on sensor
#define SD_CARD_CLK             14   //2
#define SD_CARD_MISO            21    //23sdo

/* ======== PWM ======== */
#define MOTOR_PWM_A 15 // PWM output for Motor input 1
#define MOTOR_PWM_B 16 // PWM output for Motor input 2
#define FAN_PWM     20 // PWM output for controlling the fan

/* ======== ADC   7======== */
#define VPROPI_STR  8      // Analog input channel 2 for VPROPI current sensing
#define NTC_BATTERY 4      // Analog input channel 3 for back-up battery voltage sensing
#define NTC_PROB_1 5
#define NTC_PROB_2 6
//#define SPEKER     6
//#define MIC        8

/*INPUT BUTTON*/
#define POWER_BUTTON      0 // gpio  input for Power button
#define MODE_CONTROL_BUTTON   18 //   TOUCH_PAD_NUM7,     // 'mode' button.


#define V_LCM_CNTR              3
//#define CHRG_STAT               41
//#define CHRGING_EN              42   
#define CHGR_FLAG               32
#define SD_CARD_EN              45 //sdcard coonect with lcd
#define SD_CARD_SPI_EN          38 //esp32 with sdcard




