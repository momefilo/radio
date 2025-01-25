#ifndef si4703_h
#define si4703_h 1
#include "hardware/i2c.h"

class SI4703
{
	public:
		SI4703(uint address, int rstport, int stcport);
		~SI4703();
		uint8_t *seeking(bool up);
		uint8_t *get_reg();
		uint8_t get_rds(bool neu);
		void seekMode(bool stop);
		uint8_t *tuning(uint16_t kanal);
		char programServiceName[10];
		char _RDSText[64 + 2];
		float bandoffset = 76;
		float space = 0.05;

	private:
		void DSMUTE(bool bit); // disable softmute
		void DMUTE(bool bit); // disable mute
		void MONO(bool bit); // mono
		void RDSM(bool bit); // RDS verbose mode
		void SKMODE(bool bit); // stop seek at the end of band
		void SEEKUP(bool bit); // to seek upwart
		void SEEK(bool bit); // start seek
		void RDSIEN(bool bit); // enable interrupt on GPIO2 for RDS
		void STCIEN(bool bit); // enable interrupt on GPIO2 for SEEK/TUNE
		void RDS(bool bit); // enable RDS
		void DE(bool bit); // 1=50µs for europ de-emphasis, 0=75µs usa de-emphasis
//		void AGCD(bool bit); // disable AGC
		void BLNDADJ(uint8_t level); // stereo/momo blend level adjustment: default, +6db, -12db, -6db
		void SEEKTH(uint8_t th); // 0x00-0x7F threshold for seek
		void BAND(uint8_t set); // 87,5-109, 76-108, 76-90, reserved
		void SPACE(uint8_t set); // 200kHz, 100kHz, 50kHz, not defined
		void VOLUME(uint8_t vol); // 0x00-0x0F
		void SMUTER(uint8_t set); // SOFTMUTE-ATTACK: fastest, fast, slow, slowest
		void SMUTEA(uint8_t set); // SOFTMUTE-ATTENUATION: 16dB, 14dB, 12dB, 10dB
		void VOLEXT(bool bit); // VOLEXT enable (vol down to -58dB)
		void SKSNR(uint8_t th); // SEEK-SNR Thr.: 0x00-0x07 , 0=disable, 01=min(most stops), 07=max(fewest stops)
		void SKCNT(uint8_t th); // SEEK-FM impulse Thr.: 0x00-0x0F , 0=disable, 01=max(most stops), 0F=min(fewest stops)
		void GPIO3(uint8_t set); // GPIO3 high impedance, stereo indicator, 0, 1
		void GPIO2(uint8_t set); // GPIO3 high impedance, STC/RDS indicator, 0, 1
		void GPIO1(uint8_t set); // GPIO3 high impedance, reserved, 0, 1
		void ENABLE();
		void DISABLE();

		char _PSName1[10]; // including trailing '\00' character.
		char _PSName2[10]; // including trailing '\00' character.
		uint8_t _textAB, _last_textAB, _lastTextIDX;
		uint16_t _lastRDSMinutes;

		void write_control();
		void read_control();
		void read_default();
		void printreg();
		void init();
		uint8_t myaddress = 0x10;
		uint8_t myrstport = 14;
		uint8_t mystcport = 15;
		uint8_t defaultsetting[32] = {0};
		uint8_t controlreg[28] = {0};
		i2c_inst_t *myi2cbus;
};

#endif
