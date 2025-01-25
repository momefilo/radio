#ifndef radio_h
#define radio_h 1

class menu_mono{
	public:
		menu_mono(bool state);
		void mono(int state);

	private:
		uint8_t start_col, start_page
};
menu_mono::menu_mono(bool state)
{
	int i;
}

void menu::mono(int state)
{
	printf("mono:5d\n",state);
}

#endif
