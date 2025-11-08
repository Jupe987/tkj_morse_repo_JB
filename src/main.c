
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

        //if(programState == READSENSOR) {
        
        // if button pressed (if button input high then)
        
        // how do we get the dat?  how much?? data handling ???
        float ax_sum, ay_sum, az_sum = 0.0;
        for(int i = 0;i<1000;i++){
            ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);
            if(ax > 0){
            ax_sum = ax_sum +ax;
            }
            ay_sum = ay_sum +ay;
            az_sum = az_sum +az;
        }
        printf("%s\n","ax sum");
        printf("%0.4f\n", ax_sum);

        printf("%s\n","ay sum");
        printf("%0.4f\n", ay_sum);

        printf("%s\n","az sum");
        printf("%0.4f\n", az_sum);
        //if(ax)

        //printf("%0.6f", ax, ay , az);
        printf("%0.5f\n",gx);
        printf("%0.5f\n",gy);
        printf("%0.5f\n",gz);

        // need timer to calculate the how much have we moved and fiter out anomalies

        //calca area under curve ??? in for loop ?? some treshold value ?? vector ??
        
        //First read if we are movin (acc data) 
        //if not moving then orientation (gyro data at this moment)
        // w/ else if staments can be done
        
        // activated with button no?? probably not 

        //if(gx >= something && gy >= something && gz){
        //  then MORSE_CHAR = "."
        //}
        //if(gx >= something && gy >= something && gz){
        //  then MORSE_CHAR = "_"
        //}
        // if we get new char with measurent next state

        MORSE_CHAR = '.';


        programState = DATA_READY;
        //}

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
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

    printf("%s","alustetaan usb");

    while (!stdio_usb_connected()) {
        sleep_ms(10);
       printf("%s","alustetaan usb");
    }
    //for(;;){
    //    printf("%s","alustetaan usb");
    //}

    printf("%s","alustetaan usb");

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
