	.set noreorder

#include "pspstub.s"

	STUB_START "cooleyesBridge",0x40090000,0x00030005
	STUB_FUNC  0x53E6D624,cooleyesAudioSetFrequency
	STUB_FUNC  0x90CC7DD7,cooleyesMeBootStart
	STUB_FUNC  0x7290BFF1,isPSPgo
	STUB_END
