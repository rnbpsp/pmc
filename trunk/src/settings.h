#ifndef PMC_SETTINGS_H
#define PMC_SETTINGS_H

#include <vector>
#include <cstring>
#include "utils.h"
#include "player.h"
#include <psppower.h>

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
	
	FORCE_INLINE
	const char *operator()()
	{
		return ext;
	};
};

class PMC_CPU
{
	FORCE_INLINE
	int set_cpu(int cpu);
	
public:
	PMC_CPU(){ set_cpu(333); };
	
	FORCE_INLINE
	PMC_CPU &operator=(int in)
	{
		set_cpu(in);
		return *this;
	};
	
	inline int operator()(const char *file);
	
	FORCE_INLINE
	PMC_CPU &operator=(const char* in)
	{
		set_cpu(this->operator()(in));
		return *this;
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
	PMC_CPU cpu;
	
#define NO_SORTING			0
#define DIRS_FIRST			1
#define ALPHABETICALLY	2
	int fileSort_mode;
	
//#define SET_OK_DEFAULT -1
#define SET_OK_CIRCLE 0
#define SET_OK_CROSS 1
	//int ok_button;
	char ok_char, cancel_char;
	
	int code_page;
	
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
	void show_menu();
};
extern PMC_SETTINGS settings;

inline int
PMC_CPU::set_cpu(int cpu)
{
// crashing if profiling is enabled
#if 1
//(!_PROFILE) && !defined(DEBUG)
	if (settings.dynamic_clock)
	{
		cpu = cpu==0 ? settings.clocks[DEFAULT_CLOCK] : pmc_minmax<int>(cpu, 10, 333);
	//	const int bus = pmc_minmax<int>( (cpu>>1)+37, 37, 166 );
	//	const int pll = pmc_minmax<int>( (bus<<1), cpu, 333 );
		
		//int res = scePowerSetClockFrequency(pll, cpu, bus);
		const int res = scePowerSetCpuClockFrequency(cpu);
		if (res!=0) printf("cannot set cpu speed res = %d\n", res);
	//	else printf("set cpu to: %d %d %d\n", pll, cpu, bus);
		printf("set cpu to: %d\n", cpu);
	}
#endif
	return cpu;
}

inline int PMC_CPU::operator()(const char *file)
{
	const char *ext = get_ext(file);
	for(unsigned i=0; i<settings.exts.size(); ++i)
		if (strcasecmp(ext, settings.exts[i].ext)==0)
			return set_cpu(settings.exts[i].clock);
	
	return 0; // default
};


#endif //PMC_SETTINGS_H
