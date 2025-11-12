#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <task.h>

#include "tkjhat/sdk.h"

//-------------------------this code prints acceleration data to serial monitor ------------------

void sensor(){
    
    float ax, ay, az, gx, gy, gz, t = 0;

    ICM42670_read_sensor_data(&ax, &ay , &az, &gx, &gy, &gz, &t);

    printf("%s\n","acceleration:=>");

    printf("%0.5f\n",ax);
    printf("%0.5f\n",ay);
    printf("%0.5f\n",az);

    printf("%s\n","angular acceleration:=>");
    
    printf("%0.5f\n",gx);
    printf("%0.5f\n",gy);
    printf("%0.5f\n",gz);

}

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(10);
        printf("%s","alustetaan usb");
    }

    sleep_ms(300);


    init_hat_sdk();

    init_ICM42670();

    if(ICM42670_start_with_default_values() != 0){
        printf("%s\n","gyro failed to start");
    }

    while(1){
        sensor();
    }
}