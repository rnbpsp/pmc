#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <psppower.h>
#include <pspdebug.h>
#include <psprtc.h>
#include <pspsdk.h>
#include <pspaudiocodec.h>
#include <pspaudio.h>
#include <string.h>
#include <malloc.h>

int SetupCallbacks();

PSP_MODULE_INFO("AT3 decodeTest", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

__attribute__ ((constructor))
void loaderInit(){
	pspKernelSetKernelPC();
	pspSdkInstallNoDeviceCheckPatch();
	pspSdkInstallNoPlainModuleCheckPatch();
	pspSdkInstallKernelLoadModulePatch();
}

SceCtrlData input;

#define TYPE_ATRAC3 0x270
#define TYPE_ATRAC3PLUS 0xFFFE

unsigned long at3_codec_buffer[65] __attribute__((aligned(64)));
short at3_mix_buffer[2048 * 2] __attribute__((aligned(64)));


SceUID at3_handle;
u16 at3_type;
u8* at3_data_buffer;
u16 at3_data_align;
u32 at3_sample_per_frame;
u16 at3_channel_mode;
u8 at3_at3plus_flagdata[2];
u32 at3_data_start;
u32 at3_data_size;
u8 at3_getEDRAM;
u32 at3_channels;
u32 at3_samplerate;


int main(void)
{
	SetupCallbacks();
	
	int result = pspSdkLoadStartModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL);
	pspSdkFixupImports(result);
	
	SceUID at3_handle = sceIoOpen("ms0:/Test.AT3", PSP_O_RDONLY, 0777);
	if (  ! at3_handle )
		goto wait;
	
	u32 riff_header[2];
	if ( sceIoRead( at3_handle, riff_header, 8 ) != 8 ) 
		goto wait;
	if ( riff_header[0] != 0x46464952 )
		goto wait;
	u32 wavefmt_header[3];
	if ( sceIoRead( at3_handle, wavefmt_header, 12 ) != 12 ) 
		goto wait;
	if ( wavefmt_header[0] != 0x45564157 || wavefmt_header[1] != 0x20746D66 )
		goto wait;
	u8* wavefmt_data = (u8*)malloc(wavefmt_header[2]);
	if ( wavefmt_data == NULL )
		goto wait;
	if ( sceIoRead( at3_handle, wavefmt_data, wavefmt_header[2] ) != wavefmt_header[2] ) {
		free(wavefmt_data);
		goto wait;
	}
	at3_type = *((u16*)wavefmt_data);
	at3_channels = *((u16*)(wavefmt_data+2));
	at3_samplerate = *((u32*)(wavefmt_data+4));
	at3_data_align = *((u16*)(wavefmt_data+12));
	
	if ( at3_type == TYPE_ATRAC3PLUS) {
		at3_at3plus_flagdata[0] = wavefmt_data[42];
		at3_at3plus_flagdata[1] = wavefmt_data[43];
	}
	
	free(wavefmt_data);
	
	u32 data_header[2];
	if ( sceIoRead( at3_handle, data_header, 8 ) != 8 ) 
		goto wait;
	while(data_header[0] != 0x61746164 ) {
		sceIoLseek32(at3_handle, data_header[1], PSP_SEEK_CUR);
		if ( sceIoRead( at3_handle, data_header, 8 ) != 8 ) 
			goto wait;
	}
	
	at3_data_start = sceIoLseek32(at3_handle, 0, PSP_SEEK_CUR);
	at3_data_size = data_header[1];
	
	if ( at3_data_size % at3_data_align != 0 )
		goto wait;
	
	memset(at3_codec_buffer, 0, sizeof(at3_codec_buffer));
	
	if ( at3_type == TYPE_ATRAC3 ) {
		at3_channel_mode = 0x0;
		if ( at3_data_align == 0xC0 ) // atract3 have 3 bitrate, 132k,105k,66k, 132k align=0x180, 105k align = 0x130, 66k align = 0xc0
			at3_channel_mode = 0x1;
		at3_sample_per_frame = 1024; 
		at3_data_buffer = (u8*)memalign(64, 0x180);
		if ( at3_data_buffer == NULL)
			goto wait;
		at3_codec_buffer[26] = 0x20;
		if ( sceAudiocodecCheckNeedMem(at3_codec_buffer, 0x1001) < 0 ) 
			goto wait;
		if ( sceAudiocodecGetEDRAM(at3_codec_buffer, 0x1001) < 0 )
			goto wait;
		at3_getEDRAM = 1;
		at3_codec_buffer[10] = 4;
		at3_codec_buffer[44] = 2;
		if ( at3_data_align == 0x130 )
			at3_codec_buffer[10] = 6;
		if ( sceAudiocodecInit(at3_codec_buffer, 0x1001) < 0 ) {
			goto wait;
		}
	}
	else if ( at3_type == TYPE_ATRAC3PLUS ) {
		at3_sample_per_frame = 2048;
		int temp_size = at3_data_align+8;
		int mod_64 = temp_size & 0x3f;
		if (mod_64 != 0) temp_size += 64 - mod_64;
		at3_data_buffer = (u8*)memalign(64, temp_size);
		if ( at3_data_buffer == NULL)
			goto wait;
		at3_codec_buffer[5] = 0x1;
		at3_codec_buffer[10] = at3_at3plus_flagdata[1];
		at3_codec_buffer[10] = (at3_codec_buffer[10] << 8 ) | at3_at3plus_flagdata[0];
		at3_codec_buffer[12] = 0x1;
		at3_codec_buffer[14] = 0x1;
		if ( sceAudiocodecCheckNeedMem(at3_codec_buffer, 0x1000) < 0 ) 
			goto wait;
		if ( sceAudiocodecGetEDRAM(at3_codec_buffer, 0x1000) < 0 )
			goto wait;
		at3_getEDRAM = 1;
		if ( sceAudiocodecInit(at3_codec_buffer, 0x1000) < 0 ) {
			goto wait;
		}
	}
	else
		goto wait;
	
	int eof = 0;	
	while( !eof ) {
		int samplesdecoded;
		memset(at3_mix_buffer, 0, 2048*2*2);
		unsigned long decode_type = 0x1001;
		if ( at3_type == TYPE_ATRAC3 ) {
			memset( at3_data_buffer, 0, 0x180);
			if (sceIoRead( at3_handle, at3_data_buffer, at3_data_align ) != at3_data_align) {
				eof = 1;
				continue;
			}
			if ( at3_channel_mode ) {
				memcpy(at3_data_buffer+at3_data_align, at3_data_buffer, at3_data_align);
			}
			decode_type = 0x1001;
		}
		else {
			memset( at3_data_buffer, 0, at3_data_align+8);
			at3_data_buffer[0] = 0x0F;
			at3_data_buffer[1] = 0xD0;
			at3_data_buffer[2] = at3_at3plus_flagdata[0];
			at3_data_buffer[3] = at3_at3plus_flagdata[1];
			if (sceIoRead( at3_handle, at3_data_buffer+8, at3_data_align ) != at3_data_align) {
				eof = 1;
				continue;
			}
			decode_type = 0x1000;
		}
	
		at3_codec_buffer[6] = (unsigned long)at3_data_buffer;
		at3_codec_buffer[8] = (unsigned long)at3_mix_buffer;
	
		int res = sceAudiocodecDecode(at3_codec_buffer, decode_type);
		if ( res < 0 ) {
			eof = 1;
			continue;
		}
		samplesdecoded = at3_sample_per_frame;
	}

wait:
	
	if ( at3_handle ) {
		sceIoClose(at3_handle);
	}
	if ( at3_data_buffer) {
		free(at3_data_buffer);
	}
	if ( at3_getEDRAM ) {
		sceAudiocodecReleaseEDRAM(at3_codec_buffer);
	}
	
	sceCtrlReadBufferPositive(&input, 1);
	while(!(input.Buttons & PSP_CTRL_TRIANGLE))
	{
		sceKernelDelayThread(10000);	// wait 10 milliseconds
		sceCtrlReadBufferPositive(&input, 1);
	}
	
	sceKernelExitGame();
	return 0;
}


/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}


/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();

	return 0;
}


/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}