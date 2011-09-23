#ifndef RNBRIDGE_H
#define RNBRIDGE_H

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspdisplay_kernel.h>
#include <pspimpose_driver.h>
#include <pspsysreg.h>
#include <pspctrl.h>
#include <pspsysmem_kernel.h>
#include <psppower.h>

#ifdef __cplusplus
extern "C" {
#endif

int get_PSPmodel();
SceUID get_moduleUID(const char *name);
int imposeGetBrightness();
int imposeSetBrightness();
int imposeGetVolume();
int imposeGetMute();

#ifdef __cplusplus
}
#endif

#endif //RNBRIDGE_H
