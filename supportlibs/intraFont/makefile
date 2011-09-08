TARGET_LIB = libintraFont_mod.a

OBJS = \
		intraFont.o \
		libccc.o

PSP_FW_VERSION = 500

MIPS_OPTIMIZATIONS = \
		-ffast-math -fomit-frame-pointer \
		-fsingle-precision-constant
#		-mfp32 -msingle-float -mhard-float
CFLAGS = -ggdb -G0 -O3 -DNDEBUG -Wall -DPSP -D__psp__ $(MIPS_OPTIMIZATIONS)
LIBS = -lpspgu

CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions -fcheck-new

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

install:
	install *.a ../../lib
	install *.h ../../include

reinstall: clean all install
