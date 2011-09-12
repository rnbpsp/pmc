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
}

extern const AUDIO_DECODER ffmpegAudio;
extern const AUDIO_DECODER sceMp3Aac;
extern const AUDIO_DECODER sceAsfWma;
extern const AUDIO_DECODER sceAtrac3;

char custom_tag[4][256];
int custom_info[4];


bool
AUDIO_DECODERS::open(
		AVCodecContext *codec_ctx,
		const char *filepath,
		AVIOContext* io_ctx,
		int filetype )
{
	if (codec_ctx==NULL)
	{
		if (filetype==PMC_PARSER_SCEWMA)
		{
			if (sceWma_open(filepath, io_ctx))
				audio_dec = &sceAsfWma;
		}
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
		/*
			case CODEC_ID_ATRAC3P:
				if (sceAtrac3p_open(codec_ctx))
					audio_dec = &sceAt3Aa3;
				break;
		*/
			default:
				break;
		}
	}
	
	if ( !audio_dec )
	{
		if ( codec_ctx!=NULL && ffmpegAdec_open(codec_ctx) )
			audio_dec = &ffmpegAudio;
	// no need to allocate buffers without an open decoder
		else return false;
	}
	
	printf("allocating tmp buffer:64\n");
	tmpbuf = (u8*)memalign(64, C_AUDIOBUF_MAX_SIZE);
	if (tmpbuf) return true;
	
	printf("failed allocating tmp buffer\n");
	this->close();
	
	return false;
}

void
AUDIO_DECODERS::close()
{
	memset(&custom_tag, 0, 4*256);
	if (audio_dec) audio_dec->close();
	audio_dec = NULL;
	free(tmpbuf);
	tmpbuf = NULL;
	tmpbuf_size = tmpbuf_readpos = 0;
};

int
AUDIO_DECODERS::decode(short *buf, AVPacket *pkt, int size)
{
	//printf("calling audio decoder caller\n");
	u32 buff = reinterpret_cast<u32>(buf);
	u32 tmp = reinterpret_cast<u32>(tmpbuf);
	int written = 0;
	for(;;)
	{
		if (tmpbuf_size!=0)
		{
				if (size<tmpbuf_size)
				{
					memcpy((void*)(buff + written), (void*)(tmp + tmpbuf_readpos), size);
					tmpbuf_readpos += size;
					tmpbuf_size -= size;
					return written + size;
				}
				else
				if (tmpbuf_size==size)
				{
					memcpy((void*)(buff + written), (void*)(tmp + tmpbuf_readpos), size);
					tmpbuf_readpos = tmpbuf_size = 0;
					return written + size;
				}
				else
				{
					memcpy((void*)(buff + written), (void*)(tmp + tmpbuf_readpos), tmpbuf_size);
					written += tmpbuf_size;
					size -= tmpbuf_size;
					tmpbuf_readpos = tmpbuf_size = 0;
				}
		}
		else if (pkt==NULL || pkt->size>0)
		{
			tmpbuf_size = audio_dec->decode((s16*)tmpbuf, pkt, C_AUDIOBUF_MAX_SIZE);
			if (tmpbuf_size<0)
			{
				tmpbuf_readpos = tmpbuf_size = 0;
				return written==0 ? -1 : written;
			}
		}
		else
			return written;
	}
	return written;
}

int64_t
AUDIO_DECODERS::seek(int64_t seconds)
{
	return audio_dec->seek(seconds);
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

const char *
AUDIO_DECODERS::get_str(int v)
{
	return custom_tag[v];
}
