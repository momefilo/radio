#include <stdio.h>
#include <cstring>
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "../SI4703.h"

i2c_inst_t *set_myi2c(){
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#error SH1106 requires a board with I2C pins
	return false;
#else
	//max i2c-speed on pico is  100khz
	i2c_init(i2c_default, 100*1000);
	//set the dafault GP4 and GP5 as I2C pins on pico
	gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
	//set pullup-resistors
	gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
	gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
	return i2c_default;
#endif
}

SI4703::SI4703(uint address, int rstport, int stcport)
{
//	this->myi2cbus = i2c_default;
	this->myaddress = address;
	this->myrstport = rstport;
	this->mystcport = stcport;
	printf("SI4703 Adresse = %d\n", this->myaddress);

	//init the ports
	gpio_init(this->myrstport);
	gpio_init(this->mystcport);
	gpio_init(PICO_DEFAULT_I2C_SDA_PIN);
	gpio_init(PICO_DEFAULT_I2C_SCL_PIN);
	gpio_set_drive_strength(this->myrstport, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_drive_strength(this->mystcport, GPIO_DRIVE_STRENGTH_2MA);
	gpio_set_pulls(this->mystcport, 1, 0);
	gpio_set_pulls(this->myrstport, 0, 1);
	gpio_set_pulls(PICO_DEFAULT_I2C_SDA_PIN, 0, 1);
	gpio_set_pulls(PICO_DEFAULT_I2C_SCL_PIN, 0, 1);

	gpio_set_dir(this->myrstport, 1);
	gpio_set_dir(PICO_DEFAULT_I2C_SDA_PIN, 1);
	gpio_set_dir(PICO_DEFAULT_I2C_SCL_PIN, 1);

	//startcondition for the si4703
	gpio_put(PICO_DEFAULT_I2C_SDA_PIN, 0);
	gpio_put(PICO_DEFAULT_I2C_SCL_PIN, 0);
	gpio_put(this->myrstport, 0);

	//start the si4703
	sleep_ms(1);
	gpio_put(this->myrstport, 1);
	sleep_ms(1);

	//set the i2c interface
	gpio_deinit(PICO_DEFAULT_I2C_SDA_PIN);
	gpio_deinit(PICO_DEFAULT_I2C_SCL_PIN);

	this->myi2cbus = set_myi2c();
	this->init();
}

void SI4703::init()
{
	this->read_default();
	//set the XOS and AZHIN enabled
	this->controlreg[10] |=0xC0;
	this->write_control();
	sleep_ms(500);


	this->read_default();
	this->controlreg[21]=0x00;
	this->ENABLE();
	this->DMUTE(true);
	this->GPIO2(0x01);
	this->BAND(0x00);
	this->SPACE(0x02);
	this->DE(true);
	this->SEEKTH(0x0B);
	this->SKMODE(true);
	this->SKCNT(0x03);
	this->SKSNR(0x03);
	this->BLNDADJ(0x00);
	this->RDS(true);
	this->RDSM(true);
	this->STCIEN(true);
	this->VOLUME(0x0F);

	this->write_control();
	sleep_ms(100);
	this->read_default();
}

void SI4703::read_default()
{
	int ret=i2c_read_blocking(this->myi2cbus, this->myaddress, this->defaultsetting, 32, false);
	for(int i=16; i<32; i++){
		this->controlreg[i-16] = this->defaultsetting[i];
	}
	for(int i=0; i<12; i++){
		this->controlreg[i+16] = this->defaultsetting[i];
	}
}

void SI4703::write_control()
{
	i2c_write_blocking(this->myi2cbus, this->myaddress, this->controlreg, 28, false);
}

void SI4703::read_control()
{
	uint8_t buf[32] = {0};
	i2c_read_blocking(this->myi2cbus, this->myaddress, buf, 32, false);
	for(int i=16; i<32; i++){
		this->controlreg[i-16] = buf[i];
	}
	for(int i=0; i<12; i++){
		this->controlreg[i+16] = buf[i];
	}
}


uint8_t SI4703::get_rds(bool neu)
{
	uint8_t  idx, off; // index of rdsText, RDS time offset and sign
	char c1, c2;
	uint16_t mins; // RDS time in minutes
//	printf("0x%02X%02X 0x%02X%02X 0x%02X%02X 0x%02X%02X 0x%02X%02X 0x%02X%02X\n",controlreg[16],controlreg[17],controlreg[18],controlreg[19],controlreg[20],controlreg[21],controlreg[22],controlreg[23],controlreg[24],controlreg[25],controlreg[26],controlreg[27]);
	uint16_t block2 = this->controlreg[22]<<8 | this->controlreg[23];
	uint16_t block3 = this->controlreg[24]<<8 | this->controlreg[25];
	uint16_t block4 = this->controlreg[26]<<8 | this->controlreg[27];

	if (neu) {// reset all the RDS info.
		memset(this->_PSName1, 95, sizeof(this->_PSName1));
		memset(this->_PSName2, 95, sizeof(this->_PSName2));
		memset(this->programServiceName, 95, sizeof(this->programServiceName));
		memset(this->_RDSText, 0, sizeof(this->_RDSText));
		this->_lastTextIDX = 0;
		return 0;
	}
	// analyzing Block 2
	uint16_t rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
	int ret = 0;
	switch (rdsGroupType) {
		case 0x0A:
		case 0x0B:
			// The data received is part of the Service Station Name
			idx = 2 * (block2 & 0x0003);
			// new data is 2 chars from block 4
			c1 = block4 >> 8;
			c2 = block4 & 0x00FF;
			// check that the data was received successfully twice
			// before publishing the station name
			if ((_PSName1[idx] == c1) && (_PSName1[idx + 1] == c2)) {
				// retrieved the text a second time: store to _PSName2
				_PSName2[idx] = c1;
				_PSName2[idx + 1] = c2;
				_PSName2[8] = '\0';

				if ((idx == 6) && strcmp(_PSName1, _PSName2) == 0) {
					if (strcmp(_PSName2, programServiceName) != 0) {
						// publish station name
						strcpy(programServiceName, _PSName2);
						ret = 1;
					}
				}
			}

			if ((_PSName1[idx] != c1) || (_PSName1[idx + 1] != c2)) {
				_PSName1[idx] = c1;
				_PSName1[idx + 1] = c2;
				_PSName1[8] = '\0';
			}

			return ret;
		case 0x2A:
			// The data received is part of the RDS Text.
			_textAB = (block2 & 0x0010);
			idx = 4 * (block2 & 0x000F);
			if (idx == 60/*< _lastTextIDX*/) {// the existing text might be complete
				ret = 2;
			}
			_lastTextIDX = idx;

			if (_textAB != _last_textAB) {// when this bit is toggled the whole buffer should be cleared.
				_last_textAB = _textAB;
				memset(_RDSText, 0, sizeof(_RDSText));
			}

			// new data is 2 chars from block 3
			_RDSText[idx] = (block3 >> 8);     idx++;
			_RDSText[idx] = (block3 & 0x00FF); idx++;
			// new data is 2 chars from block 4
			_RDSText[idx] = (block4 >> 8);     idx++;
			_RDSText[idx] = (block4 & 0x00FF); idx++;
			return ret;
	}
	return 0;
}

uint8_t *SI4703::get_reg()
{
	this->read_control();
	return this->controlreg;
}

uint8_t *SI4703::tuning(uint16_t kanal)
{
	this->read_control();
	this->controlreg[2] = 0x00;
	this->controlreg[3] |= kanal;
	this->controlreg[2] |= kanal >> 8;
	this->controlreg[2] |= 0x80;
	i2c_write_blocking(this->myi2cbus, this->myaddress, this->controlreg, 4, false);

	while(gpio_get(this->mystcport));
	this->read_control();
	this->controlreg[2] &= 0x7F;
	i2c_write_blocking(this->myi2cbus, this->myaddress, this->controlreg, 4, false);
	return this->controlreg;
}

uint8_t *SI4703::seeking(bool up)
{
	//seek from chan0 to chanX=(BAND_end - BAND_begin)/SPACE
	this->read_control();
	this->SEEKUP(up);
	this->SEEK(true);
	i2c_write_blocking(this->myi2cbus, this->myaddress, this->controlreg, 2, false);

	//wait for gpio
	while(gpio_get(this->mystcport));
	this->read_control();
	this->SEEK(false);
	i2c_write_blocking(this->myi2cbus, this->myaddress, this->controlreg, 2, false);
	return this->controlreg;
}

void SI4703::seekMode(bool stop)
{
	this->SKMODE(stop);
	i2c_write_blocking(this->myi2cbus, this->myaddress, this->controlreg, 2, false);
}
void SI4703::printreg()
{
/*
	printf("Default\n");
	int k= 12;
	for(int i=0; i<32; i=i+2){
		printf("reg 0x%02X: 0x%02X%02X\n", i/2, this->defaultsetting[k], this->defaultsetting[k+1]);
		k = k + 2;
		if(k == 32) k = 0;
	}
*/
	for(int i=0; i<28; i=i+2){
		printf("Control 0x%02X: 0x%02X%02X\n", i/2+2, this->controlreg[i], this->controlreg[i+1]);
	}
}

void SI4703::DSMUTE(bool bit)
{
	uint mask = 0x80;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0x7F;
		this->controlreg[0] &= mask;
	}
}

