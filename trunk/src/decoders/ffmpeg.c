//#include "main.h"
#include "./audio_dec.h"
/*
static
void ffmpegAdec_close(AVCodecContext *codec_ctx)
{
	avcodec_close(codec_ctx);
}
*/
static
int ffmpegAdec_decode(s16 *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size)
{
	int frame_size_ptr = size;
	int len = avcodec_decode_audio3( codec_ctx,
													buf, &frame_size_ptr, pkt );
//	int len = avcodec_decode_audio2( codec_ctx,
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
	return frame_size_ptr;
}


int ffmpegAdec_open(AVCodecContext *ctx)
{
	printf("finding audio codec\n");
	AVCodec *codec = avcodec_find_decoder(ctx->codec_id);
	if (!codec) return 0;
	
	if(codec->capabilities & CODEC_CAP_TRUNCATED)
		ctx->flags |= CODEC_FLAG_TRUNCATED;
	
	ctx->flags2 |= CODEC_FLAG2_CHUNKS;
	
	if (ctx->channel_layout!=AV_CH_LAYOUT_STEREO)
		ctx->request_channel_layout = AV_CH_LAYOUT_STEREO;
	
	ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;
	
	printf("Opening codec\n");
	if (avcodec_open(ctx, codec)<0)
		return 0;
	
	return 1;
}

const
AUDIO_DECODER ffmpegAudio =
{
	ffmpegAdec_decode,
	(void(*)(AVCodecContext*))avcodec_close
};
