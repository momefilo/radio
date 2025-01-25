#include "include/SH1106.h"
#include "include/SI4703.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "radio.h"
#define FLASH_TARGET_OFFSET (256 * 1024)
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
void set_ky040(uint sw, uint dt, uint ckl){
	gpio_init(sw);
	gpio_init(dt);
	gpio_init(ckl);
	//Ausgangsstromstaerke
	gpio_set_drive_strength(sw, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(dt, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(ckl, GPIO_DRIVE_STRENGTH_2MA);
	//pulldown off pullup on
	gpio_set_pulls(sw, 1, 0);
	gpio_set_pulls(dt, 1, 0);
	gpio_set_pulls(ckl, 1, 0);
}

void write_flash(uint16_t kanal)
{
	flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
	uint8_t buf[FLASH_PAGE_SIZE] = {0x00};
	buf[0] |= kanal & 0xFF00;
	buf[1] |= kanal & 0x00FF;
	flash_range_program(FLASH_TARGET_OFFSET, buf, FLASH_PAGE_SIZE);
}

uint16_t get_flash()
{
	uint16_t ergebnis = 0;
	ergebnis = flash_target_contents[0] << 8;
	ergebnis |= flash_target_contents[1];
	return ergebnis;
}

int main(){
	stdio_init_all();

	bool firststart = true;
	bool tuning = false;
	bool sw1sinked = false;
	bool ckl1sinked = false;
	bool sw2sinked = false;
	bool ckl2sinked = false;
	uint sw1 = 16;
	uint dt1 = 17;
	uint ckl1 = 18;
	uint sw2 = 19;
	uint dt2 = 20;
	uint ckl2 = 21;
	char skup_str[]={"seek +"};
	char skdw_str[]={"seek -"};
	char invalidfreq_str[]={"0     "};
	char invalid_str[]={"No Station"};
	char valid_str[]={"          "};
	SI4703 myradio(0x10, 14, 15);
	set_ky040(sw1,dt1,ckl1);
	set_ky040(sw2,dt2,ckl2);
	static SH1106 display_1(i2c_default, 0x3C, 0, 1);

	uint16_t kanal = 0;
	int counter = 0;
	int loopcnt = 0;
	while(1){
		uint8_t *control_reg;
		char freq_str[6];
		float freq = 0;


		if(firststart){
			firststart = false;
			kanal = get_flash();
			if(kanal > 0){
				control_reg = myradio.tuning(kanal);
			}else{
				display_1.setFreq(skup_str, false);
				control_reg = myradio.seeking(true);
			}
			if( (! (control_reg[16] & 0x30)) && (control_reg[16] & 0x40)){
				counter++;
				myradio.get_rds(true);
				uint16_t kanal =  (control_reg[18] & 0x03);
				kanal <<= 8;
				kanal |= control_reg[19];
				freq = ((kanal * myradio.space) + myradio.bandoffset);
				sprintf(freq_str, "%3.2f", freq);
				display_1.setFreq(freq_str,false);
				display_1.setStationNr(counter,true);
				myradio.seekMode(false);
			}else{
				display_1.setFreq(invalidfreq_str, false);
				display_1.setStationNr(0, true);
				display_1.setStationName(invalid_str);
			}
			freq = 0;
			control_reg = NULL;
		}

		if( ! gpio_get(sw1)){
			if(! sw1sinked){sw1sinked = true;sleep_ms( 20);}
			else{
				freq = ((kanal * myradio.space) + myradio.bandoffset);
				sprintf(freq_str, "%3.2f", freq);
				if(tuning){//seeking
					printf("seeking\n");
					display_1.setFreq(freq_str, false);
					display_1.setStationNr(counter,true);
				}else{//tuning
					printf("tuning\n");
					display_1.setFreq(freq_str, true);
					display_1.setStationNr(counter,false);
				}
				tuning = ! tuning;
				sw1sinked = false;
				sleep_ms( 300);
			}
		}

		if( (! gpio_get(ckl1)) && tuning){
			if(! ckl1sinked){ckl1sinked = true;/*sleep_ms( 20);*/}
			else{
				bool up = false;
				if( ! gpio_get(dt1)){//linksrum
					display_1.setFreq(skdw_str,true);
					display_1.setStationName(valid_str);
					if(kanal >= 1){ kanal--;}
					else{ kanal = (108-myradio.bandoffset)/myradio.space;}
					control_reg = myradio.tuning(kanal);
				}
				else{//rechtsrum
					up = true;
					display_1.setFreq(skup_str,true);
					display_1.setStationName(valid_str);
					if(kanal < (108-myradio.bandoffset)/myradio.space){ kanal++;}
					else{ kanal = 0;}
					printf("%d\n", kanal);
					control_reg = myradio.tuning(kanal);
				}
				if( ! (control_reg[16] & 0x30)){
					freq = ((kanal * myradio.space) + myradio.bandoffset);
					sprintf(freq_str, "%3.2f", freq);
					display_1.setFreq(freq_str, true);
					display_1.setStationNr(0, false);
					myradio.get_rds(true);
					display_1.clearDisplay(1);
				}else{
					freq = ((kanal * myradio.space) + myradio.bandoffset);
					sprintf(freq_str, "%3.2f", freq);
					display_1.setFreq(freq_str, true);
					display_1.setStationNr(0, false);
				}
			}
		}

		if( (! gpio_get(ckl1)) && ( ! tuning)){
			if(! ckl1sinked){ckl1sinked = true;/*sleep_ms( 20);*/}
			else{
				bool up = false;
				if( ! gpio_get(dt1)){//linksrum
					display_1.setFreq(skdw_str, false);
					display_1.setStationName(valid_str);
					control_reg = myradio.seeking(false);
				}
				else{//rechtsrum
					up = true;
					display_1.setFreq(skup_str,false);
					display_1.setStationName(valid_str);
					control_reg = myradio.seeking(true);
				}
				if( ! (control_reg[16] & 0x30)){
					kanal =  (control_reg[18] & 0x03);
					kanal <<= 8;
					kanal |= control_reg[19];
					freq = ((kanal * myradio.space) + myradio.bandoffset);
					write_flash(kanal);
					if(up){counter++;}else{counter--;}
					sprintf(freq_str, "%3.2f", freq);
					display_1.setFreq(freq_str, false);
					display_1.setStationNr(counter,true);
					myradio.get_rds(true);
					display_1.clearDisplay(1);
				}else{
					counter = 0;
					display_1.setStationNr(0, true);
					display_1.setFreq(invalidfreq_str, false);
					display_1.setStationName(invalid_str);
				}
//				sleep_ms( 20);
			}
		}

		if( ! gpio_get(sw2)){
			if(! sw2sinked){sw2sinked = true;sleep_ms( 20);}
			else{
				sw2sinked = false;
				sleep_ms( 300);
			}
		}

		//resd RDS Data
		if(loopcnt > 16){
			if(myradio.get_reg()[16] & 0x88){
				int i = myradio.get_rds(false);
				if(i==1){display_1.setStationName(myradio.programServiceName);}
				if(i==2){display_1.setRDSText(myradio._RDSText);}
				loopcnt = 0;
			}
		}
		loopcnt ++;
		sleep_ms(5);
	}
	return 0;
}
