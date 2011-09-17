#include "utils.h"
#include "color.h"
#include "image.h"
#include "drawing.h"

#include "font.h"
#include "color.h"
#include "image.h"
#include <cstring>
#include "utils.h"
#include "topbar.h"

#include <psprtc.h>
#include <psppower.h>
#include <psphprm.h>

//Pmc_Image *topbar_img = NULL;
Pmc_ImageTile topbar_tile[8];

// never call this without printing the time
static	FORCE_INLINE
void print_date(pspTime& cur_time, float x, float y, float size, u32 txt_color, u32 shad_color, unsigned int pos)
{
	int day = sceRtcGetDayOfWeek(cur_time.year, cur_time.month, cur_time.day);
	int month = cur_time.month-1;
	
	const char *days[] =
	{
		"Sunday",
		"Monday",
		"Tuesday",
		"Wednesday",
		"Thursday",
		"Friday",
		"Saturday"
	};
	const char *months[] =
	{
		"Jan", "Feb", "Mar",
		"Apr", "May", "Jun",
		"Jul", "Aug", "Sep",
		"Oct", "Nov", "Dec"
	};
//	char date_string[16];
//	sprintf(date_string, "%s, %s %d", days[day], months[month], time.day);

	font->set_style(size, txt_color, shad_color, pos);
	font->printf(x, y, "%s, %s %d", days[day], months[month], cur_time.day);
}

static FORCE_INLINE
void print_time(pspTime& cur_time, float x, float y, float size, u32 txt_color, u32 shad_color, unsigned int pos)
{
	sceRtcGetCurrentClockLocalTime(&cur_time);
	
	char seconds = cur_time.seconds & 1 ? ':' : ' ';
	
	int hour;
	char am_pm;
	if ( cur_time.hour > 12 )
	{
		hour = cur_time.hour-12;
		am_pm = 'P';
	}
	else
	{
		hour = cur_time.hour;
		am_pm = 'A';
		if (hour==0) hour = 12;
	}
	
//	char time_string[8];
	
//	sprintf(time_string, "%d"	"%c"	"%02d"	" %cM",
//					hour, seconds, time.minutes, am_pm
//				);
	font->set_style(size, txt_color, shad_color, pos|INTRAFONT_WIDTH_FIX|15);
	font->printf(x, y, "%d"	"%c"	"%02d"	" %cM",
					hour, seconds, cur_time.minutes, am_pm );
}

// 506*68 topbar space
void show_topbar(const char *menu_name)
{
	topbar_tile[TOP_BASE].draw_strip(0,0);
	
	int battery = battery_percent(false);
	if (battery >= 0 && battery <= 100)
	{
		topbar_tile[BAT_BASE].draw(0,0);
		topbar_tile[BAT_TAIL].draw(0,0);
		
		battery = topbar_tile[BAT_BODY].width*(battery*(1.f/100.f));
		topbar_tile[BAT_BODY].scaleX = battery;
		topbar_tile[BAT_BODY].draw(topbar_tile[BAT_TAIL].width, 0);
		
		topbar_tile[BAT_HEAD].draw(topbar_tile[BAT_TAIL].width+battery, 0);
		topbar_tile[BAT_CAN].draw(0, 0);
		
		if (scePowerIsPowerOnline()>0) topbar_tile[BAT_PLUG].draw(0, 0);
	}
	
	if (sceHprmIsHeadphoneExist())
		topbar_tile[EAR_PLUG].draw(topbar_tile[BAT_CAN].width+2, 0);
	
	if ((battery = battery_percent(true)))
	{
		font->set_style(0.7f, COL_WHITE, 0, INTRAFONT_ALIGN_CENTER);
		font->printf(28, 21, "%d%%", battery);
	}
	
	pspTime cur_time;
	print_time(cur_time, 248.f, 30.f, 0.8f, COL_WHITE, 0, INTRAFONT_ALIGN_RIGHT);
	print_date(cur_time, 248.f, 14.5f, 0.6f, COL_WHITE, 0, INTRAFONT_ALIGN_RIGHT);
	
	font->set_style(1.f, RGB(50,128,192), 0, INTRAFONT_ALIGN_CENTER);
	font->print(menu_name, 366.5f, 25.5f);
}
