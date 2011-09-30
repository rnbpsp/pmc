#include "../utils.h"
#include "../image.h"
#include "../drawing.h"
#include <psputility.h>
//#include <pspjpeg.h>
#include <cstdio>
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
	
	img->swizzle();
	img->invalidate();

	if (img->width==128 && img->height==128) return img;
	
	const float scale = 128.f/(float)pmc_max<int>(img->height,img->width);
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

///////////////////////////////////////////////////////////////////////////////////////

extern "C" {
#include <jpeglib.h>
}

static NOINLINE
Pmc_Image *decode_jpgArt(void *data, size_t size)
{
	int width=0, height=0;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1] = {NULL};
	u32 *pdata;
	
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
	if (!img) goto err_jpg;
	
	jpeg_start_decompress( &cinfo );
	row_pointer[0] = new unsigned char[cinfo.output_width*cinfo.num_components];
	if (!row_pointer[0])
	{
		delete img;
		goto err_jpg;
	}
	pdata = (u32*)img->data;
	while( cinfo.output_scanline < cinfo.output_height )
	{
			jpeg_read_scanlines( &cinfo, row_pointer, 1 );
			for ( unsigned j=0, i=0; j<cinfo.image_width; ++j, i+=3 )
				pdata[j] = RGBA8(row_pointer[0][i], row_pointer[0][i+1], row_pointer[0][i+2]);
			
			pdata += img->bufwidth;
	}
	jpeg_finish_decompress( &cinfo );
	
err_jpg:
	jpeg_destroy_decompress( &cinfo );
	delete row_pointer[0];
	fclose( infile );
	return img;
}

///////////////////////////////////////////////////////////////////////////////////////

extern "C" {
#include <libpng15/png.h>
}

static NOINLINE
Pmc_Image *decode_pngArt(void *data, size_t size)
{
	if (png_sig_cmp((u8*)data, 0, 8)) return NULL;
	Pmc_Image *img = NULL;
	
	FILE *infile = fmemopen(data, size, "rb");
	if (!infile) return NULL;
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		fclose(infile);
		return NULL;
	}
	
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) goto err_png;
	
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		delete img;
		img = NULL;
		goto err_png;
	}
	
	png_init_io(png_ptr, infile);
	png_set_sig_bytes(png_ptr, 0);
	png_read_info(png_ptr, info_ptr);
	
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	if (interlace_type!=PNG_INTERLACE_NONE)
		goto err_png;
	
//	png_set_expand(png_ptr);
	png_set_packing(png_ptr);
	png_set_scale_16(png_ptr);
	
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	
	if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	
	if (color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY)
		png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);	
	
	png_read_update_info(png_ptr, info_ptr);
	
	img = new Pmc_Image(width, height, GU_PSM_8888);
	if (!img) goto err_png;
	
	for (u32 y = 0; y < height; y++)
		png_read_row(png_ptr, (u8*)((u32*)img->data + y*img->bufwidth), NULL);
	
	png_read_end(png_ptr, info_ptr);
err_png:
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(infile);
	return img;
}

///////////////////////////////////////////////////////////////////////////////////////

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
				if (pic->mimeType() == "image/jpeg" || pic->mimeType() == "image/jpg")
					return decode_jpgArt(pic->picture().data(), pic->picture().size());
				else if (pic->mimeType() == "image/png")
					return decode_pngArt(pic->picture().data(), pic->picture().size());
				else printf("%s\n", pic->mimeType().toCString());
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
					if (pic->mimeType() == "image/jpeg" || pic->mimeType() == "image/jpg")
						return decode_jpgArt(pic->picture().data(), pic->picture().size());
					else if (pic->mimeType() == "image/png")
						return decode_pngArt(pic->picture().data(), pic->picture().size());
					else printf("%s\n", pic->mimeType().toCString());
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
					return decode_pngArt(pic.data().data(), pic.data().size());
			}
		}
	}
	
	return NULL;
}

