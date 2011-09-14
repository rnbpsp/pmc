#include "utils.h"
#include <pspkernel.h>
#include <pspdebug.h>
#include <psppower.h>

int battery_percent(int text)
{
	static int charge_effect = -25;
	
	if (scePowerIsBatteryExist())
	{
		if (scePowerIsBatteryCharging() && !text)
		{
			if 	( charge_effect==125 )
						charge_effect = -25;
			else	charge_effect++;
			return charge_effect<0 ? 0 : (charge_effect<100 ? charge_effect : 100);
		}
		else return scePowerGetBatteryLifePercent();
	}
	return -1;
}

int alphabetical_cmp(const char *str1, const char *str2)
{

	// 26 letters * 2(lower + upper cases) + 10(zero to nine)
	#define zero_to_z 62
	const char tbl[] =
		"0123456789"
		"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";
	
	// get past same chars
	while (str1[0]!='\0' && str2[0]!='\0' && str1[0]==str2[0])
	{
		++str1;
		++str2;
	}
	
	if (str1[0]=='\0')
		return (str2[0]=='\0') ? 0 : -1;
	
	if (str2[0]=='\0')
		return 1;
	
	register int char1 = -1, char2 = -1;
	int i=0;
	for(;i<zero_to_z && (char1==-1 || char2==-1); ++i)
	{
		if (tbl[i]==str1[0]) char1 = i;
		if (tbl[i]==str2[0]) char2 = i;
	}
	
	// if they're both symbols......
	if (char1<0 && char2<0)
	{
		char1 = str1[0];
		char2 = str2[0];
	}
	
	if (char1<char2)	return -1;
	if (char1>char2)	return 1;
	//if (char1==char2)
		return 0;
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 * http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
 */
char* pmc_itoa(int value, char* result, int base)
{
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

#ifdef DEBUG
void show_fps()
{
		static float curr_ms = 1.0f;
		static struct timeval time_slices[16];
		static int val = 0;

   float curr_fps = 1.0f / curr_ms;

	pspDebugScreenSetXY(0,0);
	pspDebugScreenPrintf(" FPS %d.%03d  ",(int)curr_fps,(int)((curr_fps-(int)curr_fps) * 1000.0f));
    gettimeofday(&time_slices[val & 15],0);

		val++;

		if (!(val & 15))
		{
			struct timeval last_time = time_slices[0];

			curr_ms = 0;
			int i = 1;
			for (; i < 16; ++i)
			{
				struct timeval curr_time = time_slices[i];

				if (last_time.tv_usec > curr_time.tv_usec)
				{
					curr_time.tv_sec++;
					curr_time.tv_usec-=1000000;
				}

				curr_ms += ((curr_time.tv_usec-last_time.tv_usec) + (curr_time.tv_sec-last_time.tv_sec) * 1000000) * (1.0f/1000000.0f);

				last_time = time_slices[i];
			}
			curr_ms /= 15.0f;
		}
//		sceDisplayWaitVblankStart();
}
#endif
////////////////////////////////////////////////////////

#include <malloc.h>
// TODO: Only AVPacket.data is needed to be aligned
// force ffmpeg to 64byte align everything
// so we can use it with sceAudiocodec* functions directly
void *av_malloc(size_t size)
{
	return memalign(64, size);
}
// the ff. functions are for ffmpeg 0.6 and up
// won't compile w/o these (will get undefined references)

#include <unistd.h>
int usleep(	useconds_t usec)
{
	return sceKernelDelayThread(usec);
}

//#include <math.h>
#include <pspfpu.h>
//extern "C" int isfinitef( float f );
int isfinitef( float f )
{
	return !pspFpuIsInf(f) && !pspFpuIsNaN(f);
}



// the ff are for 0.9.6 of minpspw
// w/c doesn't have fast math routines
/*
float fast_cosf(float rad)
{
	return pspFpuCos(rad);
}

float fast_sqrtf(float x)
{
	return pspFpuSqrt(x);
}

float fast_sinf(float x)
{
	return pspFpuSin(x);
}

float fast_atanf(float x)
{
	return pspFpuAtan(x);
}
*/
