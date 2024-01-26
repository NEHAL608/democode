 #include "main_driver.h"
 #include <sdkconfig.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/timers.h>
 #include <button.h>
 #include <esp_rmaker_core.h>
 #include <esp_rmaker_standard_types.h>
 #include <esp_rmaker_standard_params.h>
 #include <app_reset.h>
 #include <esp_wifi.h>
#include <wifi_provisioning/manager.h>
 #include "driver/ledc.h"
#include "BMH01XX.h"
#include "app_priv.h"

const char *TAG="driver";

DGUS_obj  BAT_VARIBLE,BAT_ICON,MOTOR_ICON,playPauseButton,OK_BTN,currentModeControl,LOAD,cancelButton,AlertBar,hour,minute,second,
          Interval_m,Interval_s,nextButton,SKIP_BTN,endButton, unlockButton,wifiButton,forcast,ROLLTEXT,LISTVIEW,RECIPE_SEND,editButton,
          SSID,PASSWORD,timerSelectionControl,getrecipeSteps,saveButton,stirControl, PROBE1_C,PROBE2_C,PROBE1_F,PROBE2_F,motorSpeedControl,textInputControl,
          dwin_record_recipe,bluetoothButton,edit_recipename, serving,preprationtime,cooking,totaltime;

DGUS_obj  *D_list[]={&playPauseButton,&OK_BTN,&currentModeControl,&cancelButton,&wifiButton,&motorSpeedControl,&Interval_s,
                     &nextButton,&unlockButton,&endButton,&hour,&minute,&Interval_m,
                     &serving,&preprationtime,&cooking,&totaltime,&edit_recipename,
                     &timerSelectionControl,&getrecipeSteps,&saveButton,&stirControl,&bluetoothButton,&editButton,NULL};

DGUS_obj  *string_list[]={&textInputControl,&dwin_record_recipe,&SSID,&PASSWORD,NULL};

volatile bool PlayPause_Flag = false; /*play btn for auto and manual */;
volatile uint32_t previousDutyCycle = 0; // Previous duty cycle value

static bool Interval_Flag,updateCSV_flag=1;
bool Countdown_Flag;      /*for clockwise and anticlcok timer tick  t tick =1 second*/
uint32_t Total_interval,Interval_counter;
extern bool ispaired ;
// Cooling fan speed
int fan_speed = 0;
extern uint16_t previous_cmd;
extern uint8_t previous_value;
// Define the weight sensor task handle
TaskHandle_t weight_sensor_task_handle = NULL;

extern uint16_t  ble_temprature,ble_recive_speed; 

AlertStatus alertStatus;
extern void  weight_sensor_task(void *pvParameters) ;
static void main_task(void *pvParameters) ;
// For Probe 1
float probe1_Fahrenheit, probe1_Celsius;
// For Probe 2
float probe2_Fahrenheit, probe2_Celsius;
// For battery ntc
float battery_Fahrenheit=0, battery_Celsius=0;

static bool isUnlockButtonPressed = false;
static esp_timer_handle_t periodicTimerHandle;
static TaskHandle_t motorControlTaskHandle = NULL;
bool wifiConnected;
uint8_t currentStep;

struct DeviceStatus myDevice;
char qrcode_text[100] = "";
// Define a structure for recipe steps
struct RecipeStep {
    int hour;
    int minute;
    int intervalMinutes;
    int intervalSeconds;
    int motorSpeed;
    int motorLoad;
    int temperatures;
    char description[60];
};

// Define a structure for a recipe
struct UserRecipe {
    int id;
    char recipeName[50];
    char recipeType[50];
    int servings;
    int prep_time;
    int cook_time;
    int total_time;
    int numberOfSteps;
    struct RecipeStep* steps; // Pointer to a dynamic array
};

struct UserRecipe myUserRecipe;

     const char *file_recipe ="/sdcard/DEVICE.csv";

void periodic_timer_callback(void* arg){
  Countdown_Flag=1;
   //       ESP_LOGI(TAG, "periodic_timer_callback");

}

void Timer_init(void){   /*1 second timer*/
  const esp_timer_create_args_t periodic_timer_args = {
          .callback = &periodic_timer_callback,
          /* name is optional, but may help identify the timer when debugging */
          .name = "periodic"
  };
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodicTimerHandle));
}

// Define a timer handle
TimerHandle_t delayTimer;

// Callback function to be called when the timer expires
void delayTimerCallback(TimerHandle_t xTimer) {
    // Perform action (e.g., setting page 3)
    dgus_set_page(3);

    // Stop the timer since the callback occurred
    if (xTimer != NULL) {
        xTimerStop(xTimer, portMAX_DELAY);
    }
}

// Function to start a delay of 1000 milliseconds (1 second)
void startDelay(void) {
    // Create a timer with a 1000 ms (1 second) delay
    delayTimer = xTimerCreate("DelayTimer", pdMS_TO_TICKS(1000), pdFALSE, (void *)0, delayTimerCallback);

    // Start the timer
    if (delayTimer != NULL) {
        xTimerStart(delayTimer, portMAX_DELAY);
    }
}

void PWM_init(void)
{
     // Prepare and then apply the LEDC PWM timer configuration 
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration motor
    ledc_channel_config_t channel_0 = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .gpio_num       = MOTOR_PWM_A,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

     // Prepare and then apply the LEDC PWM channel configuration for motor
  ledc_channel_config_t channel_1 = {
      .speed_mode     = LEDC_MODE,
      .channel        = LEDC_CHANNEL_1,
      .timer_sel      = LEDC_TIMER,
      .gpio_num       = MOTOR_PWM_B,
      .duty           = 0, // Set duty to 0%
      .hpoint         = 0
  };

    //for timer channel 2  for fan 
  ledc_channel_config_t channel_2 = {
      .gpio_num = FAN_PWM,
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_2,
      .timer_sel = LEDC_TIMER,
      .duty = 0,
      .hpoint = 0
  };
   ESP_ERROR_CHECK(ledc_channel_config(&channel_0));
   ESP_ERROR_CHECK(ledc_channel_config(&channel_1));
   ESP_ERROR_CHECK(ledc_channel_config(&channel_2));
}