void SI4703::DMUTE(bool bit)
{
	uint mask = 0x40;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0xBF;
		this->controlreg[0] &= mask;
	}
}

void SI4703::MONO(bool bit)
{
	uint mask = 0x20;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0xDF;
		this->controlreg[0] &= mask;
	}
}

void SI4703::RDSM(bool bit)
{
	uint mask = 0x08;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0xF7;
		this->controlreg[0] &= mask;
	}
}

void SI4703::SKMODE(bool bit)
{
	uint mask = 0x04;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0xFB;
		this->controlreg[0] &= mask;
	}
}

void SI4703::SEEKUP(bool bit)
{
	uint mask = 0x02;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0xFD;
		this->controlreg[0] &= mask;
	}
}

void SI4703::SEEK(bool bit)
{
	uint mask = 0x01;
	if( bit){
		this->controlreg[0] |= mask;
	}else{
		mask = 0xFE;
		this->controlreg[0] &= mask;
	}
}

void SI4703::ENABLE()
{
	this->controlreg[1] = 0x01;
}

void SI4703::DISABLE()
{
	this->controlreg[1] = 0x41;
}

void SI4703::RDSIEN(bool bit)
{
	uint mask = 0x80;
	if( bit){
		this->controlreg[4] |= mask;
	}else{
		mask = 0x7F;
		this->controlreg[4] &= mask;
	}
}

