#include "main.h"
#include "controls.h"
#include <pspctrl.h>
#include <psphprm.h>

Pmc_Ctrl ctrl;
u32 CTRL_OK = CTRL_CIRCLE;
u32 CTRL_CANCEL = CTRL_CROSS;

void Pmc_Ctrl::wait(const u32 button, const u32 *button_state)
{
	do
	{
		sceDisplayWaitVblankStart();
		//sceKernelDelayThread(MSEC(100));
		this->read();
	}
	while ( !( *button_state & button ) );
}

//#include "settings.h"

Pmc_Ctrl::Pmc_Ctrl()
:	autorepeat_lastHeld(0),
	autorepeat_counter(0),
	delay_counter(0),
	autorepeat(false),
	autorepeat_delay(0),
	autorepeat_interval(0),
	autorepeat_mask(CTRL_UP|CTRL_RIGHT|CTRL_DOWN|CTRL_LEFT)
//	x(0), y(0)
{
	held.value =
	released.value =
	pressed.value = 0;
	
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
};

void Pmc_Ctrl::read(bool read_remote) {
	SceCtrlData pad;
	
	if (state.hold_mode) return flush(false);
	
	sceCtrlReadBufferPositive(&pad, 1);
	
	unsigned int keys = pad.Buttons;
	
	if (keys & (CTRL_HOLD|CTRL_RM_HOLD)) return flush(false);
	
	register unsigned int lastHeld = held.value;

	ctrl_bit *button = (ctrl_bit*)&keys;

	button->ok = button->value & CTRL_OK;
	button->cancel = button->value & CTRL_CANCEL;
	
	if (read_remote && sceHprmIsRemoteExist())
	{
		hprm_bit hprm_keys;
		sceHprmPeekCurrentKey(&(hprm_keys.value));
		
		button->rm_play = hprm_keys.play;
		button->rm_next = hprm_keys.next;
		button->rm_prev = hprm_keys.prev;
		button->vol_up = hprm_keys.vol_up;
		button->vol_down = hprm_keys.vol_down;
	//	button->rm_hold = hprm_keys.hold;
	}
	
	if (autorepeat)
	{
		u32 autorepeat_held = keys & autorepeat_mask;
		if (autorepeat_lastHeld!=autorepeat_held)
			autorepeat_counter = delay_counter = 0;
		else
		{
			if (delay_counter < autorepeat_delay)
				++delay_counter;
			else
			{
				if (autorepeat_counter < autorepeat_interval)
					++autorepeat_counter;
				else
				{
					lastHeld &= ~autorepeat_mask;
					autorepeat_counter = 0;
				}
			}
		}
	}
	
	pressed.value = ~lastHeld & keys;
	released.value = lastHeld & ~keys;
	
	held.value = keys;
	autorepeat_lastHeld = keys & autorepeat_mask;
}

