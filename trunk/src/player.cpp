#include "utils.h"
#include "settings.h"
#include "player.h"
#include "drawing.h"
#include <cstring>
#include <pspthreadman.h>
#include <pspdisplay.h>

PMC_PLAYER player;

#define output_silence(buf, num) \
	for(int z=0; z<num; ++z) ((int*)buf)[z] = 0
//	memset((void*)buf, 0, num*4)

bool
PMC_PLAYER::sceaudio_callback(void *dest, int& written, AVPacket& packet)
{
	written = 0; //written number of samples
	short *dest0 = (short*)dest;
	while(written!=PMC_AUDIO_NUM_SAMPLES)
	{
			// remaining number of samples to write
			int remaining = PMC_AUDIO_NUM_SAMPLES-written;

			if (playing & PLAYER_PAUSED)
			{
				output_silence(dest0, remaining);
				return false;
			}
			
			int decoded = audio_decoder.decode((short*)dest0, 0, remaining*4);
			if (decoded<0)
			{
				// try two more times
				for(int i=0; i<2; ++i)
				{
					decoded = audio_decoder.decode((short*)dest0, 0, remaining*4);
					if (decoded>=0) goto retry_success;
				}
				output_silence(dest0, remaining);
				return true;
			}
			
retry_success:
			dest0 += decoded/2;
			written += decoded/4;
	}
	return false;
}

