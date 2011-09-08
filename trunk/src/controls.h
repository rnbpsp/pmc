#ifndef __PMC_CONTROLLERS_H__
#define __PMC_CONTROLLERS_H__

class Pmc_Ctrl {
	typedef union {
		struct {
			bool select:1;				// Select
				bool ok:1;					// confirm
				bool cancel:1;			// cancel
			bool start:1;					// Start
			bool up:1;						// Up (d-pad)
			bool right:1;					// Right (d-pad)
			bool down:1;					// Down (d-pad)
			bool left:1;					// Left (d-pad)
			bool L:1;							// L (shoulder)
			bool R:1;							// R (shoulder)
						bool pad1:2;		// For padding
			bool triangle:1;			// Triangle
			bool circle:1;				// Circle
			bool cross:1;					// Cross
			bool square:1;				// Square
							// the following can only be read in kernel mode (except hold)
			bool home:1;					// Home
			bool hold:1;					// Hold (power switch in the opposite direction)
			bool wlan:1;					// WLAN switch
			bool remote:1;				// Remote hold position
			bool vol_up:1;				// Volume up
			bool vol_down:1;			// Volume down
			bool screen:1;				// Screen
			bool note:1;					// Note
			bool disc:1;					// UMD disc presence
			bool ms:1;						// MS presence
		};
		unsigned int value;		// 32-bit value containing all keys
	} ctrl_bit;
	u32 autorepeat_lastHeld;
	u32 autorepeat_counter;
	u32 delay_counter;
public:
	bool autorepeat;
	u32 autorepeat_delay;// delay in number of frames before auto repeat button
	u32 autorepeat_interval;
	u32 autorepeat_mask;
	ctrl_bit held, released, pressed;
	
	// TODO: optionally treat the analog stick as a 2nd dpad
	//u8 x, y; analog axes
	
	Pmc_Ctrl();
	void wait(const u32 button, const u32 *state);
	void read();
	void flush(bool vBlank=true);
};

extern Pmc_Ctrl ctrl;
extern u32 CTRL_OK, CTRL_CANCEL;
#define CTRL_SELECT			0x000001
#define CTRL_START			0x000008
#define CTRL_UP					0x000010
#define CTRL_RIGHT			0x000020
#define CTRL_DOWN				0x000040
#define CTRL_LEFT				0x000080
#define CTRL_LTRIGGER		0x000100
#define CTRL_RTRIGGER		0x000200
#define CTRL_TRIANGLE		0x001000
#define CTRL_CIRCLE			0x002000
#define CTRL_CROSS			0x004000
#define CTRL_SQUARE			0x008000
#define CTRL_HOME				0x010000
#define CTRL_HOLD				0x020000
#define CTRL_WLAN_UP		0x040000
#define CTRL_REMOTE			0x080000 
#define CTRL_VOLUP			0x100000
#define CTRL_VOLDOWN		0x200000
#define CTRL_SCREEN			0x400000
#define CTRL_NOTE				0x800000
#define CTRL_DISC				0x1000000
#define CTRL_MS					0x2000000

#endif

/*
01				PSP_CTRL_SELECT     = 0x000001,
02				ok									=	0x000002,
03				cancel							= 0x000004,
04        PSP_CTRL_START      = 0x000008,
05        PSP_CTRL_UP         = 0x000010,
06        PSP_CTRL_RIGHT      = 0x000020,
07        PSP_CTRL_DOWN       = 0x000040,
08        PSP_CTRL_LEFT       = 0x000080,
09        PSP_CTRL_LTRIGGER   = 0x000100,
10        PSP_CTRL_RTRIGGER   = 0x000200,
11 pad
12 pad
13        PSP_CTRL_TRIANGLE   = 0x001000,
14        PSP_CTRL_CIRCLE     = 0x002000,
15        PSP_CTRL_CROSS      = 0x004000,
16        PSP_CTRL_SQUARE     = 0x008000,
17        PSP_CTRL_HOME       = 0x010000,
18        PSP_CTRL_HOLD       = 0x020000,
19        PSP_CTRL_WLAN_UP    = 0x040000,
20        PSP_CTRL_REMOTE     = 0x080000,
21        PSP_CTRL_VOLUP      = 0x100000,
22        PSP_CTRL_VOLDOWN    = 0x200000,
23        PSP_CTRL_SCREEN     = 0x400000,
24        PSP_CTRL_NOTE       = 0x800000,
25        PSP_CTRL_DISC       = 0x1000000,
26        PSP_CTRL_MS         = 0x2000000,
*/
