#include "../utils.h"
#include "./audio_dec.h"
#include "../player.h"
extern "C"
{
	#include <malloc.h>

// codec's open function is responsible for cleaning up in case of error
	int ffmpegAdec_open(AVCodecContext *ctx);
	int sceMp3Aac_open(AVCodecContext *ctx, int type);
	int sceWma_open(const char *filename, AVIOContext* io_ctx);
	int sceAtrac3_open(AVCodecContext *ctx);
	int sceAtrac3p_open(AVCodecContext *ctx);
}

extern const AUDIO_DECODER ffmpegAudio;
extern const AUDIO_DECODER sceMp3Aac;
extern const AUDIO_DECODER sceAsfWma;
extern const AUDIO_DECODER sceAtrac3;
extern const AUDIO_DECODER sceAtrac3p;

int custom_info[4];


bool
AUDIO_DECODERS::open(
		AVCodecContext *codec_ctx,
		const char *filepath,
		AVIOContext* io_ctx )
{
#if _USE_SCE_DECODERS
	if (codec_ctx==NULL)
	{
		if (sceWma_open(filepath, io_ctx))
			audio_dec = &sceAsfWma;
	}
	else
	{
		switch (codec_ctx->codec_id)
		{
			case CODEC_ID_MP2:
			case CODEC_ID_MP3: // NOTE: this may also be the codec ID for mpeg 1 and 2
				if (sceMp3Aac_open(codec_ctx, PSP_CODEC_MP3))
					audio_dec = &sceMp3Aac;
				break;
			case CODEC_ID_AAC:
				if (sceMp3Aac_open(codec_ctx, PSP_CODEC_AAC|((strcasecmp(get_ext(filepath), "aac")==0)?AAC_IS_ADTS:0)))
					audio_dec = &sceMp3Aac;
				break;
			case CODEC_ID_ATRAC3:
				if (sceAtrac3_open(codec_ctx))
					audio_dec = &sceAtrac3;
				break;
			case CODEC_ID_ATRAC3P:
				if (sceAtrac3p_open(codec_ctx))
					audio_dec = &sceAtrac3p;
				break;
			default:
				break;
		}
	}
#endif //_USE_SCE_DECODERS
	
	if ( !audio_dec )
	{
		if ( codec_ctx!=NULL && ffmpegAdec_open(codec_ctx) )
			audio_dec = &ffmpegAudio;
	// no need to allocate buffers without an open decoder
		else return false;
	}
	
	// sceAsfParser needs 64byte alignment
	tmpbuf = (short*)memalign(64, C_AUDIOBUF_MAX_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
	if (tmpbuf) return true;
	
	printf("failed allocating tmp buffer\n");
	this->close(codec_ctx);
	
	return false;
}

void
AUDIO_DECODERS::close(AVCodecContext *codec_ctx)
{
	memset(&custom_info, 0, 4*4);
	if (audio_dec) audio_dec->close(codec_ctx);
	audio_dec = NULL;
	free(tmpbuf); tmpbuf = NULL;
	tmpbuf_size = tmpbuf_readpos = 0;
};

int
AUDIO_DECODERS::decode(short *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size)
{
	//printf("calling audio decoder caller\n");
	u32 buff = (u32)buf;
	u32 tmp = (u32)tmpbuf;
	int written = 0;
	for(;;)
	{
		if (tmpbuf_size!=0)
		{
			short *bufff = (short*)(buff+written);
			const short *tmpp = (short*)(tmp+tmpbuf_readpos);
			int opsize;
			if (0)//(codec_ctx && codec_ctx->channels==1) //crashing without report
			{
				opsize = (size>>1)<=tmpbuf_size ? size>>1 : tmpbuf_size;
				
				for (int i=0;i<opsize;++i)
					bufff[i*2] = bufff[i*2+1] = tmpp[i];
				
				opsize<<=1;
				written += opsize<<1;
				size -= opsize<<1;
			}
			else
			{
				opsize = size<=tmpbuf_size ? size : tmpbuf_size;
				memcpy(bufff, tmpp, opsize);
				written += opsize;
				size -= opsize;
			}
			
			tmpbuf_readpos += opsize;
			tmpbuf_size -= opsize;
			if (size==0) break;
		}
		else if (!codec_ctx || pkt->size>0)
		{
			tmpbuf_readpos=0;
			tmpbuf_size = audio_dec->decode(tmpbuf, codec_ctx, pkt, C_AUDIOBUF_MAX_SIZE);
			if (tmpbuf_size<0)
			{
				tmpbuf_size = 0;
				return written==0 ? -1 : written;
			}
		}
		else break;
	}
	return written;
}

void
AUDIO_DECODERS::flush(AVCodecContext *codec_ctx)
{
  tmpbuf_size = tmpbuf_readpos = 0;
  
  // only needed if we use ffmpeg for decoding
  // else will crash
  if ( codec_ctx && !isSceCodec() )
    avcodec_flush_buffers(codec_ctx);
}

int
AUDIO_DECODERS::get_int(int v)
{
	return custom_info[v];
}
