#include "utils.h"
#include "callbacks.h"
#include "color.h"
#include "drawing.h"
#include "font.h"
#include "controls.h"
#include "settings.h"

void show_error(const char msg[], bool fatal)
{
	if (fatal)
	{
	pspDebugScreenEnableBackColor(1);
	pspDebugScreenSetTextColor(COL_WHITE);
		pspDebugScreenClear();
		pspDebugScreenPrintf("FATAL ERROR!!\n");
		pspDebugScreenPrintf(msg);
		pspDebugScreenPrintf("\nPress %c to quit", settings.cancel_char);
		
		ctrl.wait(CTRL_CANCEL, (const u32*)&(ctrl.released.value));
		
		state.running = false;
		sceKernelExitGame();
		wait_vblank();
		return;
	}
	
	gu_start(true);
	clear_screen(COL_BLACK);
	font->set_style(0.8f, COL_WHITE, 0, INTRAFONT_ALIGN_LEFT);
	font->print(msg, 20, 35, 440);
	font->set_style(0.8f, COL_WHITE, 0, INTRAFONT_ALIGN_CENTER);
	font->printf(240, 262, "Press %c to return", settings.cancel_char);
	gu_end();
	gu_sync();
	flip_screen(false);
	
	ctrl.wait(CTRL_CANCEL, (const u32*)&(ctrl.released.value));
}
