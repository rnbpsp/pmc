#ifndef __PMC_IMAGE_H__
#define __PMC_IMAGE_H__
/*
int create_image(Pmc_Image *image, u32 width, u32 height);
void free_image(Pmc_Image *img);
int swizzle_image(Pmc_Image *image);
*/

#include <pspgu.h>
#include "utils.h"
#include "color.h"
/*
#include <climits>
// wikipedia
template <typename T>
T get_nextpow2(T k)
{
	if (k == 0)
		return 1;
	k--;
	for (u32 i=1; i<sizeof(T)*CHAR_BIT; i<<=1)
		k = k | k >> i;
	return k+1;
}
*/
//http://www.koders.com/c/fid48BA31C1DD231039FB20F72E6FCB17339E5C565A.aspx?s=%22Bob%22
inline
u32 get_nextpow2(u32 x)
{
	x = __builtin_clz(x-1);
	x = 32 - x;
	return 1 << x;
}

// returns the size in number of bytes(or bits if bits == true)
inline
u32 get_imageSize(u32 size, u32 type, bool bits=false){
	switch (type)
	{
		case GU_PSM_8888:
			size <<= 2;
			break;
			
		case GU_PSM_5551:
		case GU_PSM_5650:
		case GU_PSM_4444:
		case GU_PSM_T16:
			size <<= 1;
			break;
			
		case GU_PSM_T8:
			break;
	}
	if (bits)	size<<=3;
	
	return size;
}

class Pmc_Image
{
public:
	bool swizzled;
	u32 type;
	int x, y;									// coords
	int scaleX, scaleY;				// scale
	int width, height;				// image width and height
	int bufwidth, bufheight;	// buffer width and height
	size_t bufsize;

	u8 *data;
	
	Pmc_Image(int sizeX, int sizeY, u32 color_space, bool alloc=true);
	~Pmc_Image();

	bool isValid(){ return (this->data!=NULL && this->bufsize!=0); };
	
	// never forget to writeback cache after un/swizzling
	void swizzle();
	void invalidate() { pmc_wbinvalidate(data, bufsize); };
	
	void draw(){ draw(x,y); };
	void draw(short X, short Y);
	
	// doesn't support scaling yet
	void draw_strip(){ draw(x,y); };
	void draw_strip(short X, short Y);
	void draw_stripScl(short X, short Y);
};

class Pmc_ImageTile {
	Pmc_Image *base;
public:
	int x, y;							// coords
	int scaleX, scaleY;		// scale
	u32 offX0, offY0;			// offsets from the base image
	u32 offX1, offY1;
	u32 width, height;		// dimensions of this tile
	
	Pmc_ImageTile()
/*	: base(NULL),
		x(0), y(0),
		scaleX(0),
		scaleY(0),
		offX0(0),
		offY0(0),
		offX1(0),
		offY1(0),
		width(0),
		height(0)*/
	{memset(this, 0, sizeof(this));};
	
	Pmc_ImageTile(Pmc_Image *img, u32 ofX0, u32 ofY0, u32 ofX1, u32 ofY1)
	: base(img),
		x(0), y(0),
		offX0(ofX0),
		offY0(ofY0),
		offX1(ofX1),
		offY1(ofY1)
	{
		width = scaleX = __builtin_abs(ofX0 - ofX1);
		height = scaleY = __builtin_abs(ofY0 - ofY1);
	};
	
	FORCE_INLINE
	void set(Pmc_Image *img, u32 ofX0, u32 ofY0, u32 ofX1, u32 ofY1)
	{
		x = y = 0;
		base = img;
		offX0 = ofX0;
		offY0 = ofY0;
		offX1 = ofX1;
		offY1 = ofY1;
		width = scaleX = __builtin_abs(ofX0 - ofX1);
		height = scaleY = __builtin_abs(ofY0 - ofY1);
	};
	
	void draw(short X, short Y);
	void draw(){ draw(x,y); };
	
	// doesn't support scaling yet
	void draw_strip(){ draw(x,y); };
	void draw_strip(short X, short Y);
};

Pmc_Image *load_tga(const char filename[], bool swizzle=true);
//extern Pmc_Image *bkg;

#endif // __PMC_IMAGE_H__
