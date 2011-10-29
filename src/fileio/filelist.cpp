#include <pspkernel.h>
#include "callbacks.h"
#include "utils.h"
#include "image.h"
#include "drawing.h"
#include "color.h"
#include "font.h"
#include "controls.h"

#include "item_list.h"
#include <pspumd.h>
#include "player.h"
#include "now_playing.h"
#include "settings.h"
#include "./fileio.h"
#include <algorithm>

Pmc_ImageTile list_bkg;
Pmc_ImageTile file_ico[2];

static char cur_dir[1024] = "";
static bool drives_shown = false, umd_open = false;

extern int show_nowplaying(const char *path=NULL, const char *name=NULL);
extern void show_notdone();


// makes sure the last character is a slash so it's easy to add filenames
static FORCE_INLINE
void dir_slash (char *dir)
{
	char *slash = strrchr(dir, '/');
	
	// as we may not find a slash if we're on the root of a drive
	if ( slash == NULL )
		strcat(dir, "/");
	else if ( slash[1] != '\0' )
		strcat(dir, "/");
}


#define sort_list() if (settings.fileSort_mode!=0) std::sort(files.begin(), files.end())
static NOINLINE
bool push_folder(std::vector<DIR_ENTRY> &files, char *dirpath)
{
		dir_slash(dirpath);
		PMC_DIR *dfd = pmc_dopen(dirpath);
		if (!dfd || !dfd->isValid())
		{
			show_errorEx("Cannot open folder:\n %s", dirpath);
			delete dfd;
			return false;
		}
		
		files.clear();
		
		DIR_ENTRY tmp;
		int res = (*dfd)(tmp);
		
		// get past "."
		// root folder doesn't have "." and ".."
		if ( res>0 && tmp=="." )
			res = (*dfd)(tmp);
		
		//ignore ".."
		if ( res>0 && tmp==".." )
			res = (*dfd)(tmp);
		
		
		while ( res > 0 )
		{
			files.push_back(tmp);
			res = (*dfd)(tmp);
		}
		
		delete dfd;
		
		if (res<0)
		{
			if (files.empty())
			{
				show_errorEx("Error:\nAn error occured while reading directory:\n%s" \
											"\n\nIoDread returned: 0x%08x, %d", \
											dirpath, res, res);
				return false;
			}
			
			show_errorEx("Warning:\nSome error occured while reading directory:\n%s" \
										"\n\nIoDread returned: 0x%08x, %d", \
										dirpath, res, res);
		}
		
		if (!files.empty()) sort_list();
		
		return true;
}

static FORCE_INLINE
bool is_fullPath(const char *file)
{
	char *colon = strrchr(file, ':');
	return (colon!=NULL);
}

static FORCE_INLINE
bool isRoot(const char *dir)
{
	char *tmp = strrchr(dir, ':');
//	if ( !tmp ) return false;
	if ( strcmp(tmp, ":")==0 || strcmp(tmp, ":/")==0 )
		return true;
	return false;
}


static FORCE_INLINE
const char *get_basedir(const char *path)
{
	char *base = strrchr(path, '/');
	*base = '\0';
	return strrchr(path, '/')+1;
}



static NOINLINE
void select_item(std::vector<DIR_ENTRY> &files,
									PMC_LIST &list,
									const char *item,
									unsigned int &top,
									unsigned int & sel)
{
	for( unsigned int i=0; i<files.size(); ++i )
		if (files[i]==item)
		{
			list.select_item(i, top, sel);
			return;
		}
	
	top = sel  = 0;
}


#define push_folder(x) push_folder(files, (x))
#include "pspmscm.h"
static NOINLINE
void list_drives(std::vector<DIR_ENTRY> &files, PMC_LIST &file_list)
{
	files.clear();
	drives_shown = true;
	
	if ( state.isPSPgo )
	{
		DIR_ENTRY tmp("ef0:/", "<Internal Flash>"); //eflash0a0f0
		files.push_back(tmp);
	}
	
	if (MScmIsMediumInserted())
	{
		DIR_ENTRY tmp("ms0:/", "<Memory Stick>"); //msstor
		files.push_back(tmp);
	}
	
	if (sceUmdCheckMedium())
	{
		sceUmdWaitDriveStat(UMD_WAITFORDISC);
		
#ifndef DEBUG
		// seems psplink is already mounting the UMD
		// Mount UMD to disc0: file system
		if (!umd_open && sceUmdActivate(1, "disc0:")<0)
		{
			umd_open = true;
			return;
		}
#endif
		
		DIR_ENTRY tmp("disc0:/", "<UMD>");
		files.push_back(tmp);
	}
	
	/*
	char drive[] = "host0:/";
	if (check_drive(drive)) // just check one
	{
		for( char d='0'; d<'8'; ++d )
		{
			drive[4] = d;
			DIR_ENTRY tmp(drive);
			files.push_back(tmp);
		}
	}
	*/
}


