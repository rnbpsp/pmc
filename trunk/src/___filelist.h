#ifndef PMC_FILELIST_H
#define PMC_FILELIST_H

class DIR_ENTRY
{
public:
	SceIoDirent dirent;
	DIR_ENTRY(){ memset(&dirent, 0, sizeof(SceIoDirent)); };
	DIR_ENTRY(const DIR_ENTRY& in){ memcpy(&dirent, &(in.dirent), sizeof(SceIoDirent)); };
	DIR_ENTRY(SceIoDirent& in){ memcpy(&dirent, &in, sizeof(SceIoDirent)); };
	DIR_ENTRY(const char *in){
		memset(&dirent, 0, sizeof(SceIoDirent));
		strcpy(dirent.d_name, in);
		dirent.d_stat.st_attr = FIO_SO_IFDIR;
		dirent.d_stat.st_mode = FIO_S_IFDIR;
	};
	
	bool isReg() const
	{
		return FIO_SO_ISREG(dirent.d_stat.st_attr) || \
						FIO_S_ISREG(dirent.d_stat.st_mode);
	};
	
	bool isDir() const
	{
		return FIO_SO_ISDIR(dirent.d_stat.st_attr) || \
						FIO_S_ISDIR(dirent.d_stat.st_mode);
	};
	
	/// overloaded operators
/*
	DIR_ENTRY& operator=(const char* in) 
	{
		memset(&dirent, 0, sizeof(SceIoDirent));
		strcpy(dirent.d_name, in);
		dirent.d_stat.st_attr = FIO_SO_IFDIR;
		dirent.d_stat.st_mode = FIO_S_IFDIR;
		return *this;
	};
*/
	DIR_ENTRY& operator=(const DIR_ENTRY& in) 
	{
		memcpy(&dirent, &(in.dirent), sizeof(SceIoDirent));
		return *this;
	};
	
	DIR_ENTRY& operator=(SceIoDirent& in)
	{
		memcpy(&dirent, &in, sizeof(SceIoDirent));
		return *this;
	};
	
	bool operator==(const char *cmp) const
	{
		return strcasecmp(this->dirent.d_name, cmp) == 0;
	};
	
	bool operator==(const DIR_ENTRY &in) const
	{
		if( ( this->isDir() && in.isDir() ) ||
				( this->isReg() && in.isReg() ) )
			if ( strcasecmp(this->dirent.d_name, in.dirent.d_name) == 0 )
				return true;
	
		return false;
	};
	
	bool operator<(const DIR_ENTRY &second) const
	{
		if (settings.fileSort_mode & DIRS_FIRST)
		{
			if (second.isDir() && this->isReg())	return false;
			if (second.isReg() && this->isDir())	return true;
		}
		
		if (settings.fileSort_mode & ALPHABETICALLY)
			if ( alphabetical_cmp(this->dirent.d_name, second.dirent.d_name) < 0 )
				return true;
		
		return false;
	};
	
	const char *operator()() const
	{
		return this->dirent.d_name;
	};
};

#endif //PMC_FILELIST_H
