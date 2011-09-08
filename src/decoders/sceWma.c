/* 
 *   Copyright (C) 2009 cooleyes 
 *   eyes.cooleyes@gmail.com 
 * 
 *  This Program is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2, or (at your option) 
 *  any later version. 
 *    
 *  This Program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 *  GNU General Public License for more details. 
 *    
 *  You should have received a copy of the GNU General Public License 
 *  along with GNU Make; see the file COPYING.  If not, write to 
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html 
 * 
 */ 
#include <pspkernel.h> 
#include <pspdisplay.h> 
#include <pspdebug.h> 
#include <pspkernel.h> 
#include <pspsdk.h> 
#include <pspaudiocodec.h> 
#include <string.h>
//extern "C" {
#include <malloc.h> 
//}
#include <psputility.h> 
#include "cooleyesBridge.h" 
#include <pspasfparser_cool.h>
#include "./audio_dec.h"

extern char custom_tag[4][256];
extern int custom_info[4];

static unsigned long wma_codec_buffer[65] __attribute__((aligned(64)));
SceAsfParser* parser = NULL;

static struct
{
	SceUID asf_modid, cool_modid;
	void* frame_buffer, *need_mem_buffer;
	int block_align;
}SceWma = { -1, -1, NULL, NULL, 1485 };

static const
unsigned char WMA_GUID[16] =
{
		0x30, 0x26, 0xB2, 0x75,
		0x8E, 0x66, 0xCF, 0x11,
		0xA6, 0xD9, 0x00, 0xAA,
		0x00, 0x62, 0xCE, 0x6C
};

#define WMA_FORMAT_TAG 0x0161

static
void sceWma_close()
{
	sceAudiocodecReleaseEDRAM(wma_codec_buffer);
	sceAudiocodecReleaseEDRAM(wma_codec_buffer);
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	
	int ret;
	if (SceWma.asf_modid>=0)
	{
		sceKernelStopModule(SceWma.asf_modid, 0, NULL, &ret, NULL);
		sceKernelUnloadModule(SceWma.asf_modid);
	}
	
	if (SceWma.cool_modid>=0)
	{
		sceKernelStopModule(SceWma.cool_modid, 0, NULL, &ret, NULL);
		sceKernelUnloadModule(SceWma.cool_modid);
	}
	SceWma.cool_modid = SceWma.asf_modid = -1;
	
	memset(wma_codec_buffer, 0, 65); 
	
	free(parser);
	free(SceWma.need_mem_buffer);
	free(SceWma.frame_buffer);
	
	parser = NULL;
	SceWma.need_mem_buffer = NULL;
	SceWma.frame_buffer = NULL;
	
	SceWma.block_align = 1485;
}

static
SceOff asf_seek_cb(void *userdata, void *buf, SceOff offset, int whence)
{
	return (SceOff)avio_seek((AVIOContext*)userdata, (SceOff)offset, whence);
}

int check_ifwma(AVIOContext* fd)
{
	char wma_guid_buf[16] = "";
	if ( avio_read(fd, &wma_guid_buf, 16) != 16 )
		return 0;
	
	if ( memcmp(wma_guid_buf, WMA_GUID, 16)!=0 )
	{
		printf("Not a WMA file\n");
		return 0;
	}
	
	return 1;
}

