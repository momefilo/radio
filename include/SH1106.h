#ifndef sh1106_h
#define sh1106_h 1
#include "hardware/i2c.h"
//1.3inch display
#define OLED13_SET_PAGE_ADDR _u(0xB0)
#define OLED13_SET_COLUMN_LOWBIT _u(0x00)
#define OLED13_SET_COLUMN_HIGHBIT _u(0x10)
// commands (see datasheet)
#define OLED_SET_CONTRAST _u(0x81)
#define OLED_SET_ENTIRE_ON _u(0xA4)
#define OLED_SET_NORM_INV _u(0xA6)
#define OLED_SET_DISP _u(0xAE)
#define OLED_SET_MEM_ADDR _u(0x20)
#define OLED_SET_COL_ADDR _u(0x21)
#define OLED_SET_PAGE_ADDR _u(0x22)
#define OLED_SET_DISP_START_LINE _u(0x40)
#define OLED_SET_SEG_REMAP _u(0xA0)
#define OLED_SET_MUX_RATIO _u(0xA8)
#define OLED_SET_COM_OUT_DIR _u(0xC0)
#define OLED_SET_DISP_OFFSET _u(0xD3)
#define OLED_SET_COM_PIN_CFG _u(0xDA)
#define OLED_SET_DISP_CLK_DIV _u(0xD5)
#define OLED_SET_PRECHARGE _u(0xD9)
#define OLED_SET_VCOM_DESEL _u(0xDB)
#define OLED_SET_CHARGE_PUMP _u(0x8D)
#define OLED_SET_HORIZ_SCROLL _u(0x26)
#define OLED_SET_SCROLL _u(0x2E)
//#define OLED_ADDR _u(0x3C)
#define OLED_HEIGHT _u(72)
#define OLED_WIDTH _u(130)
#define OLED_PAGE_HEIGHT _u(8)
#define OLED_NUM_PAGES (OLED_HEIGHT / OLED_PAGE_HEIGHT)
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)
#define OLED_WRITE_MODE _u(0xFE)
#define OLED_READ_MODE _u(0xFF)

#define TCA_ADDR _u(0x70)

class SH1106{
	public:
		//constructor
		SH1106(i2c_inst_t *i2cbus, int address, int port0, int port1);
		~SH1106();
		void setStationNr(int station, bool invert);
		void setFreq(char *freq, bool invert);
		void setStationName(char *stationname);
		void setRDSText(char *rdstext);
		void clearDisplay(int display);

	private:
		int m_address;
		int m_port0;
		int m_port1;
		uint8_t buf0[OLED_BUF_LEN] = {0};
		i2c_inst_t *m_i2cbus;
		struct render_area{
			uint8_t start_col;
			uint8_t end_col;
			uint8_t start_page;
			uint8_t end_page;
			int buflen;
		};
		struct render_area hole_area = {start_col:0, end_col:OLED_WIDTH-1, start_page:0, end_page:OLED_NUM_PAGES-1};
		void init_oled();
		void calc_renderarea(struct render_area *area);
		void send_cmd(uint8_t cmd);
		void send_buf(uint8_t buf[], int buflen);
		void render(uint8_t *buf, struct render_area *area);
		void renderStr(char buf[], int buflen, int startcol, int startpage, int font, bool invert);// font8x8=0 font12x16=1;
		void print_static();
		void set_i2cPort(int port);
};
#endif
