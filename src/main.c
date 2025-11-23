
//Berat Yildiz and Julius Pajukangas
//Tier 2
#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define CDC_ITF_TX      1
char MORSE_CHAR  = 'a';
float gravity = 0.95;
char message[64];
int count = 0;
float ax, ay, az, gx, gy, gz, t;


enum state { WAITING=1, DATA_READY, ENDOFMESG };
enum state programState = WAITING;

// button interuption for ending the message
static void btn_fxn(uint gpio, uint32_t eventMask) {

    programState = ENDOFMESG;
}

void display_morse(){
    //Julius
    //function that display morse characters on hat screen
    clear_display();
    write_text(&MORSE_CHAR);

}

void read_usb(){
    //Berat
    // Checks usb for approwed characters
    int ch = getchar_timeout_us(200);
    if (ch >= 0) {
        if(ch == '.' || ch == '-' || ch == ' ') {
            MORSE_CHAR = ch;
            programState = DATA_READY;            
        }
        if(ch == 'n'){
            programState = ENDOFMESG;
        }
    }
    
}

void play_buzzer(){
    // Plays the buzzer based on what char is stored in morse char
    //Julius
    if(MORSE_CHAR == '.')   {
       buzzer_play_tone(10000, 300);
    }
    else if(MORSE_CHAR == '-')  {
        buzzer_play_tone(15000, 500);
    }
    else 
    {
        buzzer_play_tone(5000, 300);
    } 
}
void positon_handler(){
    //Julius
    
    if(az > 0.9 && ay < 0.2 && ax < 0.2) {
        //screen upwards
        programState = DATA_READY;
        MORSE_CHAR = '.';
    }
    else if(az < 0.2 && ay > 0.9 && ax < 0.2){
        //screen away from user
        programState = DATA_READY;
        MORSE_CHAR = '-';
    }
    else if(az < 0.2 && ay < 0.2 && ax > 0.9){
        //screen to left from user
        programState = DATA_READY;
        MORSE_CHAR = ' ';
    }
    else{
        programState = WAITING;
    }
    
}
void motion_handler(){
    //Julius/berat
    //Gets gyro acc and if the board is moving in sample window enough we get morse char
    //if sum exceeds threshold value
    //intialise variales to store sum of acc and reset them each call
    float ax_sum = 0.0, ay_sum = 0.0, az_sum = 0.0;
    float ax_grav = 0.0, ay_grav = 0.0, az_grav = 0.0;
    //HOW MAMY SAMPLES
    int sample_size = 10;
    while(sample_size > 0){
        ax_grav = ax - gravity;
        ay_grav = ay - gravity;
        az_grav = az - gravity;
        if(ax_grav > 0.0){
            ax_sum = ax_sum + ax_grav;
        }
        else if(ay_grav > 0.0) {
            ay_sum = ay_sum + ay_grav;
        }
        else if(az_grav > 0.0) {
            az_sum = az_sum + az_grav;
        }

        sample_size  = sample_size  -1;
        //Little delay between samples
        sleep_ms(35);
        ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);
    }

    if(az_sum > 3){
        //updwards
        MORSE_CHAR = '.';
        programState = DATA_READY;
    }

    if(ax_sum > 2.2){
        //to left
        MORSE_CHAR = '-';
        programState = DATA_READY;
    }
}

static void print_task(void *arg){
    //HANDLES THE USB MESSAGE GETTING, AND PRINTING AND STORING THE MESSAGE 
    //ALSO BUZZER AND DISPLAY is played when data is ready
    //Berat
    (void)arg;
    puts("USB ready to recive symbols");
    //intianaliense the display
    init_display();
    clear_display();

    //intianaliense the buzzer
    init_buzzer();

    for(;;){

        if(programState == WAITING) {
            read_usb();
        }
        if(programState == ENDOFMESG){
            //HANDLES THE END OF MESSAGE
            putchar_raw((char) '\n');
            puts("message was:");
            puts(message);
            puts("new message:");
            count = 0;
            clear_display();
            write_text("msg recived");

            programState = WAITING;
        }
        // IF WE HAVE DATA THAT WE NEED TO STORE AND PRINT IT TO PC
        if(programState == DATA_READY) {

            display_morse();
            play_buzzer();
            //Store the full message
            message[count] = (char)MORSE_CHAR;
            count++;
            if(MORSE_CHAR != ' ') {
                message[count] = ' ';
                count++;
            }
            message[count] = '\0';
     
            //send data to usb
            putchar_raw((char) MORSE_CHAR);
            if(MORSE_CHAR != ' ') {
                putchar_raw((char) ' ');
            }
            MORSE_CHAR = 'a';
            programState = WAITING;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void sensor_task(void *arg){
    //Julius
    // This task reads data from sensor and based on data we get the corresponding morse char
    (void)arg;
    init_ICM42670();
    
    if(ICM42670_start_with_default_values() != 0){
        printf("%s\n","gyro failed to start");
    }
    //LED TO INDICATE WHEN DATA COLLECTION IS HAPPENING
    init_led();

    for(;;){
        
        // Read sensor
        ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);
       
        if (gpio_get(SW2_PIN) == 1 && programState == WAITING) {
            // indicate that user can move the board 
            toggle_led();
            //human reaction time
            sleep_ms(200);

            motion_handler();

            toggle_led();

            if(programState != DATA_READY && gpio_get(SW2_PIN) == 1) {
                //IF NOT CHAR BASED ON MOVING THEN IT HAS TO BE STILL
                positon_handler();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    // Julius/Berat
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(10);
       printf("%s","alustetaan usb");
    }

    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.
    
    
    init_sw1(); //BUTTONN FOR INTERUPTION
    
    init_sw2(); //button for data gathering

    gpio_set_irq_enabled_with_callback(BUTTON1, SW1_PIN, true, btn_fxn);
  
    TaskHandle_t hSensorTask, hPrintTask, hUSB = NULL;

    // Create the tasks with xTaskCreate
    BaseType_t result = xTaskCreate(sensor_task, // (en) Task function
        "sensor",                        // (en) Name of the task 
        DEFAULT_STACK_SIZE,              // (en) Size of the stack for this task (in words). Generally 1024 or 2048
        NULL,                            // (en) Arguments of the task 
        2,                               // (en) Priority of this task
        &hSensorTask);                   // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Sensor task creation failed\n");
        return 0;
    }
    result = xTaskCreate(print_task,  // (en) Task function
                "print",              // (en) Name of the task 
                DEFAULT_STACK_SIZE,   // (en) Size of the stack for this task (in words). Generally 1024 or 2048
                NULL,                 // (en) Arguments of the task 
                2,                    // (en) Priority of this task
                &hPrintTask);         // (en) A handle to control the execution of this task

    if(result != pdPASS) {
        printf("Print Task creation failed\n");
        return 0;
    }

    // Start the scheduler (never returns)
    vTaskStartScheduler();
    
    // Never reach this line.
    return 0;
}