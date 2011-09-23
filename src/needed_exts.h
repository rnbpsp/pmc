#ifndef PMC_NEEDED_EXTS_H
#define PMC_NEEDED_EXTS_H

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
	: clock(in.clock)
	{
		ext = strdup(in.ext);
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

#endif //PMC_NEEDED_EXTS_H