// Function to create or delete weight sensor task based on myDevice.cureentstatus
void manage_weight_sensor_task(){
  if (myDevice.cureentstatus == WEIGHT_SCALE)
  {
    if (weight_sensor_task_handle == NULL)
    {
        // Create weight sensor task if not already created
        xTaskCreate(weight_sensor_task, "weight_sensor_task", 2048, NULL, 2, &weight_sensor_task_handle);
        ESP_LOGI(TAG, "Weight sensor task created");
        vTaskDelete(motorControlTaskHandle);
        motorControlTaskHandle=NULL;
    }
  }
  else
  {
    if (weight_sensor_task_handle != NULL)
    {
        // Delete weight sensor task if already created
        vTaskDelete(weight_sensor_task_handle);
        weight_sensor_task_handle = NULL;
        ESP_LOGI(TAG, "Weight sensor task deleted");
        xTaskCreate(main_task, "main_task", 4096, NULL, 1, &motorControlTaskHandle);
    }
  }

}
void getMenuList(size_t ndx) {
    char buf[1024];  /* buffer to hold line */
    char string[256]; /* buffer to hold tokenized string with a null terminator */
    memset(string, 0, sizeof(string));

    size_t j = 0;  /* display index */
    FILE *fp = fopen(file_recipe, "r");  /* file pointer */

    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    // Skip the first row (header)
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        fclose(fp);
        return;  // File is empty or there was an error reading the header
    }

    while (fgets(buf, sizeof(buf), fp)) {
        char *p = buf;
        size_t i = 0;

        /* Loop until the specified column index (ndx) token found */
        for (p = strtok(p, ",\n"); p && i < ndx; p = strtok(NULL, ",\n"))
            i++;

     if (i == ndx && p) {
            // Check if adding the tokenized string exceeds the 250 character limit
            if (strlen(string) + strlen(p) + 2 > 250) {
                // Display the current string and reset it
                if (strlen(string) > 0) {
                    dgus_set_text(0x7000 + j * 0x200, string, strlen(string));
              //     ESP_LOGE(TAG, "%s end %d\n", string,strlen(string));
                    memset(string, 0, sizeof(string));
                    j++;
                }
            }

            // Concatenate the tokenized string into 'string' with a comma separator
            if (strlen(string) > 0) {
                strcat(string, ", ");  // Add a comma separator
            }
            strcat(string, p);
        }
    }

    // Send any remaining data in the string buffer to the display
    if (strlen(string) > 0) {
        dgus_set_text(0x7000 + j * 0x200, string, strlen(string));
        ESP_LOGE(TAG, "%s", string);
    }
  // ESP_LOGE(TAG," list send %d\n",j);
    dgus_set_var(0x1020,j);  // list update
    updateCSV_flag=0;
        dgus_set_page(30);

    fclose(fp);
}

void printRecipe(const struct UserRecipe *recipe) {
    ESP_LOGE(TAG, "Recipe ID: %d", recipe->id);
    ESP_LOGE(TAG, "Recipe Name: %s", recipe->recipeName);
    ESP_LOGE(TAG, "Recipe Type: %s", recipe->recipeType);
    ESP_LOGE(TAG, "Servings: %d", recipe->servings);
    ESP_LOGE(TAG, "Prep Time: %d", recipe->prep_time);
    ESP_LOGE(TAG, "Cook Time: %d", recipe->cook_time);
    ESP_LOGE(TAG, "Total Time: %d", recipe->total_time);
    ESP_LOGE(TAG, "Number of Steps: %d", recipe->numberOfSteps);

    for (int i = 1; i < recipe->numberOfSteps; i++) {
        const struct RecipeStep *step = &recipe->steps[i];
        ESP_LOGE(TAG, "Step %d:", i + 1);
        ESP_LOGE(TAG, "  Hour: %d", step->hour);
        ESP_LOGE(TAG, "  Minute: %d", step->minute);
        ESP_LOGE(TAG, "  Interval Minutes: %d", step->intervalMinutes);
        ESP_LOGE(TAG, "  Interval Seconds: %d", step->intervalSeconds);
        ESP_LOGE(TAG, "  Motor Speed: %d", step->motorSpeed);
        ESP_LOGE(TAG, "  Motor Load: %d", step->motorLoad);
        ESP_LOGE(TAG, "  Temperatures: %d", step->temperatures);
        ESP_LOGE(TAG, "  Description: %s", step->description);
    }
}

void readRecipeRow(int rowIndex, struct UserRecipe *recipe) {
     int stepIndexInRecipe;
         int count = 0;

    FILE *file = fopen(file_recipe, "r");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return;
    }

    char line[1024]; // Adjust the buffer size as per your requirements

    // Skip rows until reaching the desired index
    for (int i = 0; i < rowIndex; ++i) {
        if (fgets(line, sizeof(line), file) == NULL) {
            printf("Row index out of bounds.\n");
            fclose(file);
            return;
        }
    }

    // Read the specific row at the given index
    if (fgets(line, sizeof(line), file) != NULL) {
        char *token = strtok(line, ",");
        int tokenIndex = 0;

        while (token != NULL ) {
            switch (tokenIndex) {
                case 0: // Recipe ID
                    recipe->id = atoi(token);
                    break;
                case 1: // Recipe Name
                    strncpy(recipe->recipeName, token, sizeof(recipe->recipeName) - 1);
                 //  dgus_set_text(0x1250,recipe->recipeName,20);
                    break;
                case 2: // Recipe Type
                    strncpy(recipe->recipeType, token, sizeof(recipe->recipeType) - 1);
                    break;
                case 3: // Servings
                    recipe->servings = atoi(token);
                 dgus_set_val(&serving,recipe->servings);  // list 
                    break;
                case 4: // Prep Time
                    recipe->prep_time = atoi(token);
                             dgus_set_val(&preprationtime,recipe->prep_time);  // list 

                    break;
                case 5: // Cook Time
                    recipe->cook_time = atoi(token);
                             dgus_set_val(&cooking,  recipe->cook_time );  // list 

                    break;
                case 6: // Total Time
                    recipe->total_time = atoi(token);
                             dgus_set_val(&totaltime,  recipe->total_time);  // list 

                    break;
                case 7: // Number of Steps
                    recipe->numberOfSteps = atoi(token);
                    recipe->steps = (struct RecipeStep*)malloc(recipe->numberOfSteps * sizeof(struct RecipeStep));
                    if (recipe->steps == NULL) {
                        printf("Memory allocation for steps failed\n");
                    }
                    break;
               default:
                       stepIndexInRecipe = tokenIndex - 8;
                      if (stepIndexInRecipe >= 0 && stepIndexInRecipe < recipe->numberOfSteps) {

                         struct RecipeStep step;
                          strcpy(step.description, ""); // Initialize description
                          sscanf(token, " %d,%d,%d,%d,%d,%d,%d,", &step.hour, &step.minute, &step.intervalMinutes, &step.intervalSeconds, &step.motorSpeed, &step.motorLoad, &step.temperatures); 
                          sscanf(token, "%*16c%99[^\n]", step.description);                    
                                     printf("%s %x\n",step.description,0X7000+100*stepIndexInRecipe);
                          dgus_set_text(0X7000+100*stepIndexInRecipe,step.description,100);                  
                        recipe->steps[stepIndexInRecipe] = step;
                     }
                  break;
            }
            if (tokenIndex<7)
            {
            token = strtok(NULL, ",");
            tokenIndex++;
            }
            else
            {
            token = strtok(NULL, ";");
            tokenIndex++;
            }      
        }
    } 
    free(recipe->steps);
    fclose(file);
    dgus_set_val(&RECIPE_SEND,recipe->numberOfSteps);  // list
}

