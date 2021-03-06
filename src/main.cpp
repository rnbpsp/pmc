#include <pspkernel.h>
#include <psppower.h>
#include <pspsdk.h>
#include <pspvfpu.h>
#include <RnBridge.h>
#include "main.h"
#include "callbacks.h"
#include "font.h"
#include "controls.h"
#include "settings.h"
#include "player.h"
#include "now_playing.h"
#include "topbar.h"
#if (_PROFILE)
#include <pspprof.h>
#endif

PSP_MODULE_INFO("PSP Music Center", 0, __PMC_VER_MAJOR, __PMC_VER_MINOR);
PSP_HEAP_SIZE_KB( -(8*1024) );
PSP_MAIN_THREAD_STACK_SIZE_KB(512); // 256 is default

// keep away from vfpu as much as "possible"
// i read some slide from sony it drains more battery (than not using it?)
//	http://pc.watch.impress.co.jp/docs/2005/0323/kaigai166.htm
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);

extern void show_notdone();
extern void show_filelist(bool show_playing=false);

extern Pmc_ImageTile list_bkg;
extern Pmc_ImageTile file_ico[2];
menu_type cur_submenu = HOMEMENU;
//SceUID memcpy_sema = -1;

static FORCE_INLINE
int next_submenu(int current)
{
	if (current==ABOUT)
		return FILELIST;
	else return current+1;
}

static FORCE_INLINE
int prev_submenu(int current)
{
	if (current==FILELIST)
		return ABOUT;
	else return current-1;
}

static NOINLINE
SceUID pmc_loadmod(const char *modpath, int y)
{
	SceUID uid = sceKernelLoadModule(modpath, 0, NULL);
	if (uid<0) 
	{
		pspDebugScreenPrintf("fail");
		pspDebugScreenSetXY(33,y);
		pspDebugScreenPrintf("Load module returned 0x%08x", uid);
		gu_start(true);
		vram.set_drawbuf(vram.get_dispbuf());
		font->set_style(1.0f, COL_BLACK, 0, INTRAFONT_ALIGN_RIGHT);
		font->printf(480-5, 272-5, "Press %c to quit", settings.ok_char);
		vram.restore_drawbuf();
		gu_end();
		gu_sync();
		state.running = false;
	}
	else
	{
		int ret;
		sceKernelStartModule(uid, 0, 0, &ret, NULL);
	}
	return uid;
}


