#include <cstdio>
#include "callbacks.h"
#include "utils.h"
#include "color.h"
#include "drawing.h"
#include "controls.h"
#include "font.h"
#include "settings.h"

void show_notdone()
{
	ctrl.flush(false);
	while (state.running)
	{
		if ( gu_start() )
		{
			draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
											RGB(32,32,32), RGB(32,32,32), \
											0, 0, 480, 272 );
			
			draw_colorRect( RGB(0x61,0xb4,0xff),RGB(0x61,0xb4,0xff), \
											RGB(0,0,0x6f),RGB(0,0,0x6f), \
											0, 230, 480, 8 );

			font->set_style(1.f, COL_WHITE, 0, INTRAFONT_ALIGN_CENTER);
			font->print("This part is yet to be done.", 240, 136);
			
			font->set_style(0.8f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_RIGHT);
			font->printf(475, 267, "Press %c to quit", settings.cancel_char);
			
			show_topbar("Not Done Yet");
			
			gu_end();
			
			ctrl.read();
			
			gu_sync();
			flip_screen();
			
			if (ctrl.released.cancel) return;
		}
		else wait_vblank();
	}
}