void onUpdateRecipeSteps(){
    ESP_LOGI(TAG,"getrecipeSteps  %d" ,getrecipeSteps.value);
   readRecipeRow(getrecipeSteps.value, &myUserRecipe); // Read the first 
    printRecipe(&myUserRecipe);     // Print the recipe details
}

static void MotorspeedDCallback(){
    ESP_LOGI(TAG,"MotorspeedDCallback  %x" ,motorSpeedControl.value);
    dgus_set_text(0x1203," speed set done",30);
    if (ispaired) {
      ble_mesh_send_vendor_message(MOTOR_SPEED_COMMAND,motorSpeedControl.value); //send speed value to striaid board
        previous_cmd=MOTOR_SPEED_COMMAND,previous_value=motorSpeedControl.value;
   }
}

static void hoursetCallback(){
    ESP_LOGI(TAG,"hour   %d minute %d" ,hour.value,minute.value);
    dgus_set_text(0x1203," hour set done",30);
    esp_timer_start_periodic(periodicTimerHandle, 1000000);
}

static void intervalminutesetCallback(){
    ESP_LOGI(TAG,"interval minute   %d interval second %d" ,Interval_m.value,Interval_s.value);

     Interval_Flag=1;
    Total_interval=(Interval_s.value+ (60*Interval_m.value));
    esp_timer_start_periodic(periodicTimerHandle, 1000000);
    dgus_set_text(0x1203,"interval set done",20);
}    
/** play/pause button callback function *controll auto and manual  */
void PlayCallback(bool state ,void *ptr){

   PlayPause_Flag=!PlayPause_Flag;
  
   if(PlayPause_Flag){
       esp_timer_start_periodic(periodicTimerHandle, 1000000);
        if (ispaired) {

        ble_mesh_send_vendor_message(PLAY_PAUSE_COMMAND,1);
     //   previous_cmd=PLAY_PAUSE_COMMAND,previous_value=PlayPause_Flag;
        }
       if (myDevice.operationMode==RECIPES_MODE)
          dgus_set_page(18);
   }
    else
     {
           if (ispaired) 
                 ble_mesh_send_vendor_message(PLAY_PAUSE_COMMAND,0);
       if (myDevice.operationMode==RECIPES_MODE)
          startDelay();
        esp_timer_stop(periodicTimerHandle);
     }
    // rainamker_powerbutton_update(PlayPause_Flag);

}

void ModeCallback(void *ptr) {
    Interval_Flag = 1;
    ESP_LOGI(TAG, "ModeCallback %d",currentModeControl.value);

    myDevice.operationMode = currentModeControl.value;
    myDevice.hour = 0;
    myDevice.minute = 0;
    myDevice.cureentstatus = STIRAID_DEVICE;
    PlayPause_Flag=0;
    switch (currentModeControl.value) {
        case MANUAL_MODE:
            myDevice.minute = 30;
            myDevice.motorSpeed = 3;
            second.value = 59;

             dgus_set_page(7);
            break;

        case RECIPES_MODE:
        //if (updateCSV_flag){
          getMenuList(1);  // Column of 1 CSV file stores recipe names
     //   }
        // else
        //  { dgus_set_var(0x1020,0);  // list 
        //      dgus_set_page(30);
        //  }
         break;
        case AUTO_MODE:
            myDevice.minute = 30;
                second.value = 59;

            dgus_set_page(7);
            break;
        case RECORD_MODE:
             myUserRecipe.steps = malloc(20 * sizeof(struct RecipeStep));
             dgus_set_page(13);
             myDevice.motorSpeed = 3;
            break;
        default:
            if (currentModeControl.value > 4 && currentModeControl.value < 9) {
                myDevice.cureentstatus = MAIN_DEVICE;
            } else if (currentModeControl.value == WEIGHT_SCALE_MODE || currentModeControl.value == NUTIRTION_MODE) {
                myDevice.cureentstatus = WEIGHT_SCALE;
            }
            break;
    }

    if (myDevice.cureentstatus == STIRAID_DEVICE && ispaired) {
        ble_mesh_send_vendor_message(MODE_STATE_COMMAND,  myDevice.operationMode);
        ble_mesh_send_vendor_message(MOTOR_SPEED_COMMAND,  myDevice.motorSpeed); //send speed value to striaid board
        rainamker_modebutton_update(myDevice.operationMode);
        previous_cmd=MODE_STATE_COMMAND,previous_value=myDevice.operationMode;
    }
    manage_weight_sensor_task();
}

void onStirChange(void *ptr)
{
     ESP_LOGI(TAG,"stirControl SELECT %d",ispaired );

   if (ispaired){
    dgus_set_page(3);
   }
    else
      dgus_set_text(0x1203,"pair device long press the button 3 sec ",50);
}

void OkCallback(void *ptr)
{
  ESP_LOGI(TAG,"OK_BTN  %x" ,OK_BTN.value);
  bool provisioned;

  switch(OK_BTN.value)
  {
      case 0x23: //wifi reset
            wifi_prov_mgr_is_provisioned(&provisioned);
            if (provisioned) {
                  esp_rmaker_wifi_reset(0, 2);
                  dgus_set_page(36);
                  dgus_set_text(0x1220,"wifi reset sucess",30);
                  startDelay();

            }
      break;
   }
}

void onCancel(void *ptr)
{
    ESP_LOGE(TAG, "ncancel ");
    esp_timer_start_periodic(periodicTimerHandle, 1000000);
}


