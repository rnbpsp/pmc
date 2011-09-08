/* 
 *   Copyright (C) 2009 cooleyes 
 *   eyes.cooleyes@gmail.com 
 * 
 *  This Program is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2, or (at your option) 
 *  any later version. 
 *    
 *  This Program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 *  GNU General Public License for more details. 
 *    
 *  You should have received a copy of the GNU General Public License 
 *  along with GNU Make; see the file COPYING.  If not, write to 
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html 
 * 
 */ 
#include <pspkernel.h> 
#include <pspctrl.h> 
#include <pspdisplay.h> 
#include <pspdebug.h> 
#include <psppower.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <stdarg.h> 
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
#include <psputility.h> 
#include "cooleyesBridge.h" 
#include "libasfparser/pspasfparser.h" 
#include "mem64.h" 

int SetupCallbacks(); 

PSP_MODULE_INFO("WMA decodeTest", 0, 1, 1); 
PSP_MAIN_THREAD_ATTR(0); 
PSP_HEAP_SIZE_KB(18*1024); 

SceCtrlData input; 

unsigned long wma_codec_buffer[65] __attribute__((aligned(64))); 
short wma_mix_buffer[8192] __attribute__((aligned(64))); 
short wma_cache_buffer[16384] __attribute__((aligned(64))); 
unsigned long wma_cache_samples = 0; 

short wma_output_buffer[2][2048 * 2] __attribute__((aligned(64))); 
int wma_output_index = 0; 

void* wma_frame_buffer; 

SceAsfParser* parser = 0; 
ScePVoid need_mem_buffer = 0; 


SceUID wma_fd = -1; 

u8 wma_getEDRAM = 0; 

u16 wma_channels  = 2; 
u32 wma_samplerate = 0xAC44; 
u16 wma_format_tag = 0x0161; 
u32 wma_avg_bytes_per_sec = 3998; 
u16 wma_block_align = 1485; 
u16 wma_bits_per_sample = 16; 
u16 wma_flag = 0x1F; 

char filename[1024]; 
FILE* dumpfd = 0; 

void mesg(const char* fmt, ...) { 
	va_list ap; 
	char str[1024]; 

	va_start(ap, fmt); 
	vsnprintf(str, sizeof(str), fmt, ap); 
	str[sizeof(str) - 1] = '\0'; 

	pspDebugScreenPrintf("%s", str); 
	printf("%s", str); 

	va_end(ap); 
} 

int asf_read_cb(void *userdata, void *buf, SceSize size) { 
	 
	int ret = sceIoRead(wma_fd, buf, size); 

//   mesg("%s 0x%08x %d\n", __func__, (u32)buf, size); 

	return ret; 
} 

SceOff asf_seek_cb(void *userdata, void *buf, SceOff offset, int whence) { 

	SceOff ret = -1; 

	ret = sceIoLseek(wma_fd, offset, whence); 

//   mesg("%s@0x%08x 0x%08x %d\n", __func__, ret, offset, whence); 

	return ret; 
} 


