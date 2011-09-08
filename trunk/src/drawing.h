#ifndef __PMC_GU_DRAW_H__
#define __PMC_GU_DRAW_H__

#include <cstdlib>
#include <cstring>

#include <psputils.h>
#include <pspge.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include "image.h"

#define BUF_WIDTH 512
#define SCR_WIDTH 480
#define SCR_HEIGHT 272

	const u32 VRAM_BASE = 0x4000000;
	const u32 fbp0 = 0;
	const u32 fbp1 = (512*272*4);
	const u32 zbp = (fbp1+(512*272*4));
	const u32 ALBUM_ART_VMEM = zbp;
	const u32 PMC_VRAM_START = zbp+(128*128*4);//(zbp+(512*272*2));
	
class VRAM_MNGR {
public:
	u32 size;
	void *drawfbp;
	
	void set_drawbuf(void *fbp, int width=512){
		sceGuDrawBufferList(GU_PSM_8888, fbp, width);
	};
	
	void restore_drawbuf(){
		sceGuDrawBufferList(GU_PSM_8888, drawfbp, 512);
	};
	
	void *get_dispbuf(){
		return (drawfbp==(void*)fbp0) ? (void*)fbp1 : (void*)fbp0;
	};
	
	TEMPLATE
	T *gu_ptr(u32 ptr){
		return (T*)( ptr & ~VRAM_BASE );
	};
	
	TEMPLATE
	T *cpu_ptr(u32 ptr){
		return (T*)( ptr | VRAM_BASE );
	};
};
extern VRAM_MNGR vram;

void gu_init(/*u32 list_size = 262144*/);

inline void term_gu(void){ sceGuTerm(); }
inline void gu_end(){ sceGuFinish(); }
inline void gu_sync()
{
	// before stalling the cpu,
	// give room for other threads todo their jobs
	sceKernelDelayThread(0);
	sceGuSync(0,0);
}

#include "callbacks.h"
FORCE_INLINE
bool gu_start(bool force=false)
{
	if (state.hold_mode && !force)
	{
	//	printf("not drawing\n");
		return false;
	}
	
	extern unsigned int *list;
	sceGuStart(GU_DIRECT,(void*)list);
	
	return true;
}

FORCE_INLINE void wait_vblank(){ sceDisplayWaitVblankStart(); }

#include "utils.h"
FORCE_INLINE
void flip_screen(bool waitVblank=true, bool showfps=true)
{
#if _SHOWFPS
	if (showfps) show_fps();
#endif
	if (waitVblank) wait_vblank();
	vram.drawfbp = sceGuSwapBuffers();
}

void draw_fillRect(u32 color, int x, int y, int sizeX, int sizeY);
void draw_rect(u32 color, int x, int y, int sizeX, int sizeY);

void draw_line(u32 col0, u32 col1,
			int x0, int y0, int x1, int y1);

#include "color.h"
FORCE_INLINE
void clear_screen( u32 color=0 )
{
	draw_fillRect(color|COL_BLACK, 0,0,480,272);
}

void drawImgStrip_large(Pmc_Image *img, int x, int y);

// clockwise from the top left
void draw_colorRect(u32 col0, u32 col1,
										u32 col2, u32 col3,
						int x, int y, int sizeX, int sizeY);

#endif //__PMC_GU_DRAW_H__
