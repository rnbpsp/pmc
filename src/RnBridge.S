	.set noreorder

#include "pspstub.s"

	STUB_START "RnBridge",0x40090000,0x00060005
	STUB_FUNC  0x7BC0E230,get_PSPmodel
	STUB_FUNC  0xD7ADC68C,get_moduleUID
	STUB_FUNC  0x1191DE7B,imposeSetBrightness
	STUB_FUNC  0x75A623EA,imposeGetBrightness
	STUB_FUNC  0x521EAC81,imposeGetVolume
	STUB_FUNC  0xBF6F954F,imposeGetMute
	STUB_END
