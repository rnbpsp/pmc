
#include <utils.h>
#include <decoders/audio_dec.h>
#include <libccc_mod.h>
#include <player.h>

u16 tag_info[4][256];

static NOINLINE void fill_avtags(AVDictionary *metadata, const cccCode *filename);
static NOINLINE void fill_asftags(const char *fullpath, const cccCode *filename);

void fill_tags(AVDictionary *metadata, const char *fullpath, const cccCode *filename)
{
	if (metadata)
		fill_avtags(metadata, filename);
	else
		fill_asftags(fullpath, filename);
}

static NOINLINE
void fill_avtags(AVDictionary *metadata, const cccCode *filename)
{
	AVDictionaryEntry *avtag = NULL;
	#define get_metadata(field) avtag = av_dict_get(metadata, field, NULL, 0)
	{
			get_metadata("title");
			if (avtag && avtag->value)
			{
				int tlen = cccStrlenUTF8((const cccCode *)avtag->value);
				if (tlen>0)
				{
					cccUTF8toUCS2(tag_info[TAG_TITLE], tlen, (const cccCode *)avtag->value);
					tag_info[TAG_TITLE][tlen] = 0;
				}
				else goto no_title;
			}
			else
			{
no_title:
				int flen = strlen((const char*)filename);
				cccUTF8toUCS2(tag_info[TAG_TITLE], flen, filename);
				tag_info[TAG_TITLE][flen] = 0;
			}
	}
	
	{
			get_metadata("artist");
			tag_info[TAG_ARTIST][0] = 0;
			if (avtag && avtag->value)
			{
				int tlen = cccStrlenUTF8((const cccCode *)avtag->value);
				cccUTF8toUCS2(tag_info[TAG_ARTIST], tlen, (const cccCode *)avtag->value);
				tag_info[TAG_ARTIST][tlen] = 0;
			}
	}
	
	{
			get_metadata("album");
			tag_info[TAG_ALBUM][0] = 0;
			if (avtag && avtag->value)
			{
				int tlen = cccStrlenUTF8((const cccCode *)avtag->value);
				cccUTF8toUCS2(tag_info[TAG_ALBUM], tlen, (const cccCode *)avtag->value);
				tag_info[TAG_ALBUM][tlen] = 0;
			}
	}
	
	{
			get_metadata("copyright");
			tag_info[TAG_COPYRIGHT][0] = 0;
			if (avtag && avtag->value)
			{
				int tlen = cccStrlenUTF8((const cccCode *)avtag->value);
				cccUTF8toUCS2(tag_info[TAG_COPYRIGHT], tlen, (const cccCode *)avtag->value);
				tag_info[TAG_COPYRIGHT][tlen] = 0;
			}
	}
}

#include <taglib.h>
#include <asffile.h>
#include <asftag.h>
using namespace TagLib;
static NOINLINE
void fill_asftags(const char *fullpath, const cccCode *filename)
{
	ASF::File tagfile(fullpath, false);
	
	ASF::Tag *tag = tagfile.tag();
	if( tag )
	{
		if (!tag->title().isEmpty())
		{
			const cccCode *t = (const cccCode *)tag->title().toCString(true);
			int len = cccStrlenUTF8(t);
			if (len>0)
			{
				cccUTF8toUCS2(tag_info[TAG_TITLE], len, t);
				tag_info[TAG_TITLE][len] = 0;
			}
			else goto asf_notitle;
		}
		else
		{
asf_notitle:
			const int flen = strlen((const char *)filename);
			cccUTF8toUCS2(tag_info[TAG_TITLE], flen, filename);
			tag_info[TAG_TITLE][flen] = 0;
		}
		
		
		if (!tag->artist().isEmpty())
		{
			const cccCode *t = (const cccCode *)tag->artist().toCString(true);
			int len = cccStrlenUTF8(t);
			if (len)
			{
				cccUTF8toUCS2(tag_info[TAG_ARTIST], len, t);
				tag_info[TAG_ARTIST][len] = 0;
			}
			else tag_info[TAG_ARTIST][0] = 0;
		}
		else tag_info[TAG_ARTIST][0] = 0;
		
		
		if (!tag->album().isEmpty())
		{
			const cccCode *t = (const cccCode *)tag->album().toCString(true);
			int len = cccStrlenUTF8(t);
			if (len)
			{
				cccUTF8toUCS2(tag_info[TAG_ALBUM], len, t);
				tag_info[TAG_ALBUM][len] = 0;
			}
			else tag_info[TAG_ALBUM][0] = 0;
		}
		else tag_info[TAG_ALBUM][0] = 0;
		
		
		if (!tag->copyright().isEmpty())
		{
			const cccCode *t = (const cccCode *)tag->copyright().toCString(true);
			int len = cccStrlenUTF8(t);
			if (len)
			{
				cccUTF8toUCS2(tag_info[TAG_COPYRIGHT], len, t);
				tag_info[TAG_COPYRIGHT][len] = 0;
			}
			else tag_info[TAG_COPYRIGHT][0] = 0;
		}
		else tag_info[TAG_COPYRIGHT][0] = 0;
	}
	else
	{
		const int flen = strlen((const char *)filename);
		cccUTF8toUCS2(tag_info[TAG_TITLE], flen, filename);
		tag_info[TAG_TITLE][flen] = 0;
		
		tag_info[TAG_ARTIST	][0] = \
		tag_info[TAG_ALBUM	][0] = \
		tag_info[TAG_COPYRIGHT][0] = 0;
	}
}




