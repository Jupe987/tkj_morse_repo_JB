
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
char message[63];

enum state { WAITING=1, DATA_READY, READSENSOR, READ_USB};
enum state programState = WAITING;

// button interuption
static void btn_fxn(uint gpio, uint32_t eventMask) {
    programState = READ_USB;
    
    //printf("%s\n","Buttonpressed");
}

//float read_sensor_func(ax, ay , az, gx, gy, gz, t){
//    return ICM42670_read_sensor_data(ax, ay , az, gx, gy, gz, t);
//}

void display_morse(){
    //function that display morse characters on hat screen
    write_text(&MORSE_CHAR);

}

void read_usb(){
    // if programstate == read usb decided by pressing button
    char usb_input_buffer[31];
    printf("%s\n","HERE5");
    if(programState == READ_USB) {
        printf("usb: waiting for input\n");
        printf("%s\n","HERE4");
        // read from terminal on pc
        if(stdio_usb_connected()) {
            if(scanf("%31s", usb_input_buffer) == 1) {
                //usb_input_ready = 1;
                printf("%s\n","HERE2");
                // store in global variable and process first morse char
                if(strlen(usb_input_buffer) > 0) {
                    char first_char = usb_input_buffer[0];
                    if(first_char == '.' || first_char == '-') {
                        MORSE_CHAR = first_char;
                        programState = DATA_READY;
                        printf("%s\n","HERE7");
                        printf("%c\n",MORSE_CHAR);
                    }
                    printf("%s\n","HERE3");
                }
                
            }
        }
    }
    else{
    programState = WAITING;
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
    //intianaliense the display
    init_display();

    for(;;){
        if(programState == READ_USB) {
            read_usb();
        }
        if(programState == DATA_READY) {
            printf("%c\n", MORSE_CHAR);

            display_morse();
            play_buzzer();
            //send data to usb
            //puts(MORSE_CHAR);
            MORSE_CHAR = 'a';
            programState = WAITING;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
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
        //intialise variales to store sum of acc and reset them
        float ax_sum = 0.0, ay_sum = 0.0, az_sum = 0.0;
        float ax_grav, ay_grav, az_grav = 0.0;
        //set_led_status(1);
        //if (gpio_get(SW2_PIN) == 1 && programState == WAITING) {
            //printf("%s\n","HERE");

            // Read sensor
        ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);


        

        if (gpio_get(SW2_PIN) == 1 && programState == WAITING) {
            toggle_led();
            sleep_ms(200);
            int runs = 0;
            //while(ax < 1.1 || ay < 1.1 || az < 1.1 || runs > 2000){
            //    ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);
            //    runs ++;
            //}
            //printf("%s\n","HERE");
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
                printf("%s\n","HERE2");
            }
            toggle_led();
            
            printf("%s\n","sums acc");

            printf("%0.5f\n",ax_sum);
            printf("%0.5f\n",ay_sum);
            printf("%0.5f\n",az_sum);

            if(az_sum > 3){
                printf("%s\n","HERE az sum comp");
                MORSE_CHAR = '.';
                programState = DATA_READY;
            }

            if(ax_sum > 2.2){
                MORSE_CHAR = '-';
                programState = DATA_READY;
            }
            
            printf("%s\n","acc data");
            printf("%0.5f\n",ax);
            printf("%0.5f\n",ay);
            printf("%0.5f\n",az);

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

        //MORSE_CHAR = '.';
        //programState = DATA_READY;
        

        vTaskDelay(pdMS_TO_TICKS(3000));
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