// Function to save a recipe step
void saveRecipeStep(struct RecipeStep step) {
    if (myUserRecipe.numberOfSteps < 10) {
        myUserRecipe.steps[myUserRecipe.numberOfSteps] = step;
        myUserRecipe.numberOfSteps++;
    } else {
        // Handle maximum steps reached
    }
}
// Function to save the entire recipe to a CSV file
void saveRecipeToCSV(uint8_t save_data ,struct UserRecipe recipe) {
        char recipeStep[50] = {0};
    char recipeName[20] = {0};
    static uint8_t recipeNumber;

    FILE* file = fopen(file_recipe, "a+");
    if (file == NULL) {
        printf("Error opening file for writing\n");
        return;
    }
    recipeNumber++;
    if (save_data==43)
    {
       strcpy(recipe.recipeName,edit_recipename.stringValue);
        recipe.servings=serving.value;
        recipe.prep_time=preprationtime.value;
        recipe.cook_time=cooking.value;
        recipe.total_time=totaltime.value;
    }
    else
    {
     snprintf(recipe.recipeName, sizeof(recipeName), "recipe%d", recipeNumber);
        recipe.servings=1;
        recipe.prep_time=31;
        recipe.cook_time=14;
        recipe.total_time=1;
    }

    ESP_LOGE(TAG, " %s,recipe number%d", recipe.recipeName,recipeNumber);
    strcpy(recipe.recipeType, "record"); // Replace with the actual recipe type
    
    // Save the recipe name and type to a file or do any other necessary operations
    fprintf(file, " %d,", recipeNumber);

    fprintf(file, " %s,", recipe.recipeName);
    fprintf(file, " %s,", recipe.recipeType);
      // Save other recipe details (servings, prep time, etc.) if needed
    fprintf(file, " %d,", recipe.servings);
    fprintf(file, " %d,", recipe.prep_time);
    fprintf(file, " %d,", recipe.cook_time);
    fprintf(file, " %d,", recipe.total_time);
    fprintf(file, " %d,", recipe.numberOfSteps);

    // Write the recipe steps
    for (int i = 0; i < recipe.numberOfSteps; i++) {
        fprintf(file, " %d, %d, %d,%d,%d %d,%d", recipe.steps->hour,recipe.steps->minute, recipe.steps->intervalMinutes, recipe.steps->intervalSeconds, recipe.steps->motorSpeed, recipe.steps->motorLoad, recipe.steps->temperatures);
        fprintf(file, "%s,",  recipe.steps->description);
    }
        fprintf(file, "\n" );

    // Close the CSV file
    fclose(file);
}

// Function to save the RecipeStep
void recipemode_saveRecipeStep(int stepIndex) {
    if (stepIndex < myUserRecipe.numberOfSteps) {
        // Access the step from myUserRecipe based on the stepIndex
        struct RecipeStep step = myUserRecipe.steps[stepIndex];

        // Save the step to myDevice
        myDevice.hour = step.hour;
        myDevice.minute = step.minute;
        myDevice.intervalMin = step.intervalMinutes;
        myDevice.intervalSec = step.intervalSeconds;
        myDevice.motorSpeed = step.motorSpeed;
        myDevice.temperatures = step.temperatures;
        myDevice.loadDetect = step.motorLoad;
        // Assuming other members of myDevice are appropriately handled
    } else {
        // Handle the case when the stepIndex is out of range
        ESP_LOGE(TAG, "Step index out of range.");
    }
}

void onNext() {
    ESP_LOGE(TAG, "next button press %d", nextButton.value);
    char displayBuffer[30];

 if (myDevice.operationMode==RECORD_MODE)
 {
     struct RecipeStep step;

    // Populate step details (you need to obtain these values from user input)
    step.hour = myDevice.hour;
    step.minute =   myDevice.minute;
    step.intervalMinutes = myDevice.intervalMin;
    step.intervalSeconds = myDevice.intervalSec;
    step.motorSpeed = myDevice.motorSpeed;
    step.temperatures=ble_temprature;
    step.motorLoad = myDevice.loadDetect;
    sprintf(step.description, "Add ingredients Num_%d   ", nextButton.value);      // Save the step in the recipe
    saveRecipeStep(step);
 }

if (myDevice.operationMode==RECIPES_MODE){
recipemode_saveRecipeStep(nextButton.value-1);
}
}

void onEnd(void *ptr)
{
  ESP_LOGE(TAG, "end button press %d",endButton.value);
 // PlayPause_Flag=0;
  //esp_timer_stop(periodicTimerHandle);
  //send ble cmd to striaid
 if (myDevice.operationMode==RECORD_MODE)
    {
        struct RecipeStep step;

        // Populate step details (you need to obtain these values from user input)
        step.hour = myDevice.hour;
        step.minute =   myDevice.minute;
        step.intervalMinutes = myDevice.intervalMin;
        step.intervalSeconds = myDevice.intervalSec;
        step.motorSpeed = myDevice.motorSpeed;
        step.temperatures=ble_temprature;
        step.motorLoad = myDevice.loadDetect;
        step.temperatures=ble_temprature;
        sprintf(step.description, "add intergredient %d", endButton.value);

        // Save the step in the recipe
        saveRecipeStep(step);
    }
}

void saveButtonPressed(void *param)
{
    ESP_LOGI(TAG,"saveButtonPressed %d",saveButton.value);
    saveRecipeToCSV(saveButton.value, myUserRecipe);
  //  free( myUserRecipe.steps);

    dgus_set_page(36);
    dgus_set_text(0x1220,"record recipe save sucess",30);
   // updateCSV_flag=1;
    startDelay();
}

void onUnlockButton(void *arg)
{
    isUnlockButtonPressed = !isUnlockButtonPressed;
}


void print_qr_code(char *payload)
{
  strcpy(qrcode_text, payload); // Copy payload to qrcode_text
 // printf("%s. %d\n",qrcode_text,strlen(payload));
 
    // Open NVS handle
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    // Store QR code payload in NVS
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "qrcode_payload", payload));

    // Commit changes
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));

    // Close NVS handle
    nvs_close(nvs_handle);
}

void save_qr_code() {
    // Open NVS handle
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

    // Retrieve QR code payload from NVS
    size_t length = sizeof(qrcode_text);
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "qrcode_payload", qrcode_text, &length));

    // Close NVS handle
    nvs_close(nvs_handle);
  dgus_qr_code_genrate(0x5600,qrcode_text );
 //     dgus_qr_code_genrate(0x1203,qrcode_text );
 //   printf("%s. %d\n", qrcode_text, strlen(qrcode_text));
}

