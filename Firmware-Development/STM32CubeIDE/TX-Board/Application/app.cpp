#include "app.h"
#include "main.h"
#include "usart.h"
#include "gps.h"
#include "subghz_phy_app.h"

static GPS gps;

void App::setup(void)
{
	gps.setup();
}

void App::loop(void)
{
	gps.loop();
//	HAL_Delay(5000);
//	SwitchToRxMode();
}
