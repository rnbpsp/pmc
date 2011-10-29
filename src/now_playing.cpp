#include "callbacks.h"
#include "utils.h"
#include "image.h"
#include "drawing.h"
#include "controls.h"
#include "font.h"
#include "player.h"
#include "now_playing.h"
#include "settings.h"

Pmc_ImageTile player_icons[8];
extern Pmc_ImageTile list_bkg;

#define return(x) \
  do { \
		retval = x; \
		goto ret; \
  }while(0)

int show_nowplaying(const char *path, const char *name)
{
	float titleX = 25.f;
	float bar_len = 0.f;
	bool seeking = false;
	int seek_speed = 1;
	int seek_totime = 0;
	int retval = 0;
	
	if (player.playing==PLAYER_STOPPED)
	{
		if (path==NULL || name==NULL)
			return 0;
		
openit:
		const bool open_ret = player.open(path, name);
		if (!open_ret)
		{
			if (state.hold_mode) { return(1); }
			else show_errorEx("Cannot open file: %s", name);
			return(0);
		}
	}
	else
	{
		if ( (path!=NULL || name!=NULL) && \
			( strcasecmp(path, player.filepath)!=0 || \
				strcasecmp(name, player.filename)!=0 ) )
			goto openit;
	}
	
	cur_submenu = NOWPLAYING;
	
//	ctrl.flush();
	ctrl.autorepeat = false;

	font->set_style(0.9f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_LEFT|INTRAFONT_SCROLL_LEFT);
	
	bar_len = player.get_percentRemaining();
	settings.cpu = player.filename;
	
	while(state.running && player.playing & PLAYER_PLAYING)
	{
		if ( gu_start() )
		{
			draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
											RGB(32,32,32), RGB(32,32,32), \
											0, 0, 480, 272 );
			list_bkg.draw_strip(0, 40);
			
			if (player.album_art)
			{
				sceGuTexFilter(GU_LINEAR,GU_LINEAR);
				drawImgStrip_large(player.album_art, 325+128-player.album_art->scaleX, 66+128-player.album_art->scaleY);
			}
			
			player_icons[PL_ICON_STAT_BASE].draw(21, 174);
			player_icons[PL_ICON_BAR_BASE].draw(21+75, 174+40);
			
			player_icons[PL_ICON_BAR].offX1 = \
					player_icons[PL_ICON_BAR].scaleX = \
						player_icons[PL_ICON_BAR].width*bar_len;
			
			player_icons[PL_ICON_BAR].draw(97, 218);
			player_icons[PL_ICON_OVERLAY].draw(97, 218);
			
			{
			int player_status = PL_ICON_PLAY;
			
			if (player.playing & PLAYER_PAUSED)
				player_status = PL_ICON_PAUSE;
			else if (seeking)
				player_status = seek_speed<0 ? PL_ICON_REW : PL_ICON_FWD;
			
			player_icons[player_status].draw(26, 179);
			}
			
			{
			const int text_width = player.album_art!=NULL ? 295 : 295+128 ;
		//	font->set_encoding(CCC_CPUTF8);
			font->set_style(0.9f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_LEFT|INTRAFONT_SCROLL_LEFT);
			titleX = font->print_ucs2(player.get_str(TAG_TITLE), titleX, 85, text_width);
			
			font->set_style(0.6f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_LEFT/*|INTRAFONT_STRING_UTF8*/);
			
			sceGuScissor(25, 0, 30+text_width, 272);
			for (int i=1; i<6; ++i)
				font->print_ucs2(player.get_str(i), 30.f, 90+((font->get_height()+3.25f)*i));
			sceGuScissor(0, 0, 480, 272);
	//		font->set_encoding(0);
			}
			
			font->set_style(0.7f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_RIGHT);
			font->print_ucs2(player.get_str(TIMER_OVER_DURATION), 453, 210);
			
			font->set_style(0.7f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_LEFT);
			if (seeking)
				font->printf(21+80, 210, "X %d", __builtin_abs(seek_speed));
			
			if (player.mode&PL_MODE_LOOP_ONE) font->print("Repeat 1", 21+80+40, 210);
			else if (player.mode&PL_MODE_LOOP_ALL) font->print("Repeat All", 21+80+40, 210);
			font->printf(21+80+40+120, 210, "Boost: %d", player.boost);
			show_topbar("Now Playing");
			
			gu_end();
			gu_sync();
			flip_screen();
		}
		else wait_vblank();
		
		if (player.playing==PLAYER_STOPPED)
		{
			if (player.mode&PL_MODE_LOOP_ONE)
			{
				path = player.filepath;
				name = player.filename;
				goto openit;
			}
			else return(1);
		}
		
		if (seeking)
		{
			ctrl.read(false);
			seek_totime += seek_speed;
			if (player.exceed_duration(seek_totime/4)) return(1);
			if (seek_totime<0) return(-1);
			
			if (ctrl.pressed.ok)
			{
				settings.cpu = settings.clocks[DEFAULT_CLOCK];
				player.seek(seek_totime/4);
				settings.cpu = player.filename;
				seeking = false;
				continue;
			}
			else if (ctrl.pressed.left)
			{
				if (seek_speed>=0) seek_speed=-1;
				else seek_speed *= 2;
			}
			else if (ctrl.pressed.right)
			{
				if (seek_speed<=0) seek_speed=1;
				else seek_speed *= 2;
			}
			else if (ctrl.released.cancel)
			{
				seeking = false;
				continue;
			}
			bar_len = player.get_percentRemaining(seek_totime/4);
		}
		else
		{
			ctrl.read(true);
						if (ctrl.released.cancel)	return(0);
			else	if (ctrl.released.square)	break;
			else	if (ctrl.pressed.triangle)player.loop();
			else	if (ctrl.pressed.R)				return(1);
			else	if (ctrl.pressed.value & (CTRL_OK|CTRL_RM_PLAY|CTRL_START))
									player.pause();
			else	if (ctrl.pressed.L)
			{
				if (bar_len > .02f)
					player.seek(0);
				else
					return(-1);
			}
			else	if (ctrl.pressed.up)
				player.boost += 1;
			else	if (ctrl.pressed.down)
			{
				if (player.boost>0) player.boost -= 1;
			}
			else	if (ctrl.pressed.left)
			{
				seeking = true;
				seek_speed = -1;
			}
			else	if (ctrl.pressed.right)
			{
				seeking = true;
				seek_speed = 1;
			}
			
			if (seeking)
			{
				seek_totime = player.get_time();
				bar_len = player.get_percentRemaining(seek_totime);
				seek_totime *= 4;
			}
			else bar_len = player.get_percentRemaining();
		}
	}
	
	player.close();
	retval = 0;
	
ret:
	player_icons[PL_ICON_BAR].offX1 =
		player_icons[PL_ICON_BAR].scaleX =
			player_icons[PL_ICON_BAR].width;
	
	if (player.playing==PLAYER_STOPPED)
		settings.cpu = settings.clocks[FBROWSER_CLOCK];
	else
		settings.cpu = settings.clocks[FBROWSER_CLOCK] + settings.cpu(player.filename);
	ctrl.autorepeat = true;
	return retval;
}