void onWifiProvisioning()
{
    bool isProvisioned;
    char ssid[20] = {0};
    char password[20] = {0};
    wifi_config_t wifiConfig;

    wifi_prov_mgr_is_provisioned(&isProvisioned);

    strcpy(ssid,SSID.stringValue);
    strcpy(password,PASSWORD.stringValue);
    ESP_LOGE(TAG, "SSID: %s, Password: %s", ssid, password);

    if (!isProvisioned) 
    {
        memset(&wifiConfig, 0, sizeof(wifiConfig));
        strcpy((char*)wifiConfig.sta.ssid, ssid);
        strcpy((char*)wifiConfig.sta.password, password);

        esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
        esp_wifi_start();
        esp_restart();
    }
    else
    {
        dgus_set_text(0x1203, "hi  nehal Device already connected reset wifi ", 30);
    }
}

void onTimerSelection(void *ptr)
{
    ESP_LOGI(TAG,"timer select");
    esp_timer_stop(periodicTimerHandle);
}
void onTextInput(void *ptr)
{
        ESP_LOGI(TAG,"text select");
        // dgus_set_var(0x1011,1);  // list 
}

void onEditButton(void *ptr) {
    char buffer[250]={0};

    if (myDevice.operationMode == RECORD_MODE) {
        strcpy(buffer,dwin_record_recipe.stringValue);
       // ESP_LOGI(TAG, "EDIT BUTTON select %d %s", editButton.value,buffer);

        // Ensure editButton.value is within a valid range
        if (editButton.value >= 0 && editButton.value < myUserRecipe.numberOfSteps) {

            // Update the description of the specified step
            strncpy(myUserRecipe.steps[editButton.value].description, buffer, sizeof(myUserRecipe.steps[editButton.value].description) - 1);
            myUserRecipe.steps[editButton.value].description[sizeof(myUserRecipe.steps[editButton.value].description) - 1] = '\0';
        } else {
            // Handle invalid index (out of bounds)
            ESP_LOGE(TAG, "Invalid step index for editing");
        }
    }
}


void anticlockwise_Time()
{
    if (!Countdown_Flag)
        return;

#ifdef SELF_TEST
    if (myDevice.hour >= 10 || myDevice.minute >= 60 || second.value > 60)
        return;
#endif

    if (myDevice.minute == 0 && myDevice.hour == 0) // Countdown timer zero
    {
#ifdef DEBUG_ON
        ESP_LOGI(TAG, "Nonzero_Timer");
#endif
        myDevice.motorSpeed = 0;
        return;
    }
    if (Countdown_Flag) // Timer starts if the play button is on
    {
        Countdown_Flag = 0;
        second.value--;
         ESP_LOGI(TAG, "second: %d  ",  second.value);

        if (Total_interval)
            Interval_counter++;

        if (second.value == 0)
        {
             second.value=59;
            if (myDevice.minute == 0 && myDevice.hour != 0)
            {
                myDevice.hour--;
                myDevice.minute = 59;
            }
            else if (myDevice.minute != 0)
            {
                myDevice.minute--;
            }
            dgus_set_val(&minute, myDevice.minute);
            dgus_set_val(&hour, myDevice.hour);
            
//#ifdef DEBUG_ON
    ESP_LOGI(TAG, "hour: %d minute:%d ", myDevice.hour, myDevice.minute);
//#endif
        }
    }
}

/* Clockwise timer */
void clockwise_Time(void) {
    if (!Countdown_Flag)
        return;

#ifdef SELF_TEST
    if (myDevice.hour > 8) {
        // ESP_LOGI(TAG,"device not work more than hours ");
        return;
    }
#endif

    if (Countdown_Flag) {
        Countdown_Flag = 0;
        second.value++;
        if (Total_interval)
            Interval_counter++;
    }

    if (second.value > 59) {
        myDevice.minute += 1;
        second.value = 0;

    ESP_LOGI(TAG, "hour: %d minute:%d ", myDevice.hour, myDevice.minute);
        dgus_set_val(&minute, myDevice.minute);
    }

    if (myDevice.minute == 60) {
        myDevice.hour += 1;
        myDevice.minute = 0;

    ESP_LOGI(TAG, "hour: %d minute:%d ", myDevice.hour, myDevice.minute);
        dgus_set_val(&hour, myDevice.hour);
    }

    if (myDevice.hour >= 8) {
        myDevice.hour = 0;
        myDevice.minute = 0;
        second.value = 0;
    }

}

/* Interval_function :Interval Time controls ON and OFF time of the Motor*/
void Interval_function()
{
  if (Total_interval > 0)
  {
    if (Interval_counter >= Total_interval)
    {
      Interval_counter = 0;
      Interval_Flag = !MOTOR_ICON.value;
      dgus_set_val(&MOTOR_ICON, Interval_Flag);
    
    if (ispaired) 
      ble_mesh_send_vendor_message(MOTOR_STATUS_COMMAND,Interval_Flag);
//#ifdef DEBUG_ON 
      if (Interval_Flag)
        ESP_LOGI("interval", "motor on"); //send motor on cmd
      else
        ESP_LOGI("interval", "motor off");  //send motor off cmd
//#endif 
    }
  }
}


void fan_control(uint32_t  desiredSpeed)
{
  uint64_t   motorDutyCycle = (desiredSpeed*10)*4095/100;  // Calculate duty cycle based on speed

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, motorDutyCycle));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2));
}

/* Set the motor speed.
 * @param speed - Speed value from 0 to 9.
 */
void setMotorSpeed(int32_t desiredSpeed) {
    if (desiredSpeed < 0 || desiredSpeed > 10) {
        ESP_LOGI(TAG, "Invalid speed value. S");
        return;
    }

    uint32_t motorDutyCycle = (desiredSpeed * 10) * (4095 / 100);
    myDevice.motorStatus = (desiredSpeed != 0);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, motorDutyCycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, motorDutyCycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
    dgus_set_var(0x1068, desiredSpeed);

#ifdef DEBUG_ON
    ESP_LOGI(TAG, "Motor speed: %d motor status: %d", desiredSpeed, myDevice.motorStatus);
#endif
    rainamker_speed_update(desiredSpeed);
}
// static void maindeviccontrol(uint8_t rxval)
// {
//   switch (rxval)
//   {
//       case BLENDING_MODE/* constant-expression */:
//       myDevice.motorSpeed=1;
//       break;

