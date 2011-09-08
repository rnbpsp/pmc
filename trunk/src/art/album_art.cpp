#include "../utils.h"
#include "../image.h"
#include "../drawing.h"
#include <psputility.h>
//#include <pspjpeg.h>
#include <cstdio>
extern "C" {
#include <jpeglib.h>
}

#if 0
#include <fileref.h>
#include <tag.h>
#include <tbytevector.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2framefactory.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <attachedpictureframe.h>
#else
#include "fileref.h"
#include "tag.h"
#include "toolkit/tbytevector.h"
#include "mpeg/mpegfile.h"
#include "mpeg/id3v2/id3v2tag.h"
#include "mpeg/id3v2/id3v2frame.h"
#include "mpeg/id3v2/id3v2header.h"
#include "mpeg/id3v2/frames/attachedpictureframe.h"
#endif

static Pmc_Image *load_id3art(const char *file);

Pmc_Image *load_albumArt(const char *file)
{
	Pmc_Image *img = NULL;
	
	// ID3v2(APIC, PIC) frames
	img = load_id3art(file);
	if (!img) return NULL;
	if (!img->isValid())
	{
		delete img;
		return NULL;
	}
	img->scaleX = img->scaleY = 128;
	if (img->width>512) img->width = 512;
	if (img->height>512) img->height = 512;
	/*
	if (img->width==128 && img->height==128) return img;
	
	gu_start(true);
	vram.set_drawbuf((void*)ALBUM_ART_VMEM, 128);
	drawImgStrip_large(img, 0,0);
	vram.restore_drawbuf();
	gu_end();
	gu_sync();
	
	delete img;
	
	img = new Pmc_Image(128, 128, GU_PSM_8888,false);
	img->data = (u8*)(ALBUM_ART_VMEM|VRAM_BASE);
	*/
	return img;
}

/*
// http://www.psp-programming.com/forums/index.php?topic=4449.0
static int getJpegDimensions(u8* data, int data_size, int &width, int &height)
{
   int i = 0;
   if (data[i] == 0xFF && data[i+1] == 0xD8 && data[i+2] == 0xFF && data[i+3] == 0xE0) {
      i += 4;
      // Check for valid JPEG header (null terminated JFIF)
      if (data[i+2] == 'J' && data[i+3] == 'F' && data[i+4] == 'I' && data[i+5] == 'F' && data[i+6] == 0x00){
         //Retrieve the block length of the first block since the first block will not contain the size of file
         int block_length = data[i] * 256 + data[i+1];
         while (i < data_size){
               i += block_length;                  //Increase the file index to get to the next block
               if (i >= data_size) return -1;      //Check to protect against segmentation faults
               if (data[i] != 0xFF) return -1;     //Check that we are truly at the start of another block
               if (data[i+1] == 0xC0) {            //0xFFC0 is the "Start of frame" marker which contains the file size
               //The structure of the 0xFFC0 block is quite simple [0xFFC0][ushort length][uchar precision][ushort x][ushort y]
                  height = data[i+5]*256 + data[i+6];
                  width = data[i+7]*256 + data[i+8];
                  return 0;
               }
               else{
                  i += 2;                                     //Skip the block marker
                  block_length = data[i] * 256 + data[i+1];   //Go to the next block
               }
         }
         return -1;                //If this point is reached then no size was found
      }
      else
         return -1;                //Not a valid JFIF string
   }
   else
      return -1;                   //Not a valid SOI header
}

// only jpeg is supported for now
static Pmc_Image *decode_jpgArt(void *data, size_t size)
{
	Pmc_Image *img = NULL;
	u8 *tmp = NULL;
	u32 *imgdata, *tmpdata;
	int width, height;
	
	if ( sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0 )
		return NULL;
	
	if ( sceJpegInitMJpeg() < 0 )
		goto error;
	
	if ( getJpegDimensions((u8*)data, size, width, height) < 0 )
		goto error1;
	
	if (width==0 || height==0)
		goto error1;
	
	if ( sceJpegCreateMJpeg(width, height) < 0 )
		goto error1;
	
	tmp = new u8[width*height*4];
	if (!tmp) goto error2;
	
	if ( sceJpegDecodeMJpeg((u8*)data, size, tmp, 0) < 0 )
		goto error2;
	
	img = new Pmc_Image(width, height, GU_PSM_8888);
	if (!img) goto error2;
	if (!img->isValid())
	{
		delete img;
		goto error2;
	}
	
	imgdata = (u32*)img->data;
	tmpdata = (u32*)tmp;
	for (int i=0; i<height; ++i)
	{
		for(int j=0; j<width; ++j)
			imgdata[j] = tmpdata[j]|0xff000000;
		
		imgdata += img->bufwidth;
		tmpdata += width;
	}
	
	img->swizzle();
	img->invalidate();
error2:
	delete [] tmp;
	sceJpegDeleteMJpeg();
error1:
	sceJpegFinishMJpeg();
error:
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	return img;
}
*/

