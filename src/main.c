
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

enum state { WAITING=1, DATA_READY, ENDOFMESG };
enum state programState = WAITING;

// button interuption
static void btn_fxn(uint gpio, uint32_t eventMask) {
    //
    programState = ENDOFMESG;
}

void display_morse(){
    //function that display morse characters on hat screen
    clear_display();
    write_text(&MORSE_CHAR);

}

void read_usb(){
    
    int ch = getchar_timeout_us(1000);
    if (ch >= 0) {
        if(ch == '.' || ch == '-' || ch == ' ') {
            MORSE_CHAR = ch;
            programState = DATA_READY;            
        }
        if((char)ch == '\n'){
            programState = ENDOFMESG;
        }
    }
    
}

void play_buzzer(){

    //if char = . then that ........ called from write task
    if(MORSE_CHAR == '.')   {
       buzzer_play_tone(10000, 300);
    }
    else if(MORSE_CHAR == '-')  {
        buzzer_play_tone(15000, 300);
    }
    else 
    {
        buzzer_play_tone(5000, 300);
    } 
}

static void print_task(void *arg){
    (void)arg;
    puts("USB ready to recive symbols");
    //intianaliense the display
    init_display();
    clear_display();

    for(;;){
        if(programState == ENDOFMESG){
        putchar_raw((char) '\n');
        puts("message was:");
        puts(message);
        puts("new message");

        programState = WAITING;
        }
        if(programState == WAITING) {
            read_usb();
        }
        // IF WE HAVE DATA 
        if(programState == DATA_READY) {

            display_morse();
            play_buzzer();
            message[count] = (char)MORSE_CHAR;
            count++;
            message[count] = '\0';
     
            //send data to usb
            putchar_raw((char) MORSE_CHAR);
            putchar_raw((char) ' ');
            MORSE_CHAR = 'a';
            programState = WAITING;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
static void sensor_task(void *arg){
    // This task reads data from sensor and based on data we get the morse char
    (void)arg;
    init_ICM42670();
    float ax, ay, az, gx, gy, gz, t;

    
    if(ICM42670_start_with_default_values() != 0){
        printf("%s\n","gyro failed to start");
    }

    for(;;){
        //intialise variales to store sum of acc and reset them
        float ax_sum = 0.0, ay_sum = 0.0, az_sum = 0.0;
        float ax_grav, ay_grav, az_grav = 0.0;

        // Read sensor
        ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);
       
        if (gpio_get(SW2_PIN) == 1 && programState == WAITING) {
            // indicate that we are ready to move board
            toggle_led();
            sleep_ms(200);

            //How many readings we want
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
                else{
                    sample_size = sample_size -1;
                }
                sample_size  = sample_size  -1;
                //Little delay between samples
                sleep_ms(35);
                ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);
                //printf("%s\n","HERE2");
            }
            toggle_led();

            if(az_sum > 3){
                //printf("%s\n","HERE az sum comp");
                MORSE_CHAR = '.';
                programState = DATA_READY;
            }

            if(ax_sum > 2.2){
                MORSE_CHAR = '-';
                programState = DATA_READY;
            }

            // gets the position
            if(programState != DATA_READY && gpio_get(SW2_PIN) == 1) {
                if(az > 0.9 && ay < 0.2 && ax < 0.2) {
                    programState = DATA_READY;
                    MORSE_CHAR = '.';
                }
                else if(az < 0.2 && ay > 0.9 && ax < 0.2){
                    programState = DATA_READY;
                    MORSE_CHAR = '-';
                }
                else if(az < 0.2 && ay < 0.2 && ax > 0.9){
                    programState = DATA_READY;
                    MORSE_CHAR = ' ';
                }
                else{
                    programState = WAITING;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

int main() {

    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(10);
       printf("%s","alustetaan usb");
    }

    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    init_buzzer();
    init_led();

    init_sw1();
    init_sw2();
    gpio_set_irq_enabled_with_callback(BUTTON1, SW1_PIN, true, btn_fxn);
    //gpio_set_irq_enabled_with_callback(BUTTON2, SW2_PIN, true, btn2_fxn);



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