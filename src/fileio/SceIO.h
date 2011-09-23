#ifndef PMC_SCEIO_H
#define PMC_SCEIO_H

#include "fileio.h"
#include <pspiofilemgr.h>
#include <pspumd.h>

class SceDio : public PMC_DIR
{
	SceUID dfd;
	bool isUmd;
public:
	SceDio(const char *path)
	{
		isUmd = strncasecmp(path, "disc0:", 6)==0;
		if (isUmd) sceUmdWaitDriveStat(PSP_UMD_READY);
		
		dfd = sceIoDopen(path);
		if ( (dfd==(int)0x80020321) && isUmd)
		{
			sceUmdWaitDriveStat(PSP_UMD_READY);
			dfd = sceIoDopen(path);
		}
	};
	
	bool isValid()
	{
		return dfd >= 0;
	};
	
	int operator()(DIR_ENTRY &entry)
	{
		if (dfd<0) return dfd;
		
		if (isUmd) sceUmdWaitDriveStat(PSP_UMD_READY);
		
		SceIoDirent dirent;
		memset(&dirent, 0, sizeof(SceIoDirent));
		int ret = sceIoDread(dfd, &dirent);
		if (ret<0) return ret;
		
		free(entry.name);
		delete entry.lfn;
		
		entry.name = strdup(dirent.d_name);
		entry.lfn = NULL;
		entry.dtype = ( FIO_SO_ISDIR(dirent.d_stat.st_attr) || \
				FIO_S_ISDIR(dirent.d_stat.st_mode) ) ? DT_DIR : DT_REG;
		
		return ret;
	};
	
	~SceDio()
	{
		if (dfd>=0) sceIoDclose(dfd);
	};
};

class SceFio : public PMC_FILE
{
	SceUID fd;
public:
	SceFio(const char *file)
	{
		fd = sceIoOpen(file, PSP_O_RDONLY, 0);
	};
	
	bool isValid()
	{
		return fd>=0;
	};
	
	int operator()(void *buf, size_t size)
	{
		return sceIoRead(fd, buf, size);
	};
	
	int64_t lseek(int64_t pos, int whence)
	{
		return sceIoLseek(fd, pos, whence);
	};
	
	~SceFio()
	{
		if (fd>=0) sceIoClose(fd);
	};
};

#endif //PMC_SCEIO_H
