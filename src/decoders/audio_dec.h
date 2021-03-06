#ifndef __PMC_DECODERS_H__
#define __PMC_DECODERS_H__

#include <pspaudiocodec.h>
#include <psptypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct
{
	int (*decode)(s16 *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size);
	void (*close)(AVCodecContext *codec_ctx);
}AUDIO_DECODER;

#ifdef __cplusplus
}// extern "C"

class AUDIO_DECODERS
{
	short *tmpbuf;
	int tmpbuf_size;
	int tmpbuf_readpos;
	const AUDIO_DECODER *audio_dec;
public:
	AUDIO_DECODERS()
	: tmpbuf(NULL),
		tmpbuf_size(0),
		tmpbuf_readpos(0),
		audio_dec(NULL)
	{};
	~AUDIO_DECODERS(){ this->close(NULL); };
	
	bool open(AVCodecContext *codec_ctx,
							const char *filepath,
							AVIOContext* io_ctx);
	
	void close(AVCodecContext *codec_ctx);
	int decode(short *buf, AVCodecContext *codec_ctx, AVPacket *pkt, int size);
	bool isSceCodec() const
	{
		extern const AUDIO_DECODER ffmpegAudio;
		return audio_dec!=&ffmpegAudio;
	};
	
	//called when seeking
	void flush(AVCodecContext *codec_ctx);
	
	int get_int(int v);
};

#endif // __cplusplus

// S = short, C = char
#define C_AUDIOBUF_MAX_SIZE ( (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2 )
#define S_AUDIOBUF_MAX_SIZE ( C_AUDIOBUF_MAX_SIZE / 2 )
#define PSP_CODEC_WMA 0x1005
#define AAC_IS_ADTS 0x10000

// for tags
// used by the now playing gui
// TODO: album art
#define TAG_TITLE				0
#define TAG_ARTIST			1
#define TAG_ALBUM				2
#define TAG_COPYRIGHT		3
#define TAG_BITRATE			4
#define TAG_SAMPRATE		5


// for tags/info not fetched from ffmpeg
// should only be used internally by decoding code
#define NFF_TAG_BITRATE			0
#define NFF_TAG_SAMPRATE		1
#define NFF_TAG_DURATION		2
#define NFF_TAG_YEAR				3

#endif
