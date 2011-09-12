#include "main.h"
#include <cstdlib>

#include <pspdisplay.h>
#include "image.h"

#include <pspgu.h>
#include "utils.h"
#include "font.h"
#include "color.h"
#include "drawing.h"

// memalign
extern "C"
{
	#include <malloc.h>
}

VRAM_MNGR vram;
bool intrafont_used = false;
PMC_FONT *font = NULL;

void *list = NULL;
///*static */unsigned int __attribute__((aligned(16))) list[262144];
static void *cur_texture = NULL;

// vertices must be 32bits aligned
#pragma pack(4)
typedef struct
{
	unsigned short u, v;
	short x, y, z;
}image_vertex;

typedef struct
{
	u32 color;
	short x, y, z;
}shape_vertex;

typedef struct
{
	unsigned short u, v;
	u32 color;
	short x, y, z;
}imageColored_vertex;

typedef struct
{
	unsigned short u, v;
	float x,y,z;
}imgPrecise_vertex;

typedef struct
{
	float u,v, x,y,z;
}image_vertexf;

#pragma pack()

TEMPLATE
T *gu_allocVert(int num)
{
	return (T*)( (u32)sceGuGetMemory(sizeof(T)*num) | UNCACHED_ADDRESS_SPACE );
}

// cached version
TEMPLATE
T *gu_allocVertC(int num)
{
	return (T*)( (u32)sceGuGetMemory(sizeof(T)*num) & ~UNCACHED_ADDRESS_SPACE );
}

void Pmc_Image::draw(short X, short Y) {
	if (!isValid()) return;
	
	if (cur_texture != data || intrafont_used)
	{
		cur_texture = data;
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuTexMode(type, 0, 0, swizzled);
		sceGuTexImage(0, get_nextpow2(bufwidth<512?bufwidth:512),
										get_nextpow2(bufheight<512?bufheight:512),
											bufwidth, data);
		intrafont_used = false;
	}
	
	image_vertex *vertex = gu_allocVert<image_vertex>(2);
	
	vertex[0].u = 0;
	vertex[0].v = 0;
	vertex[0].x = X;
	vertex[0].y = Y;
	vertex[0].z = 0;

	vertex[1].u = width<512?width:512;
	vertex[1].v = height<512?height:512;
	vertex[1].x = X + scaleX;
	vertex[1].y = Y + scaleY;
	vertex[1].z = 0;

	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2, 0, vertex);
}

void Pmc_Image::draw_strip(short X, short Y) {
	if (!isValid()) return;
	
	if (cur_texture != data || intrafont_used)
	{
		cur_texture = data;
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuTexMode(type, 0, 0, swizzled);
		sceGuTexImage(0, get_nextpow2(bufwidth<512?bufwidth:512),
										get_nextpow2(bufheight<512?bufheight:512),
											bufwidth, data);
		intrafont_used = false;
	}
	
	int vert_num = (ALIGN_SIZE(width,64)>>6/*/64*/)*2;
	image_vertex *vertex = gu_allocVertC<image_vertex>(vert_num);
	
	for(int i=0, k=0; i<width; k++)
	{
		vertex[k].u = i;
		vertex[k].v = 0;
		vertex[k].x = X+i;
		vertex[k].y = Y;
		vertex[k].z = 0;
		k++;
		
		vertex[k].u = i+=64;
		vertex[k].v = height;
		vertex[k].x = X+i;
		vertex[k].y = Y+height;
		vertex[k].z = 0;
	}
	
	if (width&63) //width%64
	{
		int last_index = vert_num-1;
		vertex[last_index].u = width;
		vertex[last_index].x = X+width;
	}
	
	pmc_wb(vertex, vert_num*sizeof(image_vertex));
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D, vert_num, 0, vertex);
}

void Pmc_Image::draw_stripScl(short X, short Y)
{
	if (!isValid()) return;
	
	if (cur_texture != data || intrafont_used)
	{
		cur_texture = data;
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuTexMode(type, 0, 0, swizzled);
		sceGuTexImage(0, get_nextpow2(bufwidth<512?bufwidth:512),
										get_nextpow2(bufheight<512?bufheight:512),
											bufwidth, data);
		intrafont_used = false;
	}
	
	int vert_num = (ALIGN_SIZE(width,64)>>6/*/64*/)*2;
	imgPrecise_vertex *vertex = gu_allocVertC<imgPrecise_vertex>(vert_num);
	
	const float strip = 64.f*(scaleX/(float)width);
	const float h_scaled = height*(scaleY/(float)height);
	float xCoord = X;
	for(int i=0, k=0; i<width; k++)
	{
		vertex[k].u = i;
		vertex[k].v = 0;
		vertex[k].x = xCoord;
		vertex[k].y = Y;
		vertex[k].z = 0;
		k++;
		
		vertex[k].u = i += 64;
		vertex[k].v = height;
		vertex[k].x = xCoord += strip;
		vertex[k].y = Y+h_scaled;
		vertex[k].z = 0;
	}
	
	if (width&63) //width%64
	{
		const int last_index = vert_num-1;
		vertex[last_index].u = width;
		vertex[last_index].x = X+scaleX;
	}
	
	pmc_wb(vertex, vert_num*sizeof(imgPrecise_vertex));
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_32BITF|GU_TRANSFORM_2D, vert_num, 0, vertex);
}