//       case WHISKHING_MODE/* constant-expression */:
//       myDevice.motorSpeed=2;
//       break;

//       case FROTHING_MODE/* constant-expression */:
//       myDevice.motorSpeed=3;
//       break;

//       case MIXING_MODE/* constant-expression */:
//       myDevice.motorSpeed=4;
//       break;
      
//       case CHOPPING_MODE/* constant-expression */:
//       myDevice.motorSpeed=5;
//       break;

//       default:
//           myDevice.motorSpeed=0;
//       break;
//   }
// }


/* motor overload detection 
  VIPROPI_STR   shall be used to determine the load on the Motor.
  This is only for horizontal operational mode.*/
static void motorOverloadDetection(void)
{
   
    float voltage = getOverloadVoltage();

    if (voltage >= 2.1)
    {
#ifdef DEBUG_ON
        ESP_LOGI(TAG, "System shutdown due to overload detection");
#endif
        return;
    }

    if (voltage < 0.42)
    {
       // myDevice.loadDetection = 0; // No Load
            LOAD.value =0;
#ifdef DEBUG_ON
        ESP_LOGI(TAG, "No Load");
#endif
    }
    else
    {
        //myDevice.loadDetection = 1; // Load detected
        if (voltage >= 0.43 && voltage <= 0.59)
            LOAD.value = 1;
        else if (voltage >= 0.60 && voltage <= 0.85)
            LOAD.value = 2;
        else if (voltage >= 0.86 && voltage <= 1.11)
            LOAD.value =3;
        else if (voltage >= 1.12 && voltage <= 1.37)
            LOAD.value =4;
        else if (voltage >= 1.38 && voltage <= 1.9)
            LOAD.value = 5;
        else if (voltage >= 2)
        {
            alertStatus = OVERLOAD;
            LOAD.value =6;
#ifdef DEBUG_ON
            ESP_LOGI(TAG, "Overload detected");
#endif
        }
        if (alertStatus == OVERLOAD && voltage <= 1.9)
        {
          alertStatus = CLEAR;
        }
    }
   // dgus_set_val(&LOAD, LOAD.value);
}

/* "VIPROPI_OC" will provide protection against voltage overcurrent.*/
// static void motor_overcurrent_detection(void)
// {
//   float  votage_oc=get_Overcurrent_V();
  
//   #ifdef SELF_TEST 
//   if(votage_oc>=2)
//   {
//    #ifdef debug_on
//     ESP_LOGI(TAG,"overload detect system shutdown ");
//      #endif

//     return;
//   }
//   #endif
// //  for(i=0;i<10;i+=0.8)
//  // {
//    #ifdef debug_on 
//    ESP_LOGI(TAG,"votage_oc: %f",votage_oc);
//      #endif

//     if(votage_oc>=1.71)
//     {
//       dgus_get_page(&pageId);
//       AlertSystem=overcurrent;
//      #ifdef debug_on 
//      ESP_LOGI(TAG,"overcurrent detect");
//        #endif

//       dgus_set_text(notification.addr,"overcurrent detect   ");
//       dgus_set_page(10);
//     }
//     else if(AlertSystem==overcurrent && votage_oc<1.71 )
//       AlertSystem=clear;   
//  // }
// }

// Function to calculate the battery percentage based on the voltage reading
static void calculate_battery_percentage(void)
{
    uint8_t batteryPercentage = 0;
    float batteryVoltage;
    
    // Get the battery voltage reading
    getBusVoltage_V(&batteryVoltage);
    
    // Check the battery voltage and calculate the percentage accordingly
    if (batteryVoltage < 6.4)
    {
        batteryPercentage = 0;
        //set an alertStatus here
    }
    else
    {
        // Clear the previous alertStatus, if any
        if (alertStatus == BATTERY_LOW)
        {
            // dgus_clear_text(AlertBar.addr);
            alertStatus = CLEAR;
        } 
        
        if (batteryVoltage >= 9.7)
            batteryPercentage = 100;
        else if (batteryVoltage >= 9.0 && batteryVoltage < 9.7)
            batteryPercentage = 80;
        else if (batteryVoltage >= 8.2 && batteryVoltage < 9.0)
            batteryPercentage = 60;
        else if (batteryVoltage >= 7.3 && batteryVoltage < 8.2)
            batteryPercentage = 40;
        else if (batteryVoltage >= 6.4 && batteryVoltage < 7.3)
            batteryPercentage = 20;
    }
          dgus_set_val(&BAT_VARIBLE, batteryPercentage);
     //     dgus_set_val(&BAT_ICON, probe1_Celsius);

    #ifdef DEBUG_ON
//    ESP_LOGI(TAG, "batteryPercentage: %d batteryVoltage: %f", batteryPercentage, batteryVoltage);
    #endif
}

// Callback function for button press event
void button_press_callback(void *arg) {

 if (myDevice.cureentstatus == MAIN_DEVICE) {

    PlayPause_Flag = true;

    myDevice.motorSpeed=motorSpeedControl.value;
    ESP_LOGI(TAG, "Button Pressed %d %d ",PlayPause_Flag,myDevice.motorSpeed);
    setMotorSpeed(myDevice.motorSpeed);
 //  fan_control(1); // Turn on cooling fan

}
}

