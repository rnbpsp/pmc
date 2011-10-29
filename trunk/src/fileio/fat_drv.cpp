#include "fat_drv.h"

#define PMC_DISK_MS0 0
#define PMC_DISK_EF0 1

#define FAT_FILEATTR_READONLY		0x01
#define FAT_FILEATTR_HIDDEN			0x02
#define FAT_FILEATTR_SYSTEM			0x04
#define FAT_FILEATTR_VOLUME			0x08
#define FAT_FILEATTR_DIRECTORY	0x10
#define FAT_FILEATTR_ARCHIVE		0x20

static const char *drives[2] = {
	"msstor:", "eflash0a0f0"
};

// TODO: kinda feeling lazy right now
//static std::list<std::string> fat_rpath;
static PMC_FAT *fat=NULL;
static char inited_drive = 'm';

bool init_drive(const char d)
{
#ifdef _DEBUG
	assert(sizeof(BOOT_SECTOR)==512);
#endif
	const char *drive;
	if (d=='m')
		drive = drives[0];
	else if (d=='e')
		drive = drives[1];
	else return false;
	
	fat = (PMC_FAT*)malloc(sizeof(PMC_FAT));
	if (!fat) return false;
	
	u64 dbr_pos, total_sec, fat_sec, root_sec, data_sec, data_clus;
	u64 fattbl_size, reserve_sec_size;
	BOOT_SECTOR *dbr = &fat->dbr;
	
	SceUID ffd = sceIoOpen(drive, PSP_O_RDONLY, 0);
	if (ffd<0)
	{
		free(fat);
		return false;
	}
	fat->ffd = ffd;
	
	if (sceIoRead(ffd, &fat->mbr, sizeof(FAT_MBR))!=sizeof(FAT_MBR))
		goto fat_err;
	
	dbr_pos = fat->mbr.part[0].start_sec * 512U;
	if (sceIoLseek(ffd, dbr_pos, PSP_SEEK_SET)!=dbr_pos ||
		sceIoRead(ffd, dbr, sizeof(BOOT_SECTOR))<sizeof(BOOT_SECTOR) )
	{
		dbr_pos = 0;
		if (sceIoLseek(ffd, dbr_pos, PSP_SEEK_SET)!=dbr_pos ||
			sceIoRead(ffd, dbr, sizeof(BOOT_SECTOR))<sizeof(BOOT_SECTOR) )
			goto fat_err;
	}
	if (dbr->dbr->bytes_per_sec!=512U)
		goto fat_err;
	
	fat->dbr_pos = dbr_pos;
	
	total_sec = (dbr->total_sec) ? dbr->total_sec : dbr->big_total_sec;
	fat_sec = (dbr->sec_per_fat) ? dbr->sec_per_fat : dbr->fat32.sec_per_fat;
	root_sec = (dbr->root_entry * 32 + 512 - 1) / 512;
	data_sec = total_sec - dbr->reserved_sec - (dbr->num_fats * fat_sec) - root_sec;
	data_clus = data_sec / dbr->sec_per_clus;

	if (data_clus < 4085) {
		goto fat_err;
	} else if (data_clus < 65525) {
		fat->fat_type = FAT16;
		fat->clus_max = 0xFFF0;
	} else {
		fat->fat_type = FAT32;
		fat->clus_max = 0x0FFFFFF0;
	}
	
	if (fat->fat_type == FAT32) {
		fat->data_pos = 1ull * dbr_pos;
		fat->data_pos += 512ull * (dbr->fat32.sec_per_fat * dbr->num_fats + dbr->reserved_sec);
		fat->root_pos = fat->data_pos;
		fat->root_pos += 512ull * dbr->sec_per_clus * dbr->fat32.root_clus;
		fattbl_size = dbr->fat32.sec_per_fat * 512;
	} else {
		fat->root_pos = 1ull * dbr_pos;
		fat->root_pos += 512ull * (dbr->num_fats * dbr->sec_per_fat + dbr->reserved_sec);
		fat->data_pos = root_pos;
		fat->data_pos += 1ull * dbr->root_entry * sizeof(FAT_ENTRY);
		fattbl_size = dbr->sec_per_fat * 512;
	}
	
	fat->fat_table = (u32*)malloc(fattbl_size);
	if (!fat->fat_table) goto fat_err;
	
	reserve_sec_size = dbr->reserved_sec * 512;
	if (sceIoLseek(ffd, reserve_sec_size, PSP_SEEK_CUR)!=reserve_sec_size+dbr_pos)
		goto fat_err;
	
	if (sceIoRead(ffd, fat->fat_table, fattbl_size)!=fattbl_size)
		goto fat_err2;
	
	if ( fat->fat_type==FAT16 )
	{
		u16 *fattbl = (u16*)(fat->fat_table);
		u32 bytes_per_fat = dbr->sec_per_fat * (512/2);
		
		u32 *new_fattbl = (u32*)malloc(bytes_per_fat*4);
		if (!new_fattbl) goto fat_err2;
		
		fat->fat_table = new_fattbl;
		u16 *old_fattbl = fattbl;
		
		while (bytes_per_fat--)
			*(new_fattbl++) = *(old_fattbl++);
		
		free(old_fattbl);
	}
	
	inited_drive = drive[0];
	return fat;
fat_err2:
	free(fat->fat_table);
fat_err:
	sceIoClose(fat->ffd);
	free(fat);
	fat = NULL;
	return false;
}

