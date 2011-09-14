#ifndef __PMC_COLOR_H__
#define __PMC_COLOR_H__

extern "C" {

typedef unsigned char color_bit;
//typedef unsigned long pmc_color;
#define pmc_color u32

#pragma pack(1)
// ************************************************************
//	8888
// ************************************************************
typedef union {
	struct {
		color_bit r:8;
		color_bit g:8;
		color_bit b:8;
		color_bit a:8;
	};
	u32 color;
}PACKED C_ABGR8; // psp's native color

typedef union {
	struct {
		color_bit a:8;
		color_bit r:8;
		color_bit g:8;
		color_bit b:8;
	};
	u32 color;
}PACKED C_BGRA8;

typedef union {
	struct {
		color_bit b:8;
		color_bit g:8;
		color_bit r:8;
		color_bit a:8;
	};
	u32 color;
}PACKED C_ARGB8;

typedef union {
	struct {
		color_bit a:8;
		color_bit b:8;
		color_bit g:8;
		color_bit r:8;
	};
	u32 color;
}PACKED C_RGBA8;

typedef struct {
	color_bit r;
	color_bit g;
	color_bit b;
}PACKED C_BGR8;

typedef struct {
	color_bit b;
	color_bit g;
	color_bit r;
}PACKED C_RGB8;

// ************************************************************
//	5551
// ************************************************************
typedef union {
	struct {
		color_bit r:5;
		color_bit g:5;
		color_bit b:5;
		color_bit a:1;
	};
	u16 color;
}PACKED C_ABGR5; // psp's native color

typedef union {
	struct {
		color_bit a:1;
		color_bit r:5;
		color_bit g:5;
		color_bit b:5;
	};
	u16 color;
}PACKED C_BGRA5;

typedef union {
	struct {
		color_bit b:5;
		color_bit g:5;
		color_bit r:5;
		color_bit a:1;
	};
	u16 color;
}PACKED C_ARGB5;

typedef union {
	struct {
		color_bit a:1;
		color_bit b:5;
		color_bit g:5;
		color_bit r:5;
	};
	u16 color;
}PACKED C_RGBA5;


// ************************************************************
//	5650
// ************************************************************

typedef union {
	struct {
		color_bit r:5;
		color_bit g:6;
		color_bit b:5;
	};
	u16 color;
}PACKED C_BGR6; // psp's native color

typedef struct {
	struct {
		color_bit b:5;
		color_bit g:6;
		color_bit r:5;
	};
	u16 color;
}PACKED C_RGB6;

// ************************************************************
//	4444
// ************************************************************
typedef union {
	struct {
		color_bit r:4;
		color_bit g:4;
		color_bit b:4;
		color_bit a:4;
	};
	u16 color;
}PACKED C_ABGR4; // 0xABGR, psp's native color

typedef union {
	struct {
		color_bit a:4;
		color_bit r:4;
		color_bit g:4;
		color_bit b:4;
	};
	u16 color;
}PACKED C_BGRA4; // 0xBGRA

typedef union {
	struct {
		color_bit b:4;
		color_bit g:4;
		color_bit r:4;
		color_bit a:4;
	};
	u16 color;
}PACKED C_ARGB4; // 0xARGB

typedef union {
	struct {
		color_bit a:4;
		color_bit b:4;
		color_bit g:4;
		color_bit r:4;
	};
	u16 color;
}PACKED C_RGBA4; // 0xRGBA
#pragma pack()

}// extern "C"

// generates colors usable by the gu (0xABGR)
inline u32 RGBA8(u8 r, u8 g, u8 b, u8 a=0xff)
{
	u32 ret;
	asm(
		"ins %0, %4, 24, 8"		"\n\t"
		"ins %0, %3, 16, 8"		"\n\t"
		"ins %0, %2, 8,  8"		"\n\t"
		"ins %0, %1, 0,  8"		"\n\t"
	: "=r" (ret)
	: "r" (r), "r" (g), "r" (b), "r" (a)
	);
	return ret;
}

inline u16 RGBA5(u8 r, u8 g, u8 b, u8 a=0xff)
{
	u32 ret;
	asm(
		"ins %0, %4, 15, 1"		"\n\t"
		"ins %0, %3, 10, 5"		"\n\t"
		"ins %0, %2, 5,  5"		"\n\t"
		"ins %0, %1, 0,  5"		"\n\t"
	: "=r" (ret)
	: "r" (r), "r" (g), "r" (b), "r" (a)
	);
	return ret;
}

inline u16 RGBA4(u8 r, u8 g, u8 b, u8 a=0xff)
{
	u32 ret;
	asm(
		"ins %0, %4, 12, 4"		"\n\t"
		"ins %0, %3, 8,  4"		"\n\t"
		"ins %0, %2, 4,  4"		"\n\t"
		"ins %0, %1, 0,  4"		"\n\t"
	: "=r" (ret)
	: "r" (r), "r" (g), "r" (b), "r" (a)
	);
	return ret;
}

inline u16 RGB6(u8 r, u8 g, u8 b)
{
	u32 ret = 0;
	asm(
		"ins %0, %3, 11, 5"		"\n\t"
		"ins %0, %2, 5,  6"		"\n\t"
		"ins %0, %1, 0,  5"		"\n\t"
	: "=r" (ret)
	: "r" (r), "r" (g), "r" (b)
	);
	return ret;
}

// 8888
inline u32 BGRA8(u8 b, u8 g, u8 r, u8 a=0xff)	{ return RGBA8(r,g,b,a); }
inline u32 ARGB8(u8 a, u8 r, u8 g, u8 b)				{ return RGBA8(r,g,b,a); }
inline u32 ABGR8(u8 a, u8 b, u8 g, u8 r)				{ return RGBA8(r,g,b,a); }

// 5551
inline u16 BGRA5(u8 b, u8 g, u8 r, u8 a=0xff)	{ return RGBA5(r,g,b,a); }
inline u16 ARGB5(u8 a, u8 r, u8 g, u8 b)				{ return RGBA5(r,g,b,a); }
inline u16 ABGR5(u8 a, u8 b, u8 g, u8 r)				{ return RGBA5(r,g,b,a); }

// 4444
inline u16 BGRA4(u8 b, u8 g, u8 r, u8 a=0xff)	{ return RGBA4(r,g,b,a); }
inline u16 ARGB4(u8 a, u8 r, u8 g, u8 b)				{ return RGBA4(r,g,b,a); }
inline u16 ABGR4(u8 a, u8 b, u8 g, u8 r)				{ return RGBA4(r,g,b,a); }

// 5650
inline u16 BGR6(u8 b, u8 g, u8 r) 							{ return RGB6(r,g,b); }

// used to pass colors to functions as gcc don't optimize
// use these only for constants
inline u32 RGB(u8 r, u8 g, u8 b, u8 a=0xff)
{
	return (r) | (g<<8) | (b<<16) | (a<<24);
}

// predefined opaque colors
#define COL_WHITE 0xffffffff
#define COL_BLACK RGB(0,0,0)

#endif
