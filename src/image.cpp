#include "main.h"
#include <cstdlib>
#include <cstdio>

#include "image.h"
#include "drawing.h"
#include "utils.h"

extern "C" {
	#include <malloc.h>
}

Pmc_Image *bkg = NULL;

Pmc_Image::~Pmc_Image(){
	/*if (!((u32)data & VRAM_BASE))*/ free(data);
//	delete palette;
};

//#include "asm_utils.h"
Pmc_Image::Pmc_Image(int sizeX, int sizeY, u32 color_space, bool alloc)
:	swizzled(0),
	type(color_space),
	x(0),
	y(0),
	scaleX(sizeX),
	scaleY(sizeY),
	width(sizeX),
	height(sizeY)
{
//	memset(this, 0, sizeof(Pmc_Image));
	if (sizeX<=0 || sizeY<=0)
	{
	//	memset(this, 0, sizeof(Pmc_Image));
		bufwidth = bufheight = bufsize = 0;
		data = NULL;
		return;
	}
	
	bufwidth = ALIGN_SIZE(sizeX, 16);
	bufheight = ALIGN_SIZE(sizeY, 16);
	
//	printf("buf width: %d, height: %d\n", bufwidth, bufheight);

	bufsize = get_imageSize(bufwidth * bufheight, color_space);
	
	if (alloc)
	{
		data = (u8*)memalign(16, bufsize);
		if (data==NULL)
		{
			printf("image alloc failed\n");
			bufsize = 0;
		}
	}
}

void Pmc_Image::swizzle()
{
	if (swizzled || !isValid()) return;
	u8 *data2 = (u8*)memalign(16, bufsize);
	if (!data2) return;

//	u32 blockx, blocky;
//	u32 j;

	u32 width1 = get_imageSize(bufwidth, type);
	u32 height1 = bufheight;

	u32 width_blocks = (width1 / 16);
	u32 height_blocks = (height1 / 8);

	u32 src_pitch = (width1-16)/4;
	u32 src_row = width1 * 8;

	const u8* ysrc = (u8*)(data);
	u32* dst = (u32*)(data2);

	for (u32 blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xsrc = ysrc;
		for (u32 blockx = 0; blockx < width_blocks; ++blockx)
		{
			const u32* src = (u32*)xsrc;
			for (u32 j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
	free(data);
	data = data2;
	swizzled = true;
}
/*
void Pmc_Image::unswizzle(){
	if (!swizzled || !isValid()) return;
	
	u8 *data2 = (u8*)memalign(16, bufsize);
	if (!data2) return;

//	u32 blockx, blocky;
//	u32 j;

	u32 width1 = get_imageSize(bufwidth, type);
	u32 height1 = bufheight;

	u32 width_blocks = (width1 / 16);
	u32 height_blocks = (height1 / 8);

	u32 dst_pitch = (width1-16)/4;
	u32 dst_row = width1 * 8;

	u32* src = (u32*)(data);
	u8* ydst = (u8*)(data2);

	for (u32 blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xdst = ydst;
		for (u32 blockx = 0; blockx < width_blocks; ++blockx)
		{
			u32* dst= (u32*)xdst;
			for (u32 j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				dst += dst_pitch;
			}
			xdst += 16;
		}
		ydst += dst_row;
	}

	free(data);
	data = data2;
	swizzled = false;
}
*/