static FORCE_INLINE
void up_oneDir(char *out)
{
	char *tmp = strrchr(out, '/');
	*tmp = '\0';
	
	// if slash is not the last char,
	// then we already have what we want
	if (tmp[1]!='\0') return;
	
	tmp = strrchr(out, '/');
	*tmp = '\0';
}


static NOINLINE
bool go_UpOneDir(std::vector<DIR_ENTRY> &files, \
			PMC_LIST &file_list, unsigned int &top, unsigned int &sel)
{
	if (drives_shown) return false;
	
	if (isRoot(cur_dir))
	{
		list_drives(files, file_list);
		file_list.fixup(files);
		dir_slash(cur_dir);
		select_item(files, file_list, cur_dir, top, sel);
		strcpy(cur_dir, "* DRIVES *");
		return true;
	}
	
	char tmp_dir[1024];
	strcpy(tmp_dir, cur_dir);
	up_oneDir(tmp_dir);
	
	if ( push_folder(tmp_dir) )
	{
		file_list.fixup(files);
		select_item(files, file_list, get_basedir(cur_dir), top, sel);
		strcpy(cur_dir, tmp_dir);
		dir_slash(cur_dir);
		return true;
	}
	
	push_folder(cur_dir);
	file_list.fixup(files);
	dir_slash(cur_dir);
	return false;
}


#define NUMOF_ITEMS			12
#define LIST_YPOS				95
#define LIST_FONTHEIGHT	(font->get_ySize()*0.6f)
#define ITEMS_DISTANCE	(LIST_FONTHEIGHT+2.f)
#define POS_CURSOR(x)		( (LIST_YPOS-LIST_FONTHEIGHT) + (ITEMS_DISTANCE*(x)) )

enum {
 CTX_NONE,
 CTX_OPEN,
 CTX_REFRESH
};

static int file_ctxmenu(const char *path, const char *file, const u16 *disp_name)
{
	int sel_item = 0;
	float dirX = 24;
	int cursor_bar = POS_CURSOR(sel_item);
			const bool file_isNeeded = settings.isNeeded(file,false);
	while (state.running)
	{
			if ( gu_start() )
			{
				draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
												RGB(32,32,32), RGB(32,32,32), \
												0, 0, 480, 272 );
				
				list_bkg.draw_strip(0,40);
				
				draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
												RGB(32,32,32), RGB(32,32,32), \
												20, 60, 440, 20);
				
				font->set_style(0.9f, COL_WHITE, 0, INTRAFONT_ALIGN_LEFT|INTRAFONT_SCROLL_LEFT);
				if (disp_name)
					dirX = font->print_ucs2(disp_name, dirX, 76, 430);
				else
					dirX = font->print(file, dirX, 76, 430);
				
				{
					#define BASE_INCREMENT 2
					const int cursor_dest = POS_CURSOR(sel_item);
					if (cursor_bar!=cursor_dest)
					{
						const int bar_inc = __builtin_abs(cursor_bar-cursor_dest)>10?BASE_INCREMENT*4:BASE_INCREMENT;
						if (cursor_bar<cursor_dest)
							cursor_bar = cursor_bar+bar_inc>cursor_dest?cursor_dest:cursor_bar+bar_inc;
						else
							cursor_bar = cursor_bar-bar_inc<cursor_dest?cursor_dest:cursor_bar-bar_inc;
					}
					
					player_icons[PL_ICON_BAR].scaleX = 414+20;
					player_icons[PL_ICON_BAR].draw(24, cursor_bar);
				}
				
				font->set_style(0.6f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_CENTER);
				font->print("Delete", 480/2, LIST_YPOS+ITEMS_DISTANCE);
				font->print("Refresh current directory", 480/2, LIST_YPOS+(4*ITEMS_DISTANCE));
				
			if (file_isNeeded)
				font->set_style(0.6f, COL_WHITE, COL_BLACK, INTRAFONT_ALIGN_CENTER);
			else
				font->set_style(0.6f, RGB(120,120,120), 0, INTRAFONT_ALIGN_CENTER);
				
				font->print("Play", 480/2, LIST_YPOS);
				font->print("Bookmarks", 480/2, LIST_YPOS+(2*ITEMS_DISTANCE));
				font->print("Add to playlist", 480/2, LIST_YPOS+(3*ITEMS_DISTANCE));
				
				show_topbar("File Browser");
				
				gu_end();
				gu_sync();
				
				flip_screen();
			}
			else wait_vblank();
			
			ctrl.read();
			if (ctrl.released.cancel||ctrl.pressed.square) return 0;
			else if (ctrl.pressed.up) sel_item = sel_item==0?4:sel_item-1;
			else if (ctrl.pressed.down) sel_item = sel_item==4?0:sel_item+1;
			else if (ctrl.pressed.ok)
			{
				switch (sel_item)
				{
					case 0: return file_isNeeded?CTX_OPEN:CTX_NONE;
					case 2:
					case 3: if (file_isNeeded) show_notdone(); return CTX_NONE;
					case 4: return CTX_REFRESH;
					case 1: if (strncasecmp(path, "disc0:", 6)) show_notdone(); return CTX_NONE;
				}
			}
	}
	return 0;
}


