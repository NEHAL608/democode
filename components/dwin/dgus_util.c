/**
 * @file dgus_util.c
 * @author Barry Carter
 * @date 01 Jan 2021
 * @brief DGUS II LCD Driver Utility functions
 */
#include "dgus_lcd.h"
#include <stdint.h>
#include <stddef.h>
#include "dgus_util.h"

/*
void dgus_set_alert(uint8_t alertID,uint8_t page,char *buffer)
{
  uint16_t adress=0x1033;
  dgus_set_var(adress,alertID);
  dgus_set_text(0x2100,buffer);
  dgus_set_page(page);
}*/

/* Change the page on the DGUS */
DGUS_RETURN dgus_set_page(uint8_t page) {
  return dgus_set_var(PicSetPage, PIC_SET_PAGE_BASE + page);
}

DGUS_RETURN dgus_get_page(uint8_t *page) {
  uint16_t pg = 0;
  DGUS_RETURN r = dgus_get_var(PicPage, &pg, 1);
  *page = pg;
  return r;
}

/* Set the icon ID for the icon at address */
DGUS_RETURN dgus_set_icon(uint16_t icon_addr, uint8_t val) {
  return dgus_set_var(icon_addr, val);
}

DGUS_RETURN dgus_play_sound(uint8_t sound_id, uint8_t section_id, uint8_t volume, uint8_t play_mode) {
  dgus_cmd_music m = {
    .start_id = sound_id,
    .section_id = section_id,
    .volume = volume,
    .play_mode = play_mode
  };
  return dgus_set_var8(MusicPlaySet, (uint8_t *)&m, sizeof(m));
}

DGUS_RETURN dgus_set_volume(uint8_t volume)
{
  uint16_t vol = (uint16_t)volume << 8;
  return dgus_set_var8(MusicPlaySet + 1, (uint8_t *)&vol, 2);
}

DGUS_RETURN dgus_get_volume(uint8_t *volume) {
  dgus_cmd_music m;
  DGUS_RETURN r = dgus_get_var8(MusicPlaySet, (uint8_t *)&m, sizeof(m));
  if (r != DGUS_OK)
    return r;

  *volume = m.volume;
  return DGUS_OK;
}

DGUS_RETURN dgus_set_rtc(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint8_t year) {

     rtc_cmd_register c= {
        .year=year,                    
        .month=month,                
        .day=day,                    
        .week=0x01,   
        .hour=hour,         
        .minute=minute,         
        .second=second,   
        .notdefine=0x00,
  };
  return dgus_set_var8(0x0010 , (uint8_t *)&c,sizeof(c));
}

DGUS_RETURN dgus_get_rtc(uint16_t icon_addr, uint8_t val) {
  return 0;
}

DGUS_RETURN dgus_set_brightness(uint8_t brightness) {
  uint16_t delay = 1000;
  dgus_cmd_led_config l = {
    .dim_wait_ms = SWP16(delay),
    .backlight_brightness = brightness,
    .backlight_brightness_running = brightness
  };
  return dgus_set_var8(LedConfig, (uint8_t *)&l, sizeof(l));
}

DGUS_RETURN dgus_get_brightness(uint8_t *brightness) {
  dgus_cmd_led_config l;
  DGUS_RETURN r = dgus_get_var8(LedConfig, (uint8_t *)&l, sizeof(l));
  if (r != DGUS_OK)
    return r;
  *brightness = l.backlight_brightness;
  return DGUS_OK;
}

/*DGUS_RETURN dgus_get_system_config(dgus_cmd_system_config *config) {
  DGUS_RETURN r = dgus_get_var(SystemConfig, config, sizeof(dgus_cmd_system_config));
  if (r != DGUS_OK)
    return r;

  return DGUS_OK;
}*/

DGUS_RETURN dgus_set_system_config(dgus_cmd_system_config *config) {
  DGUS_RETURN r = dgus_set_var8(SystemConfig, (uint8_t *)config, sizeof(dgus_cmd_system_config));
  
  if (r != DGUS_OK)
    return r;

  return DGUS_OK;
}
/*
7: Serial port CRC check 0=off 1=on, read only.
.6: Reserved, write 0.
.5: Power on load 22 file to initialize variable space. 1= load 0= no load, read only.
.4: Variable automatic upload setting 1= on, 0= off, read and write.
.3: Touch panel audio control 1= on 0= off, read and write.
.2: Touch panel backlight standby control 1= open 0= close,
1. 0: display direction 00 = 0 °, 01 = 90 °, 10 =180 ° ,11 = 270 °, read and
write
*/
DGUS_RETURN dgus_config_set(void)
{
dgus_cmd_system_config config ;

  config.read_write_mode=0x5A;
  config.enble=0x3D;

 return dgus_set_system_config(&config);
}

DGUS_RETURN dgus_enable_touchsound(void)
{
dgus_cmd_system_config config ;

  config.read_write_mode=0x5A;
  config.enble=0x3D;

 return dgus_set_system_config(&config);
}

DGUS_RETURN dgus_disble_touchsound(void)
{
dgus_cmd_system_config config ;

  config.read_write_mode=0x5A;
  config.enble=0x32;

 return dgus_set_system_config(&config);
}

void sys_config(uint8_t is_beep,uint8_t is_sleep)
{
	#define CONFIG_ADDR		0x80
	uint8_t config_cmd[4];
	
	dgus_get_var(CONFIG_ADDR,(uint16_t*)&config_cmd,2);
	
	if(is_beep)
		config_cmd[3] |= 0x08;
	else
		config_cmd[3] &= 0xf7;
	if(is_sleep)
		config_cmd[3] |= 0x04;
	else
		config_cmd[3] &= 0xfb;
	
	config_cmd[0] = 0x5a;//Æô¶¯Ð´²Ù×÷
	dgus_set_var8(CONFIG_ADDR,(uint8_t *)config_cmd,2);
}

/*1. 0: display direction 00 = 0 °, 01 = 90 °, 10 =180 ° ,11 = 270 °, read and
write*/
DGUS_RETURN dgus_set_direction(uint16_t var) 
{
 dgus_cmd_system_config config;
 config.read_write_mode=0x5A;
  switch (var)
  {
    case 0:
      config.enble=0x30;
    break;
    case 90:
      config.enble=0x31;
    break; 
    case 180:
     config.enble=0x32;
    break; 
    case 270:
    config.enble=0x33;
    break;
  }
 return dgus_set_system_config(&config);
}


DGUS_RETURN dgus_system_reset(uint8_t full_reset) 
{
  uint32_t r ;

  if (full_reset)
    r = SWP32(DGUS_RESET_HARD);
    else
    r = SWP32(DGUS_RESET);
  return dgus_set_var8(SystemReset, (uint8_t *)&r, sizeof(r));
}

DGUS_RETURN dgus_play_buzzer(void) 
{
uint32_t r= SWP32(DGUS_BUZZER_ON);
// r = SWP32(DGUS_BUZZER_OFF);
return dgus_set_var8(MusicPlaySet, (uint8_t *)&r, sizeof(r));
}


