#ifndef __PMC_FONT__
#define __PMC_FONT__

#include <cstring>
#include <intraFont_mod.h>
#include <cstdio>
#include <cstdarg>
//#include "drawing.h"

/*
	NOTE:
		Never forget to disable depth test before printing text
*/
extern bool intrafont_used;

class PMC_FONT
{
	intraFont *font;
public:
	float size;
	PMC_FONT(const char *filename, u32 options=0)
	: size(1.f)
	{
		this->font = intraFontLoad(filename, options);
	};
	~PMC_FONT(){ intraFontUnload(this->font); };
	
	bool isValid(){ return (this->font!=NULL); };
	
	u16 get_ySize(){ return this->font->texYSize; };
	u16 get_height(){ return get_ySize()*this->size; };
	
	void set_encoding(u32 cp)
	{
		intraFontSetEncoding(this->font, cp*0x00010000);
	}
	
	void set_style(float scale, u32 color, u32 shadow, u32 options)
	{
		size = scale;
		intraFontSetStyle(this->font, size, color, shadow, options);
	};
	
	float print(const char *txt, float x, float y, float width=0.0f, int len=0)
	{
		intrafont_used = true;
		return intraFontPrintColumnEx(this->font, x, y, width, txt, (len==0) ? strlen(txt) : len);
	};
	
	float print_ucs2(const u16 *txt, float x, float y, float width=0.0f, int len=0)
	{
		intrafont_used = true;
		return intraFontPrintColumnUCS2Ex(this->font, x, y, width, txt, (len==0) ? cccStrlenUCS2(txt) : len);
	};
	
	void __attribute__((format (printf, 4, 5)))
	printf(float x, float y, const char *txt, ...)
	{
		char str[1024];
		
		va_list arglist;
		va_start(arglist, txt);
		vsnprintf(str, 1024, txt, arglist);
		va_end(arglist);
		
		str[1023] = '\0';
		
		this->print(str, x, y);
	};
	
	float txtlen(const char *str){ return intraFontMeasureText(this->font, str); };
	float txtlen_ucs2(const u16 *str){ return intraFontMeasureTextUCS2(this->font, str); };
};
extern PMC_FONT *font;

inline void init_font(){ intraFontInit(); };
inline void term_font(){ intraFontShutdown(); };

#endif
