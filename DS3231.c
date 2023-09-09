#include "headers/DS3231.h"
#include <stdint.h>
#include "stdbool.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


RTC_Time c_time, c_alarm1, c_alarm2;


// converts BCD to decimal
uint8_t bcd_to_decimal(uint8_t number)
{
  return ( (number >> 4) * 10 + (number & 0x0F) );
}

// converts decimal to BCD
uint8_t decimal_to_bcd(uint8_t number)
{
  return ( ((number / 10) << 4) + (number % 10) );
}



// sets time and date
void RTC_Set(RTC_Time *time_t)
{
  // convert decimal to BCD
  time_t->day     = decimal_to_bcd(time_t->day);
  time_t->month   = decimal_to_bcd(time_t->month);
  time_t->year    = decimal_to_bcd(time_t->year);
  time_t->hours   = decimal_to_bcd(time_t->hours);
  time_t->minutes = decimal_to_bcd(time_t->minutes);
  time_t->seconds = decimal_to_bcd(time_t->seconds);
  // end conversion

  // write data to the RTC chip
  RTC_Write_Date(time_t);
}

// reads time and date
uint8_t RTC_Get(RTC_Time *c_time)
{

	//RTC_Time c_time;
	uint8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  uint8_t reg = DS3231_REG_SECONDS;
  uint8_t buf[7];
  i2c_write_blocking(i2c_default,DS3231_ADDRESS,&reg,1,true);
  i2c_read_blocking(i2c_default,DS3231_ADDRESS,buf,7,false);
  c_time->seconds = buf[0];
  c_time->minutes = buf[1];
  c_time->hours = buf[2];
  c_time->dow = buf[3];
  c_time->day = buf[4];
  c_time->month = buf[5];
  c_time->year = buf[6];

    
    
	c_time->seconds = bcd_to_decimal(c_time->seconds);
	c_time->minutes = bcd_to_decimal(c_time->minutes);
	c_time->hours = bcd_to_decimal(c_time->hours);
	c_time->day = bcd_to_decimal(c_time->day);
	c_time->month = bcd_to_decimal(c_time->month);
	c_time->year = bcd_to_decimal(c_time->year);
  

	return rslt;

}

int8_t RTC_Write_Date_Start()
{
  
    return 0;
}

int8_t RTC_Set_Date_Registers(RTC_Time *time_t)
{
    return 0;
}

int8_t RTC_Write_Date(RTC_Time *time_t)
{
  uint8_t buf[8];
  buf[0] = DS3231_REG_SECONDS;
  buf[1] = time_t->seconds;
  buf[2] = time_t->minutes;
  buf[3] = time_t->hours;
  buf[4] = time_t->dow;
  buf[5] = time_t->day;
  buf[6] = time_t->month;
  buf[7] = time_t->year;
  uint8_t reg = DS3231_REG_SECONDS;
  //uint8_t test_buf[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
  //i2c_write_blocking(i2c_default,DS3231_ADDRESS,&reg,1,true);
  //i2c_write_blocking(i2c_default,DS3231_ADDRESS,buf,7,false);
  i2c_write_blocking(i2c_default,DS3231_ADDRESS,buf,8,false);

  return 0;

}


// writes 'reg_value' to register of address 'reg_address'
int8_t RTC_Write_Reg(uint8_t reg_address, uint8_t reg_value)
{
    return 0;
}

// returns the value stored in register of address 'reg_address'
uint8_t RTC_Read_Reg(uint8_t reg_address)
{
  return 0;
}