// TODO: Use sceJpeg?
static Pmc_Image *decode_jpgArt(void *data, size_t size)
{
	int width=0, height=0;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1] = {NULL};
	
	// TODO
	FILE *infile = fmemopen(data, size, "rb");
	if (!infile) return NULL;
	
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_decompress( &cinfo );
	jpeg_stdio_src( &cinfo, infile );
	jpeg_read_header( &cinfo, TRUE );
	width = cinfo.image_width;
	height = cinfo.image_height;
	
	Pmc_Image *img = new Pmc_Image(width, height, GU_PSM_8888);
	if (!img) goto err;
//	img->data = (u8*)ALBUM_ART_VMEM;
	
	jpeg_start_decompress( &cinfo );
	row_pointer[0] = new unsigned char[cinfo.output_width*cinfo.num_components];
	while( cinfo.output_scanline < cinfo.image_height )
	{
			jpeg_read_scanlines( &cinfo, row_pointer, 1 );
			u32 *data = (u32*)img->data + (img->bufwidth*cinfo.output_scanline);

			for ( int j=0, i=0; j<cinfo.image_width; ++j, i+=3 )
				data[j] = RGBA8(row_pointer[0][i], row_pointer[0][i+1], row_pointer[0][i+2]);
	}
	jpeg_finish_decompress( &cinfo );
	
//	img->swizzle();
	img->invalidate();
err:
	jpeg_destroy_decompress( &cinfo );
	delete row_pointer[0];
	fclose( infile );
	return img;
}

using namespace TagLib;
static Pmc_Image *load_id3art(const char *file)
{
	MPEG::File tagfile(file);

	if( tagfile.ID3v2Tag() )
	{
		// Get the list of frames for a specific frame type
		ID3v2::FrameList l = tagfile.ID3v2Tag()->frameListMap()["APIC"];
		if( !l.isEmpty() )
		{
			ID3v2::AttachedPictureFrame *pic =
				static_cast<ID3v2::AttachedPictureFrame*> (l.front());
			
			if (pic->picture().size() > 0)
			{
				if (pic->mimeType() == "image/jpeg")
					return decode_jpgArt(pic->picture().data(), pic->picture().size());
				else if (pic->mimeType() == "image/png")
					printf("album art is png.\n");
			}
		}/*
		else // check for id3v2.2 frames
		{
			ID3v2::FrameList l2 = tagfile.ID3v2Tag()->frameListMap()["PIC"];
			if( !l2.isEmpty() )
			{
				ID3v2::AttachedPictureFrameV22 *pic =
					static_cast<ID3v2::AttachedPictureFrameV22*> (l2.front());
				
				if (pic->picture().size() > 0)
				{
					if (pic->mimeType() == "image/jpeg")
						return decode_jpgArt(pic->picture().data(), pic->picture().size());
					else if (pic->mimeType() == "image/png")
						printf("album art is png.\n");
				}
			}
		}*/
	}
	
	return NULL;
}