#include <cooleyesBridge.h>
int sceWma_open(const char *filename, AVIOContext* io_ctx)
{
// so goto's will not generate errors
	int ret;
	u16* p16;
	int npt = 0;
	u16 wma_avg_bytes_per_sec;
	
//	strcpy(custom_tag[TAG_TITLE], strrchr(file, '/')+1);
	
	if ( sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0 )
		goto error;
	
  SceWma.asf_modid = sceKernelLoadModule("flash0:/kd/libasfparser.prx", 0,NULL);
	if (SceWma.asf_modid<0) goto error;
	else sceKernelStartModule(SceWma.asf_modid, 0, 0, &ret, NULL);
	
  SceWma.cool_modid = sceKernelLoadModule("cooleyesBridge.prx", 0,NULL);
	if (SceWma.cool_modid<0) goto error;
	else sceKernelStartModule(SceWma.cool_modid, 0, 0, &ret, NULL);
	
	cooleyesMeBootStart(sceKernelDevkitVersion(), 3);
	
	parser = (SceAsfParser*)memalign(64, sizeof(SceAsfParser));
	if (!parser) goto error;
	
	memset(parser, 0, sizeof(SceAsfParser));
	
	parser->iUnk0 = 0x01;
	parser->iUnk1 = 0x00;
	parser->iUnk2 = 0x000CC003;
	parser->iUnk3 = 0x00;
	parser->iUnk4 = 0x00000000;
	parser->iUnk5 = 0x00000000;
	parser->iUnk6 = 0x00000000;
	parser->iUnk7 = 0x00000000;
	
	ret = sceAsfCheckNeedMem(parser);
	if ( ret < 0 )
	{
		printf("sceAsfCheckNeedMem() = 0x%08x\n", ret);
		goto error;
	}
	
	if ( parser->iNeedMem > 0x8000 )
	{
		 printf("iNeedMem > 0x8000\n");
		 goto error;
	}
	
	//sceAsfInitParser
	SceWma.need_mem_buffer = memalign(64, parser->iNeedMem);
	if ( !SceWma.need_mem_buffer ) goto error;
	
	parser->pNeedMemBuffer = SceWma.need_mem_buffer;
	parser->iUnk3356 = 0x00000000;
	
	ret = sceAsfInitParser(parser, io_ctx, &avio_read, /*&sceIoRead, */&asf_seek_cb);
	if ( ret < 0 )
	{
		printf("sceAsfInitParser() = 0x%08x\n", ret);
		goto error;
	}
	
//	wma_channels = *((u16*)need_mem_buffer); 
	custom_info[NFF_TAG_SAMPRATE] = *((u32*)(SceWma.need_mem_buffer+4)); 
	
	wma_avg_bytes_per_sec = *((u32*)(SceWma.need_mem_buffer+8));
	custom_info[NFF_TAG_BITRATE] = wma_avg_bytes_per_sec * 8;
	
	SceWma.block_align = *((u16*)(SceWma.need_mem_buffer+12));
//	wma_info[WMA_INFO_FLAG] = *((u16*)(need_mem_buffer+14));
	custom_info[NFF_TAG_DURATION] = parser->lDuration/10000000;
	
	//CheckNeedMem
	//memset(wma_codec_buffer, 0, 65);
	wma_codec_buffer[5] = 1;
	ret = sceAudiocodecCheckNeedMem(wma_codec_buffer, 0x1005);
	if ( ret < 0 )
	{
		printf("sceAudiocodecCheckNeedMem=0x%08X\n", ret);
		goto error;
	}
	
	//GetEDRAM
	ret = sceAudiocodecGetEDRAM(wma_codec_buffer, 0x1005);
	if ( ret < 0 )
	{
		printf("sceAudiocodecGetEDRAM=0x%08X\n", ret);
		goto error;
	}
	
	//Init
	p16 = (u16*)(&wma_codec_buffer[10]);
	p16[0] = WMA_FORMAT_TAG;
	p16[1] = *((u16*)SceWma.need_mem_buffer); //channels
	
	wma_codec_buffer[11] = custom_info[NFF_TAG_SAMPRATE];//wma_samplerate;
	wma_codec_buffer[12] = wma_avg_bytes_per_sec;
	
	p16 = (u16*)(&wma_codec_buffer[13]);
	p16[0] = SceWma.block_align;
	p16[1] = 16; //wma_bits_per_sample;
	p16[2] = *((u16*)(SceWma.need_mem_buffer+14)); //wma_flag;
	
	printf("wma flag = %d\n", p16[2]);
	
	ret=sceAudiocodecInit(wma_codec_buffer, 0x1005); 
	if ( ret < 0 )
	{
		printf("sceAudiocodecInit=0x%08X\n", ret);
		goto error;
	}
	
	SceWma.frame_buffer = memalign(64, SceWma.block_align);
	if (!SceWma.frame_buffer)
	{
		printf("malloc_64(frame_data_buffer) failed\n");
		goto error;
	}
	//memset(wma_frame_buffer, 0, wma_info[WMA_INFO_BLOCK_ALIGN]);
	
	ret = sceAsfSeekTime(parser, 1, &npt);
	if (ret < 0)
	{
		printf("sceAsfSeekTime = 0x%08x\n", ret);
		goto error;
	}
	
	return 1;
error:
	sceWma_close();
	return 0;
}

static
int sceWma_decode(s16 *buf, AVPacket *pkt, int size)
{
	parser->sFrame.pData = SceWma.frame_buffer;
	
	if ( sceAsfGetFrameData(parser, 1, &parser->sFrame) < 0)
		return -1;
	
	wma_codec_buffer[6] = (unsigned long)SceWma.frame_buffer;
	wma_codec_buffer[7] = (unsigned long)SceWma.block_align;
	wma_codec_buffer[8] = (unsigned long)buf;
	wma_codec_buffer[9] = size;//16384;
	
	wma_codec_buffer[15] = parser->sFrame.iUnk2;
	wma_codec_buffer[16] = parser->sFrame.iUnk3;
	wma_codec_buffer[17] = parser->sFrame.iUnk5;
	wma_codec_buffer[18] = parser->sFrame.iUnk6;
	wma_codec_buffer[19] = parser->sFrame.iUnk4;
	
	int ret = sceAudiocodecDecode(wma_codec_buffer, 0x1005); 
	if (ret<0)
	{
		printf("scewma err = 0x%08x\n", ret );
		return -1;
	}
	return wma_codec_buffer[9]<0?-1:wma_codec_buffer[9];
}

static
int64_t sceWma_seek(int64_t seconds)
{
	int secs = seconds*1000;
	if ( sceAsfSeekTime(parser, 1, &secs) < 0 )
		return -1;
	
	return (int64_t)secs/1000;
}

const
AUDIO_DECODER sceAsfWma =
{
	sceWma_decode,
	sceWma_close,
	sceWma_seek
};
