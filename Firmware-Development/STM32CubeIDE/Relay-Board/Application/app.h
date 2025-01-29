#ifndef APPLICATION_H
#define APPLICATION_H

//#define CUSTOM_BOARD_ID 1
#define BOARD_ID 0x1234
#define DAY_TIMEOUT 60000U // (1000U * 60U * 60U * 24U) // 1 Day Timeout

class App
{
public:
	void setup(void);
	void loop(void);
};
#endif // APPLICATION_H
