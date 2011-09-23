#ifndef PMC_FILEIO_H
#define PMC_FILEIO_H

#include <psptypes.h>
#include "./dirent.h"

class PMC_DIR
{
public:
	virtual int operator()(DIR_ENTRY &entry)=0;
	virtual bool isValid()=0;
	virtual ~PMC_DIR(){};
};

class PMC_FILE
{
public:
	virtual int operator()(void *buf, size_t size)=0;
	virtual int64_t lseek(int64_t pos, int whence)=0;
	virtual bool isValid()=0;
	virtual ~PMC_FILE(){};
};

PMC_DIR *pmc_dopen(const char *path);
PMC_FILE *pmc_fopen(const char *file);

#endif //PMC_FILEIO_H
