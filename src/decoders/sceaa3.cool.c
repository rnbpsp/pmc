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

PSP_MODULE_INFO("AA3 decodeTest", 0x1000, 1, 1);
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

unsigned long aa3_codec_buffer[65] __attribute__((aligned(64)));
short aa3_mix_buffer[2048 * 2] __attribute__((aligned(64)));


SceUID aa3_handle;
u16 aa3_type;
u8* aa3_data_buffer;
u16 aa3_data_align;
u32 aa3_sample_per_frame;
u16 aa3_channel_mode;
u8 aa3_at3plus_flagdata[2];
u32 aa3_data_start;
u32 aa3_data_size;
u8 aa3_getEDRAM;
u32 aa3_channels;
u32 aa3_samplerate;


int main(void)
{
	SetupCallbacks();
	
	int result = pspSdkLoadStartModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL);
	pspSdkFixupImports(result);
	
	SceUID aa3_handle = sceIoOpen("ms0:/Test.AA3", PSP_O_RDONLY, 0777); // or ms0:/Test.OMA
	if (  ! aa3_handle )
		goto wait;
	
	sceIoLseek32(aa3_handle, 0x0C00, PSP_SEEK_SET);
	
	u8 ea3_header[0x60];
	if ( sceIoRead( aa3_handle, ea3_header, 0x60 ) != 0x60 ) 
		goto wait;
	if ( ea3_header[0] != 0x45 || ea3_header[1] != 0x41 || ea3_header[2] != 0x33 || ea3_header[3] != 0x01 )
		goto wait;
	
	aa3_at3plus_flagdata[0] = ea3_header[0x22];
	aa3_at3plus_flagdata[1] = ea3_header[0x23];
	
	aa3_type = (ea3_header[0x22] == 0x20) ? TYPE_ATRAC3 : ((ea3_header[0x22] == 0x28) ? TYPE_ATRAC3PLUS : 0x0);
	
	if ( aa3_type != TYPE_ATRAC3 && aa3_type != TYPE_ATRAC3PLUS )
		goto wait;
	
	aa3_channels = 2;
	aa3_samplerate = 44100;
	if ( aa3_type == TYPE_ATRAC3 ) 
		aa3_data_align = ea3_header[0x23]*8;
	else
		aa3_data_align = (ea3_header[0x23]+1)*8;
	
	aa3_data_start = 0x0C60;
	aa3_data_size = sceIoLseek32(aa3_handle, 0, PSP_SEEK_END) - aa3_data_start;
	
	if ( aa3_data_size % aa3_data_align != 0 )
		goto wait;
	
	sceIoLseek32(aa3_handle, aa3_data_start, PSP_SEEK_SET);
	
	memset(aa3_codec_buffer, 0, sizeof(aa3_codec_buffer));
	
	if ( aa3_type == TYPE_ATRAC3 ) {
		aa3_channel_mode = 0x0;
		if ( aa3_data_align == 0xC0 ) // atract3 have 3 bitrate, 132k,105k,66k, 132k align=0x180, 105k align = 0x130, 66k align = 0xc0
			aa3_channel_mode = 0x1;
		aa3_sample_per_frame = 1024; 
		aa3_data_buffer = (u8*)memalign(64, 0x180);
		if ( aa3_data_buffer == NULL)
			goto wait;
		aa3_codec_buffer[26] = 0x20;
		if ( sceAudiocodecCheckNeedMem(aa3_codec_buffer, 0x1001) < 0 ) 
			goto wait;
		if ( sceAudiocodecGetEDRAM(aa3_codec_buffer, 0x1001) < 0 )
			goto wait;
		aa3_getEDRAM = 1;
		aa3_codec_buffer[10] = 4;
		aa3_codec_buffer[44] = 2;
		if ( aa3_data_align == 0x130 )
			aa3_codec_buffer[10] = 6;
		if ( sceAudiocodecInit(aa3_codec_buffer, 0x1001) < 0 ) {
			goto wait;
		}
	}
	else if ( aa3_type == TYPE_ATRAC3PLUS ) {
		aa3_sample_per_frame = 2048;
		int temp_size = aa3_data_align+8;
		int mod_64 = temp_size & 0x3f;
		if (mod_64 != 0) temp_size += 64 - mod_64;
		aa3_data_buffer = (u8*)memalign(64, temp_size);
		if ( aa3_data_buffer == NULL)
			goto wait;
		aa3_codec_buffer[5] = 0x1;
		aa3_codec_buffer[10] = aa3_at3plus_flagdata[1];
		aa3_codec_buffer[10] = (aa3_codec_buffer[10] << 8 ) | aa3_at3plus_flagdata[0];
		aa3_codec_buffer[12] = 0x1;
		aa3_codec_buffer[14] = 0x1;
		if ( sceAudiocodecCheckNeedMem(aa3_codec_buffer, 0x1000) < 0 ) 
			goto wait;
		if ( sceAudiocodecGetEDRAM(aa3_codec_buffer, 0x1000) < 0 )
			goto wait;
		aa3_getEDRAM = 1;
		if ( sceAudiocodecInit(aa3_codec_buffer, 0x1000) < 0 ) {
			goto wait;
		}
	}
	else
		goto wait;
	
	int eof = 0;	
	while( !eof ) {
		int samplesdecoded;
		memset(aa3_mix_buffer, 0, 2048*2*2);
		unsigned long decode_type = 0x1001;
		if ( aa3_type == TYPE_ATRAC3 ) {
			memset( aa3_data_buffer, 0, 0x180);
			if (sceIoRead( aa3_handle, aa3_data_buffer, aa3_data_align ) != aa3_data_align) {
				eof = 1;
				continue;
			}
			if ( aa3_channel_mode ) {
				memcpy(aa3_data_buffer+aa3_data_align, aa3_data_buffer, aa3_data_align);
			}
			decode_type = 0x1001;
		}
		else {
			memset( aa3_data_buffer, 0, aa3_data_align+8);
			aa3_data_buffer[0] = 0x0F;
			aa3_data_buffer[1] = 0xD0;
			aa3_data_buffer[2] = aa3_at3plus_flagdata[0];
			aa3_data_buffer[3] = aa3_at3plus_flagdata[1];
			if (sceIoRead( aa3_handle, aa3_data_buffer+8, aa3_data_align ) != aa3_data_align) {
				eof = 1;
				continue;
			}
			decode_type = 0x1000;
		}
	
		aa3_codec_buffer[6] = (unsigned long)aa3_data_buffer;
		aa3_codec_buffer[8] = (unsigned long)aa3_mix_buffer;
	
		int res = sceAudiocodecDecode(aa3_codec_buffer, decode_type);
		if ( res < 0 ) {
			eof = 1;
			continue;
		}
		samplesdecoded = aa3_sample_per_frame;
	}

wait:
	
	if ( aa3_handle ) {
		sceIoClose(aa3_handle);
	}
	if ( aa3_data_buffer) {
		free(aa3_data_buffer);
	}
	if ( aa3_getEDRAM ) {
		sceAudiocodecReleaseEDRAM(aa3_codec_buffer);
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