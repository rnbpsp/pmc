#ifndef PMC_NOWPLAYING_H
#define PMC_NOWPLAYING_H

#define PL_ICON_STAT_BASE 0
#define PL_ICON_BAR_BASE 1
#define PL_ICON_BAR 2
#define PL_ICON_OVERLAY 3

#define PL_ICON_PLAY 4
#define PL_ICON_PAUSE 5
#define PL_ICON_REW 6
#define PL_ICON_FWD 7
extern Pmc_ImageTile player_icons[8];
int show_nowplaying(const char *path, const char *name);

#endif //PMC_NOWPLAYING_H
