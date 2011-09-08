/*
 *	TGA loader written by RNB_PSP (rnbpsp at gmail.com)
 *		specifications were from wotsit.org
 */

#include <cstdio>
#include <pspiofilemgr.h>
#include <cstdlib>

#include "utils.h"
#include "image.h"
#include "drawing.h"

#include <string>

typedef enum {
	NO_IMAGE 					= 0,
	
	UNCOMPRESSED_PAL	= 1,
	UNCOMPRESSED_RGB	= 2, // can be handled
	UNCOMPRESSED_BNW	= 3,
	
	RLE_PAL						= 9,
	RLE_RGB						= 10,
	
	COMPRESSED_BNW		= 11,
	COMPRESSED_PAL		= 32,
	COMPRESSED_PAL4		= 33
}targa_type;

#pragma pack(1)
typedef struct
{
	u8		id_len;
	u8		isPaletted;
	union {
		struct {
			bool isPAL:1;
			bool isRGB:1;
				bool pad1:1;
			bool isRLE:1;
				bool pad2:1;
			bool isCompressed:1;
		};
		u8		type;
	};

	s16		pal_off;
	s16		pal_num;
	u8		pal_bpp; //bits

	s16		x_off;
	s16		y_off;
	
	u16		width;
	u16		height;
	
	u8		bpp; // bits
	union {
		struct {
			int attr_bits:4;
				int pad3:1;
			int screen_origin:1;
			int interleaving:2;
		};
		u8		desc;
	};
} PACKED TARGAHEAD;
#pragma pack()

static inline
Pmc_Image *alloc_tgaSpace(SceUID file, TARGAHEAD *head)
{
	Pmc_Image *img = NULL;
		sceIoLseek32(file, 18 + head->id_len + ( head->pal_num * (head->pal_bpp>>3) ), SEEK_SET);
		switch(head->bpp){
			case 16:
				img = new Pmc_Image(head->width, head->height, GU_PSM_5551);
				break;
			case 24:
			case 32:
				img = new Pmc_Image(head->width, head->height, GU_PSM_8888);
				break;
			default:
				return NULL;
		}
	if (img->isValid())
		return img;
	
	delete img;
	return NULL;
}

static inline
void tga_readPixelsRAW(SceUID fd, Pmc_Image *img, TARGAHEAD *head)
{
	u32 line_size = head->width*head->bpp>>3;
	u8 *in = new u8[line_size];
	
	u16 *data16 = (u16*)img->data;
	u32 *data32 = (u32*)img->data;
	
		C_ARGB5 *in16 = (C_ARGB5*)in;
	
	switch(head->bpp){
		case 16:
			for ( u32 i=0; i<head->height; ++i){
				sceIoRead(fd, in, line_size);
				for ( u32 j=0; j<head->width; ++j )
					data16[j] = ARGB5(in16[j].a, in16[j].r, in16[j].g, in16[j].b);
				data16+=img->bufwidth;
			}
			delete [] in;
			return;

		case 24:
			for(u32 i=0; i<head->height; ++i){
				u8 *in8 = in;
				sceIoRead(fd, in, line_size);
				for ( u32 j=0; j<head->width; ++j )
					data32[j] = BGRA8(*in8++, *in8++, *in8++);
				data32+=img->bufwidth;
			}
			delete [] in;
			return;

		case 32:
			for(u32 i=0; i<head->height; ++i){
				u8 *in8 = (u8*)in;
				sceIoRead(fd, in, line_size);
				for ( u32 j=0; j<head->width; ++j )
					data32[j] = BGRA8(*in8++, *in8++, *in8++, *in8++);
				data32+=img->bufwidth;
			}
			delete [] in;
			return;
	}
}

static inline
bool tga_readPixelsBuf(SceUID fd, Pmc_Image *img, TARGAHEAD *head)
{
	u32 line_size = head->width*head->bpp>>3;
	u8 *in = (u8*)malloc(line_size*head->height);
	if (!in) return false;
	sceIoRead(fd, in, line_size*head->height);
	
	u8 *in8 = in;
	u16 *data16 = (u16*)img->data;
	u32 *data32 = (u32*)img->data;
	
		C_ARGB5 *in16 = (C_ARGB5*)in;
	
	switch(head->bpp){
		case 16:
			line_size /= 2;
			for ( u32 i=0; i<head->height; ++i){
				for ( u32 j=0; j<head->width; ++j )
					data16[j] = ARGB5(in16[j].a, in16[j].r, in16[j].g, in16[j].b);
				data16+=img->bufwidth;
				in16 += line_size;
			}
			free(in);
			return true;

		case 24:
			for(u32 i=0; i<head->height; ++i){
				for ( u32 j=0; j<head->width; ++j )
					data32[j] = BGRA8(*in8++, *in8++, *in8++);
				data32+=img->bufwidth;
			}
			free(in);
			return true;

		case 32:
			for(u32 i=0; i<head->height; ++i){
				for ( u32 j=0; j<head->width; ++j )
					data32[j] = BGRA8(*in8++, *in8++, *in8++, *in8++);
				data32+=img->bufwidth;
			}
			free(in);
			return true;
	}
	return false;
}

Pmc_Image *load_tga(const char filename[], bool swizzle)
{
	Pmc_Image *img = NULL;
	if (!filename) return NULL;
	SceUID fd = sceIoOpen(filename, PSP_O_RDONLY, 0);
	if (fd<0) goto error;
	
	TARGAHEAD header;
	if (sceIoRead(fd, &header, 18)<=0) goto error;
	
	/*
	switch(header.type){
			// we only handle the ff. image types
		case UNCOMPRESSED_RGB:
			break;
		case NO_IMAGE:
			printf("TGA: No image data\n");
		default:
			printf("TGA: Cannot handle image type\n");
			goto error;
	}
	*/
	if (header.type!=UNCOMPRESSED_RGB) goto error;
	
	// idk how interleaving works
	if (header.interleaving!=0) goto error;
	
	switch(header.bpp){
		case 16:
		case 24:
		case 32:
			break;
		default:
			printf("TGA: Cannot handle bpp\n");
			goto error;
	}
	
	img = alloc_tgaSpace(fd, &header);
	if (!img) goto error;
	
	if ( !tga_readPixelsBuf(fd, img, &header) )
		tga_readPixelsRAW(fd, img, &header);
	
	if (swizzle) img->swizzle();
	img->invalidate();
	sceIoClose(fd);
	return img;
error:
	if (fd>=0) sceIoClose(fd);
	show_error("Cannot load image!\nPlease reinstall application.", true);
	return NULL;
}
