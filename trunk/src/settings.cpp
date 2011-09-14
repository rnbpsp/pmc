#include "utils.h"
#include "callbacks.h"
#include "settings.h"
#include "controls.h"
#include "drawing.h"
#include "font.h"
#include "player.h"
#include "item_list.h"

PMC_SETTINGS settings;
extern void show_notdone();

#include <minIni.h>
#include <psputility.h>

#define FILE_SETS "ini/settings.ini"
#define FILE_EXTS "ini/cpu.ini"

#define SET_MAIN 0
#define SET_EXTS 1
#define SET_BOOKMARK 2

static const struct {
	const char *name;
	int cp;
}cp_struct[] = {
	{"ASCII",			CCC_CP000},
	{"US",				CCC_CP437},
	{"Multilingual Latin I", CCC_CP850},
	{"Russian",		CCC_CP866},
	{"S-JIS",			CCC_CP932},
	{"GBK",				CCC_CP936},
	{"Korean",		CCC_CP949},
	{"Big5",			CCC_CP950},
	{"Cyrillic",	CCC_CP1251},
	{"Latin II",	CCC_CP1252},
	{"UTF-8",			CCC_CPUTF8}
};

u32
PMC_SETTINGS::get_codepage()
{
	cp_struct[code_page].cp;
}

/*
static show_osk(const char *desc, bool ext)
{
	unsigned short description[32], ;
	
	while ()
	{
			draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
											RGB(32,32,32), RGB(32,32,32), \
											0, 0, 480, 272 );
								//			RGB(0x6f,0x6f,0x6f),RGB(0x6f,0x6f,0x6f), \
			
			draw_colorRect( RGB(0x61,0xb4,0xff),RGB(0x61,0xb4,0xff), \
											RGB(0,0,0x6f),RGB(0,0,0x6f), \
											0, 230, 480, 8 );
			
	}
}
*/
static FORCE_INLINE
int next_submenu(int current)
{
	if (current==SET_BOOKMARK)
		return SET_MAIN;
	else return current+1;
}

static FORCE_INLINE
int prev_submenu(int current)
{
	if (current==SET_MAIN)
		return SET_BOOKMARK;
	else return current-1;
}

static
const char *ext_vec_helper(int pos)
{
	static char cpu_str[10];
	pmc_itoa(settings.exts[pos].clock, cpu_str, 10);
	return cpu_str;
}

static void show_exts()
{
	PMC_LIST list(COL_BLACK, COL_WHITE, COL_WHITE, COL_BLACK,
								9, 85, font->get_height()*0.8f + .5f, 40, 0.8f, 480-40,
								INTRAFONT_ALIGN_LEFT, INTRAFONT_ALIGN_LEFT);
								
	list.fixup<NEEDED_EXT>(settings.exts);
	
	unsigned cur_sel = 0, top_item = 0;
	bool ext_menu = true;
	while (state.running)
	{
		if (gu_start())
		{
				draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
												RGB(32,32,32), RGB(32,32,32), \
												0, 0, 480, 272 );
									//			RGB(0x6f,0x6f,0x6f),RGB(0x6f,0x6f,0x6f), \
				
				draw_colorRect( RGB(0x61,0xb4,0xff),RGB(0x61,0xb4,0xff), \
												RGB(0,0,0x6f),RGB(0,0,0x6f), \
												0, 230, 480, 8 );
				
				if (list.show_scroll())
				{
					float scroll_size = 145*list.scroll_size(settings.exts.size());
					scroll_size = scroll_size < 5 ? 5 : scroll_size;
					draw_colorRect( RGB(0x51,0xa4,0xff),RGB(10,10,0x6f), \
													RGB(0x51,0xa4,0xff),RGB(10,10,0x6f), \
												20, 75+((145-scroll_size)*list.scroll_pos(top_item)), \
												7, scroll_size );
				}
				
				font->set_style(0.8f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_RIGHT);
				font->printf(475, 267, "Press %c to return", settings.cancel_char);
				
				font->set_style(0.8f, RGB(50,128,192), COL_WHITE, INTRAFONT_ALIGN_CENTER);
				font->print("File Extensions and CPU Speed", 480/2, 60);
				
				list.print<NEEDED_EXT>(top_item, cur_sel, settings.exts);
				list.print<NEEDED_EXT>(top_item, cur_sel, settings.exts, &ext_vec_helper, 480-30, INTRAFONT_ALIGN_RIGHT);
				
				show_topbar("Settings");
				gu_end();
				
				ctrl.read();
				if (ctrl.pressed.ok)
				{/*
					if (ext_menu)
						show_osk();
					else
						show_osk();*/
					show_notdone();
				}
				else if (ctrl.pressed.up)		list.up(top_item, cur_sel);
				else if (ctrl.pressed.down)	list.down(top_item, cur_sel);
				else if (ctrl.pressed.left)	list.page_up(top_item, cur_sel);
				else if (ctrl.pressed.right)	list.page_down(top_item, cur_sel);
				else if (ctrl.released.cancel)break;
				
				gu_sync();
				flip_screen();
		}
		else wait_vblank();
	}
}