void SI4703::STCIEN(bool bit)
{
	uint mask = 0x40;
	if( bit){
		this->controlreg[4] |= mask;
	}else{
		mask = 0xBF;
		this->controlreg[4] &= mask;
	}
}

void SI4703::RDS(bool bit)
{
	uint mask = 0x10;
	if( bit){
		this->controlreg[4] |= mask;
	}else{
		mask = 0xEF;
		this->controlreg[4] &= mask;
	}
}

void SI4703::DE(bool bit)
{
	uint mask = 0x08;
	if( bit){
		this->controlreg[4] |= mask;
	}else{
		mask = 0xF7;
		this->controlreg[4] &= mask;
	}
}

void SI4703::BLNDADJ(uint8_t level)
{
	this->controlreg[5] &= 0x3F;
	this->controlreg[5] |=  level << 6;
}

void SI4703::GPIO3(uint8_t set)
{
	this->controlreg[5] &= 0xCF;
	this->controlreg[5] |=  set << 4;
}

void SI4703::GPIO2(uint8_t set)
{
	this->controlreg[5] &= 0xF3;
	this->controlreg[5] |=  set << 2;
}

void SI4703::GPIO1(uint8_t set)
{
	this->controlreg[5] &= 0xFC;
	this->controlreg[5] |=  set;
}

void SI4703::SEEKTH(uint8_t th)
{
	this->controlreg[6] =  th;
}

void SI4703::BAND(uint8_t set)
{
	if(set == 0x00)this->bandoffset = 87.5;
	if(set == 0x01 || set == 0x02)this->bandoffset = 76;
	this->controlreg[7] &= 0x3F;
	this->controlreg[7] |=  set << 6;
}

void SI4703::SPACE(uint8_t set)
{
	if(set == 0x00)this->space = 0.2;
	if(set == 0x01)this->space = 0.1;
	if(set == 0x02)this->space = 0.05;
	this->controlreg[7] &= 0xCF;
	this->controlreg[7] |=  set << 4;
}

void SI4703::VOLUME(uint8_t vol)
{
	this->controlreg[7] &= 0xF0;
	this->controlreg[7] |=  vol;
}

void SI4703::SMUTER(uint8_t set)
{
	this->controlreg[8] &= 0x3F;
	this->controlreg[8] |=  set << 6;
}

void SI4703::SMUTEA(uint8_t set)
{
	this->controlreg[8] &= 0xCF;
	this->controlreg[8] |=  set << 4;
}

void SI4703::VOLEXT(bool bit)
{

	uint8_t mask = 0x01;
	if(bit){
		this->controlreg[8] |=  mask;
	}else{
		mask = 0xFE;
		this->controlreg[8] &=  mask;
	}
}

void SI4703::SKSNR(uint8_t th)
{
	this->controlreg[9] &= 0x0F;
	this->controlreg[9] |=  th << 4;
}

void SI4703::SKCNT(uint8_t th)
{
	this->controlreg[9] &= 0xF0;
	this->controlreg[9] |=  th;
}

SI4703::~SI4703()
{

}