int main()
{
#if !(_PROFILE)
	if (scePowerSetClockFrequency(333, 333, 166)!=0)
		printf("Cannot set initial cpu speed\n");
#endif //	!(_PROFILE)

  SceUID hold_modid = -1, rnb_modid = -1;
  Pmc_Image *icons_img = NULL;
  
	printf("set up callbacks\n");
	set_callbacks();
	
	ctrl.autorepeat = true;
	ctrl.autorepeat_delay = 10;
	ctrl.autorepeat_interval = 5;
	ctrl.autorepeat_mask = (CTRL_RIGHT|CTRL_LEFT);
	/*
	memcpy_sema = sceKernelCreateSema("VFPU memcpy", 0, 1, 1, 0);
	if (memcpy_sema<0) goto err;
	*/
	gu_init();
	
	pspDebugScreenInit();
	
	{
	Pmc_Image *intro_img = load_tga("res/splash.tga");
	
	gu_start(true);
	intro_img->draw_strip(0,0);
	gu_end();
	gu_sync();
	flip_screen(false, false);
	
	delete intro_img;
	}
	
	pspDebugScreenEnableBackColor(0);
	pspDebugScreenSetTextColor(COL_BLACK);
	pspDebugScreenSetXY(33,14);
	
	pspDebugScreenPrintf("Loading settings...............");
	settings.refresh();
	
	pspDebugScreenPrintf("done");
	wait_vblank();
	pspDebugScreenSetXY(33,15);
	pspDebugScreenPrintf("Loading font...................");

	init_font();
	font = new PMC_FONT("flash0:/font/ltn0.pgf", INTRAFONT_CACHE_LARGE);
	font->set_style(0.8f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_RIGHT);
	cccSetErrorCharUCS2((cccUCS2)' ');
	
	pspDebugScreenPrintf("done");
	wait_vblank();
	pspDebugScreenSetXY(33,16);
	pspDebugScreenPrintf("Loading images and icons.......");
	
	icons_img = load_tga("res/icons.tga");
	
	list_bkg.set(icons_img, 0, 75, 480, 232+75);

	{
	player_icons[PL_ICON_STAT_BASE].set(icons_img, 0, 324, 75, 398);
	player_icons[PL_ICON_BAR_BASE].set(icons_img, 75, 364, 436, 389);
	player_icons[PL_ICON_BAR].set(icons_img, 0, 307, 351, 324);
	player_icons[PL_ICON_OVERLAY].set(icons_img, 77, 343, 428, 360);
	
	#define FILE_ICO_DIR 0
	#define FILE_ICO_REG 1
	file_ico[FILE_ICO_REG].set(icons_img, 351, 307, 351+16, 307+16);
	file_ico[FILE_ICO_DIR].set(icons_img, 351+16, 307, 351+32, 307+16);
	
/*	
	player_icons[PL_ICON_PLAY].set(icons_img, 256, 0, 320, 64);
	player_icons[PL_ICON_PAUSE].set(icons_img, 256+64, 0, 384, 64);
	player_icons[PL_ICON_REW].set(icons_img, 256+(64*2), 0, 448, 64);
	player_icons[PL_ICON_FWD].set(icons_img, 256+(64*3), 0, 512, 64);
*/	
	for (int i=0; i<4; ++i)
		player_icons[i+PL_ICON_PLAY].set(icons_img, 256+(i*64), 0, 256+(i*64)+64, 64);
	}
	
	{ //TODO
	topbar_tile[TOP_BASE].set(icons_img,  0,  0, 256, 41);
	topbar_tile[BAT_BASE].set(icons_img,  0, 41,  64, 75);
	topbar_tile[BAT_TAIL].set(icons_img, 64, 41,  72, 75);
	topbar_tile[BAT_BODY].set(icons_img, 72, 41, 115, 75);
	topbar_tile[BAT_HEAD].set(icons_img,115, 41, 128, 75);
	topbar_tile[BAT_CAN ].set(icons_img,128, 41, 192, 75);
	topbar_tile[BAT_PLUG].set(icons_img,192, 41, 256, 75);
	topbar_tile[EAR_PLUG].set(icons_img,351+32, 307, 351+64, 307+32);
	}
	
	Pmc_Image *menu_ico[5];
	menu_ico[	FILELIST	] =	load_tga("res/file browser ico.tga");
	menu_ico[	NOWPLAYING] =	load_tga("res/now playing ico.tga");
	menu_ico[	PLAYLIST	] =	load_tga("res/playlist ico.tga");
	menu_ico[	SETTINGS	] =	load_tga("res/settings ico.tga");
	menu_ico[		ABOUT		] =	load_tga("res/about ico.tga");
	
	pspDebugScreenPrintf("done");
	wait_vblank();
	pspDebugScreenSetXY(33,17);
	pspDebugScreenPrintf("Loading support module.........");
	
  rnb_modid = pmc_loadmod("RnBridge.prx", 18);
	if (rnb_modid<0)
		goto err;
	
	state.isPSPgo = get_PSPmodel()==4;
	
	pspDebugScreenPrintf("done");
	wait_vblank();
	pspDebugScreenSetXY(33,18);
	pspDebugScreenPrintf("Unloading incompatible modules.");
	
	{
  SceUID holdp_modid = get_moduleUID("Hold");
	if (holdp_modid>=0)
	{
		int ret;
		sceKernelStopModule(holdp_modid, 0, NULL, &ret, NULL);
		sceKernelUnloadModule(holdp_modid);
	}
	}
	
	pspDebugScreenPrintf("done");
	wait_vblank();
	pspDebugScreenSetXY(33,19);
	pspDebugScreenPrintf("Loading hold- module...........");
	
  hold_modid = pmc_loadmod("hold-.prx", 20);
	if (hold_modid<0)
		goto err;
	
	pspDebugScreenPrintf("done");
	wait_vblank();
	
	pspDebugScreenEnableBackColor(1);
	pspDebugScreenSetTextColor(COL_WHITE);
	
	gu_start(true);
	vram.set_drawbuf(vram.get_dispbuf());
	font->set_style(1.0f, COL_BLACK, 0, INTRAFONT_ALIGN_RIGHT);
	font->printf(480-5, 272-5, "Press %c to continue", settings.ok_char);
	vram.restore_drawbuf();
	gu_end();
	gu_sync();
	
err:
	settings.cpu = 25;
	
	ctrl.wait(CTRL_OK, (const u32*)&(ctrl.pressed.value));
	
	cur_submenu = HOMEMENU;
	int cur_ico = 0;
	while(state.running)
	{
		if ( gu_start() )
		{
				draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
												RGB(32,32,32), RGB(32,32,32), \
												0, 0, 480, 272 );
									//			RGB(0x6f,0x6f,0x6f),RGB(0x6f,0x6f,0x6f), \
				
				draw_colorRect( RGB(0x61,0xb4,0xff),RGB(0x61,0xb4,0xff), \
												RGB(0,0,0x6f),RGB(0,0,0x6f), \
												0, 230, 480, 8 );
				
				font->set_style(0.8f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_RIGHT);
				font->printf(475, 267, "Press %c to quit", settings.cancel_char);
				
				menu_ico[cur_ico]->draw_strip(180, 82);
				
				show_topbar("Main Menu");
				gu_end();
				
				ctrl.read();
				
							if (ctrl.pressed.cancel)	state.running = false;
				else	if (ctrl.pressed.left)		cur_ico = prev_submenu(cur_ico);
				else	if (ctrl.pressed.right)		cur_ico = next_submenu(cur_ico);
				else	if (ctrl.pressed.ok)
				{
					gu_sync();
					flip_screen(false);
					
					switch (cur_ico)
					{
						case FILELIST:
							show_filelist(false);
							break;
						case NOWPLAYING:
							show_filelist(true);
							break;
						case SETTINGS:
							settings.show_menu();
							break;
						default:
							show_notdone();
							break;
					}
					
					cur_submenu = HOMEMENU;
					
					if (player.playing==PLAYER_STOPPED)
						settings.cpu = 25;
					else
						settings.cpu = player.filename;
					
					ctrl.flush();
					
					ctrl.autorepeat = true;
					ctrl.autorepeat_delay = 10;
					ctrl.autorepeat_interval = 5;
					ctrl.autorepeat_mask = (CTRL_RIGHT|CTRL_LEFT);
					continue;
				}
				gu_sync();
				flip_screen();
		}
		else wait_vblank();
	}
	
	settings.cpu = 333;
	
	delete icons_img;
	
	for(int i=0;i<5;i++) delete menu_ico[i];
	
	delete font;
	
	term_font();
	term_gu();
	
	if (hold_modid>=0)
	{
		int ret;
		sceKernelStopModule(hold_modid, 0, NULL, &ret, NULL);
		sceKernelUnloadModule(hold_modid);
	}
	if (rnb_modid>=0)
	{
		int ret;
		sceKernelStopModule(rnb_modid, 0, NULL, &ret, NULL);
		sceKernelUnloadModule(rnb_modid);
	}
	
	//if (memcpy_sema>=0) sceKernelDeleteSema(memcpy_sema);
	
#if (_PROFILE)
	if (!state.exit_viaCallback)
		gprof_cleanup();
#endif
	state.guith_done = true;

	if (state.exit_viaCallback) sceKernelDelayThread( SECONDS(1) );
		//wait_vblank();
	
	sceKernelExitGame();
	wait_vblank();
	return 0;
}