void drawImgStrip_large(Pmc_Image *img, int x, int y)
{
	if (!img->isValid()) return;
	
	if (img->width<=512 && img->height<=512)
	{
		img->draw_stripScl(x,y);
		return;
	}
	
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	sceGuTexMode(img->type, 0, 0, img->swizzled);
	
	const float scaleX = img->scaleX/(float)img->width;
	const float scaleY = img->scaleY/(float)img->height;
	const float strip = 64.f*scaleX;
	
	intrafont_used = true;
	for(int h=0; h<img->height; h+=512)
	{
		for(int w=0; w<img->width; w+=512)
		{
			{
			const int bufheight = get_nextpow2(pmc_min<int>(img->bufheight-h,512));
			const int bufwidth = get_nextpow2(pmc_min<int>(img->bufwidth-w,512));
			sceGuTexImage(0, bufwidth, bufheight, img->bufwidth, ((u32*)img->data) + (h*img->bufwidth)+(img->swizzled?w*8:w));
			}
			
	const int width = pmc_min<int>(img->width-w,512);
	const int height = pmc_min<int>(img->height-h,512);
	const float dst_height = (float)height*scaleY;
	
				int vert_num = (ALIGN_SIZE(width,64)>>6)*2;
				imgPrecise_vertex *vertex = gu_allocVertC<imgPrecise_vertex>(vert_num);
				
				const float Y = y + ((float)h*scaleY);
				float X = x + ((float)w*scaleX);
				for(int i=w, k=0; i<w+width; k++)
				{
					vertex[k].u = i;
					vertex[k].v = h;
					vertex[k].x = X;
					vertex[k].y = Y;
					vertex[k].z = 0;
					k++;
					
					vertex[k].u = i += 64;
					vertex[k].v = height+h;
					vertex[k].x = X += strip;
					vertex[k].y = Y+dst_height;
					vertex[k].z = 0;
				}
				
				if (width&63) //width%64
				{
					const int last_index = vert_num-1;
					vertex[last_index].u = w+width;
					vertex[last_index].x = x + (w*scaleX) + (width*scaleX);
				}
				
				pmc_wb(vertex, vert_num*sizeof(imgPrecise_vertex));
				sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_32BITF|GU_TRANSFORM_2D, vert_num, 0, vertex);
		}
	}
}

void Pmc_ImageTile::draw(short X, short Y) {

	if (!base->isValid()) return;
	
	image_vertex *vertex = gu_allocVert<image_vertex>(2);//(image_vertex*)sceGuGetMemory(sizeof(image_vertex)*2);

	if (cur_texture != base->data || intrafont_used) {
		cur_texture = base->data;
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuTexMode(base->type, 0, 0, base->swizzled);
		sceGuTexImage(0, get_nextpow2(base->bufwidth<512?base->bufwidth:512),
										get_nextpow2(base->bufheight<512?base->bufheight:512),
											base->bufwidth, base->data);
		intrafont_used = false;
	}
	
	vertex[0].u = offX0;
	vertex[0].v = offY0;
	vertex[0].x = X;
	vertex[0].y = Y;
	vertex[0].z = 0;

	vertex[1].u = offX1;
	vertex[1].v = offY1;
	vertex[1].x = X + scaleX;
	vertex[1].y = Y + scaleY;
	vertex[1].z = 0;

	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2, 0, vertex);
}

void Pmc_ImageTile::draw_strip(short X, short Y) {
	if (!base->isValid()) return;
	
	if (cur_texture != base->data || intrafont_used) {
		cur_texture = base->data;
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuTexMode(base->type, 0, 0, base->swizzled);
		sceGuTexImage(0, get_nextpow2(base->bufwidth<512?base->bufwidth:512),
										get_nextpow2(base->bufheight<512?base->bufheight:512),
											base->bufwidth, base->data);
		intrafont_used = false;
	}
	
	int vert_num = (ALIGN_SIZE(width,64)>>6/*/64*/)*2;
	image_vertex *vertex = gu_allocVertC<image_vertex>(vert_num);
	
	for(int i=offX0, k=0; k<vert_num/*i<width*/; k++)
	{
		vertex[k].u = i;
		vertex[k].v = offY0;
		vertex[k].x = X+i;
		vertex[k].y = Y;
		vertex[k].z = 0;
		k++;
		
		vertex[k].u = i+=64;
		vertex[k].v = offY1;
		vertex[k].x = X+i;
		vertex[k].y = Y+height;
		vertex[k].z = 0;
	}
	
	if (width&63) //width%64
	{
		const int last_index = vert_num-1;
		vertex[last_index].u = offX1;
		vertex[last_index].x = X+width;
	}
	
	pmc_wb(vertex, vert_num*sizeof(image_vertex));
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D, vert_num, 0, vertex);
}

