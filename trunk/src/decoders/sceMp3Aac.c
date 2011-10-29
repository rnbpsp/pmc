//#include "utils.h"
#include "./audio_dec.h"
#include <psputility.h>
#include <pspkernel.h>
#include "malloc.h"

static int codec_type = 0;
static unsigned long sceMp3Aac_buf[65] __attribute__((aligned(64)));
static AVBitStreamFilterContext *bsf_ctx = NULL;
//AVCodecContext *codec_ctx_ = NULL;

// AVPacket.data is only 16bytes aligned
// sceAudiocodec requires 64bytes alignment
//static void *sceMp3Aac_tmpbuf = NULL;

static
void sceMp3Aac_close(AVCodecContext *ctx)
{
	codec_type = 0;
	
	sceAudiocodecReleaseEDRAM(sceMp3Aac_buf);
	memset(sceMp3Aac_buf,0,65);
	
	if (bsf_ctx) av_bitstream_filter_close(bsf_ctx);
	bsf_ctx = NULL;
	
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

int sceMp3Aac_open(AVCodecContext *ctx, int type_)
{
	const int type = type_&0xffff;
	if ( sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0 ) return 0;

//	memset(sceMp3Aac_buf, 0, 65);
	if ( sceAudiocodecCheckNeedMem(sceMp3Aac_buf, type) < 0 ) goto err;
	if ( sceAudiocodecGetEDRAM(sceMp3Aac_buf, type) < 0 ) goto err;
	
	sceMp3Aac_buf[10] = ctx->sample_rate;
	if ( sceAudiocodecInit(sceMp3Aac_buf, type) < 0 )
	{
		sceMp3Aac_close(ctx);
		return 0;
	}
	
	if (type_&AAC_IS_ADTS)
	{
		bsf_ctx = av_bitstream_filter_init("aac_adtstoasc");
		if (!bsf_ctx)
		{
			sceMp3Aac_close(ctx);
			return 0;
		}
	}
	
	codec_type = type;
	return 1;
	
err:
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	return 0;
}

static
int sceMp3Aac_decode(s16 *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size)
{
	unsigned long data = 0, data_size = 0;
	if (bsf_ctx)
	{
		av_bitstream_filter_filter(bsf_ctx, codec_ctx, NULL,
													(u8**)&data, (int*)&data_size,
													pkt->data, pkt->size, 0);
	}
	else
	{
		data = (unsigned long)pkt->data;
		data_size = (unsigned long)pkt->size;
	}
	sceMp3Aac_buf[6] = data;
	sceMp3Aac_buf[7] = data_size;
	sceMp3Aac_buf[8] = (unsigned long)buf;
	sceMp3Aac_buf[9] = size;
	
	int res = sceAudiocodecDecode(sceMp3Aac_buf, codec_type);
	if (res<0) printf("scemp3 err = 0x%08x\n", res );
	
	// packet contains exactly one frame for codecs w /
	//	variable frame size (eg. mpeg codecs like mp3 and aac)
	// see av_read_frame
	pkt->size=0;

	return (res<0) ? -1 : sceMp3Aac_buf[9];
}

const
AUDIO_DECODER sceMp3Aac = 
{
	sceMp3Aac_decode,
	sceMp3Aac_close
};
