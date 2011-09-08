

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

PSP_MODULE_INFO("MP3 decodeTest", 0x1000, 1, 1); 
PSP_MAIN_THREAD_ATTR(0); 

__attribute__ ((constructor)) 
void loaderInit(){ 
	pspKernelSetKernelPC(); 
	pspSdkInstallNoDeviceCheckPatch(); 
	pspSdkInstallNoPlainModuleCheckPatch(); 
	pspSdkInstallKernelLoadModulePatch(); 
} 

SceCtrlData input; 

static int bitrates[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 }; 

unsigned long aac_codec_buffer[65] __attribute__((aligned(64))); 
short aac_mix_buffer[1024 * 2] __attribute__((aligned(64))); 


SceUID aac_handle; 
u8* aac_data_buffer; 
u16 aac_data_align; 
u32 aac_sample_per_frame; 
u16 aac_channel_mode; 
u32 aac_data_start; 
u32 aac_data_size; 
u8 aac_getEDRAM; 
u32 aac_channels; 
u32 aac_samplerate; 


int main(void) 
{ 
	SetupCallbacks(); 
	 
	int result = pspSdkLoadStartModule("flash0:/kd/me_for_vsh.prx", PSP_MEMORY_PARTITION_KERNEL); 
	 
	result = pspSdkLoadStartModule("flash0:/kd/videocodec.prx", PSP_MEMORY_PARTITION_KERNEL); 
	 
	result = pspSdkLoadStartModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL); 
	 
	result = pspSdkLoadStartModule("flash0:/kd/mpegbase.prx", PSP_MEMORY_PARTITION_KERNEL); 
	 
	result = pspSdkLoadStartModule("flash0:/kd/mpeg_vsh.prx", PSP_MEMORY_PARTITION_USER); 
	 
	pspSdkFixupImports(result); 
	 
	sceMpegInit(); 
	 
	aac_handle = sceIoOpen("ms0:/Test.AAC", PSP_O_RDONLY, 0777); 
	if (  ! aac_handle ) 
		 goto wait; 
	 
	aac_channels = 2; 
	aac_samplerate = 44100; //this is aac file's samplerate, also can be 48000,.... 
	aac_sample_per_frame = 1024; 
	 
	aac_data_start = sceIoLseek32(aac_handle, 0, PSP_SEEK_CUR); 
	 
	memset(aac_codec_buffer, 0, sizeof(aac_codec_buffer)); 
	 
	if ( sceAudiocodecCheckNeedMem(aac_codec_buffer, 0x1003) < 0 ) 
		 goto wait; 
	if ( sceAudiocodecGetEDRAM(aac_codec_buffer, 0x1003) < 0 ) 
				goto wait; 
	aac_getEDRAM = 1; 
	 
	aac_codec_buffer[10] = aac_samplerate; 
	if ( sceAudiocodecInit(aac_codec_buffer, 0x1003) < 0 ) { 
		 goto wait; 
	} 
	 
	int eof = 0;    
	while( !eof ) { 
		 int samplesdecoded; 
		 memset(aac_mix_buffer, 0, aac_sample_per_frame*2*2); 
		 unsigned char aac_header_buf[7]; 
		 if ( sceIoRead( aac_handle, aac_header_buf, 7 ) != 7 ) { 
				eof = 1; 
				continue; 
		 } 
		 int aac_header = aac_header_buf[3]; 
		 aac_header = (aac_header<<8) | aac_header_buf[4]; 
		 aac_header = (aac_header<<8) | aac_header_buf[5]; 
		 aac_header = (aac_header<<8) | aac_header_buf[6]; 
			
		 int frame_size = aac_header & 67100672; 
		 frame_size = frame_size >> 13; 
		 frame_size = frame_size - 7; 
			
		 if ( aac_data_buffer ) 
				free(aac_data_buffer); 
		 aac_data_buffer = (u8*)memalign(64, frame_size); 
			
		 if ( sceIoRead( aac_handle, aac_data_buffer, frame_size ) != frame_size ) { 
				eof = 1; 
				continue; 
		 } 
			
		 aac_data_start += (frame_size+7); 
			
		 aac_codec_buffer[6] = (unsigned long)aac_data_buffer; 
		 aac_codec_buffer[8] = (unsigned long)aac_mix_buffer; 
			
		 aac_codec_buffer[7] = frame_size; 
		 aac_codec_buffer[9] = aac_sample_per_frame * 4; 
			
	 
		 int res = sceAudiocodecDecode(aac_codec_buffer, 0x1003); 
		 if ( res < 0 ) { 
				eof = 1; 
				continue; 
		 } 
		 samplesdecoded = aac_sample_per_frame; 
	} 

wait: 
	 
	if ( aac_handle ) { 
		 sceIoClose(aac_handle); 
	} 
	if ( aac_data_buffer) { 
		 free(aac_data_buffer); 
	} 
	if ( aac_getEDRAM ) { 
		 sceAudiocodecReleaseEDRAM(aac_codec_buffer); 
	} 
	 
	sceCtrlReadBufferPositive(&input, 1); 
	while(!(input.Buttons & PSP_CTRL_TRIANGLE)) 
	{ 
		 sceKernelDelayThread(10000);   // wait 10 milliseconds 
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