///////////////////////////////////////////////////////////////
//	Shapes
///////////////////////////////////////////////////////////////

void draw_fillRect(u32 color, int x, int y, int sizeX, int sizeY)
{
	shape_vertex *vertex = gu_allocVert<shape_vertex>(2);//(shape_vertex*)sceGuGetMemory(sizeof(shape_vertex)*2);

	sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
	sceGuEnable(GU_BLEND);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuShadeModel(GU_FLAT);
	
	vertex[0].color = color;
	vertex[0].x = x;
	vertex[0].y = y;
	vertex[0].z = 0;

	vertex[1].color = color;
	vertex[1].x = x + sizeX;
	vertex[1].y = y + sizeY;
	vertex[1].z = 0;

	sceGuDrawArray(GU_SPRITES, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2, 0, vertex);
	
	sceGuEnable(GU_TEXTURE_2D);
}

void draw_rect(u32 color, int x, int y, int sizeX, int sizeY) {

	// make it 5 to close the polygon
	shape_vertex *vertex = gu_allocVert<shape_vertex>(5);

	sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
	sceGuEnable(GU_BLEND);
	sceGuFrontFace(GU_CW);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuShadeModel(GU_FLAT);

	vertex[0].color = color;
	vertex[0].x = x;
	vertex[0].y = y;
	vertex[0].z = 0;

	vertex[1].color = color;
	vertex[1].x = x + sizeX;
	vertex[1].y = y;
	vertex[1].z = 0;

	vertex[2].color = color;
	vertex[2].x = x + sizeX;
	vertex[2].y = y + sizeY;
	vertex[2].z = 0;

	vertex[3].color = color;
	vertex[3].x = x;
	vertex[3].y = y + sizeY;
	vertex[3].z = 0;
	
	vertex[4].color = color;
	vertex[4].x = x;
	vertex[4].y = y-1;
	vertex[4].z = 0;
	
	sceGuDrawArray(GU_LINE_STRIP, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 5, 0, vertex);
	
	sceGuEnable(GU_TEXTURE_2D);
}

void draw_colorRect(u32 col0, u32 col1, u32 col2, u32 col3, int x, int y, int sizeX, int sizeY)
{
	shape_vertex *vertex = gu_allocVert<shape_vertex>(4);//(shape_vertex*)sceGuGetMemory(sizeof(shape_vertex)*5);

	sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
	sceGuEnable(GU_BLEND);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuDisable(GU_TEXTURE_2D);
	
	vertex[0].color = col0;
	vertex[0].x = x;
	vertex[0].y = y;
	vertex[0].z = 0;

	vertex[1].color = col1;
	vertex[1].x = x + sizeX;
	vertex[1].y = y;
	vertex[1].z = 0;

	vertex[2].color = col2;
	vertex[2].x = x + sizeX;
	vertex[2].y = y + sizeY;
	vertex[2].z = 0;

	vertex[3].color = col3;
	vertex[3].x = x;
	vertex[3].y = y + sizeY;
	vertex[3].z = 0;
	
	sceGuDrawArray(GU_TRIANGLE_FAN, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 4, 0, vertex);
	
	sceGuShadeModel(GU_FLAT);
	sceGuEnable(GU_TEXTURE_2D);
}

void draw_line(u32 col0, u32 col1, int x0, int y0, int x1, int y1)
{
	shape_vertex *vertex = gu_allocVert<shape_vertex>(5);//(shape_vertex*)sceGuGetMemory(sizeof(shape_vertex)*5);

	sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
	sceGuEnable(GU_BLEND);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuDisable(GU_TEXTURE_2D);
	
	vertex[0].color = col0;
	vertex[0].x = x0;
	vertex[0].y = y0;
	vertex[0].z = 0;

	vertex[1].color = col1;
	vertex[1].x = x1;
	vertex[1].y = y1;
	vertex[1].z = 0;

	sceGuDrawArray(GU_LINES, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2, 0, vertex);
	
	sceGuShadeModel(GU_FLAT);
	sceGuEnable(GU_TEXTURE_2D);
}

void gu_init(/*u32 list_size*/)
{
	sceGuInit();

	list = memalign(16, 262144*4/*512*1024*4*/ );
	if (!list) show_error("Cannot allocate display list.", true);

	sceGuStart(GU_DIRECT,(void*)list);
	sceGuDrawBuffer(GU_PSM_8888,(void*)fbp0,BUF_WIDTH); vram.drawfbp = (void*)fbp0;
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,(void*)fbp1,BUF_WIDTH);
//	sceGuDepthBuffer((void*)zbp,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
  sceGuViewport(SCR_WIDTH/2, SCR_HEIGHT/2, SCR_WIDTH, SCR_HEIGHT);
//	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CW);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuAmbientColor(0xffffffff);
	sceGuEnable(GU_BLEND);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT/*|GU_DEPTH_BUFFER_BIT*/);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask(0);
	sceGuFinish();
	sceGuSync(0,0);

	sceGuDisplay(1);
	sceKernelDcacheWritebackInvalidateAll();
}
