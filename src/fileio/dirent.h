#ifndef PMC_DIRENT_H
#define PMC_DIRENT_H

#include "font.h"
#include "settings.h"
extern "C"
{
	#include "malloc.h"
}

enum DIRENT_TYPE{
	DT_REG,		// regular file
	DT_DIR		// directory
};

class DIR_ENTRY
{
	char* name; // in ascii(required)
	u16 *lfn;		// in ucs2 for display(optional)
	DIRENT_TYPE dtype;
	friend class PMC_DIR;
	friend class SceDio;
public:

/////////////////////////////////////////////
// Contstructors
/////////////////////////////////////////////
	DIR_ENTRY()
	:	name(NULL),
		lfn(NULL),
		dtype(DT_REG)
	{};
	
	DIR_ENTRY(const DIR_ENTRY &in)
	:	dtype(in.dtype)
	{
		name = strdup(in.name);
		
		if (in.lfn)
		{
			const int len = cccStrlenUCS2(in.lfn);
			lfn = new u16[len+1];
			memcpy((void*)lfn, (void*)in.lfn, len*2);
			lfn[len] = 0;
		}
		else lfn = NULL;
	};
	
	DIR_ENTRY(const char *in, const char *in_lfn)
	:	dtype(DT_DIR)
	{
		name = strdup(in);
		if (in_lfn)
		{
			const int len = strlen(in_lfn);
			lfn = new u16[len*2 + 2];
			cccUTF8toUCS2(lfn, len, (const cccCode*)in_lfn);
			lfn[len] = 0;
		}
		else lfn = NULL;
	};
/////////////////////////////////////////////
	
	~DIR_ENTRY()
	{
		free(name);
		delete lfn;
	};
	
	const FORCE_INLINE
	bool isDir() const
	{
		return dtype==DT_DIR;
	};
	
	const FORCE_INLINE
	bool isReg() const
	{
		return dtype==DT_REG;
	};
	
/////////////////////////////////////////////
// Overloaded pperators
/////////////////////////////////////////////
	DIR_ENTRY& operator=(const DIR_ENTRY& in) 
	{
		free(name);
		delete lfn;
		
		name = strdup(in.name);
		
		if (in.lfn)
		{
			const int len = cccStrlenUCS2(in.lfn);
			lfn = new u16[len+1];
			memcpy(lfn, in.lfn, len*2);
			lfn[len] = 0;
		}
		else lfn = NULL;
		
		dtype = in.dtype;
		return *this;
	};
	
	bool operator==(const char *cmp) const
	{
		return strcasecmp(this->name, cmp) == 0;
	};
	
	bool operator==(const DIR_ENTRY &in) const
	{
		if( ( this->isDir() && in.isDir() ) ||
				( this->isReg() && in.isReg() ) )
			if ( strcasecmp(this->name, in.name) == 0 )
				return true;
	
		return false;
	};
	
	bool operator<(const DIR_ENTRY &second) const
	{
		if ( (settings.fileSort_mode & ALPHABETICALLY) \
					 && (this->dtype==second.dtype) )
		{
			if ( /*alphabetical_cmp*/strcasecmp(this->name, second.name) < 0 )
				return true;
			return false;
		}
		
		if ( (settings.fileSort_mode & DIRS_FIRST) \
						&& this->isDir() )
				return true;
		
		return false;
	};
	
	const char *operator()() const
	{
		return this->name;
	};
/////////////////////////////////////////////
	
	const u16 *get_lfn() const
	{
		return this->lfn;
	};
	
	static FORCE_INLINE
	void concat_path(char *path, const DIR_ENTRY &in)
	{
		strcat(path, in.name);
	};
};


#endif //PMC_DIRENT_H
