#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include "hardware/flash.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "headers/DS3231.h"
#include "headers/GPIO_DEFS.h"
#include "pico/types.h"


typedef struct{
    RTC_Time sunrise_time;
    RTC_Time sunset_time;
    int brightness_lvl;
    bool sunrise;
    bool sunset;
}config_data;

config_data configuration;

void init_configuration(){
    load_data();
    configuration.sunrise = true;
    configuration.sunset = false;
}

void load_data(){
    /*
        Load sunrise and sunset time from the flash
        first 3 int values - sunrise time (seconds,minutes, hours)
        next 3 int values - sunset time (seconds,minutes, hours)
    */
    int *p;
    int addr = XIP_BASE +  FLASH_TARGET_OFFSET;
    p = (int *)addr; 
    int value = *p;
    configuration.sunrise_time.seconds = ((uint8_t)*p > -1 && (uint8_t)*p < 60) ? (uint8_t)*p : 0;
    p++;
    configuration.sunrise_time.minutes = ((uint8_t)*p > -1 && (uint8_t)*p <60) ? (uint8_t)*p : 0;
    p++;
    configuration.sunrise_time.hours = ((uint8_t)*p > -1 && (uint8_t)*p < 24) ? (uint8_t)*p : 11;
    p++;
    configuration.sunset_time.seconds = ((uint8_t)*p > -1 && (uint8_t)*p < 60) ? (uint8_t)*p : 0;
    p++;
    configuration.sunset_time.minutes = ((uint8_t)*p > -1 && (uint8_t)*p <60) ? (uint8_t)*p : 0;
    p++;
    configuration.sunset_time.hours = ((uint8_t)*p > -1 && (uint8_t)*p < 24) ? (uint8_t)*p : 21;

    //Brightness lvl
    addr = XIP_BASE + BRIGHTNESS_LVL_FLASH_OFFSET;
    p = (int *)addr;
    configuration.brightness_lvl = *p;
}

bool check_if_night(uint8_t hour){
    uint8_t night_hours[11] = {22,23,0,1,2,3,4,5,6,7,8};
    for(int i=0;i<11;i++){
        if(hour == night_hours[i])
            return true;
    }
    return false;
}


int update_RTC(uint8_t day, uint8_t month, uint8_t year, uint8_t dow, uint8_t seconds, uint8_t minutes, uint8_t hours){
    uart_puts(UART_ID, "Updating RTC");
    RTC_Time time;
    //Check if data format is correct
    int error_code = 0; //0 - no error
    error_code = (day >0 && day<32) ? error_code : 1;
    error_code = (month > 0 && month < 13) ? error_code : 1;
    error_code = (year > -1) ? error_code : 1;
    error_code = (dow > -1 && dow < 7) ? error_code : 1;
    error_code = (seconds > -1 && seconds < 60) ? error_code : 1;
    error_code = (minutes > -1 && minutes < 60) ? error_code : 1;
    error_code = (hours > -1 && hours < 24) ? error_code : 1;
    if(!error_code){
        time.day = day;
        time.month = month;
        time.year = year-2000;
        time.dow = dow;
        time.seconds = seconds;
        time.minutes = minutes;
        time.hours = hours;
        RTC_Set(&time);
        return error_code; // no error
    }
    else
        return error_code; //error 
}

bool check_date(int seconds, int minutes, int hours){
    int error_code = 0; //0 - no error
    error_code = (seconds > -1 && seconds < 60) ? error_code : 1;
    error_code = (minutes > -1 && minutes < 60) ? error_code : 1;
    error_code = (hours > -1 && hours < 24) ? error_code : 1;
    if(error_code)
        return 1;
    return 0;
}

uint configure_pwm(){
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Find out which PWM slice is connected to GPIO 16 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 10000.f); // Set divider, reduces counter clock to sysclock/this value 9000
    pwm_init(slice_num, &config, false); // Load the configuration into our PWM slice, and set it running.
    return slice_num;
}

void diodes_on(int brightness_lvl){
    if(brightness_lvl == 4){
        gpio_init(PWM_PIN);
        gpio_set_dir(PWM_PIN,GPIO_OUT);
        gpio_put(PWM_PIN,0);
    }
    else{
        uint slice_num = configure_pwm();
        int fade;
        switch(brightness_lvl){
            case 1:
                fade = 64700;
                break;
            case 2:
                fade = 63000; 
                break;
            case 3:
                fade = 62000;
                break;
            default:
                fade = 0;
        }
        pwm_set_gpio_level(PWM_PIN, fade);
        pwm_set_enabled(slice_num, true);
    }
    configuration.sunset = false;//1.1 hotfix
    uart_puts(UART_ID,"Brightness adjusted!\n");
}