// Callback function for button release event
void button_release_callback(void *arg) {
     if (myDevice.cureentstatus == MAIN_DEVICE) {

    PlayPause_Flag = false;
    ESP_LOGI(TAG, "Button Released %d",PlayPause_Flag);
     fan_control(0); // Turn off cooling fan
    setMotorSpeed(0);
     }
}
void modecontrol(uint8_t value)
{
    ESP_LOGI(TAG, "stiraid temperature %d speed %d", ble_temprature,ble_recive_speed);

    switch (value){
    case MANUAL_MODE:
        dgus_set_var(0X1047, ble_temprature);
    break;   
 

    case AUTO_MODE:
        dgus_set_var(0X1047, ble_temprature);
        dgus_set_val(&motorSpeedControl, ble_recive_speed);
        break;
  
    default:
        break;
    }
}
// Task for mode control and motor operation
static void main_task(void *pvParameters) {
    char buf[10]={0};
        TickType_t lastTime = xTaskGetTickCount();
       save_qr_code();
    
    while (1) {
      //  setMotorSpeed(4);
        if (myDevice.cureentstatus == STIRAID_DEVICE) {
         if (ispaired) {
                if ((xTaskGetTickCount() - lastTime) >= pdMS_TO_TICKS(30000)) {
                   ble_mesh_GET_vendor_message();
                      modecontrol(myDevice.operationMode);
                    lastTime = xTaskGetTickCount();
                }
            }

            if (PlayPause_Flag) {
                 Interval_function(); // Interval for motor on and off
                if (myDevice.operationMode == AUTO_MODE || myDevice.operationMode == MANUAL_MODE) {
                    anticlockwise_Time();
                } else {
                    clockwise_Time(); // For timer display
                }
            }
        }

        //   if (myDevice.cureentstatus == MAIN_DEVICE) {
        //     //  motorOverloadDetection();
        //       //adc2_read();
        //       if (PlayPause_Flag) {
        //         uint32_t dutyCycle = myDevice.motorSpeed ;

        //     if (dutyCycle != previousDutyCycle) {
        //              setMotorSpeed(myDevice.motorSpeed );
        //         //     fan_control(1); // Turn on cooling fan
        //          previousDutyCycle = myDevice.motorSpeed;
        //     }

        //       } 
              //else {
        // //      //   if (previousDutyCycle != 0) {
        // //             // If the button is released and the motor was running, stop the motor
        // //             fan_control(0); // Turn off cooling fan
        //              setMotorSpeed(0);
        // //             previousDutyCycle = 0;
        // //     //    }

        // //         vTaskDelay(10 / portTICK_PERIOD_MS);
        //      }
     //    }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Adjust the delay as needed
    }
}

// Function to control cooling fan speed
void control_coolfan_speed() {
    if (battery_Celsius > MAX_BATT_TEMP_WARNING) {
        // Battery temperature is too high, turn off the fan
        fan_speed = 0;
    } else if (battery_Celsius > (MAX_BATT_TEMP_WARNING + TEMP_INCREASE_THRESHOLD)) {
        // Temperature increased by the threshold, increase fan speed
        fan_speed += 1;
        if (fan_speed > MAX_FAN_SPEED) {
            fan_speed = MAX_FAN_SPEED;
        }
    } else {
        // Battery temperature is within safe range, fan off
        fan_speed = 0;
    }
    
    // Set the fan speed using your fan control logic
    fan_control(fan_speed);
}

// Function to control cooling fan speed
void control_cooling_fan_speed() {
    if (battery_Celsius > MAX_BATT_TEMP_WARNING) {
        // Battery temperature is too high, turn off the fan
        fan_speed = 0;
    } else if (battery_Celsius > (MAX_BATT_TEMP_WARNING + TEMP_INCREASE_THRESHOLD)) {
        // Temperature increased by the threshold, increase fan speed
        fan_speed += 1;
        if (fan_speed > MAX_FAN_SPEED) {
            fan_speed = MAX_FAN_SPEED;
        }
    } else {
        // Battery temperature is within safe range, fan off
        fan_speed = 0;
    }
    
    // Set the fan speed using your fan control logic
    fan_control(fan_speed);
}

// Function to handle battery protection
void battery_protection() {
    calculate_battery_percentage();

    // Read battery temperature
    temperature_sensor_read(&battery_Celsius, &battery_Fahrenheit);
  
    // Check for overheat condition
    if (battery_Celsius > MAX_BATT_TEMP_SHUTDOWN) {
        // Battery temperature too high, display warning and shutdown
        ESP_LOGE(TAG, "Battery temperature too high. Shutting down...");
        dgus_set_text(0x1203, "BATTERY IS TOO HOT. SYSTEM IS SHUT DOWN.", 50);
        // system_shutdown(); // Uncomment this when you're ready to implement the shutdown
    }

    // Control cooling fan speed based on temperature
    control_cooling_fan_speed();

    #ifdef DEBUG_ON
 //   ESP_LOGI(TAG, "Battery Celsius: %.2f Battery Fahrenheit: %.2f", battery_Celsius, battery_Fahrenheit);
    #endif
}

// Main task for system backend
void system_backend_task(void *pvParameters) {
    while (1) {
       // battery_protection();
      // if (myDevice.operationMode==TEMPRATUREPROBE_MODE)        {
          temperature_probe_1(&probe1_Celsius, &probe1_Fahrenheit);
          temperature_probe_2(&probe2_Celsius, &probe2_Fahrenheit);
          // Update display values
          dgus_set_val(&PROBE1_C, probe1_Celsius);
          dgus_set_val(&PROBE1_F, probe1_Fahrenheit);
          dgus_set_val(&PROBE2_C, probe2_Celsius);
          dgus_set_val(&PROBE2_F, probe2_Fahrenheit);

          #ifdef DEBUG_ON
          ESP_LOGI(TAG, "Probe1 Celsius: %.2f Probe1 Fahrenheit: %.2f", probe1_Celsius, probe1_Fahrenheit);
          ESP_LOGI(TAG, "Probe2 Celsius: %.2f Probe2 Fahrenheit: %.2f", probe2_Celsius, probe2_Fahrenheit);
          #endif
     //   }
        vTaskDelay(pdMS_TO_TICKS( 5000)); 
    }
}

static void power_button_init(void)
{
    button_handle_t btn_handle = iot_button_create(POWER_BUTTON, 0);
    button_handle_t main_btn_handle = iot_button_create(MODE_CONTROL_BUTTON, 0);

    if (btn_handle) {
          //  iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "RELEASE");
            app_reset_button_register(btn_handle, 1, 5);

      // iot_button_add_on_press_cb(btn_handle,2, main_button_press, "PRESS");
    }
       if (main_btn_handle) {
             iot_button_set_evt_cb(main_btn_handle, BUTTON_CB_PUSH, button_press_callback,"PRESS");
               iot_button_set_evt_cb(main_btn_handle, BUTTON_CB_RELEASE, button_release_callback, "RELEASE");
    }
   
}
static uint32_t previousIcon = 0; // Initialize with a value that won't match any valid icon

static bool fileServerStarted = false;

static void check_wifi_signal_strength() {
    wifi_ap_record_t ap_info;
    uint32_t icon = 26; // Default icon for a strong signal

    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        wifiConnected = 1;
        ESP_LOGI(TAG, "Signal Strength: %d", ap_info.rssi);
        if (ap_info.rssi <= -70) {
            icon = 28; // Weak signal
        } else if (ap_info.rssi <= -45) {
            icon = 27; // Medium signal
        }

        // Start the file server only if it hasn't been started yet
        if (!fileServerStarted) {
       //     file_server_init();
           fileServerStarted = true; // Set the flag to indicate that the file server has been started
     }
    } else {
        icon = 29; // No signal (failed to get AP info)
        ESP_LOGE(TAG, "Failed to get AP info");
        wifiConnected = 0;
    }

    // Only update the Wi-Fi icon if it has changed
    if (icon != previousIcon) {
        dgus_set_var(0x1013, icon); // Update the Wi-Fi icon
        previousIcon = icon; // Update the previous icon value
    }
}

