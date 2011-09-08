TARGET_LIB = libminIni.a

OBJS = minIni.o

PSP_FW_VERSION = 500

MIPS_OPTIMIZATIONS = \
		-ffast-math -fomit-frame-pointer \
		-fsingle-precision-constant
#		-mfp32 -msingle-float -mhard-float
CFLAGS = -ggdb -G0 -O3 -DNDEBUG -Wall -DPSP -D__psp__ $(MIPS_OPTIMIZATIONS)
#LIBS = -lpspgu

CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

install:
	install *.a ../../lib
	install minIni.h ../../include
	install minGlue.h ../../include

reinstall: clean all install
