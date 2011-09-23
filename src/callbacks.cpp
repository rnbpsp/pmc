#include <pspkernel.h>
#include <psptypes.h>
#include <psppower.h>
#include <pspdisplay.h>
#include <RnBridge.h>

#include "utils.h"
#include "callbacks.h"
#include "font.h"
#if (_PROFILE)
#include <pspprof.h>
#endif

PMC_STATE state;

/* Exit callback */
static
int exit_callback(int arg1, int arg2, void *common)
{
	state.exit_viaCallback = true;
	state.running = false;
	
	//wait for other threads to finish
	wait_for(!state.guith_done, 1000);
	
#if (_PROFILE)
	gprof_cleanup();
#endif
	sceKernelExitGame();
	sceDisplayWaitVblank();
	
	return 0;
}

#include <pspgu.h>
/* Power Callback */
static
int power_callback(int unknown, int pwrflags, void *common)
{
		printf("power callback called\n");
		
		// check for power switch and suspending as one is manual and the other automatic
		if (pwrflags & (PSP_POWER_CB_POWER_SWITCH|PSP_POWER_CB_SUSPENDING) )
		{
			printf("suspending....\n");
		}
		/*
		if (pwrflags & PSP_POWER_CB_RESUMING)
		if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE)
		if (pwrflags & PSP_POWER_CB_STANDBY)
		*/
		
		if (pwrflags & PSP_POWER_CB_HOLD_SWITCH)
		{
			printf("hold switch held.\n");
			state.hold_mode = true;
	//		sceGuDisplay(0);
		}
		else //if(state.hold_mode)
		{
			state.hold_mode = false;
	//		sceGuDisplay(1);
		}
		
		sceDisplayWaitVblank();
		printf("power callback called\n");

	return 0;
}

/* Callback thread */
static
int CallbackThread(SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	
	cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);

	sceKernelSleepThreadCB();
	return 0;
}

/* Sets up the callback thread */
void set_callbacks(void)
{
	SceUID thid = sceKernelCreateThread("Callbacks Thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
		state.running = true;
	}
	else printf("Cannot create callback thread.\n");
}