void wifi_task(void* pvParameters)
{
    while (1) {
        check_wifi_signal_strength();
        vTaskDelay(pdMS_TO_TICKS(1*60 * 1000));  // Check signal strength every  1 min
    }
}

void lcdCommunicationTask(void *pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // Convert 10ms to ticks
    uint32_t data=5 ;
    // Initialization code here...

    while (1) {
        // Code to send dgus_set_var command to LCD
        DGUS_RETURN response = dgus_set_var(0x1018, data);

        if (response == DGUS_OK) {
            // Stop sending data to LCD
          printf("File loaded successfully.\n");
            xTaskCreate(main_task, "main_task", 4096, NULL,10,&motorControlTaskHandle );
            xTaskCreate(system_backend_task, "system_backend",3000, NULL,6,NULL);

            break; // Exit the task
        }
        // Wait for 10ms
       vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

    vTaskDelete(NULL); // Delete the task (optional)
}

void initializeDriver()
{
  char myString[] = ""; // Example string

   dgus_driver_install(UART_NUM_2,115200);

    CreateDGUSObject(&BAT_ICON,0x1015,0); //recipe save BUTTON
    CreateDGUSObject(&BAT_VARIBLE,0x1018,0);
    CreateDGUSObject(&wifiButton,0x1019,0);
    CreateDGUSObject(&LISTVIEW,0x1020,0); //LISTVIEW UPDATE FLAG
    CreateDGUSObject(&RECIPE_SEND,0x1021,0); //recipe infor  UPDATE FLAG
    CreateDGUSObject(&currentModeControl,0x1022,0);
    CreateDGUSObject(&playPauseButton,0x1023,0);//playPauseButton BUTTON
    CreateDGUSObject(&OK_BTN,0x1025,0); //OK BUTTON
    CreateDGUSObject(&cancelButton,0x1026,0); //CANCEL BUTTON
    CreateDGUSObject(&endButton,0x1027,0); //page 5
    CreateDGUSObject(&saveButton,0x1028,0); //CANCEL BUTTON
    CreateDGUSObject(&nextButton,0x1029,0); //start button
    CreateDGUSObject(&stirControl,0x1030,0);
    CreateDGUSObject(&editButton,0x1031,0);
    CreateDGUSObject(&MOTOR_ICON,0x1043,0);
    CreateDGUSObject(&LOAD,0x1045,0);
    CreateDGUSObject(&timerSelectionControl,0x1048,0); 
    CreateDGUSObject(&hour,0x1050,0); 
    CreateDGUSObject(&minute,0x1054,30);
    CreateDGUSObject(&second,0x1058,0);
    CreateDGUSObject(&Interval_m,0x1060,0);
    CreateDGUSObject(&Interval_s,0x1064,0);
    CreateDGUSObject(&motorSpeedControl,0x1068,3);
    CreateDGUSObject(&getrecipeSteps,0x1070,0);
    CreateDGUSObject(&unlockButton,0x1072,0);
    CreateDGUSObject(&PROBE1_F,0x1074,0);
    CreateDGUSObject(&PROBE1_C,0x1076,0);
    CreateDGUSObject(&PROBE2_F,0x1078,0);
    CreateDGUSObject(&PROBE2_C,0x1080,0);
     CreateDGUSObject(&bluetoothButton,0x1099,0); 
    CreateDGUSObject(&ROLLTEXT,0x1203,0);
    CreateDGUSObject(&AlertBar,0x1220,0);
    CreateDGUSObject(&edit_recipename,0x124f,0);

    CreateDGUSObject(&dwin_record_recipe,0x14ff,0); 
    CreateDGUSObject(&serving,0x1450,0);
    CreateDGUSObject(&preprationtime,0x1452,0);
    CreateDGUSObject(&cooking,0x1454,0);
    CreateDGUSObject(&totaltime,0x1456,0);

    CreateDGUSObject(&textInputControl,0X134f,0);
    CreateDGUSObject(&SSID,0x56ff,0); 

    // Assign string value to DGUS_obj
    CreateDGUSObject_assign_string(&SSID, myString);
    CreateDGUSObject_assign_string(&edit_recipename, myString);

    CreateDGUSObject(&PASSWORD,0x571f,0); 
    CreateDGUSObject_assign_string(&PASSWORD, myString);

    Touch_attach_cb(&motorSpeedControl, MotorspeedDCallback, 0);
    Touch_attach_cb(&hour, hoursetCallback, 0);
    Touch_attach_cb(&Interval_m, intervalminutesetCallback, 0);
    Touch_attach_cb(&playPauseButton, PlayCallback, 0);
    Touch_attach_cb(&currentModeControl, ModeCallback, 0);
    Touch_attach_cb(&stirControl, onStirChange, 0); 
    Touch_attach_cb(&OK_BTN, OkCallback, 0);
    Touch_attach_cb(&cancelButton, onCancel, 0);
    Touch_attach_cb(&nextButton, onNext, 0);
    Touch_attach_cb(&endButton, onEnd, 0);
    Touch_attach_cb(&saveButton, saveButtonPressed, 0);
    Touch_attach_cb(&unlockButton, onUnlockButton, 0);
    Touch_attach_cb(&getrecipeSteps, onUpdateRecipeSteps, 0);
    Touch_attach_cb(&wifiButton, onWifiProvisioning, 0);
    Touch_attach_cb(&timerSelectionControl, onTimerSelection, 0);
    Touch_attach_cb(&textInputControl, onTextInput, 0);
    Touch_attach_cb(&bluetoothButton, onBluetoothTap, 0);
    Touch_attach_cb(&editButton, onEditButton, 0);

    I2C0_init();
    Timer_init();
    PWM_init();
    ADC_init();
    weightsensor_init();
    power_button_init();

   xTaskCreate(lcdCommunicationTask, "LCD Task", 4096, NULL, tskIDLE_PRIORITY, NULL);
 // Start the Wi-Fi task
  //        xTaskCreate(weight_sensor_task, "weight_sensor_task", 2048, NULL, 2, &weight_sensor_task_handle);

}
