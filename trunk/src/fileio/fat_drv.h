#ifndef PMC_FAT_DRIVER
#define PMC_FAT_DRIVER

#include "utils.h"

struct PARTITION
{
	u8 stat;
	u8 start[3];
	u8 type;
	u8 end[3];
	u32 start_sec;
	u32 total_sec;
} PACKED;

struct FAT_MBR
{
	u8 code_area[446];
	PARTITION part[4];
	u16 sig;
} PACKED;

struct BOOT_SECTOR
{
	u8 jmp_boot[3];
	char oem_name[8];
	u16 bytes_per_sec;
	u8 sec_per_clus;
	u16 reserved_sec;
	u8 num_fats;
	u16 root_entry;
	u16 total_sec;
	u8 media_type;
	u16 sec_per_fat;
	u16 sec_per_track;
	u16 heads;
	u32 hidd_sec;
	u32 big_total_sec;
	union
	{
		struct
		{
			u8 drv_num;
			u8 reserved;
			u8 boot_sig;
			u8 vol_id[4];
			char vol_lab[11];
			char file_sys_type[8];
		} PACKED fat16;
		struct
		{
			u32 sec_per_fat;
			u16 extend_flag;
			u16 sys_ver;
			u32 root_clus;
			u16 info_sec;
			u16 back_sec;
			u8 reserved[10];
		} PACKED fat32;
	};
	u8 exe_code[448];
	u16 ending_flag;
} PACKED;

struct FAT_ENTRY
{
	u8 name[8];
	u8 ext[3];
	u8 attr;
	u8 reserved;
	u8 create_time_ms;
	u16 create_time;
	u16 create_date;
	u16 last_access;
	u16 clus_high;
	u16 lastmod_time;
	u16 lastmod_date;
	u16 cluse_low;
	u16 filesize;
} PACKED;

struct FAT_LFN
{
	union {
		struct {
			u8 order:6;
			u8 isLast:1;
			u8 isDeleted:1;
		};
		u8 ord;
	};
	u16 name0[5];
	u8 attr;
	u8 reserved;
	u8 checksum;
	u16 name1[6];
	u16 clus;
	u16 name2[2];
} PACKED;

struct PMC_FAT
{
	SceUID ffd;
	u64 dbr_pos;
	u64 root_pos;
	u64 data_pos;
	u64 bytes_per_clus;
	u32 clus_max;
	u32 *fat_table;
	FAT_TYPE fat_type;
	FAT_MBR mbr;
	BOOT_SECTOR dbr;
};

static bool check_fat_drive(const char d)
{
	if (fat_inited && opened_drive!=d)
	{
		fat_uninit();
		fat_inited = false;
	}
	
	if (!fat_inited && !fat_init(d))
		return false;
	
	fat_inited = true;
	opened_drive = d;
	
	return true;
}

class FatDio : public PMC_DIR
{
	bool fat_error;
	bool eoc;
	u32 cur_clus;
public:
	FatDio(const char *path);
	
	bool isValid();
	
	int operator()(DIR_ENTRY &entry);
}

#endif //PMC_FAT_DRIVER