	.set noreorder

#include "pspstub.s"

	STUB_START	"sceVaudio",0x40090000,0x00030005
	STUB_FUNC	0x03B6807D,sceVaudioChReserve
	STUB_FUNC	0x67585DFD,sceVaudioChRelease
	STUB_FUNC	0x8986295E,sceVaudioOutputBlocking
	STUB_END
