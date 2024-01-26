
#define MOTOR_STATUS_COMMAND     1  // Provides motor status (on or off)
#define PLAY_PAUSE_COMMAND       2  // Play and pause command to server node
#define MOTOR_SPEED_COMMAND      3
#define OK_COMMAND               4
#define MODE_STATE_COMMAND      5
#define CANCEL_COMMAND           6
#define SAVE_COMMAND             7



void ble_mesh_send_vendor_message(uint16_t command,uint8_t VAL);
void ble_mesh_GET_vendor_message();

void button_press_5s_cb(void* arg);
void onBluetoothTap(void* arg);
void ble_mesh_device_init_client(void);
