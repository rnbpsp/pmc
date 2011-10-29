#include "utils.h"
#include "fileio.h"
#include "SceIO.h"

PMC_DIR *pmc_dopen(const char *path)
{
	PMC_DIR *ret = NULL;
//	if ( (!strncasecmp(path, "ms0", 3)) ||
//			(!strncasecmp(path, "ef0", 3)) )
//		ret = new FatDio(path);
	
	if (!ret || !ret->isValid())
	{
		if (ret) delete ret;
		ret = new SceDio(path);
	}
	return ret;
}


PMC_FILE *pmc_fopen(const char *file)
{
	return new SceFio(file);
}


