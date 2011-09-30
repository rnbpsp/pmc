//#include "utils.h"
#include "./audio_dec.h"
#include "libavutil/intreadwrite.h"
#include <psputility.h>
#include <pspkernel.h>
#include "malloc.h"

static unsigned long sceAtrac3_buf[65] __attribute__((aligned(64)));
//static unsigned long sceAtrac3_tmpbuf[1024*32/4] __attribute__((aligned(64)));
static AVCodecContext *at3codec_ctx = NULL;

static
void sceAtrac3_close()
{
	at3codec_ctx = NULL;
	sceAudiocodecReleaseEDRAM(sceAtrac3_buf);
	memset(sceAtrac3_buf, 0, 65);
	
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

int sceAtrac3_open(AVCodecContext *ctx)
{
	if ( sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0 ) return 0;

		sceAtrac3_buf[26] = 0x20;
	if ( sceAudiocodecCheckNeedMem(sceAtrac3_buf, PSP_CODEC_AT3) < 0 )
		goto err;
	if ( sceAudiocodecGetEDRAM(sceAtrac3_buf, PSP_CODEC_AT3) < 0 )
		goto err;
	
	sceAtrac3_buf[10] = ctx->block_align==0x130 ? 6 : 4;
	sceAtrac3_buf[44] = 2;
	if ( sceAudiocodecInit(sceAtrac3_buf, PSP_CODEC_AT3) < 0 )
	{
		sceAtrac3_close();
		return 0;
	}
	
	at3codec_ctx = ctx;
	
	return 1;
	
err:
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	return 0;
}

static
int sceAtrac3_decode(s16 *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size)
{/*
	if ( AV_RL16( &at3codec_ctx->extradata[6] ) )
	{
		memcpy((void*)sceAtrac3_tmpbuf, pkt->data, pkt->size);
		memcpy((u32*)((u8*)sceAtrac3_tmpbuf+pkt->size), (void*)((u8*)pkt->data), pkt->size);
		sceAtrac3_buf[6] = (unsigned long)sceAtrac3_tmpbuf;
	}
	else*/ sceAtrac3_buf[6] = (unsigned long)pkt->data;
	sceAtrac3_buf[7] = (unsigned long)pkt->size;
	sceAtrac3_buf[8] = (unsigned long)buf;
	sceAtrac3_buf[9] = size;
	
	int res = sceAudiocodecDecode(sceAtrac3_buf, PSP_CODEC_AT3);
	//if (res<0) printf("sceat3 err = 0x%08x\n", res );
	
	pkt->size -= at3codec_ctx->block_align;
	if (pkt->size<0) pkt->size = 0;
	if (pkt->size!=0) pkt->data  += at3codec_ctx->block_align;
	
	return (res<0) ? -1 : sceAtrac3_buf[9];
}

const
AUDIO_DECODER sceAtrac3 = 
{
	sceAtrac3_decode,
	sceAtrac3_close,
	NULL
};
