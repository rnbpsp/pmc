#include "utils.h"
#include "settings.h"
#include "player.h"
#include "drawing.h"
#include <cstring>
#include <pspthreadman.h>
#include <pspdisplay.h>
#include <psputility.h>
#include <pspvaudio.h>
#include <cooleyesBridge.h>

PMC_PLAYER player;

#define output_silence(buf, num) \
	memset((void*)buf, 0, num*4)


bool
PMC_PLAYER::audio_callback(void *dest, int& written, AVPacket* packet)
{
	written = 0; //written number of samples
	short *dest0 = (short*)dest;
	int retries = NUMOF_RETRIES;
	
	while(written!=PMC_AUDIO_NUM_SAMPLES)
	{
			// remaining number of samples to write
			int remaining = PMC_AUDIO_NUM_SAMPLES-written;

			if (player.playing & PLAYER_PAUSED)
			{
				output_silence(dest0, remaining);
				return false;
			}
			
			if (packet && packet->size<=0)
			{
					while( av_read_frame(player.format_ctx, packet) >= 0 )
					{
						if( packet->stream_index == player.audio_stream )
							goto dec;
						
						else av_free_packet(packet);
						
						if (player.playing & PLAYER_PAUSED)
						{
							output_silence(dest0, remaining);
							return false;
						}
						
			//			sceKernelDelayThread(0);
					}
					output_silence(dest0, remaining);
					return true;
			}
			
dec:
			int decoded = player.audio_decoder.decode((short*)dest0, player.codec_ctx, packet, remaining*4);
			if (decoded<=0)
			{
				if (retries==0)
				{
					output_silence(dest0, remaining);
					return true;
				}
				else retries -= 1; // try _NUMOF_RETRIES_ more times
			}
			else
			{
				retries = NUMOF_RETRIES;
				dest0 += decoded/2;
				written += decoded/4;
			}
	}
	return false;
}


int PMC_PLAYER::audio_main(SceSize argc, void* argv)
{
	// sceAudioSrc only supports stereo?
	short buf[2][PMC_AUDIO_NUM_SAMPLES*2];//[2];
	
	// put this here so it'll exist until this thread is done
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	int curbuf = 0;
	while(player.playing != PLAYER_STOPPED)
	{
		bool end = false;
		int written = 0;
		
		if (player.did_seek)
    {
			if (player.parser==PMC_PARSER_FFMPEG)
			{
				pkt.data = NULL;
				pkt.size = 0;
			}
      player.did_seek = false;
    }
		
		if (player.playing & PLAYER_PAUSED)
		{
			//output_silence(buf[curbuf], PMC_AUDIO_NUM_SAMPLES);
			sceDisplayWaitVblankStart();
			continue;
		}
		else
			end = player.audio_callback(buf[curbuf], written, player.parser==PMC_PARSER_FFMPEG?&pkt:NULL);

		player.frame_timer += written;
		if (player.channel==8)
			sceAudioSRCOutputBlocking(player.volume, buf[curbuf]);
		else
		{
			int rest = sceAudioGetChannelRestLen(player.channel);
			if (rest > 0)
			{
				rest = (float)rest*(player.get_samprate()==44100?(1000000.f/44100.f):(1000000.f/48000.f));
				if (rest > 150) sceKernelDelayThread(rest-100);
				
				while (sceAudioGetChannelRestLen(player.channel)>0);
			}
			
			if (!end)
				sceAudioOutput(player.channel, player.volume, buf[curbuf]);
			else
				sceAudioOutputBlocking(player.channel, player.volume, buf[curbuf]);
		}
		
		curbuf ^= 1;
		if (end)
		{
			if (strcasecmp(get_ext(player.filename), "mp3") && (player.mode&PL_MODE_LOOP_ONE) && player.seek(0))
				continue;
			player.playing = PLAYER_STOPPED;
		}
	}
	
	sceKernelExitThread(0);
	return 0;
}

