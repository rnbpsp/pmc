#ifndef PMC_ITEM_LIST_H
#define PMC_ITEM_LIST_H

#include "utils.h"
#include "font.h"
#include <vector>

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif /* MAX_PATH */

#ifndef MAX_FILENAME
#define MAX_FILENAME 256
#endif /* MAX_FILENAME */

#define DIRS_FIRST			1
#define ALPHABETICALLY	2
extern u32 sort_mode;

// for text scrolling,
// not doing this will cause long texts to disappear

//#define too_long(i) font->txtlen(list[i]()) > width
#define too_longEx(xc, s, x, w, wf) \
		if ( font->txtlen(x) > (wf) ) \
			(s) = (xc)+((w)/2); \
		else \
			(s) = (xc)

#define too_long(i) too_longEx(xCoord, scroll, vec[i](), width, width*1.5f)

class PMC_LIST
{
	float scroll;
	float yfirst, dist, xCoord, scale, width;
	unsigned int last_sel, num;
//	bool sel_changed;
	u32 sel_color, sel_shad, item_color, item_shad;
	unsigned int last_selection, last_first;
	int opts, opts_sel;
public:
	
	PMC_LIST(
		u32 select_color,
		u32 select_shad,
		u32 print_color,
		u32 print_shad,
		unsigned int count,	// number of items to be printed
		float y1,							// y coord of the first item
		float distance,				// y distance between items
		float x,
		float size,					// size of text
		float ySize,
		int options=INTRAFONT_ALIGN_LEFT,
		int options_sel=(INTRAFONT_ALIGN_LEFT|INTRAFONT_SCROLL_SEESAW)
	)
	:	scroll(x),
		
		yfirst(y1),
		dist(distance),
		xCoord(x),
		scale(size),
		width(ySize),
		
		last_sel(0),
		num(count),
	//	sel_changed(false),
		sel_color(select_color),
		sel_shad(select_shad),
		item_color(print_color),
		item_shad(print_shad),
		last_selection(0),
		last_first(0),
		opts(options),
		opts_sel(options_sel)
	{};
	//~PMC_LIST(){};
	
	bool show_scroll(){ return last_first!=0; };
	float scroll_pos(unsigned int top)
	{
		return (top==0) ? 0 : (top / (float)last_first);
	};
	float scroll_size(size_t size)
	{
		return num / (float)size;
	};
	
	template<class T> FORCE_INLINE
	void fixup(std::vector<T> &vec)
	{
			last_sel = 0;
		//	sel_changed = false;
		//	char dir[1024];
		//	make_fullPath(dir, directory);
			
			if (vec.empty())
			{
				last_selection = last_first= 0;
				return;
			}
			else if (vec.size()<num)
			{
				last_selection = vec.size()-1;
				last_first= 0;
			}
			else
			{
				last_selection = num-1;
				last_first = vec.size()-num;
			}
			
		//	scroll = too_long(0) ? xCoord+(width/2) : xCoord ;
			too_long(0);
	};
	
	void select_item(unsigned int index, unsigned int& f1rst, unsigned int& sel)
	{
		if (index==0 && index>last_selection)
		{
			f1rst = sel = 0;
		}
		else if (index >= last_first && index <=last_selection)
		{
			f1rst = last_first;
			sel = index-last_first;
		}
		else
		{
			f1rst = index;
			sel = 0;
		}
	};
	
	template<class T>
	void print(
		unsigned int first,			// index of the first item to be printed
		unsigned int selected,		// index of the selected item from the first to be printed
		std::vector<T> &vec
	)
	{
			if (vec.empty())
			{
				font->set_style(scale, item_color, item_shad, opts);
				font->print("* EMPTY *", xCoord, yfirst);//, width );
				return;
			}
	
			const unsigned int cur_sel = first+selected;
			if (cur_sel!=last_sel)
			{
				last_sel = cur_sel;
		//		scroll = too_long(cur_sel) ? xCoord+(width/2) : xCoord ;
					too_long(cur_sel);
		//		sel_changed = true;
			}
			
			for(unsigned int i=0; i<num && (first+i)<vec.size(); ++i)
			{
				//	const char *item = strcmp(list[first+i](), "..") == 0 ? "< . . >" : list[first+i]() ;
					const char *item = vec[first+i]();
					const float ypos = yfirst + ((float)i * dist);
					
					if (i==selected)
					{
						font->set_style(scale, sel_color, sel_shad, opts_sel);
						if (opts_sel&0x00002000)
							scroll = font->print(item, scroll, ypos, width );
						else
							scroll = font->print(item, xCoord, ypos, width );
					}
					else
					{
						sceGuScissor(xCoord-5,0,xCoord+width,272);
						font->set_style(scale, item_color, item_shad, opts);
						font->print(item, xCoord, ypos);
						sceGuScissor(0, 0, 480, 272);
					}
			}
	};

	template<class T>
	void print(
		unsigned int first,			// index of the first item to be printed
		unsigned int selected,		// index of the selected item from the first to be printed
		std::vector<T> &vec,
		const char* (*read_func)(int),
		int X, // diffferent from xCoord if using align right
		int options
	)
	{
			if (vec.empty())
			{
				font->set_style(scale, item_color, item_shad, options);
				font->print("* EMPTY *", xCoord, yfirst);//, width );
				return;
			}
	
			const unsigned int cur_sel = first+selected;
			if (cur_sel!=last_sel)
			{
				last_sel = cur_sel;
		//		scroll = too_long(cur_sel) ? xCoord+(width/2) : xCoord ;
					too_long(cur_sel);
		//		sel_changed = true;
			}
			
			for(unsigned int i=0; i<num && (first+i)<vec.size(); ++i)
			{
				//	const char *item = strcmp(list[first+i](), "..") == 0 ? "< . . >" : list[first+i]() ;
					const char *item = (*read_func)(first+i);
					const float ypos = yfirst + ((float)i * dist);
					
					if (i==selected)
					{
						font->set_style(scale, sel_color, sel_shad, options);
						font->print(item, X, ypos, width );
					}
					else
					{
						sceGuScissor(xCoord-5,0,xCoord+width,272);
						font->set_style(scale, item_color, item_shad, options);
						font->print(item, X, ypos);
						sceGuScissor(0, 0, 480, 272);
					}
			}
	};

	void up(unsigned int& f1rst, unsigned int& sel)
	{
			if (sel==0)
			{
				if ( f1rst==0 )
				{
					f1rst = last_first;
					sel = last_selection;
				} else --f1rst;
			} else --sel;
	};
	
	void down(unsigned int& f1rst, unsigned int& sel)
	{
			if (sel==last_selection)
			{
				if ( f1rst>=last_first )
				{
					f1rst = 0;
					sel = 0;
				} else ++f1rst;
			} else ++sel;
	};
	
	void page_up(unsigned int& f1rst, unsigned int& sel)
	{
			if (sel==0)
				// TODO
				for(unsigned int i=0; i<num && f1rst!=0; ++i)
					--f1rst;
			else
				sel = 0;
	};
	
	void page_down(unsigned int& f1rst, unsigned int& sel)
	{
			if (sel==last_selection)
			{/* TODO
				if (f1rst+num<last_first)
					f1rst += num;
				else
					f1rst = last_first;*/
				for(unsigned int i=0; i<num && f1rst<last_first; ++i)
					++f1rst;
			}
			else
				sel = last_selection;
	};
};



#endif //PMC_ITEM_LIST_H