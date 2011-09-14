#ifndef PSP_MININI_GLUE
#define PSP_MININI_GLUE

#include <pspkernel.h>
#include <psptypes.h>
#include <pspiofilemgr.h>
#include <stdio.h>

#define INI_BUFFERSIZE 1024
#define INI_LINETERM "\r\n"

#define INI_FILETYPE                  SceUID
#define ini_openread(filename,file)   ((*(file) = sceIoOpen((filename), PSP_O_RDONLY, 0)) >= 0)
#define ini_openwrite(filename,file)  ((*(file) = sceIoOpen((filename), PSP_O_WRONLY, 0777)) >= 0)
#define ini_close(file)               ( sceIoClose(*(file)) )
#define ini_read(buffer,size,file)    (sceIoRead(*(file),(buffer),(size)) == (size))
#define ini_write(buffer,file)        ( sceIoWrite(*(file),(buffer),strlen(buffer)) )
#define ini_rename(source,dest)       ( sceIoRename((source), (dest)) )
#define ini_remove(filename)          ( sceIoRemove(filename) )

#define INI_FILEPOS                   int64_t
#define ini_tell(file,pos)            ( *(pos) = sceIoLseek(*(file), 0, SEEK_CUR) )
#define ini_seek(file,pos)            ( sceIoLseek(*(file), *(pos), SEEK_SET) )

/* for floating-point support, define additional types and functions */
#define INI_REAL                      float
#define ini_ftoa(string,value)        sprintf((string),"%f",(value))
#define ini_atof(string)              (INI_REAL)strtod((string),NULL)

#endif //PSP_MININI_GLUE
