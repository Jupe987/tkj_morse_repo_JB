
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
float gravity = 1.0;

enum state { WAITING=1, DATA_READY, READSENSOR };
enum state programState = WAITING;

// button interuption
static void btn_fxn(uint gpio, uint32_t eventMask) {
    programState = READSENSOR;
    
    printf("%s\n","Buttonpressed");
}


static void sensor_task(void *arg){
    // read data from sensor
    (void)arg;
    init_ICM42670();
    float ax, ay, az, gx, gy, gz, t;

    
    if(ICM42670_start_with_default_values() != 0){
        printf("%s\n","gyro failed to start");
    }

    for(;;){
        // if progarmstate == Sensor data decided by pressing button ??
        //printf("%s\n","gyro failed to start2");

        ax, ay, az, gx, gy, gz = read_sensor_func(&ax, &ay, &az, &gx, &gy, &gz);
        float ax_sum, ay_sum, az_sum = 0.0;
        float ax_grav, ay_grav, az_grav = 0.0;
        
        //ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);

        ax_grav = ax - gravity;
        ay_grav = ay - gravity;
        az_grav = az - gravity;
        int num = 10;
        while(num){
            if(ax_grav > gravity){
                ax_sum = ax_sum + ax_grav;
            }
            else if(ay_grav > gravity) {
                ay_sum = ay_sum + ay_grav;
            }
            else if(az_grav > gravity) {
                az_sum = ay_sum + az_grav;
            }
            else{
                num = -1;
                break;
            }
            num = num -1;
            sleep_ms(25);
            ax, ay, az, gx, gy, gz = read_sensor_func(&ax, &ay , &az, &gx, &gy, &gz);
            ax_grav = ax - gravity;
            ay_grav = ay - gravity;
            az_grav = az - gravity;
        }
        printf("%s\n","sums acc");

        printf("%0.5f\n",ax_sum);
        printf("%0.5f\n",ax_sum);
        printf("%0.5f\n",ax_sum);

        if(az_sum > 3){
            MORSE_CHAR = '.';
            programState = DATA_READY;
        }
        
        printf("%s\n","gyro data");
        printf("%0.5f\n",gx);
        printf("%0.5f\n",gy);
        printf("%0.5f\n",gz);
        
        if (gpio_get(SW2_PIN) == 1) {

            if(az > 0.9 && ay < 0.2 && ax < 0.2) {
                programState = DATA_READY;
                MORSE_CHAR = '.';
            }
            else if(az > 0.2 && ay < 0.9 && ax < 0.2){
                programState = DATA_READY;
                MORSE_CHAR = '-';
            }
            else{
                programState = READSENSOR;
            }
        }

        //MORSE_CHAR = '.';
        programState = DATA_READY;
        

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

float read_sensor_func(ax, ay , az, gx, gy, gz, t){
    return ICM42670_read_sensor_data(ax, ay , az, gx, gy, gz, t);
}

static void print_task(void *arg){
    (void)arg;
    //intianaliense the display
    init_display();

    for(;;){
        if(programState == DATA_READY) {
            printf("%c\n", MORSE_CHAR);

            display_morse();
            play_buzzer();
            //send data to usb

            programState = WAITING;
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void display_morse(){
    //function that display morse characters on hat screen
    // maybe this function inside print task or call
    write_text(&MORSE_CHAR);
}

void read_usb(){

    // if progarmstate == read usb decided by pressing button ??

    // read from terminal form pc and store in global variable see lovelace mod 2 chap5 
    // this funtion to write task or call from there
    
    // if in correct programstate == read usb 

}

void play_buzzer(){

    //if char = . then that ........ called from write task
    if(MORSE_CHAR == '.')   {
        buzzer_play_tone(10000, 300);
    }
    else if(MORSE_CHAR == '-')  {
        buzzer_play_tone(15000, 300);
    }
    else {
        buzzer_play_tone(5000, 300);
    } 
}


int main() {

    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(10);
       printf("%s","alustetaan usb");
    }
    //for(;;){
    //    printf("%s","alustetaan usb");
    //}

    sleep_ms(300);

    init_hat_sdk();

    sleep_ms(300); //Wait some time so initialization of USB and hat is done.

    init_buzzer();

    // need button iteruption handler maybe not if we do it with sensor interuption
    init_sw1();
    init_sw2();
    gpio_set_irq_enabled_with_callback(BUTTON1, SW1_PIN, true, btn_fxn);
    //gpio_set_irq_enabled_with_callback(BUTTON2, SW2_PIN, true, btn_fxn);
    

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