void diodes_off(){
    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN,GPIO_OUT);
    gpio_put(PWM_PIN,1);
    configuration.sunset = true;//1.1 hotfix

}

void sunrise_simulation(){
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Find out which PWM slice is connected to GPIO 16 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 10000.f); // Set divider, reduces counter clock to sysclock/this value 9000
    pwm_init(slice_num, &config, false); // Load the configuration into our PWM slice, and set it running.
    char buf[10];    
    int fade = 65024;//65024
    pwm_set_gpio_level(PWM_PIN, fade);
    pwm_set_enabled(slice_num, true);
    int i=0;
    uint64_t sleep_time = 300000; 
    uart_puts(UART_ID,"Turning diodes on!\n");
    int limit;
    switch(configuration.brightness_lvl){
        case 1:
            limit = 64700;
            break;
        case 2:
            limit = 63000;
            break;
        case 3:
            limit = 62000;
            break;
        case 4:
            limit = 0;
            break;
        default:
            limit = 0;
    }
    while(fade > limit){
        fade--;
        sprintf(buf,"%d \0",fade);
        if (fade%100 == 0)
            uart_puts(UART_ID,buf);
        if(fade > 62000)
            busy_wait_us(sleep_time);
        else if(fade > 61000)
            busy_wait_us(sleep_time/3);
        else if(fade > 60000)
            busy_wait_us(sleep_time/10);
        else if (fade > 48000){
            fade-=3;
            busy_wait_us(sleep_time/20);
        }
        else
            busy_wait_us(sleep_time/100);
        
        pwm_set_gpio_level(PWM_PIN, fade);
    }
    configuration.sunset = false;
    uart_puts(UART_ID,"Diodes ON!\n");
}

void sunset_simulation(){
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Find out which PWM slice is connected to GPIO 16 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 10000.f); // Set divider, reduces counter clock to sysclock/this value 9000 - was 10000
    pwm_init(slice_num, &config, false); // Load the configuration into our PWM slice, and set it running.
    int i=0;
    uint64_t sleep_time = 300;
    uart_puts(UART_ID,"Turning diodes off!\n");
    char buf[10];
    int fade = 0;

    switch(configuration.brightness_lvl){
        case 1:
            fade = 64700;
            break;
        case 2:
            fade = 63000;
            break;
        case 3:
            fade = 62000;
            break;
        case 4:
            fade = 0;
            break;
        default:
            fade = 0;
    }
    pwm_set_gpio_level(PWM_PIN, fade);
    pwm_set_enabled(slice_num, true);
    while(fade < 65025){
        fade++;
        sprintf(buf,"%d \0",fade);
        if (fade%100 == 0)
            uart_puts(UART_ID,buf);
        if(fade < 48000)
            busy_wait_us(sleep_time);
        else if(fade < 60000)
            busy_wait_us(sleep_time*10);
        else if (fade < 63000){
            fade+=20;
            busy_wait_us(sleep_time*20000);
        }
        else
            busy_wait_us(sleep_time*1000);
        
        pwm_set_gpio_level(PWM_PIN, fade);            
    }        
    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN,GPIO_OUT);
    gpio_put(PWM_PIN,1);
    configuration.sunset = true;
    uart_puts(UART_ID,"Diodes OFF!\n");
}

void brightness_up(){
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Find out which PWM slice is connected to GPIO 16 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 10000.f); // Set divider, reduces counter clock to sysclock/this value 9000
    pwm_init(slice_num, &config, false); // Load the configuration into our PWM slice, and set it running.
    char buf[10];  
    int fade;
    switch(configuration.brightness_lvl){
        case 1:
            fade = 64700;
            break;
        case 2:
            fade = 63000;
            break;
        case 3:
            fade = 62000;
            break;
        case 4:
            fade = 0;
            break;
        default:
            fade = 0;
    }  
    pwm_set_gpio_level(PWM_PIN, fade);
    pwm_set_enabled(slice_num, true);
    int i=0;
    uint64_t sleep_time = 300000; 
    uart_puts(UART_ID,"Brightness up!\n");

    while(fade > 0){
        fade--;
        sprintf(buf,"%d \0",fade);
        if (fade%100 == 0)
            uart_puts(UART_ID,buf);
        if(fade > 62000)
            busy_wait_us(sleep_time);
        else if(fade > 61000)
            busy_wait_us(sleep_time/3);
        else if(fade > 60000)
            busy_wait_us(sleep_time/10);
        else if (fade > 48000){
            fade-=3;
            busy_wait_us(sleep_time/20);
        }
        else
            busy_wait_us(sleep_time/100);
        
        pwm_set_gpio_level(PWM_PIN, fade);
    }
}

