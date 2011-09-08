#ifndef PMC_MAIN_H
#define PMC_MAIN_H

#define TEMPLATE template<typename T>
#define PACKED __attribute__((packed))

#include <psptypes.h>
#include <pspkernel.h>
#include "utils.h"

enum menu_type
{
	FILELIST=0x0,
	NOWPLAYING,
	PLAYLIST,
	SETTINGS,
	ABOUT,
	HOMEMENU,
	NUMOF_MENUS
};
extern menu_type cur_submenu;

#endif  //PMC_MAIN_H
