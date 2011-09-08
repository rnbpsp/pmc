# TagLib Makefile for PSP
#   © 2002 - 2008 by Scott Wheeler

PSPSDK = $(shell psp-config --pspsdk-path)
PSPDIR = $(shell psp-config --psp-prefix)

TARGET_LIB = libtaglib.a

APE = ape/apefooter.o ape/apeitem.o ape/apetag.o
ID3V1 = mpeg/id3v1/id3v1genres.o mpeg/id3v1/id3v1tag.o
ID3V2_FRAMES = mpeg/id3v2/frames/attachedpictureframe.o \
			mpeg/id3v2/frames/textidentificationframe.o \
			mpeg/id3v2/frames/unknownframe.o
ID3V2 = $(ID3V2_FRAMES) mpeg/id3v2/id3v2extendedheader.o mpeg/id3v2/id3v2footer.o \
			mpeg/id3v2/id3v2frame.o mpeg/id3v2/id3v2framefactory.o mpeg/id3v2/id3v2header.o \
			mpeg/id3v2/id3v2synchdata.o mpeg/id3v2/id3v2tag.o
MPEG = $(APE) $(ID3V1) $(ID3V2) mpeg/mpegfile.o mpeg/mpegheader.o mpeg/mpegproperties.o \
			mpeg/xingheader.o
	
TOOLKIT = toolkit/tbytevector.o toolkit/tbytevectorlist.o toolkit/tdebug.o \
			toolkit/tfile.o toolkit/tstring.o toolkit/tstringlist.o \
			toolkit/unicode.o
OBJS = $(FLAC) $(MPC) $(MPEG) $(OGG) $(WAVPACK) $(TRUEAUDIO) $(TOOLKIT) \
			audioproperties.o fileref.o tag.o tagunion.o
CFLAGS = -O3 -G0 -Wall -ffast-math
CXXFLAGS = $(CFLAGS) -fno-exceptions

INCDIR = ape flac mpc mpeg mpeg/id3v1 mpeg/id3v2  mpeg/id3v2/frames  \
		 ogg ogg/flac ogg/vorbis ogg/speex toolkit wavpack trueaudio

include $(PSPSDK)/lib/build.mak