int main(void) 
{ 
	SetupCallbacks(); 
	 
	pspDebugScreenInit(); 
	pspDebugScreenSetXY(0, 2); 
	 
	int result; 
	 
	result = sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC); 
	if ( result < 0 ) { 
		 pspDebugScreenPrintf("\nerr: sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC)\n"); 
		 goto wait; 
	} 
	 
	result = sceUtilityLoadAvModule(PSP_AV_MODULE_ATRAC3PLUS); 
	if ( result < 0 ) { 
		 pspDebugScreenPrintf("\nerr: sceUtilityLoadAvModule(PSP_AV_MODULE_ATRAC3PLUS)\n"); 
		 goto wait; 
	} 

	SceUID modid; 
	int status; 
	 
	int devkitVersion = sceKernelDevkitVersion(); 
	 /*
	getcwd(filename, 256); 
	 
	if ( devkitVersion < 0x03050000) 
		 strcat(filename, "/libasfparser330.prx"); 
	else if ( devkitVersion < 0x03070000) 
		 strcat(filename, "/libasfparser350.prx"); 
	else 
		 strcat(filename, "/libasfparser370.prx"); 
			*/
	strcpy(filename, "flash0:/kd/libasfparser.prx");
	modid = sceKernelLoadModule(filename, 0, NULL); 
	if(modid >= 0) { 
		 modid = sceKernelStartModule(modid, 0, 0, &status, NULL); 
		 pspDebugScreenPrintf("\n%s\n", filename); 
	} 
	else { 
		 pspDebugScreenPrintf("\nerr=0x%08X : sceKernelLoadModule\n", modid); 
		 goto wait; 
	} 
	 
	getcwd(filename, 256); 
	strcat(filename, "/cooleyesBridge.prx"); 
	modid = sceKernelLoadModule(filename, 0, NULL); 
	if(modid >= 0) { 
		 modid = sceKernelStartModule(modid, 0, 0, &status, NULL); 
		 pspDebugScreenPrintf("\n%s\n", filename); 
	} 
	else { 
		 pspDebugScreenPrintf("\nerr=0x%08X : sceKernelLoadModule(cooleyesBridge)\n", modid); 
		 goto wait; 
	} 
	 
	cooleyesMeBootStart(devkitVersion, 3); 
	 
	u32 cpu = scePowerGetCpuClockFrequency(); 
	u32 bus = scePowerGetBusClockFrequency(); 
	 
	mesg("cpu=%d, bus=%d\n", cpu, bus); 
	 
	if (wma_fd < 0) { 
		 wma_fd = sceIoOpen("ms0:/music/Test.wma", PSP_O_RDONLY, 0777); 

		 if (wma_fd < 0) { 
				mesg("sceIoOpen() = 0x%08x\n", wma_fd); 
				goto wait; 
		 } 
	} 
			
	int ret; 
	 
	//sceAsfCheckNeedMem    
	parser = malloc_64(sizeof(SceAsfParser)); 
	if ( !parser ) { 
		 mesg("malloc_64(SceAsfParser) fail"); 
		 goto wait; 
	} 
	memset(parser, 0, sizeof(SceAsfParser)); 
	 
	parser->iUnk0 = 0x01; 
	parser->iUnk1 = 0x00; 
	parser->iUnk2 = 0x000CC003; 
	parser->iUnk3 = 0x00; 
	parser->iUnk4 = 0x00000000; 
	parser->iUnk5 = 0x00000000; 
	parser->iUnk6 = 0x00000000; 
	parser->iUnk7 = 0x00000000; 
	 
	ret = sceAsfCheckNeedMem(parser); 
	mesg("sceAsfCheckNeedMem() = 0x%08x\n", ret); 
	if ( ret < 0 ) { 
		 goto wait; 
	} 
	 
	mesg("parser->iNeedMem = 0x%08x\n", parser->iNeedMem); 
	 
	if ( parser->iNeedMem > 0x8000 ) { 
		 mesg("iNeedMem > 0x8000"); 
		 goto wait; 
	} 
	 
	//sceAsfInitParser 
	need_mem_buffer = malloc_64(parser->iNeedMem); 
	if ( !need_mem_buffer ) { 
		 mesg("malloc_64(need_mem_buffer) fail"); 
		 goto wait; 
	} 
	 
	parser->pNeedMemBuffer = need_mem_buffer; 
	parser->iUnk3356 = 0x00000000; 
	 
	ret = sceAsfInitParser(parser, 0, &asf_read_cb, &asf_seek_cb); 
	mesg("sceAsfInitParser() = 0x%08x\n", ret); 
	if ( ret < 0 ) { 
		 goto wait; 
	} 
	wma_channels = *((u16*)need_mem_buffer); 
	wma_samplerate = *((u32*)(need_mem_buffer+4)); 
	wma_format_tag = 0x0161; 
	wma_avg_bytes_per_sec = *((u32*)(need_mem_buffer+8)); 
	wma_block_align = *((u16*)(need_mem_buffer+12)); 
	wma_bits_per_sample = 16; 
	wma_flag = *((u16*)(need_mem_buffer+14)); 
	 
	u64 duration = parser->lDuration/100000; 
	int hour, minute, second, msecond; 
	msecond = duration %100; 
	duration /= 100; 
	second =  duration % 60; 
	duration /= 60; 
	minute = duration % 60; 
	hour = duration / 60; 
	mesg("Music Duration = %02d:%02d:%02d,%03d\n", hour, minute, second, msecond); 
	 
//   dumpfd = fopen("ms0:/sceAsfInitParser(MEM).dat", "wb+"); 
//   fwrite(need_mem_buffer, parser->iNeedMem, 1, dumpfd); 
//   fclose(dumpfd); 
	 
	memset(wma_codec_buffer, 0, sizeof(wma_codec_buffer)); 
	 
	//CheckNeedMem 
	wma_codec_buffer[5] = 1; 
	ret = sceAudiocodecCheckNeedMem(wma_codec_buffer, 0x1005); 
	mesg("sceAudiocodecCheckNeedMem=0x%08X\n", ret); 
	if ( ret < 0 ) { 
		 goto wait; 
	} 
	 
//   dumpfd = fopen("ms0:/sceAudiocodecCheckNeedMem1005.dat", "wb+"); 
//   fwrite(wma_codec_buffer, 260, 1, dumpfd); 
//   fclose(dumpfd); 
	 
	//GetEDRAM 
	ret=sceAudiocodecGetEDRAM(wma_codec_buffer, 0x1005); 
	mesg("sceAudiocodecGetEDRAM=0x%08X\n", ret); 
	if ( ret < 0 ) { 
		 goto wait; 
	} 
	 
