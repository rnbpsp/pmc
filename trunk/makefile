TARGET = pmc

PMC_GDB_DEBUG = 0

PMC_DEBUG = 1
PMC_SHOWFPS = 1
PMC_PROFILE = 0

# 0.8
FFMPEG_PORT = 0.8

PMC_VER_MAJOR = 0
PMC_VER_MINOR = 3


MAIN = \
		src/controls.o			\
		src/drawing.o				\
		src/image.o					\
		src/main.o					\
		src/error.o					\
		src/utils.o					\
		src/topbar.o				\
		src/callbacks.o			\
		src/notdone.o				\
		src/memcpy_vfpu.o		\
		src/load_tga.o
#		src/item_list.o			\

FILEIO = \
		src/fileio/filelist.o		\
		src/fileio/fileio.o

MUSIC_PLAYER = \
		src/player.o \
		src/art/album_art.o \
		src/art/tags.o
#		src/scevaudio.o
#		ffmpeg-prx/ffmpegMusic.o

MUSIC_DECODER = \
		src/decoders/audio_dec.o \
		src/decoders/sceMp3Aac.o \
		src/decoders/sceWma.o \
		src/decoders/sceAtrac3.o \
		src/decoders/sceAtrac3p.o \
		src/decoders/ffmpeg.o

NOW_PLAYING = \
		src/now_playing.o

SETTINGS = \
		src/settings.o

PROFILING = \
		src/prof/prof.o \
		src/prof/mcount.o

OBJS := $(MAIN) \
				$(FILEIO) \
				$(NOW_PLAYING) \
				$(MUSIC_DECODER) \
				$(MUSIC_PLAYER) \
				$(SETTINGS) \
				src/RnBridge.o \
				src/cooleyesBridge.o
	
# $(INTRAFONT) $(PICTURES) $(MUSIC) $(VIDEO)

BUILD_PRX = 1
PSP_FW_VERSION = 500

MIPS_OPTIMIZATIONS = \
		-ffast-math \
		-fsingle-precision-constant
#		-mfp32 -msingle-float -mhard-float

PMC_VERSION = \
	-D__PMC_VER_MAJOR=$(PMC_VER_MAJOR) \
	-D__PMC_VER_MINOR=$(PMC_VER_MINOR)

TAGLIB_DEFINES  = -DHAVE_ZLIB=1 -DWITH_ASF -DWITH_MP4 -DTAGLIB_NO_CONFIG

CFLAGS = -G0 -Wall -DPSP -D__psp__ -MD \
				-D_SHOWFPS=$(PMC_SHOWFPS) \
				-D_PROFILE=$(PMC_PROFILE) \
				$(MIPS_OPTIMIZATIONS) \
				$(TAGLIB_DEFINES) \
				$(PMC_VERSION)
				
LIBS = -ljpeg -ltaglib -lavformat -lavcodec -lbz2 -lz -lpng15 -lavutil -lm \
		-lpspaudio -lpspaudiocodec -lpspasfparser \
		-lpspfpu -lintraFont_mod -lpspgu -lminIni \
		-lpspumd -lpsphprm -lpsprtc -lpsppower -lstdc++
#-lminIni -lswscale -lpspvfpu
#

LIBDIR = ./lib ./lib/ffmpeg-$(FFMPEG_PORT)
INCDIR = ./src ./include ./include/taglib-1.7 ./include/ffmpeg-$(FFMPEG_PORT)
#			./taglib ./taglib/toolkit ./taglib/ape \
#			./taglib/mpeg ./taglib/mpeg/id3v1 \
#			./taglib/mpeg/id3v2

LDFLAGS = -Wl,-allow-multiple-definition

ifeq ($(PMC_DEBUG),1)

ifeq ($(PMC_GDB_DEBUG),1)
CFLAGS += -ggdb3 -DDEBUG -Winline -Wdisabled-optimization
else
CFLAGS += -O2 -ggdb3 -DDEBUG -Wdisabled-optimization
endif

ifeq ($(PMC_PROFILE), 1)
OBJS += $(PROFILING)
CFLAGS += -pg
LIBS += -lpspprof
else 
CFLAGS += -fomit-frame-pointer
endif

else
CFLAGS += -O3 -DNDEBUG -fomit-frame-pointer -Winline
PSP_LARGE_MEMORY = 1
endif


CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions -fcheck-new

DEP_FILES := $(OBJS:.o=.d)

EXTRA_CLEAN=$(DEP_FILES) $(TARGET).sym gmon.out

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP Music Center v$(PMC_VER_MAJOR).$(PMC_VER_MAJOR) by RNB_PSP
PSP_EBOOT_ICON  = ./res/ICON0.PNG
PSP_EBOOT_PIC1 = ./res/PIC0.PNG

PSPSDK=$(shell psp-config --pspsdk-path)
include sdk/build.mak
#$(PSPSDK)/lib/build.mak

symfile:
	prxtool -y $(TARGET).elf > $(TARGET).sym

install:
	cp -f -u ./EBOOT.PBP ./release/EBOOT.PBP
	cp -f -u ./README ./release/README
	cp -f -u ./Changelog ./release/Changelog
	cp -f -u ./CREDITS ./release/CREDITS
	cp -f -u ./res/icons.tga ./release/res/icons.tga
	cp -f -u "./res/about ico.tga" "./release/res/about ico.tga"
	cp -f -u "./res/file browser ico.tga" "./release/res/file browser ico.tga"
	cp -f -u "./res/now playing ico.tga" "./release/res/now playing ico.tga"
	cp -f -u "./res/playlist ico.tga" "./release/res/playlist ico.tga"
	cp -f -u "./res/settings ico.tga" "./release/res/settings ico.tga"
	cp -f -u "./res/splash.tga" "./release/res/splash.tga"
	cp -f -u ./cooleyesBridge.prx ./release/cooleyesBridge.prx
	cp -f -u ./hold-.prx ./release/hold-.prx
	cp -f -u ./RnBridge.prx ./release/RnBridge.prx

help:
	@echo "symfile:   generate symbols file using prxtool"
	@echo "install:		copy to release folder"

# pull in dependency info for *existing* .o files
-include $(DEP_FILES)
