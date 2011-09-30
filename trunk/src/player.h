#ifndef PMC_PLAYER_H
#define PMC_PLAYER_H

#include "utils.h"
#include "image.h"

extern "C"
{
	#include <malloc.h>
	#include <libavutil/avutil.h>
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

#include <pspaudio.h>
#include "decoders/audio_dec.h"

// number of audio samples per output per channel (17-4111)
#define PMC_AUDIO_NUM_SAMPLES 4096

// ffmpeg's buffed io is kind of unstable
// after seeking back to zero from eof
#define PMC_BUFIO_SIZE (1024*1024)

// number of times to retry decoding if decoder returns an error
#define NUMOF_RETRIES 3

class PMC_PLAYER
{
	AVFormatContext *format_ctx;
	AVCodecContext *codec_ctx;
	AVStream *stream_ptr;
	
	AVIOContext *io_ctx;

	AUDIO_DECODERS audio_decoder;
	
	int channel, audio_stream;
	SceUID athread;
	bool did_seek;
	
	int64_t duration; // duration in seconds
	int64_t frame_timer; // number of samples played
	
	static bool audio_callback(void *dest, int& written, AVPacket* packet);
	static int audio_main(SceSize argc, void* argv);
//	friend int show_nowplaying(const char *path, const char *name);
public:
	char *filepath, *filename;
	Pmc_Image *album_art;
	
	// TODO: only asf/wma needs special attention
	// use libavformat to fetch frames
#define PMC_PARSER_FFMPEG 0
	// some sce codecs needs sce parser
#define PMC_PARSER_SCEWMA 1
//#define PMC_PARSER_SCEAT3 2
	int parser;

// playing
#define PLAYER_STOPPED	0
#define PLAYER_PLAYING	1
#define PLAYER_PAUSED		2

// mode
#define PL_MODE_LOOP_ONE 1
#define PL_MODE_LOOP_ALL 2
#define PL_MODE_CHECK_LOOP 3

#define PL_MODE_SHUFFLE  4
	int volume, playing, mode;
	
	PMC_PLAYER()
	{
		memset(this, 0, sizeof(this));
		channel = audio_stream = athread = -1;
		volume = PSP_AUDIO_VOLUME_MAX;
		av_register_all();
	};
	
	~PMC_PLAYER()
	{
		close();
	};
	
	bool open(const char *path, const char *name);
	void close();
	
	void pause()
	{
		if (playing!=PLAYER_STOPPED)
			playing ^= PLAYER_PAUSED;
	};
	
	void loop()
	{
#define REMOVE_LOOP (~PL_MODE_CHECK_LOOP)
	
		if (mode & PL_MODE_LOOP_ONE) mode = (mode & REMOVE_LOOP) | PL_MODE_LOOP_ALL;
		else	if (mode & PL_MODE_LOOP_ALL) mode &= REMOVE_LOOP;
		else mode = (mode & REMOVE_LOOP) | PL_MODE_LOOP_ONE;
	};
	
	/*
	int get_channels()
	{
		return (codec_ctx?codec_ctx->channels:audio_decoder.get_int(NFF_TAG_CHANNELS));
	};*/
	int get_bitrate()
	{
		return (codec_ctx?codec_ctx->bit_rate:audio_decoder.get_int(NFF_TAG_BITRATE))/1000;
	};
	
	int get_samprate()
	{
		return (codec_ctx?codec_ctx->sample_rate:audio_decoder.get_int(NFF_TAG_SAMPRATE));
	};
	
	int get_time() // returns time in seconds
	{
		return frame_timer / get_samprate();
	};
	
	float get_percentRemaining()
	{
    return frame_timer / (float)(duration * get_samprate());
	};
	float get_percentRemaining(int64_t seconds)
	{
		return seconds / (float)duration;
	};
	
	bool exceed_duration(int64_t seconds){return seconds>duration;};
	
	bool seek(int64_t seconds);
	
// see decoders/audio_dec.h
#define TIMER_STRING		6
#define DURATION_STRING 7
#define TIMER_OVER_DURATION 8
	const u16 *get_str(int tag);
};
extern PMC_PLAYER player;

#endif //PMC_PLAYER_H