bool
PMC_PLAYER::ffaudio_callback(void *dest, int& written, AVPacket& packet)
{
	written = 0; //written number of samples
	short *dest0 = (short*)dest;
	int retries = NUMOF_RETRIES;
	
	while(written!=PMC_AUDIO_NUM_SAMPLES)
	{
			// remaining number of samples to write
			int remaining = PMC_AUDIO_NUM_SAMPLES-written;

			if (playing & PLAYER_PAUSED)
			{
				output_silence(dest0, remaining);
				return false;
			}
			
			if (packet.size<=0)
			{
					while( av_read_frame(format_ctx, &packet) >= 0 )
					{
						if( packet.stream_index == audio_stream )
							goto dec;
						
						else av_free_packet(&packet);
						
						if (playing & PLAYER_PAUSED)
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
			int decoded = audio_decoder.decode((short*)dest0, &packet, remaining*4);
			if (decoded<=0)
			{
				if (retries==0)
				{
					output_silence(dest0, remaining);
					return true;
				}
				else retries -= 1; // try two more times
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

//static
int audio_main(SceSize argc, void* argv)
{
	// i think double buffering is still necessary
	// sceAudioSrc only supports stereo?
	short buf[2][PMC_AUDIO_NUM_SAMPLES*2];//[2];
	
	// put this here so it'll exist until this thread is done
	AVPacket pkt;
	av_init_packet(&pkt);
	
	int curbuf = 0;
	while(player.playing != PLAYER_STOPPED)
	{
		bool end = false;
		int written = 0;
		
		if (player.did_seek)
    {
      pkt.size = 0;
      player.did_seek = false;
    }
		
		if (player.playing & PLAYER_PAUSED)
		{
			//output_silence(buf[curbuf], PMC_AUDIO_NUM_SAMPLES);
			sceDisplayWaitVblankStart();
			continue;
		}
		else
		{
			if (player.parser==PMC_PARSER_FFMPEG)
				end = player.ffaudio_callback(buf[curbuf], written, pkt);
			else
				end = player.sceaudio_callback(buf[curbuf], written, pkt);
		}
		player.frame_timer += written;
		sceAudioSRCOutputBlocking(player.volume, buf[curbuf]);
		if (end)
		{
			if (player.mode&PL_MODE_LOOP_ONE)
				player.seek(0);
			else player.playing = PLAYER_STOPPED;
		}
		curbuf ^= 1;
	}

	sceKernelExitThread(0);
	return 0;
}

// AVIOContext functions
/*
static
int pmc_avio_read(void *opaque, uint8_t *buf, int buf_size)
{
	return sceIoRead(reinterpret_cast<SceUID>(opaque), buf, buf_size);
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
		delete full_path; \
		this->close(); \
		return false; \
	}while(0)

extern "C" int check_ifwma(AVIOContext* fd);
Pmc_Image *load_albumArt(const char *file);
bool
PMC_PLAYER::open(const char *path, const char *name)
{
	settings.cpu = 333;
	this->close();
	
	char *full_path;
	SceUID fd;
	u8 *io_mem;
	
	filepath = strdup(path);
	filename = strdup(name);
	
	full_path = new char[strlen(filepath) + strlen(filename) + 1];
	strcpy(full_path, filepath);
	strcat(full_path, filename);
	
	album_art = load_albumArt(full_path);
	
	fd = sceIoOpen(full_path, PSP_O_RDONLY, 0);
	if (fd<0) error_openfile();
	
	io_mem = (u8*)av_malloc(PMC_BUFIO_SIZE);
	if (!io_mem)
	{
		printf("Not enough memory to alloc buffed io mem. size = " AV_STRINGIFY(PMC_BUFIO_SIZE) "\n");
		sceIoClose(fd);
		error_openfile();
	}
	
	io_ctx = avio_alloc_context(io_mem, PMC_BUFIO_SIZE, 0, \
												reinterpret_cast<void*>(fd), \
		reinterpret_cast<int (*)(void*, uint8_t*, int)>(&sceIoRead), \
												NULL,
		reinterpret_cast<int64_t (*)(void*, int64_t,int)>(&sceIoLseek));
	if (!io_ctx)
	{
		av_free(io_mem);
		sceIoClose(fd);
		error_openfile();
	}
	
	printf("opening file\n");
	if (strcasecmp(get_ext(name), "wma")==0 && check_ifwma(io_ctx))
		parser = PMC_PARSER_SCEWMA;
	else
	{
ffmpeg_fallback:
		parser = PMC_PARSER_FFMPEG;
	//	int ret = 0;
	//	if (!pmc_ff_open(format_ctx, io_ctx, full_path))
	//	{
	//		error_openfile();
	//	}
		
		format_ctx = avformat_alloc_context();
		if (!format_ctx)
		{
			error_openfile();
		}
		
		format_ctx->pb = io_ctx;
		format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
		
		int ret = avformat_open_input(&format_ctx, full_path, NULL, NULL);
		if(ret!=0)
		{
			// it is freed on error
			// see avformat_open_input@avformat.h
			format_ctx = NULL;
			printf("Error open file \"%s\", ret = %d\n", path, ret);
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
	
	channel = sceAudioSRCChReserve(PMC_AUDIO_NUM_SAMPLES, codec_ctx?codec_ctx->sample_rate:audio_decoder.get_int(NFF_TAG_SAMPRATE), 2);
	if (channel<0)
	{
		printf("Cannot reserve audio channel: 0x%08x\n", channel);
		show_error("Error: Cannot reserve audio channel.");
		error_openfile();
	}
	
	athread = sceKernelCreateThread("Audio Player Thread", \
										&audio_main, /*50*/0x12, 1024*1024, 0, NULL);
	if (athread<0)
	{
		printf("Error: cannot create audio decoder thread.\n");
		error_openfile();
	}
	
	if (stream_ptr!=NULL)
		duration = av_rescale(stream_ptr->duration, stream_ptr->time_base.num, stream_ptr->time_base.den); //stream_ptr->duration * av_q2d(stream_ptr->time_base);
	else
		duration = audio_decoder.get_int(NFF_TAG_DURATION);
	
	// just to make sure
	if (duration==0) error_openfile();
	
	playing = PLAYER_PLAYING;
	sceKernelStartThread(athread,0,NULL);
	
	delete full_path;
	return true;
}

void
PMC_PLAYER::close()
{
	playing = PLAYER_STOPPED;
	sceKernelDelayThread(1000);
	
	if (athread>=0) sceKernelTerminateDeleteThread(athread);
	if (channel>=0)
		while (sceAudioSRCChRelease() < 0) sceKernelDelayThread(0);
	
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
}

const char*
PMC_PLAYER::get_str(int tag)
{
	static char temp_str[512] = "";

	if (format_ctx==NULL && tag<=TAG_COPYRIGHT)
	{
		switch(tag)
		{
			case TAG_TITLE:
			{
				const char *from_tag = audio_decoder.get_str(TAG_TITLE);
				return strlen(from_tag) ? from_tag : filename;
			}
			case TAG_ARTIST:
			{
				return audio_decoder.get_str(TAG_ARTIST);
			}
			case TAG_ALBUM:
			{
				return audio_decoder.get_str(TAG_ALBUM);
			}
			case TAG_COPYRIGHT:
			{
				const char *from_tag = audio_decoder.get_str(TAG_COPYRIGHT);
				const int year = audio_decoder.get_int(NFF_TAG_YEAR);
				
				if (strlen(from_tag)>0)
				{
					if (year > 0)
						sprintf(temp_str, "%s, %d", from_tag, year);
					else
						return from_tag;
				}
				else
				{
					//sprintf(taginfo_buf, "%d", year);
					if (year>0) pmc_itoa(year, temp_str, 10);
					else return "";
				}
				return temp_str;
			}
		}
	}
	else
	{
		AVDictionaryEntry *avtag = NULL;
		switch(tag)
		{
		#define get_metadata(field) avtag = av_dict_get(format_ctx->metadata, field, NULL, 0)

			case TAG_TITLE:
				get_metadata("title");
				if (!avtag) return filename;
				if (avtag->value)
					return strlen(avtag->value) ? avtag->value : filename;
				else return filename;
			
			case TAG_ARTIST:
				get_metadata("artist");
				if (!avtag) return "";
				return (avtag->value) ? avtag->value : "";
			
			case TAG_ALBUM:
				get_metadata("album");
				if (!avtag) return "";
				return (avtag->value) ? avtag->value : "";
			
			case TAG_COPYRIGHT:
				get_metadata("copyright");
				if (!avtag) return "";
				return (avtag->value) ? avtag->value : "";
		
			case TIMER_STRING:
			{
				int64_t cur_sec =  this->get_time();
				const int seconds = cur_sec % 60;
				cur_sec /= 60;
				const int minutes = cur_sec % 60;
				cur_sec /= 60; // costs just a register move so put it here
				
				if ( duration < (60*60) )
					sprintf(temp_str, "%d:%02d", minutes, seconds);
				else
					sprintf(temp_str, "%lld:%02d:%02d", cur_sec, minutes, seconds);
				
				return temp_str;
			}
			case DURATION_STRING:
			{
				int64_t cur_sec =  duration;
				const int seconds = cur_sec % 60;
				cur_sec /= 60;
				const int minutes = cur_sec % 60;
				cur_sec /= 60;
				
				if ( cur_sec == 0 )
					sprintf(temp_str, "%d:%02d", minutes, seconds);
				else
					sprintf(temp_str, "%lld:%02d:%02d", cur_sec, minutes, seconds);
				
				return temp_str;
			}
			case TIMER_OVER_DURATION:
			{
				int64_t time_sec =  this->get_time();
				const int seconds = time_sec % 60;
				time_sec /= 60;
				const int minutes = time_sec % 60;
				time_sec /= 60;
				
				int64_t dur_sec =  duration;
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
				
				return temp_str;
			}
			
			case TAG_BITRATE:
				pmc_itoa((codec_ctx?codec_ctx->bit_rate:audio_decoder.get_int(NFF_TAG_BITRATE))/1000, temp_str, 10);
				strcat(temp_str, " kbps");
				return temp_str;
			
			case TAG_SAMPRATE:
				pmc_itoa((codec_ctx?codec_ctx->sample_rate:audio_decoder.get_int(NFF_TAG_SAMPRATE)), temp_str, 10);
				strcat(temp_str, " Hz");
				return temp_str;
			default:
				return "";
		}
	}	
	//return filename;
	return "";
}

void
PMC_PLAYER::seek(int64_t seconds)
{
	playing |= PLAYER_PAUSED; // to pause decoder thread
	sceKernelDelayThread(1000);
	
	if (this->parser==PMC_PARSER_FFMPEG)
	{
	/*
		register int seek_flag = 0;
		if (seconds<0) seek_flag = AVSEEK_FLAG_BACKWARD;
		seconds = __builtin_abs(seconds);
		
		int64_t ts = seconds / av_q2d(stream_ptr->time_base);
		
		av_seek_frame(format_ctx, audio_stream, ts, seek_flag);
	*/
		int64_t target = av_rescale(seconds, stream_ptr->time_base.den, stream_ptr->time_base.num);
		if (avformat_seek_file(format_ctx, audio_stream, 0, target, target, AVSEEK_FLAG_FRAME) <0 )
		{
			playing ^= PLAYER_PAUSED;
			return;
		}
		seconds = av_rescale(target, stream_ptr->time_base.num, stream_ptr->time_base.den);
	}
	else
	{
		int64_t ret = audio_decoder.seek(seconds);
		if ( ret < 0 )
		{
			playing ^= PLAYER_PAUSED;
			return;
		}
		seconds = ret;
	}
	
	audio_decoder.flush(codec_ctx);
	frame_timer =  seconds*( codec_ctx?codec_ctx->sample_rate:audio_decoder.get_int(NFF_TAG_SAMPRATE) );
	did_seek = true;
	playing ^= PLAYER_PAUSED;
}
