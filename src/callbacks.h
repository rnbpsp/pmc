#ifndef __PMC_CALLBACKS_H__
#define __PMC_CALLBACKS_H__

// TODO
class PMC_STATE
{
public:
	bool running, exit_viaCallback, guith_done, hold_mode, isPSPgo;
	
	PMC_STATE()
	:	running(false),
		exit_viaCallback(false),
		guith_done(false),
		hold_mode(false),
		isPSPgo(false)
	{};
};
extern PMC_STATE state;

#include "drawing.h"

void set_callbacks();

#endif // __PMC_CALLBACKS_H__