void PMC_SETTINGS::show_menu()
{
	ctrl.autorepeat = true;
	ctrl.autorepeat_delay = 10;
	ctrl.autorepeat_interval = 5;
	ctrl.autorepeat_mask = (CTRL_RIGHT|CTRL_LEFT|CTRL_UP|CTRL_DOWN);
	
	player.close();
	
	this->cpu = 166;
	
	int cur_ico = 0;
	while (state.running)
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
				font->printf(475, 267, "Press %c to return", settings.cancel_char);
				
				const char *setmenu_names[] = {
					"Main Settings",
					"File Extensions and CPU Speed",
					"Bookmarks Manager"
				};
				
				for(int i=0; i<3; ++i)
				{
					if (cur_ico==i)
						font->set_style(0.8f, RGB(50,128,192), 0, INTRAFONT_ALIGN_CENTER);
					else
						font->set_style(0.8f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_CENTER);
					
					font->print(setmenu_names[i], 480/2, 70+(i*font->get_height()));
				}
				
				show_topbar("Settings");
				gu_end();
				
				ctrl.read();
				
							if (ctrl.released.cancel)	break;
				else	if (ctrl.pressed.up)			cur_ico = prev_submenu(cur_ico);
				else	if (ctrl.pressed.down)		cur_ico = next_submenu(cur_ico);
				else	if (ctrl.pressed.ok)
				{
					switch (cur_ico)
					{
						case SET_EXTS:
							show_exts();
							break;
						case SET_MAIN:
						case SET_BOOKMARK:
						default:
							show_notdone();
							break;
					}
				}
				
				gu_sync();
				flip_screen();
		}
		else wait_vblank();
	}
}

void PMC_SETTINGS::refresh()
{
	// OK button
	{
		int ok_button = 0;
		sceUtilityGetSystemParamInt(9, &ok_button);
		
		if (ok_button==SET_OK_CIRCLE)
		{
			ok_char = 'O';
			cancel_char = 'X';
			CTRL_OK = CTRL_CIRCLE;
			CTRL_CANCEL = CTRL_CROSS;
		} else {
			ok_char = 'X';
			cancel_char = 'O';
			CTRL_OK = CTRL_CROSS;
			CTRL_CANCEL = CTRL_CIRCLE;
		}
	}
	
	// Menu cpu clocks
	{
#define GET_MENU_CLOCK(x) itmp = static_cast<int>(ini_getl("CPU", x, 222, FILE_SETS))
#define SET_MENU_CLOCK(x, min) clocks[x] = pmc_minmax<int>(itmp, min, 333)

		int	itmp;
		GET_MENU_CLOCK("Default");
		SET_MENU_CLOCK(DEFAULT_CLOCK, 111); // ms access is so slow
		
		GET_MENU_CLOCK("file browser");
		SET_MENU_CLOCK(FBROWSER_CLOCK, 50);
		
		GET_MENU_CLOCK("settings");
		SET_MENU_CLOCK(SETTINGS_CLOCK, 50);
	}
	
	dynamic_clock = static_cast<bool>(ini_getbool("SETTINGS", "dynamic cpu change", 0, FILE_SETS));

	fileSort_mode = static_cast<int>(ini_getl("SETTINGS", "file sort mode", 3, FILE_SETS));
	fileSort_mode = pmc_minmax<int>(fileSort_mode, 0, 3);
	
	show_unknown = static_cast<bool>(ini_getbool("SETTINGS", "show unknown files", 1, FILE_SETS));
	
	code_page = static_cast<int>(ini_getl("SETTINGS", "code page", 9, FILE_SETS));
	code_page = code_page<0||code_page>10?9:code_page;
	
	// file extensions
	{
	//	NEEDED_EXT tmp;
		char buf[INI_BUFFERSIZE];
		
		exts.clear();
		for (int i=0; ini_getkey("File Extensions", i, buf, INI_BUFFERSIZE, FILE_EXTS) > 0; ++i)
		{
			int cpu = static_cast<int>(ini_getl("File Extensions", buf, 0, FILE_EXTS));
		//	cpu = pmc_max<int>(cpu, 0); // make sure it's not negative
			
		//	if (cpu==0) cpu = clocks[DEFAULT_CLOCK];
			cpu = (cpu<=0) ? 0 : pmc_minmax<int>(cpu, 10, 333);
			
			printf("audio ext: %s = %d\n", buf, cpu);
		//	tmp.set(buf, cpu);
			NEEDED_EXT tmp(buf, cpu);
			exts.push_back(tmp);
		}

		// default extensions if ever reading from ini file fails
		if (exts.empty())
		{
			NEEDED_EXT tmp;
			printf("exts is still empty?\n");
			tmp = "MP3";
			exts.push_back(tmp);
			tmp = "WMA";
			exts.push_back(tmp);
			tmp = "MP4";
			exts.push_back(tmp);
			tmp = "M4A";
			exts.push_back(tmp);
			tmp = "AAC";
			exts.push_back(tmp);
			tmp = "OGG";
			exts.push_back(tmp);
			tmp = "FLAC";
			exts.push_back(tmp);
			tmp = "AT3";
			exts.push_back(tmp);
			tmp = "WAV";
			exts.push_back(tmp);
			tmp = "AA3";
			exts.push_back(tmp);
		}
	}
}

bool
PMC_SETTINGS::isNeeded(const char *file, bool list)
{
	if (list && show_unknown)
		return true;
	
	const char *ext = strrchr(file, '.');
	if (!ext) return false;
	ext += 1;
	
	for(int i=0; i<exts.size(); ++i)
		if (strcasecmp(ext, exts[i].ext)==0)
			return true;
	
	return false;
}
