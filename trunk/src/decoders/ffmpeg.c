//#include "main.h"
#include "./audio_dec.h"

static AVCodecContext *codec_ctx_ = NULL;
static AVCodec *codec_ = NULL;

static
void ffmpegAdec_close()
{
	avcodec_close(codec_ctx_);
	codec_ctx_ = NULL;
	codec_ = NULL;
}

static
int ffmpegAdec_decode(s16 *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size)
{
	int frame_size_ptr = size;
	int len = avcodec_decode_audio3( codec_ctx,
													buf, &frame_size_ptr, pkt );
//	int len = avcodec_decode_audio2( codec_ctx_,
//													buf, &frame_size_ptr, pkt->data, pkt->size );
	
	// if error, skip packet
	if (len<0)
	{
		pkt->size = 0;
		return -1;
	}
	
	pkt->data += len;
	pkt->size -= len;
	
	if (frame_size_ptr<0)
		printf("ffmpeg decoder error: 0x%08x, %d\n", frame_size_ptr, frame_size_ptr);
	codec_ctx_ = codec_ctx;
	return frame_size_ptr;
}


int ffmpegAdec_open(AVCodecContext *ctx)
{
	printf("finding audio codec\n");
	codec_ = avcodec_find_decoder(ctx->codec_id);
	if (!codec_) return 0;
	
	if(codec_->capabilities & CODEC_CAP_TRUNCATED)
		ctx->flags |= CODEC_FLAG_TRUNCATED;
	
	printf("Opening codec\n");
	if (avcodec_open(ctx, codec_)<0)
	{
		codec_ = NULL;
		return 0;
	}
	
	// pspaudiolib only outputs 44100 stereo shorts
	//if (codec_ctx_->sample_rate!=44100)
	
//	codec_ctx_ = ctx;
//	ctx->parse_only = 1;
	return 1;
}

const
AUDIO_DECODER ffmpegAudio =
{
	ffmpegAdec_decode,
	ffmpegAdec_close,
	NULL
};