void brighntess_down(){
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Find out which PWM slice is connected to GPIO 16 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 10000.f); // Set divider, reduces counter clock to sysclock/this value 9000 - was 10000
    pwm_init(slice_num, &config, false); // Load the configuration into our PWM slice, and set it running.
    int i=0;
    uint64_t sleep_time = 300;
    uart_puts(UART_ID,"Brightness down!\n");
    char buf[10];
    int limit = 0;
    int fade = 0;
    switch(configuration.brightness_lvl){
        case 1:
            limit = 64700;
            break;
        case 2:
            limit = 63000;
            break;
        case 3:
            limit = 62000;
            break;
        case 4:
            limit = 0;
            break;
        default:
            limit = 0;
    }
    pwm_set_gpio_level(PWM_PIN, fade);
    pwm_set_enabled(slice_num, true);
    while(fade < limit){
        fade++;
        sprintf(buf,"%d \0",fade);
        if (fade%100 == 0)
            uart_puts(UART_ID,buf);
        if(fade < 48000)
            busy_wait_us(sleep_time);
        else if(fade < 60000)
            busy_wait_us(sleep_time*10);
        else if (fade < 63000){
            fade+=20;
            busy_wait_us(sleep_time*20000);
        }
        else
            busy_wait_us(sleep_time*1000);
        
        pwm_set_gpio_level(PWM_PIN, fade);            
    }        
}