void fat_uninit()
{
	if (!fat) return;
	sceIoClose(fat->ffd);
	free(fat->fat_table);
	free(fat);
	fat = NULL;
}

bool
FatDio::isValid()
{
	return fat!=NULL && fat_error==false;
}

static
void get_sfn(char *out, char *sfn, char *ext)
{
	*(out++) = (name[0]==0x05) ? 0xe5 : name[0];
	for(int i=1; i<8 && name[i]!=' '; ++i)
		*(out++) = name[i];
	if (ext[0]!=' ')
	{
		*(out++) = '.';
		for(int i=0; i<3 && ext[i]!=' '; ++i)
			*(out++) = ext[i];
	}
	*out = 0;
}

static FORCE_INLINE
bool weNeed(FAT_ENTRY *in)
{
	switch(*(u8*)in)
	{
		case 0:
		case 0xe5:
			return false;
		default:
			break;
	}
	return true;
}

u32 fat_follow(u32 clus)
{
	
}

bool fat_locate(const char *name)
{
#ifdef _DEBUG
	assert(sizeof(FAT_ENTRY)==sizeof(FAT_LFN));
#endif
	FAT_ENTRY ent;
	char sfn[16];
	if (cur_pos<2)
	{
		if (sceIoLseek(fat->ffd, fat->root_pos, PSP_SEEK_SET)!=fat->root_pos)
			return -1;
			for(int i=0; i<fat->root_clus)
			{
				if (sceIoRead(fat->ffd, &ent, sizeof(FAT_ENTRY))!=sizeof(FAT_ENTRY))
					goto loc_err;
				get_sfn(sfn, ext.sfn, ext.ext);
			};
	}
	return true;
loc_err:
	fat_error = true;
	return false;
}

static bool check_fat_drive(const char d)
{
	if (fat && inited_drive!=d)
		fat_uninit(&fat);
	
	if (!fat && !(fat=fat_init(d)))
		return false;
	
	inited_drive = d;
	
	return true;
}

FatDio::FatDio(const char *path)
: fat_error(true), eoc(true), cur_clus(2)
{
	if (!check_fat_drive(*path)) return;
	
	cur_pos = cur_fat->fat_type==FAT32 ? fat->fat32.root_clus : 1;
	if (strcasecmp(path+3, ":") || strcasecmp(path+3, ":/"))
		return;
	
	char dir[strlen(path) + 1];
	strcpy(dir, strchr(path, ':')+1);
	
	char *tok = strtok(dir, "/");
	if (!tok) return;
	
	do
	{
		if (!fat_locate(tok)) return;
		tok = strtok(NULL, "/");
	} while (tok);
	
	fat_error = false;
}

















