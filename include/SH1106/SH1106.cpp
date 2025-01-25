#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
//#include "pico/binary_info.h"
#include "../SH1106.h"
#include "../../../font/12x16.h"
#include "../../../font/8x16.h"

void fill(uint8_t buf[], uint8_t fill){
	for(int i=0; i<OLED_BUF_LEN; i++){buf[i]=fill;}
}

SH1106::~SH1106()
{

}

void SH1106::clearDisplay(int display)
{
	if( display >= 0) this->set_i2cPort(display);
	this->render(buf0, &hole_area);
}

SH1106::SH1106(i2c_inst_t *i2cbus, int address, int port0, int port1)
{
	this->m_address = address;
	this->m_port0 = port0;
	this->m_port1 = port1;
	this->m_i2cbus = i2cbus;
	this->set_i2cPort(1);
	fill(this->buf0, 0x00);
	this->calc_renderarea(&hole_area);
	this->init_oled();
/*
	char *tmp = "A";
	this->renderStr(tmp, 1, 2, 0, 0);
	while(1);
*/
	this->set_i2cPort(0);
	this->init_oled();
	this->print_static();
}

void SH1106::init_oled()
{
	this->send_cmd(OLED_SET_DISP | 0x00);
	this->send_cmd(OLED_SET_DISP_START_LINE);
	this->send_cmd(OLED_SET_DISP_OFFSET);
	this->send_cmd(0x00);
	//Horizontal von rechts nach links
	this->send_cmd(OLED_SET_SEG_REMAP | 0x01); //set segment re-map. Column address 127 is mapped to SEG0
	//Vertikal von unten nach oben
	this->send_cmd(OLED_SET_COM_OUT_DIR | 0x08); // set COM (common) output scan direction from bottom to up, COM[N-1] to COM0
	this->send_cmd(OLED_SET_CONTRAST);
	this->send_cmd(0xFF); //full
	this->send_cmd(OLED_SET_ENTIRE_ON); //set entire display on to follow RAM content
	this->send_cmd(OLED_SET_NORM_INV); //set not invertet
	this->send_cmd(OLED_SET_CHARGE_PUMP);
	this->send_cmd(0x14); //Vcc internally generated on board
	this->send_cmd(OLED_SET_DISP | 0x01);

	//zero the entire display
	this->clearDisplay(-1);
}

void SH1106::setStationNr(int station, bool invert)
{
	this->set_i2cPort(0);
	char text[2];
	sprintf(text, "%d", station);
	this->renderStr(text, 2, OLED_WIDTH-24, 0, 1, invert);
}

void SH1106::setFreq(char *freq, bool invert)
{
	this->set_i2cPort(0);
	this->renderStr(freq, 6, 2, 3, 1, invert);
}

void SH1106::setRDSText(char *rdstext)
{
	this->set_i2cPort(1);
	this->clearDisplay(-1);
	this->renderStr(rdstext, 64, 2, 0, 0, false);
}

void SH1106::setStationName(char *stationname)
{
	this->set_i2cPort(0);
	this->renderStr(stationname, 9, 2, 6, 1, false);
}

void SH1106::calc_renderarea(struct render_area *area)
{
	area->buflen = (area->end_col - area->start_col +1)*(area->end_page - area->start_page +1);
}

void SH1106::send_cmd(uint8_t cmd)
{
	//Co=1, D/C=0 => the driver expects a command
	uint8_t buf[2] = {0x80, cmd};
	//Attempt to write specified number of bytes to address, blocking.
	i2c_write_blocking(this->m_i2cbus, (m_address & OLED_WRITE_MODE), buf, 2, false);
}

void SH1106::send_buf(uint8_t buf[], int buflen)
{
	uint8_t tmp_buf[buflen+1];
	for(int i=1; i<buflen+1; i++) { tmp_buf[i] = buf[i-1]; }
	tmp_buf[0] = 0x40;
	i2c_write_blocking(this->m_i2cbus, (m_address & OLED_WRITE_MODE), tmp_buf, buflen + 1, false);
}

void SH1106::set_i2cPort(int port)
{
	if(port > 7) return;
	uint8_t buf[] = {uint8_t(1 << port)};
	i2c_write_blocking(this->m_i2cbus, TCA_ADDR, buf, 1, false);
}

void SH1106::print_static()
{
	this->set_i2cPort(0);
//	char station[] = "Station";
	char mhz[] = "MHz";
//	this->renderStr(station, 7, 2, 0, 1);
	this->renderStr(mhz, 3, OLED_WIDTH-48, 3, 1, false);
}

void SH1106::renderStr(char buf[], int buflen, int startcol, int startpage, int font, bool invert)
{
	int mywidth = 8;
	int myheight = 1;
	if(font > 0){ mywidth = 12; myheight = 1;}
	struct render_area area;
	area.start_col=(startcol);
	area.end_col=(startcol+mywidth-1);
	area.start_page=(startpage);
	area.end_page=(startpage+myheight);
	this->calc_renderarea(&area);

	for(int pos=0; pos<buflen; pos++){
		int zeichen = buf[pos];
		if(font > 0){
			if(invert){ this->render(FONT_12x16invert[zeichen], &area);}
			else{ this->render(FONT_12x16[zeichen], &area);}
		}
		else{
			if(invert){ this->render(FONT_8x16invert[zeichen], &area);}
			else{ this->render(FONT_8x16[zeichen], &area);}
		}
		area.start_col += mywidth;
		area.end_col = area.start_col + mywidth-1;
		if(area.end_col > (OLED_WIDTH - 1)){
			area.start_col = 2;
			area.end_col = area.start_col + mywidth - 1;
			area.start_page = area.start_page + 1 + myheight;
			area.end_page = area.start_page + myheight;
			if(area.end_page > OLED_NUM_PAGES-1){
				area.start_page = startpage;
				area.end_page = startpage + myheight;
			}
		}
	}
}

void SH1106::render(uint8_t *buf, struct render_area *area)
{
	//update a portion of the display with a render area
	int buf_pos = 0;
	int tmpbuf_len = area->buflen / (1 + area->end_page - area->start_page);
	uint8_t tmp_buf[tmpbuf_len];
	for(uint8_t page=area->start_page; page<=area->end_page; page++){
		//fill tmp_buf with data for one page
		for(uint8_t col=area->start_col; col<=area->end_col; col++){
			tmp_buf[col - area->start_col] = buf[buf_pos];
			buf_pos++;
		}
		//take the hi- and lo- byte from startcol seperatily send in two commands
		uint8_t highbits = area->start_col;
		highbits >>= 4;
		this->send_cmd(OLED13_SET_COLUMN_LOWBIT | (0x0F & area->start_col));
		this->send_cmd(OLED13_SET_COLUMN_HIGHBIT | (0x0F & highbits));
		//send startpage
		this->send_cmd(OLED13_SET_PAGE_ADDR | (0x0F & page));
		//send data
		this->send_buf(tmp_buf, tmpbuf_len);
	}
}