void check_valid_command(const char *received_string, size_t string_length){
    //Turn on the diodes
    if(strcmp(received_string,"Diodes ON") == 0){
        diodes_on(configuration.brightness_lvl);
        uart_puts(UART_ID,"Diodes ON!\n");
    }
    else if(strcmp(received_string, "Diodes OFF") == 0){
        diodes_off();
        uart_puts(UART_ID,"Diodes OFF!\n");
    }
    //Sunrise simulation
    else if(strcmp(received_string,"sunrise") == 0){
        sunrise_simulation();
    }
    //Sunset simulation
    else if(strcmp(received_string,"sunset") == 0){
        sunset_simulation();
    }
    else if(strstr(received_string, "Update RTC")){
        char *result;
        char delim[] = " ";
        result = strtok(received_string, delim);
        char *arr[9];
        int i = 0;
        while(result != NULL && i < 9){
            arr[i] = result;
            result = strtok(NULL, delim);
            i++;
        }
        if(i!=9){
            uart_puts(UART_ID,"Not valid number of parameters!\n");
        }
        else{
            //uint8_t day, uint8_t month, uint8_t year, uint8_t dow, uint8_t seconds, uint8_t minutes, uint8_t hours;
            uint8_t date[7];
            for(int i=0;i<7;i++){
                date[i] = atoi(arr[i+2]);
            }

            if(update_RTC(date[0],date[1],date[2],date[3],date[4],date[5],date[6])==0){
                uart_puts(UART_ID,"RTC Updated!\n");
            }
            else{
                uart_puts(UART_ID, "Error updating RTC!\n");
            }
        }
    }
    else if(strcmp(received_string,"Get RTC") == 0){
        uart_puts(UART_ID,"Reading RTC\n");
        RTC_Time c_time;
        RTC_Get(&c_time);
        uint8_t buf[128];
        sprintf(buf,"%u:%u:%u dow: %u %02u %02u %u",c_time.hours,c_time.minutes,c_time.seconds,c_time.dow,c_time.day,c_time.month,c_time.year+2000);
        int i=0;
        while(buf[i]!='\0' && i<128){
            uart_putc(UART_ID,buf[i]);
            busy_wait_us(10000);  
            i++;
        }
        uart_puts(UART_ID, "Done!\n");
    }
    else if(strstr(received_string,"Set cycle")){
        uart_puts(UART_ID,"Setting sunrise and sunset time!\n");
        char *result;
        char delim[] = " ";
        result = strtok(received_string, delim);
        char *arr[8];
        int i = 0;
        while(result != NULL && i<8){
            arr[i] = result;
            result = strtok(NULL, delim);
            i++;
        }
        if(i!=8){
            uart_puts(UART_ID,"Not valid number of parameters!\n");
        }
        else{
            //date[0-3] -> sunrise date[4-6] -> sunset
            int date[6];
            for(int i=0;i<6;i++){
                date[i] = atoi(arr[i+2]);
            }
            if(check_date(date[0],date[1],date[2])==0 && check_date(date[3],date[4],date[5])==0){
                int buf[FLASH_PAGE_SIZE/sizeof(int)]; // One page worth of 32-bit ints
                buf[0] = date[0];  // Put the data into the first four bytes of buf[]
                buf[1] = date[1];
                buf[2] = date[2];
                buf[3] = date[3];
                buf[4] = date[4];
                buf[5] = date[5];
                flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
                flash_range_program((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), (uint8_t *)buf, FLASH_PAGE_SIZE);
                load_data(); //Update global variables
                uart_puts(UART_ID,"Done!\n");
            }
            else{
                uart_puts(UART_ID,"Invalid date format!\n");
            }
        }

    }
    else if(strcmp(received_string,"Get cycle") == 0){
        uart_puts(UART_ID,"Getting sunrise and sunset time!\n");
        int *p;
        int addr = XIP_BASE +  FLASH_TARGET_OFFSET;
        char temp_str[100];
        uart_puts(UART_ID, "Sunrise:\n");
        sprintf(temp_str,"%d %d ",configuration.sunrise_time.minutes, configuration.sunrise_time.hours);
        uart_puts(UART_ID, temp_str);
        uart_putc(UART_ID,'\n');
        uart_puts(UART_ID, "Sunset:\n");
        sprintf(temp_str,"%d %d ",configuration.sunset_time.minutes, configuration.sunset_time.hours);
        uart_puts(UART_ID, temp_str);
        uart_putc(UART_ID,'\n');

    }
    else if(strstr(received_string,"Set brightness")){
        uart_puts(UART_ID,"Setting LED brightness!\n");
        char *result;
        char delim[] = " ";
        result = strtok(received_string, delim);
        char *arr[3];
        int i = 0;
        while(result != NULL && i<3){
            arr[i] = result;
            result = strtok(NULL, delim);
            i++;
        }
        if(i!=3){
            uart_puts(UART_ID,"Not valid number of parameters!\n");
        }      
        else{
            int brightness_lvl = atoi(arr[2]);
            if(brightness_lvl < 1 || brightness_lvl > 4){
                uart_puts(UART_ID,"Brightness level should be in range between 1-4 !\n");
            }
            else{
                int buf[FLASH_PAGE_SIZE/sizeof(int)]; 
                buf[0] = brightness_lvl;  
                flash_range_erase((PICO_FLASH_SIZE_BYTES - 2*FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
                flash_range_program((PICO_FLASH_SIZE_BYTES - 2*FLASH_SECTOR_SIZE), (uint8_t *)buf, FLASH_PAGE_SIZE);
                configuration.brightness_lvl = brightness_lvl;
                if(configuration.sunrise == true)
                    diodes_on(configuration.brightness_lvl); //adjust brightness of the diodes
            }
        }  
    }
    //Not handled command
    else{
        if(uart_is_writable(UART_ID))
            uart_puts(UART_ID,"Not valid command!\n");
    }
}


/*
    RX interrupt handler
*/
void on_uart_rx() {
    char received_string[100]; 
    uint8_t received_char;
    size_t chars_received = 0; 
    //Read whole buffer
    while (uart_is_readable_within_us(UART_ID,10000)) {
        received_string[chars_received] = uart_getc(UART_ID);
        chars_received++;
    }
    received_string[chars_received] = '\0'; //add null termination char
    check_valid_command(received_string, chars_received);

}

int main(){
    /*
        UART_CONFIG
    */
    uart_init(UART_ID, 9600);
    gpio_set_function(UART_TX_PIN,GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN,GPIO_FUNC_UART);
    int actual = uart_set_baudrate(UART_ID, BAUD_RATE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, false);
    int UART_IRQ = UART0_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx); // And set up and enable the interrupt handlers
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    /* 
        PWM CONFIG
    */
    gpio_set_function(PWM_PIN,GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Find out which PWM slice is connected to GPIO 16 
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 10000.f); // Set divider, reduces counter clock to sysclock/this value 9000
    pwm_init(slice_num, &config, false); // Load the configuration into our PWM slice, and set it running.
    /*
        I2C configuration
    */
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    /*
        Init the configuration of the system
    */
    init_configuration();



    RTC_Time c_time; //current time
    RTC_Get(&c_time);

    if(check_if_night(c_time.hours)){
        diodes_off();
        configuration.sunset = true;
        //configuration.sunrise = false;
    }
    else{
        diodes_on(configuration.brightness_lvl);
        configuration.sunset = false;
        //configuration.sunrise = true;
    }

    while (1){
        //check if sunrise
        RTC_Get(&c_time);
        if(c_time.hours == configuration.sunrise_time.hours && c_time.minutes == configuration.sunrise_time.minutes && configuration.sunset == true){
            sunrise_simulation();
            configuration.sunset = false;
            uart_puts(UART_ID,"Sunrise!\n");
        }
        else if(c_time.hours == configuration.sunset_time.hours && c_time.minutes == configuration.sunset_time.minutes && configuration.sunset == false){
            sunset_simulation();
            configuration.sunset = true;
            uart_puts(UART_ID,"Sunset!\n");
        }
        busy_wait_us(1000000);
        tight_loop_contents();
    }
    return 0;
}