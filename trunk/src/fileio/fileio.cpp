#include "utils.h"
#include "fileio.h"
#include "SceIO.h"


PMC_DIR *pmc_dopen(const char *path)
{
//	if (strncasecmp(path, "ms0:")==0)
		return new SceDio(path);
	
	
}


PMC_FILE *pmc_fopen(const char *file)
{
	return new SceFio(file);
}


