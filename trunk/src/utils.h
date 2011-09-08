#ifndef __PMC_UTILS_H__
#define __PMC_UTILS_H__

#ifdef __cplusplus
#include "main.h"
#define UNCACHED_ADDRESS_SPACE 0x40000000

#define FORCE_INLINE inline __attribute__((always_inline))
#define ALIGN_DECL(align) __attribute__((aligned(align)))
#define ALIGN_ADDR(s, align) (( (u32)(s) ) & ~(align-1))
#define ALIGN_SIZE(s, align) (( ((u32)(s)) + align-1) & ~(align-1))

FORCE_INLINE
void pmc_invalidate(void *ptr, size_t size)
{
	sceKernelDcacheInvalidateRange((void*)ALIGN_ADDR(ptr, 64), ALIGN_SIZE(size, 64)+64);
}

FORCE_INLINE
void pmc_wbinvalidate(void *ptr, size_t size)
{
	sceKernelDcacheWritebackInvalidateRange((void*)ALIGN_ADDR(ptr, 64), ALIGN_SIZE(size, 64)+64);
}

FORCE_INLINE
void pmc_wb(void *ptr, size_t size)
{
	sceKernelDcacheWritebackRange((void*)ALIGN_ADDR(ptr, 64), ALIGN_SIZE(size, 64)+64);
}

TEMPLATE
T *get_uncachedP(T *ptr)
{
	return (T*)( (u32)ptr | UNCACHED_ADDRESS_SPACE );
}

TEMPLATE
T *get_cachedP(T *ptr)
{
	return (T*)( (u32)ptr & ~UNCACHED_ADDRESS_SPACE );
}

// never print date w/o printing time
//void print_time(float x, float y, float size, u32 txt_color, u32 shad_color, unsigned int pos);
//void print_date(float x, float y, float size, u32 txt_color, u32 shad_color, unsigned int pos);
void show_topbar(const char menu_name[]);

extern "C"
{
#endif //__cplusplus

int battery_percent(int text);
void show_fps();
char* pmc_itoa(int value, char* result, int base);

// microseconds to miliseconds
#define MSEC(usec) (usec*1000)
#define SECONDS(usec) MSEC(usec)*100

#ifdef __cplusplus
}
inline int battery_percent(){ return battery_percent(1); };

TEMPLATE
T pmc_abs(T x){
	return (x<0) ? -(x) : x;
}

TEMPLATE
T pmc_min(T a, T b){
	return (a<b) ? a : b;
}

TEMPLATE
T pmc_max(T a, T b){
	return (b<a) ? a : b;
}

// min <= x <= max
TEMPLATE
T pmc_minmax(T x, T min, T max){
	return pmc_min<T>(( pmc_max<T>(x,min) ), max);
}

#include <cstring>
// add 1 as '.' == '.'
inline const char *get_ext(const char *file)
{
	return strrchr(file, '.') + 1;
}

void show_error(const char msg[], bool fatal=false);
inline void check_error(bool test, const char msg[], bool fatal=false)
{
	if (!test) show_error(msg, fatal);
}

#define show_errorEx(x, ...); \
	{ \
		char __err_str[1024]; \
		snprintf(__err_str, 1024, x, __VA_ARGS__); \
		show_error(__err_str); \
	}

extern "C"
{
#endif //__cplusplus

int alphabetical_cmp(const char *str1, const char *str2);

#define wait_for(a, time) while (a) sceKernelDelayThread(time)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__PMC_UTILS_H__
