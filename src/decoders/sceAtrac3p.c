#include "../utils.h"
#include "./audio_dec.h"
#include "libavutil/intreadwrite.h"
#include <psputility.h>
#include <pspkernel.h>
#include "malloc.h"

static unsigned long sceAtrac3p_buf[65] __attribute__((aligned(64)));
static u8 *sceAtrac3p_tmpbuf = NULL;

// [2] = block align
static u32 at3p_flags[3];

static
void sceAtrac3p_close()
{
	sceAudiocodecReleaseEDRAM(sceAtrac3p_buf);
	memset(sceAtrac3p_buf, 0, 65);
	
	av_freep(&sceAtrac3p_tmpbuf);
	
	at3p_flags[0] = 0;
	at3p_flags[1] = 0;
	at3p_flags[2] = 0;
	
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

int sceAtrac3p_open(AVCodecContext *ctx)
{
	if (ctx->extradata==NULL) return 0;
	
	if ( sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0 ) return 0;

		sceAtrac3p_buf[5] = 0x1;
		sceAtrac3p_buf[10] = *((u16*)ctx->extradata);
		sceAtrac3p_buf[12] = 0x1;
		sceAtrac3p_buf[14] = 0x1;
	if ( sceAudiocodecCheckNeedMem(sceAtrac3p_buf, PSP_CODEC_AT3PLUS) < 0 )
		goto err;
	if ( sceAudiocodecGetEDRAM(sceAtrac3p_buf, PSP_CODEC_AT3PLUS) < 0 )
		goto err;
	
	if ( sceAudiocodecInit(sceAtrac3p_buf, PSP_CODEC_AT3PLUS) < 0 )
	{
		sceAtrac3p_close();
		return 0;
	}
	
	const int at3tmpbuf_size = ALIGN_SIZE(ctx->block_align+8, 64);
	sceAtrac3p_tmpbuf = av_malloc(at3tmpbuf_size);
	if (!sceAtrac3p_tmpbuf)
	{
		sceAtrac3p_close();
		return 0;
	}
	memset(sceAtrac3p_tmpbuf, 0, at3tmpbuf_size);
	
	at3p_flags[0] = ctx->extradata[0];
	at3p_flags[1] = ctx->extradata[1];
	at3p_flags[2] = ctx->block_align;
	
	return 1;
	
err:
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	return 0;
}

static
int sceAtrac3p_decode(s16 *buf, AVPacket *pkt, int size)
{
	sceAtrac3p_tmpbuf[0] = 0x0F;
	sceAtrac3p_tmpbuf[1] = 0xD0;
	sceAtrac3p_tmpbuf[2] = at3p_flags[0];
	sceAtrac3p_tmpbuf[3] = at3p_flags[1];
	memcpy(sceAtrac3p_tmpbuf+8, pkt->data, at3p_flags[2]);
	
	sceAtrac3p_buf[6] = (unsigned long)sceAtrac3p_tmpbuf;
	sceAtrac3p_buf[7] = (unsigned long)at3p_flags[2]+8;
	sceAtrac3p_buf[8] = (unsigned long)buf;
	sceAtrac3p_buf[9] = size;
	
	const int res = sceAudiocodecDecode(sceAtrac3p_buf, PSP_CODEC_AT3PLUS);
	//if (res<0) printf("sceAa3 err = 0x%08x\n", res );
	
	pkt->size -= at3p_flags[2];
	if (pkt->size<0) pkt->size = 0;
	if (pkt->size!=0) pkt->data += at3p_flags[2];
	
	return (res<0) ? -1 : sceAtrac3p_buf[9];
}

const
AUDIO_DECODER sceAtrac3p = 
{
	sceAtrac3p_decode,
	sceAtrac3p_close,
	NULL
};