/*
// AVIOContext functions
static
int pmc_avio_read(void *opaque, uint8_t *buf, int buf_size)
{
	SceUID fd = reinterpret_cast<SceUID>(opaque);
	if (sceIoReadAsync(fd, buf, buf_size)<0) return -1;
	
	int64_t asyncres;
	if (sceIoWaitAsync(fd, &asyncres)<0) return -1;
	return (int)asyncres;
}

static
int64_t pmc_avio_seek(void *opaque, int64_t offset, int whence)
{
#define seek(x,y) sceIoLseek(reinterpret_cast<SceUID>(opaque), x, y)

	if (whence==AVSEEK_SIZE)
	{
		const int64_t pos = seek(0,SEEK_CUR); // save current position
		const int64_t size = seek(0,SEEK_END); // seek end to get file size
		seek(pos,SEEK_SET); // get back to current position
		return size;
	}
	
	return seek(offset,whence);
#undef seek
}
*/
#define error_openfile() \
	do { \
		this->close(); \
		return false; \
	}while(0)
//		delete full_path;

#include <libccc_mod.h>
extern void fill_tags(AVDictionary *metadata, const char *fullpath, const cccCode *filename);
extern "C" int check_ifwma(AVIOContext* fd);
Pmc_Image *load_albumArt(const char *file);
bool
PMC_PLAYER::open(const char *path, const char *name)
{
	settings.cpu = 333;
	this->close();
	
	SceUID fd;
	u8 *io_mem;
	
	filepath = strdup(path);
	filename = strdup(name);
	
	char full_path[strlen(filepath) + strlen(filename) + 1];
	strcpy(full_path, filepath);
	strcat(full_path, filename);
	
	album_art = load_albumArt(full_path);
	
	fd = sceIoOpen(full_path, PSP_O_RDONLY, 0);
	if (fd<0) error_openfile();/*
	if (sceIoChangeAsyncPriority(fd, 0x11) < 0)
		error_openfile();*/
	
	io_mem = (u8*)av_malloc(PMC_BUFIO_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
	if (!io_mem)
	{
		printf("Not enough memory to alloc buffed io mem. size = " AV_STRINGIFY(PMC_BUFIO_SIZE) "\n");
		sceIoClose(fd);
		error_openfile();
	}
	
	io_ctx = avio_alloc_context(io_mem, PMC_BUFIO_SIZE, 0, \
												reinterpret_cast<void*>(fd), \
		reinterpret_cast<int (*)(void*, uint8_t*, int)>(&sceIoRead/*pmc_avio_read*/), \
												NULL,
		reinterpret_cast<int64_t (*)(void*, int64_t,int)>(&sceIoLseek));
	if (!io_ctx)
	{
		av_free(io_mem);
		sceIoClose(fd);
		error_openfile();
	}
	
	printf("opening file\n");
	if (check_ifwma(io_ctx))
		parser = PMC_PARSER_SCEWMA;
	else
	{
ffmpeg_fallback:
		parser = PMC_PARSER_FFMPEG;
		
		format_ctx = avformat_alloc_context();
		if (!format_ctx) error_openfile();
		
		format_ctx->pb = io_ctx;
		format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
		
		int ret = avformat_open_input(&format_ctx, full_path, NULL, NULL);
		if(ret!=0)
		{
			// it is freed on error
			// see avformat_open_input@avformat.h
			format_ctx = NULL;
	//		char err_buf[256] = "";
			printf("Error open file \"%s\", ret = 0x%08x, nomem = %d\n", path, ret, ret == AVERROR(ENOMEM)?1:0);
	//		av_strerror(ret, err_buf, 256);
	//		err_buf[254] = '\n';
	//		err_buf[255] = '\0';
	//		printf(err_buf);
			error_openfile();
		}
		
		printf("finding stream info\n");
		format_ctx->max_analyze_duration = 5000000/8;
		// Retrieve stream information
		ret = av_find_stream_info(format_ctx);
		if(ret<0)
		{
			printf("Error finding stream info \"%s\", ret = %d\n", name, ret);
			error_openfile();
		}
		
		for(unsigned int i=0; i<format_ctx->nb_streams; ++i)
			if (format_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
			{
				audio_stream = i;
				break;
			}

		if (audio_stream<0)
		{
			show_error("Error: cannot open file.\nNo audio stream found.");
			error_openfile();
		}
		
		stream_ptr = format_ctx->streams[audio_stream];
		codec_ctx = stream_ptr->codec;
		
		if (codec_ctx->codec_id!=CODEC_ID_ATRAC3P && \
				codec_ctx->sample_fmt!=SAMPLE_FMT_S16)
		{
			printf("sample format is not S16.\n");
			error_openfile();
		}
	}
	
	if (!audio_decoder.open(codec_ctx, full_path, io_ctx, parser))
	{
		if (codec_ctx==NULL)
		{
			printf("Opening decoder failed, falling back to ffmpeg.\n");
			avio_seek(io_ctx, 0, SEEK_SET);
			goto ffmpeg_fallback; // haven't tried ffmpeg
		}
		show_error("Error: cannot open decoder.\nIt's either file is unsupported\nor there's not enough memory.");
		error_openfile();
	}
	
	// TODO: 48khz
	if (get_samprate()==44100 || get_samprate()==48000)
	{
		if (cooleyesAudioSetFrequency(sceKernelDevkitVersion(), get_samprate())<0)
			goto try_asrc;
		channel = sceAudioChReserve(-1, PMC_AUDIO_NUM_SAMPLES, PSP_AUDIO_FORMAT_STEREO);
		if (channel<0) goto try_asrc;
	}
	else
	{
try_asrc:
		channel = sceAudioSRCChReserve(PMC_AUDIO_NUM_SAMPLES, get_samprate(), 2);
		if (channel<0)
		{
			printf("Cannot reserve audio channel: 0x%08x\n", channel);
			show_errorEx("Error: Cannot reserve audio channel.\n\tsceAudioSRCChReserve = 0x%08x\n\tSample rate = %d", channel, get_samprate());
			error_openfile();
		}
		channel = 8;
	}
	
	athread = sceKernelCreateThread("Audio Player Thread", \
		reinterpret_cast<int (*)(SceSize, void*)>(&PMC_PLAYER::audio_main),
														0x12, 1024*1024, \
						PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU, NULL);
	if (athread<0)
	{
		printf("Error: cannot create audio decoder thread.\n");
		error_openfile();
	}
	
	if (stream_ptr!=NULL)
		duration = av_rescale(stream_ptr->duration, stream_ptr->time_base.num, stream_ptr->time_base.den);
	else
		duration = audio_decoder.get_int(NFF_TAG_DURATION);
	
	// just to make sure
	if (duration==0) error_openfile();
	
	fill_tags(format_ctx?format_ctx->metadata:NULL, full_path, (const cccCode*)filename);
	
	playing = PLAYER_PLAYING;
	sceKernelStartThread(athread,0,NULL);
	
//	delete full_path;
	return true;
}

void
PMC_PLAYER::close()
{
	playing = PLAYER_STOPPED;
	sceKernelDelayThread(1000);
	
	if (athread>=0) sceKernelTerminateDeleteThread(athread);
	if (channel==8)
		while (sceAudioSRCChRelease() < 0)
			sceKernelDelayThread(100);
	else if (channel>=0)
	{
		while (sceAudioGetChannelRestLen(channel) > 0)
			sceKernelDelayThread(100);
		
		sceAudioChRelease(channel);
	}
	
	duration = frame_timer = 0;
	channel = athread = audio_stream = -1;
	
	audio_decoder.close();
	codec_ctx = NULL;
	
	if (format_ctx!=NULL)
	{
		av_close_input_file(format_ctx);
		format_ctx = NULL;
		stream_ptr = NULL;
	}
	
	if (io_ctx!=NULL)
	{
		const SceUID fd = reinterpret_cast<SceUID>(io_ctx->opaque);
		if (fd>=0) sceIoClose(fd);
		av_free(io_ctx->buffer);
		av_freep(&io_ctx);
	}
	
	delete album_art;
	album_art = NULL;
	
	parser = PMC_PARSER_FFMPEG;
	
	free(filepath);
	free(filename);
	
	filepath = filename = NULL;
	
	extern u16 tag_info[6][256];
	memset(tag_info, 0, sizeof(tag_info));
}

const u16*
PMC_PLAYER::get_str(int tag)
{
	char temp_str[256] = "";
	static u16 temp_ustr[256];
	extern u16 tag_info[6][256];

	if (tag<=TAG_SAMPRATE)
		return tag_info[tag];
	else
	{
		switch(tag)
		{
			case TIMER_STRING:
			{
				int64_t cur_sec =  this->get_time();
				const int seconds = cur_sec % 60;
				cur_sec /= 60;
				const int minutes = cur_sec % 60;
				cur_sec /= 60;
				
				if ( duration < (60*60) )
					sprintf(temp_str, "%d:%02d", minutes, seconds);
				else
					sprintf(temp_str, "%lld:%02d:%02d", cur_sec, minutes, seconds);
				
				break;
			}
			case DURATION_STRING:
			{
				int64_t cur_sec = duration;
				const int seconds = cur_sec % 60;
				cur_sec /= 60;
				const int minutes = cur_sec % 60;
				cur_sec /= 60;
				
				if ( cur_sec == 0 )
					sprintf(temp_str, "%d:%02d", minutes, seconds);
				else
					sprintf(temp_str, "%lld:%02d:%02d", cur_sec, minutes, seconds);
				
				break;
			}
			case TIMER_OVER_DURATION:
			{
				int64_t time_sec =  this->get_time();
				const int seconds = time_sec % 60;
				time_sec /= 60;
				const int minutes = time_sec % 60;
				time_sec /= 60;
				
				int64_t dur_sec = duration;
				const int dur_seconds = dur_sec % 60;
				dur_sec /= 60;
				const int dur_minutes = dur_sec % 60;
				dur_sec /= 60;
				
				if ( dur_sec==0 )
					sprintf(temp_str, "%d:%02d" " / " "%d:%02d", minutes, seconds, dur_minutes, dur_seconds);
				else
					sprintf(temp_str, "%lld:%02d:%02d" " / " "%lld:%02d:%02d", \
									time_sec, minutes, seconds, \
									dur_sec, dur_minutes, dur_seconds);
				
				break;
			}
		}
		int tlen = strlen(temp_str);
		cccUTF8toUCS2(temp_ustr, tlen, (const cccCode*)temp_str);
		temp_ustr[tlen] = 0;
		return temp_ustr;
	}
	//return filename;
	return temp_ustr;
}

bool
PMC_PLAYER::seek(int64_t seconds)
{
	playing |= PLAYER_PAUSED; // pause decoder thread
	sceKernelDelayThread(1000);
	
	if (this->parser==PMC_PARSER_FFMPEG)
	{
		int64_t target = av_rescale(seconds, stream_ptr->time_base.den, stream_ptr->time_base.num);
		if (avformat_seek_file(format_ctx, audio_stream, 0, target, target, AVSEEK_FLAG_FRAME) <0 )
		{
			playing ^= PLAYER_PAUSED;
			return false;
		}
		seconds = av_rescale(target, stream_ptr->time_base.num, stream_ptr->time_base.den);
	}
	else
	{
		int64_t ret = audio_decoder.seek(seconds);
		if ( ret < 0 )
		{
			playing ^= PLAYER_PAUSED;
			return false;
		}
		seconds = ret;
	}
	
	audio_decoder.flush(codec_ctx);
	frame_timer =  seconds*( codec_ctx?codec_ctx->sample_rate:audio_decoder.get_int(NFF_TAG_SAMPRATE) );
	did_seek = true;
	playing ^= PLAYER_PAUSED;
	return true;
}
