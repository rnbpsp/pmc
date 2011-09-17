#include "../utils.h"
#include "../image.h"
#include "../drawing.h"
#include <psputility.h>
//#include <pspjpeg.h>
#include <cstdio>
extern "C" {
#include <jpeglib.h>
}

#if 1
#include <fileref.h>
#include <tag.h>
#include <tbytevector.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2framefactory.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <attachedpictureframe.h>
#include <mp4file.h>
#include <mp4tag.h>
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
static Pmc_Image *load_mp4art(const char *file);

Pmc_Image *load_albumArt(const char *file)
{
	Pmc_Image *img = NULL;
	
	/* Some containers supports multiple tag formats */
						img = load_id3art(file); // ID3v2(APIC, PIC) frames
	if (!img)	img = load_mp4art(file); // MP4(covr) Cover art
	if (!img) return NULL;
	if (!img->isValid())
	{
		delete img;
		return NULL;
	}
	
	if (img->width==128 && img->height==128) return img;
	
//	const float scale = 128.f/(float)pmc_min<int>(pmc_max<int>(img->height,img->width),512);
	const float scale = 128.f/(float)pmc_max<int>(img->height,img->width);
//	if (img->width>512) img->width = 512;
//	if (img->height>512) img->height = 512;
	img->scaleX = img->width*scale;
	img->scaleY = img->height*scale;
	
	/* TODO: pre-scale the image into a buffer in vram
	
	memset((void*)(ALBUM_ART_VMEM|VRAM_BASE), 0, 128*128*4);
	pmc_wbinvalidate((void*)(ALBUM_ART_VMEM|VRAM_BASE), 128*128*4);
	
	gu_start(true);
	vram.set_drawbuf((void*)ALBUM_ART_VMEM, 128);
	//drawImgStrip_large(img, 0,0);
	img->draw(0,0);
	vram.restore_drawbuf();
	gu_end();
	gu_sync();
	
	delete img;
	
	img = new Pmc_Image(128, 128, GU_PSM_8888,false);
	img->data = (u8*)(ALBUM_ART_VMEM|VRAM_BASE);
	*/
	return img;
}

static NOINLINE Pmc_Image *decode_jpgArt(void *data, size_t size)
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
	
	jpeg_start_decompress( &cinfo );
	row_pointer[0] = new unsigned char[cinfo.output_width*cinfo.num_components];
	while( cinfo.output_scanline < cinfo.image_height )
	{
			jpeg_read_scanlines( &cinfo, row_pointer, 1 );
			u32 *data = (u32*)img->data + (img->bufwidth*cinfo.output_scanline);

			for ( unsigned j=0, i=0; j<cinfo.image_width; ++j, i+=3 )
				data[j] = RGBA8(row_pointer[0][i], row_pointer[0][i+1], row_pointer[0][i+2]);
	}
	jpeg_finish_decompress( &cinfo );
	
	img->swizzle();
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
	MPEG::File tagfile(file, false);

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
		}
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
		}
	}
	
	return NULL;
}

// http://stackoverflow.com/questions/6542465/c-taglib-cover-art-from-mpeg-4-files
static Pmc_Image *load_mp4art(const char *file)
{
	MP4::File tagfile(file, false);

	if( tagfile.tag() )
	{
		// Get the list of frames for a specific frame type
		MP4::CoverArtList l = tagfile.tag()->itemListMap()["covr"].toCoverArtList();
		if( !l.isEmpty() )
		{
			MP4::CoverArt pic = l.front();
			
			if (pic.data().size() > 0)
			{
				if (pic.format() == MP4::CoverArt::JPEG)
					return decode_jpgArt(pic.data().data(), pic.data().size());
				else if (pic.format() == MP4::CoverArt::PNG)
					printf("album art is png.\n");
			}
		}
	}
	
	return NULL;
}

