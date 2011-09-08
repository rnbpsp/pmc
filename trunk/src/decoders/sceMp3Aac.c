//#include "utils.h"
#include "./audio_dec.h"
#include <psputility.h>
#include <pspkernel.h>
#include "malloc.h"

static int codec_type = 0;
static unsigned long sceMp3Aac_buf[65] __attribute__((aligned(64)));

// AVPacket.data is only 16bytes aligned
// sceAudiocodec requires 64bytes alignment
//static void *sceMp3Aac_tmpbuf = NULL;

// what is the maximum size of an mp3/2/1/aac frame?
#define MAX_FRAME_SIZE_MP3AAC 32000

static
void sceMp3Aac_close()
{
	codec_type = 0;
	
	sceAudiocodecReleaseEDRAM(sceMp3Aac_buf);
	//memset(sceMp3Aac_buf,0,65);
	int i=0;
	for(;i<65;++i) sceMp3Aac_buf[i] = 0;
	
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

// maybe we should try decoding a frame to see if it works
int sceMp3Aac_open(AVCodecContext *ctx, int type)
{
	if ( sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0 ) return 0;

//	memset(sceMp3Aac_buf, 0, 65);
	if ( sceAudiocodecCheckNeedMem(sceMp3Aac_buf, type) < 0 ) goto err;
	if ( sceAudiocodecGetEDRAM(sceMp3Aac_buf, type) < 0 ) goto err;
	
	sceMp3Aac_buf[10] = ctx->sample_rate;
	if ( sceAudiocodecInit(sceMp3Aac_buf, type) < 0 )
	{
		sceMp3Aac_close();
		return 0;
	}
	
	codec_type = type;
	return 1;
	
err:
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	return 0;
}

static
int sceMp3Aac_decode(s16 *buf, AVPacket *pkt, int size)
{
	sceMp3Aac_buf[6] = (unsigned long)pkt->data;//sceMp3Aac_tmpbuf;
	sceMp3Aac_buf[7] = (unsigned long)pkt->size;
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
	sceMp3Aac_close,
	NULL
};