void show_filelist(bool show_playing)
{
	if (show_playing && player.playing==PLAYER_STOPPED)
		return;
	
	settings.cpu = settings.clocks[FBROWSER_CLOCK] + (player.playing ? settings.cpu(player.filename) : 0);
	
	cur_submenu = FILELIST;
	
	if (cur_dir[0]=='\0' || drives_shown)
	{
		if (state.isPSPgo)
			strcpy(cur_dir, "ef0:/MUSIC/");
		else
			strcpy(cur_dir, "ms0:/MUSIC/");
		
		drives_shown = false;
	}
	
	PMC_LIST file_list(RGB(32,32,32), COL_WHITE,
											COL_WHITE, COL_BLACK,
							NUMOF_ITEMS, LIST_YPOS, ITEMS_DISTANCE,
											48.f, 0.6f, 404.0f);
	
	std::vector<DIR_ENTRY> files;
	
	unsigned int top_item = 0;
	unsigned int sel_item = 0;
	
	
	ctrl.autorepeat = true;
	ctrl.autorepeat_delay = 10;
	ctrl.autorepeat_interval = 5;
	ctrl.autorepeat_mask = (CTRL_RIGHT|CTRL_LEFT|CTRL_UP|CTRL_DOWN);
	
	float dirX = 24;
	int player_ret = 0;
	
	if (show_playing)
	{
		strcpy(cur_dir, player.filepath);
		dir_slash(cur_dir);
	}
	push_folder(cur_dir);
	file_list.fixup(files);
	
	if (show_playing)
	{
		select_item(files, file_list, player.filename, top_item, sel_item);
		player_ret = show_nowplaying(NULL, NULL);
	}
	
	int cursor_bar = POS_CURSOR(sel_item);
	while(state.running)
	{
		if (player_ret==0)
		{
			if ( gu_start() )
			{
				draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
												RGB(32,32,32), RGB(32,32,32), \
												0, 0, 480, 272 );
				
				list_bkg.draw_strip(0,40);
				
				if (!files.empty())
				{
					#define BASE_INCREMENT 2
					const int cursor_dest = POS_CURSOR(sel_item);
					if (cursor_bar!=cursor_dest)
					{
						const int bar_inc = __builtin_abs(cursor_bar-cursor_dest)>10?BASE_INCREMENT*4:BASE_INCREMENT;
						if (cursor_bar<cursor_dest)
							cursor_bar = cursor_bar+bar_inc>cursor_dest?cursor_dest:cursor_bar+bar_inc;
						else
							cursor_bar = cursor_bar-bar_inc<cursor_dest?cursor_dest:cursor_bar-bar_inc;
					}
					
					player_icons[PL_ICON_BAR].scaleX = 414;
					player_icons[PL_ICON_BAR].draw(44, cursor_bar);

					for(unsigned i=0; i<NUMOF_ITEMS && (i+top_item)<files.size(); ++i)
						if (files[top_item+i].isDir())
							file_ico[0].draw(28, POS_CURSOR(i) );
				}
				
				draw_colorRect( RGB(0x36,0x36,0x36),RGB(0x36,0x36,0x36), \
												RGB(32,32,32), RGB(32,32,32), \
												20, 60, 440, 20);
				
				font->set_style(0.9f, COL_WHITE, 0, INTRAFONT_ALIGN_LEFT|INTRAFONT_SCROLL_LEFT);
				dirX = font->print(cur_dir, dirX, 76, 430);
				
				file_list.print_ucs2<DIR_ENTRY>(top_item, sel_item, files);
				
				if (!files.empty() && file_list.show_scroll())
				{
					float scroll_size = 170*file_list.scroll_size(files.size());
					scroll_size = scroll_size < 5 ? 5 : scroll_size;
					draw_colorRect( RGB(0x36,0x36,0x36),RGB(32,32,32), \
												RGB(32,32,32), RGB(0x36,0x36,0x36), \
												20, 85+((170-scroll_size)*file_list.scroll_pos(top_item)), \
												7, scroll_size );
				}
				
				show_topbar("File Browser");
				
				gu_end();
				gu_sync();
				
				flip_screen();
				ctrl.read();
			}
			else ctrl.flush(true);
			
			if (ctrl.released.cancel) break;
			else if (ctrl.pressed.triangle)
			{
					go_UpOneDir(files, file_list, top_item, sel_item);
					cursor_bar = POS_CURSOR(sel_item);
			}
			else if (ctrl.pressed.select && player.playing!=PLAYER_STOPPED)
			{
					if (strcasecmp(cur_dir, player.filepath))
					{
						strcpy(cur_dir, player.filepath);
						dir_slash(cur_dir);
						
						push_folder(cur_dir);
					}
					file_list.fixup(files);
					select_item(files, file_list, player.filename, top_item, sel_item);
					player_ret = show_nowplaying(NULL, NULL);
					cursor_bar = POS_CURSOR(sel_item);
					drives_shown = false;
					continue;
			}
			else if (ctrl.pressed.up)		file_list.up(top_item, sel_item);
			else if (ctrl.pressed.down)	file_list.down(top_item, sel_item);
			else if (ctrl.pressed.left)	file_list.page_up(top_item, sel_item);
			else if (ctrl.pressed.right)	file_list.page_down(top_item, sel_item);
			else if (ctrl.pressed.value&(CTRL_OK|CTRL_SQUARE)) goto open_item;
		}
		else
		{
open_item:
				if (files.empty()) continue;
				
				unsigned int cur_item = top_item+sel_item;
				
				// temp workaround for some bug, see TODO (fixed)
				// but keep this just to be sure
				if (cur_item>=files.size())
					cur_item = files.empty() ? 0 : files.size()-1;
				
				if (ctrl.pressed.square)
				{
					int ctx_res = 0;
					if (files[cur_item].isReg())
					{
						DIR_ENTRY& cur_ent = files[cur_item];
						ctx_res = file_ctxmenu(cur_dir, cur_ent(), cur_ent.get_lfn());
					}
					
					switch (ctx_res)
					{
						case CTX_OPEN: break;
						case CTX_REFRESH:
							dir_slash(cur_dir);
							push_folder(cur_dir);
							file_list.fixup(files);
							cursor_bar = POS_CURSOR(sel_item);
						default: continue;
					}
				}
				
				if (player_ret)
				{/*
					const int up_cond = player_ret < 0 ? 0 : (files.empty() ? 0 : (files.size()-1));
					static char err_dir[1024];
					strcpy(err_dir, cur_dir);
					static unsigned last_sel = cur_item;
					
					while (cur_item==up_cond && \
						(	(strncasecmp(cur_dir, "ms0:/MUSIC", 10)) && \
							(strncasecmp(cur_dir, "ef0:/MUSIC", 10)) ))
						if (!go_UpOneDir(files, file_list, top_item, sel_item))
						{
							push_folder(err_dir);
							file_list.fixup(files);
							file_list.select_item(last_sel, top_item, sel_item);
							strcpy(cur_dir, err_dir);
							dir_slash(cur_dir);
							player_ret=0;
							continue;
						}
				*/
					if (player_ret<0 && cur_item!=0)
						cur_item -= 1;
					else if (player_ret>0 && cur_item!=(files.size()-1))
						cur_item += 1;
					else if (player.mode&PL_MODE_LOOP_ALL)
					{
						if (player_ret<0) cur_item = (files.size()-1);
						else cur_item = 0;
					}
					else
					{
						player_ret = 0;
						continue;
					}
					
					file_list.select_item(cur_item, top_item, sel_item);
				}
				
				if ( files[cur_item].isDir() )
				{
					// TODO: temporary
					if (player_ret!=0) continue;
					
					char tmp_dir[1024];
					if (!drives_shown)
					{
						strcpy(tmp_dir, cur_dir);
						strcat(tmp_dir, files[cur_item]());
					}
					else strcpy(tmp_dir, files[cur_item]());
					
					if ( push_folder(tmp_dir) )
					{
						strcpy(cur_dir, tmp_dir);
						dir_slash(cur_dir);
						file_list.fixup(files);
						drives_shown = false;
					}
					else if (drives_shown)
						list_drives(files, file_list);
					else
					{
						push_folder(cur_dir);
						file_list.fixup(files);
					}
					
					top_item = sel_item = 0;
					cursor_bar = POS_CURSOR(sel_item);
				}
				else if ( files[cur_item].isReg() )
				{
					const char *cur_file = files[cur_item]();
					if (!settings.isNeeded(cur_file,false)) continue;
					player_ret = show_nowplaying(cur_dir, cur_file/*, files[cur_item].get_lfn()*/);
					cursor_bar = POS_CURSOR(sel_item);
				}
		}
	}
}