//   dumpfd = fopen("ms0:/sceAudiocodecGetEDRAM1005.dat", "wb+"); 
//   fwrite(wma_codec_buffer, 260, 1, dumpfd); 
//   fclose(dumpfd); 
	 
	wma_getEDRAM = 1; 
	 
	//Init 
	u16* p16 = (u16*)(&wma_codec_buffer[10]); 
	p16[0] = wma_format_tag; 
	p16[1] = wma_channels; 
	wma_codec_buffer[11] = wma_samplerate; 
	wma_codec_buffer[12] = wma_avg_bytes_per_sec; 
	p16 = (u16*)(&wma_codec_buffer[13]); 
	p16[0] = wma_block_align; 
	p16[1] = wma_bits_per_sample; 
	p16[2] = wma_flag; 
	 
	ret=sceAudiocodecInit(wma_codec_buffer, 0x1005); 
	mesg("sceAudiocodecInit=0x%08X\n", ret); 
	if ( ret < 0 ) { 
		 goto wait; 
	} 
	 
//   dumpfd = fopen("ms0:/sceAudiocodecInit1005.dat", "wb+"); 
//   fwrite(wma_codec_buffer, 260, 1, dumpfd); 
//   fclose(dumpfd); 
	 
	wma_frame_buffer = malloc_64(wma_block_align); 
	if (!wma_frame_buffer) { 
		 mesg("malloc_64(frame_data_buffer) fail"); 
		 goto wait; 
	} 
	 
	int npt = 0 * 1000; 

	ret = sceAsfSeekTime(parser, 1, &npt); 
	mesg("sceAsfSeekTime = 0x%08x\n", ret); 
	if (ret < 0) {    
		 goto wait; 
	} 
	 
//   int audio_channel = sceAudioChReserve(0, 2048 , PSP_AUDIO_FORMAT_STEREO); 
	sceAudioSRCChReserve(2048, wma_samplerate, 2); 
	 
	memset(wma_cache_buffer, 0, 32768); 
	wma_cache_samples = 0; 
	 
	while(1) { 
			
		 if ( wma_cache_samples >= 2048 ) { 
				memcpy(wma_output_buffer[wma_output_index], wma_cache_buffer, 8192); 
				wma_cache_samples -= 2048; 
				if ( wma_cache_samples > 0 ) 
					 memmove(wma_cache_buffer, wma_cache_buffer+4096, wma_cache_samples*2*2); 
				sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, wma_output_buffer[wma_output_index]); 
//         sceAudioOutputBlocking(audio_channel, PSP_AUDIO_VOLUME_MAX, wma_output_buffer[wma_output_index]); 
				wma_output_index = (wma_output_index + 1) % 2;    
				continue; 
		 } 
			
		 memset(wma_frame_buffer, 0, wma_block_align); 
		 parser->sFrame.pData = wma_frame_buffer; 
		 ret = sceAsfGetFrameData(parser, 1, &parser->sFrame); 
		 //mesg("sceAsfGetFrameData = 0x%08x\n", ret); 
			
		 if (ret < 0) { 
				break; 
		 } 
		 memset(wma_mix_buffer, 0, 16384); 
			
		 wma_codec_buffer[6] = (unsigned long)wma_frame_buffer; 
		 wma_codec_buffer[8] = (unsigned long)wma_mix_buffer; 
		 wma_codec_buffer[9] = 16384;; 
			
		 wma_codec_buffer[15] = parser->sFrame.iUnk2; 
		 wma_codec_buffer[16] = parser->sFrame.iUnk3; 
		 wma_codec_buffer[17] = parser->sFrame.iUnk5; 
		 wma_codec_buffer[18] = parser->sFrame.iUnk6; 
		 wma_codec_buffer[19] = parser->sFrame.iUnk4; 
			
		 ret = sceAudiocodecDecode(wma_codec_buffer, 0x1005); 
		 //mesg("sceAudiocodecDecode = 0x%08x\n", ret); 
			
		 if (ret < 0) {    
				break; 
		 } 
			
		 //mesg("wma_codec_buffer[9] = %d\n", wma_codec_buffer[9]); 
			
		 memcpy(wma_cache_buffer+(wma_cache_samples*2), wma_mix_buffer, wma_codec_buffer[9]); 
			
		 wma_cache_samples += (wma_codec_buffer[9]/4); 
			
	} 
	 
	while(sceAudioOutput2GetRestSample() > 0); 
	sceAudioSRCChRelease(); 


wait: 

	if ( wma_getEDRAM ) { 
		 sceAudiocodecReleaseEDRAM(wma_codec_buffer); 
	} 
	 
	if ( !(wma_fd<0) ) 
		 sceIoClose(wma_fd); 

//   if ( wma_reader ) 
//      buffered_reader_close(wma_reader); 
			
	if ( parser ) 
		 free_64(parser); 
	if ( need_mem_buffer ) 
		 free_64(need_mem_buffer); 
			
	if (wma_frame_buffer) 
		 free_64(wma_frame_buffer); 
	 
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