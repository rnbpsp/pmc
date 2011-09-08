#ifndef PMC_SETTINGS_H
#define PMC_SETTINGS_H

#include <vector>
#include <cstring>
#include "utils.h"

extern "C"
{
	#include <malloc.h>
}

class NEEDED_EXT
{
public:
	char *ext;
	int clock;
	NEEDED_EXT() : ext(NULL), clock(0) {};
	NEEDED_EXT(const char *str)
	:	clock(222)
	{ ext = strdup(str); };
	
	NEEDED_EXT(const char *str, int cpu)
	:	clock(cpu)
	{ ext = strdup(str); };
	
	NEEDED_EXT(const NEEDED_EXT& in)
	{
		ext = strdup(in.ext);
		clock = in.clock;
	};
	
	~NEEDED_EXT()
	{
		free((void*)ext);
	};
	
	FORCE_INLINE
	NEEDED_EXT& operator=(const NEEDED_EXT& in)
	{
		this->set(in.ext, in.clock);
		return *this;
	};
	
	FORCE_INLINE
	NEEDED_EXT& operator=(const char *str)
	{
		this->set(str, 222);
		return *this;
	};
	
	FORCE_INLINE
	void set(const char *str, int cpu)
	{
		free((void*)ext);
		ext = strdup(str);
		clock = cpu;
	};
};

class PMC_SETTINGS
{
	bool show_unknown;
public:
	std::vector<NEEDED_EXT> exts;

#define DEFAULT_CLOCK		0
#define FBROWSER_CLOCK	1
#define SETTINGS_CLOCK	2
	int clocks[3];
	bool dynamic_clock;
	
#define NO_SORTING			0
#define DIRS_FIRST			1
#define ALPHABETICALLY	2
	int fileSort_mode;
	
//#define SET_OK_DEFAULT -1
#define SET_OK_CIRCLE 0
#define SET_OK_CROSS 1
	//int ok_button;
	char ok_char, cancel_char;

	PMC_SETTINGS()
	:	show_unknown(false),
		dynamic_clock(true),
		fileSort_mode(DIRS_FIRST|ALPHABETICALLY),
		ok_char('O'),
		cancel_char('X')
	{
		clocks[0] = clocks[1] = clocks[2] = 222;
	};
	
	void refresh();
	bool isNeeded(const char *file, bool list=true);
};
extern PMC_SETTINGS settings;

FORCE_INLINE
int set_cpu(int cpu)
{
// crashing if profiling is enabled
#if (!_PROFILE)
#ifndef DEBUG

	cpu = pmc_minmax<int>(cpu, 10, 333);
//	const int bus = pmc_minmax<int>( (cpu>>1)+37, 37, 166 );
//	const int pll = pmc_minmax<int>( (bus<<1), cpu, 333 );
	
	//int res = scePowerSetClockFrequency(pll, cpu, bus);
	const int res = scePowerSetCpuClockFrequency(cpu);
	if (res!=0) printf("cannot set cpu speed res = %d\n", res);
	else printf("set cpu to: %d %d %d\n", pll, cpu, bus);
//	printf("set cpu to: %d\n", cpu);

#endif
#endif
	return cpu;
}

FORCE_INLINE
int set_cpu(const char *file)
{
	for(int i=0; i<(int)settings.exts.size(); ++i)
		if (strcasecmp(get_ext(file), settings.exts[i].ext)==0)
			return set_cpu(settings.exts[i].clock);
	
	return set_cpu(settings.clocks[DEFAULT_CLOCK]);
}
#endif //PMC_SETTINGS_H
