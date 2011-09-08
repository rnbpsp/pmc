/* SCE CONFIDENTIAL 
 PSP (TM) Programmer Tool Runtime Library Release 1.5.0 
 * 
 * Copyright (C) 2004 Sony Computer Entertainment Inc. 
 * All Rights Reserved. 
 * 
 */ 
/* Header for Font Library 
 * Version 1.00 
 * Shift-JIS 
 * 
 * Libfont.h 
 * 
 * Version Date Design Log 
 * ------------------------------------------------- ------------------- 
 * 1.00 2004-09-10 kaku first version 
 */ 


#ifndef _SCE_LIBFONT_H_ 
#define _SCE_LIBFONT_H_ 


#include <psptypes.h> 


#if defined (_LANGUAGE_C_PLUS_PLUS) || defined (__cplusplus) || defined (c_plusplus) 
extern "C" { 
#endif /* defined (_LANGUAGE_C_PLUS_PLUS) || defined (__cplusplus) || defined (c_plusplus) */ 


/* Macro definitions ---------------------------------------------- ---------*/ 
/* Nothing */ 


/* Constant definitions ---------------------------------------------- ------*/ 
#define SCE_FONT_MAX_OPEN (9) /* J maximum number of fonts that can be open simultaneously */ 

/* J * Code List */
#define SCE_FONT_ERROR_BASEVALUE (0x0000) 
#define SCE_FONT_NOERROR (SCE_OK) /* J No error (normal) */ 

#define SCE_FONT_ERR_NOMEMORY \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0001) /* J 0x80460001: Failed to allocate memory */ 
#define SCE_FONT_ERR_LIBID \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0002) /* J 0x80460002: Invalid library instance */ 
#define SCE_FONT_ERR_ARG \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0003) /* J 0x80460003: Invalid argument */ 
#define SCE_FONT_ERR_NOFILE \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0004) /* J 0x80460004: file does not exist */ 

#define SCE_FONT_ERR_FILEOPEN \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0005) /* J 0x80460005: failed to open file */ 
#define SCE_FONT_ERR_FILECLOSE \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0006) /* J 0x80460006: Failed to close file */ 
#define SCE_FONT_ERR_FILEREAD \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0007) /* J 0x80460007: Failed to read file */ 
#define SCE_FONT_ERR_FILESEEK \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0008) /* J 0x80460008: failed to seek the file */ 

#define SCE_FONT_ERR_TOOMANYOPENED \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x0009) /* J 0x80460009: too many fonts open */ 

#define SCE_FONT_ERR_ILLEGALVERSION \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x000a) /* J 0x8046000a: does not support the version of the font */ 
#define SCE_FONT_ERR_DATAINCONSISTENT \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x000b) /* J 0x8046000b: Data inconsistencies in font */ 
#define SCE_FONT_ERR_EXPIRED \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x000c) /* J 0x8046000c: Using expired */ 

#define SCE_FONT_ERR_REGISTRY \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x000d) /* J 0x8046000d: Factors related to the system */ 
#define SCE_FONT_ERR_NOSUPPORT \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0x000e) /* J 0x8046000e: not supported by source */ 

#define SCE_FONT_ERR_UNKNOWN \
    (SCE_ERROR_MAKE_ERROR (SCE_ERROR_FACILITY_FONT, \
SCE_FONT_ERROR_BASEVALUE) \
     | 0xffff) /* J 0x8046ffff: Unknown error */ 

/* J * font-family Codes */
enum SceFontFamilyCode { 
    SCE_FONT_DEFAULT_FAMILY_CODE = (0), /* J * System Standard */

    SCE_FONT_FAMILY_SANSERIF = (1), 
    SCE_FONT_FAMILY_SERIF = (2), 

    SCE_FONT_FAMILY_ROUNDED = (3) 
}; 

/* J * Style Codes */
enum SceFontStyleCode { 
    SCE_FONT_DEFAULT_STYLE_CODE = (0), /* J * System Standard */

    SCE_FONT_STYLE_REGULAR = (1), /* J * standard design */
    SCE_FONT_STYLE_OBLIQUE = (2), /* J italics */ 
    SCE_FONT_STYLE_NARROW = (3), /* J * fine body */
    SCE_FONT_STYLE_NARROW_OBLIQUE = (4), /* J italics fine */ 
    SCE_FONT_STYLE_BOLD = (5), /* J * bold */
    SCE_FONT_STYLE_BOLD_OBLIQUE = (6), /* J * Bold Italic */
    SCE_FONT_STYLE_BLACK = (7), /* J thicker bold */ 
    SCE_FONT_STYLE_BLACK_OBLIQUE = (8), /* J thicker bold italic */ 

    /* J Note) The following style code is used when specifying the main Japanese font */
    SCE_FONT_STYLE_L = (101), /* J thin */ 
    SCE_FONT_STYLE_M = (102), /* J ? */ 
    SCE_FONT_STYLE_DB = (103), /* J | */ 
    SCE_FONT_STYLE_B = (104), /* J | */ 
    SCE_FONT_STYLE_EB = (105), /* J ? */ 
    SCE_FONT_STYLE_UB = (106) /* J thick */ 
}; 

/* J pspFont_t_userImageBufferRec of the grant pixelFormat */ 
enum SceFontImageByfferPixelFormatType { 
    SCE_FONT_USERIMAGE_DIRECT4_L = (0), /* J is located to the left of the screen in the lower 4 bits of 8-bit 
The 4-bit Dairekutokaragureisukeru */ 
    SCE_FONT_USERIMAGE_DIRECT4_R = (1), /* J is the right of the screen in the lower 4 bits of 8-bit 
The 4-bit Dairekutokaragureisukeru */ 
    SCE_FONT_USERIMAGE_DIRECT8 = (2), /* J Dairekutokaragureisukeru 8-bit 256-color */ 
    SCE_FONT_USERIMAGE_DIRECT24 = (3), /* J RGB24-bit direct color */ 
    SCE_FONT_USERIMAGE_DIRECT32 = (4) /* J RGBA32 bit direct color */ 
}; 

/* J code (using language fonts) */ 
enum SceFontLanguageCode { 
    SCE_FONT_DEFAULT_LANGUAGE_CODE = (0), /* J * unspecified language */
    SCE_FONT_LANGUAGE_J = (1), /* J Japanese */ 
    SCE_FONT_LANGUAGE_LATIN = (2), /* J * English */
    SCE_FONT_LANGUAGE_K = (3), /* J Korean */ 
    SCE_FONT_LANGUAGE_C = (4), /* J * Chinese */

    SCE_FONT_LANGUAGE_CJK = (5) /* J Japanese / Chinese / Korean full support */ 
}; 

/* J region code (using local fonts) */ 
enum SceFontRegionCode { 
    SCE_FONT_GENERIC_REGION_CODE = (0), /* J * unspecified Region */
    SCE_FONT_REGION_001 = (1), /* J 001 Region (Japan) */ 
    SCE_FONT_REGION_002 = (2), /* J 002 Region (Region unknown) */ 
    SCE_FONT_REGION_003 = (3), /* J 003 Region (Region unknown) */ 
    SCE_FONT_REGION_004 = (4), /* J 004 Region (Region unknown) */ 
    SCE_FONT_REGION_005 = (5), /* J 005 Region (Region unknown) */ 
    SCE_FONT_REGION_006 = (6), /* J 006 Region (Region unknown) */ 
    SCE_FONT_REGION_007 = (7) /* J 007 Region (Region unknown) */ 
}; 

/* J country code (country Fontobenda) */ 
enum SceFontFontVendorCountryCode { 
    SCE_FONT_GENERIC_COUNTRY_CODE = (0), /* J * unspecified country */
    SCE_FONT_COUNTRY_JAPAN = (1), /* J Japanese */ 
    SCE_FONT_COUNTRY_USA = (2) /* J USA */ 
}; 

enum SceFontBoolValue {/* J boolean */ 
    SCE_FONT_FALSE = (0), 
    SCE_FONT_TRUE = (1) 
}; 

/* J sub mask value style attributes (specified by the bold handling of raster and ?? mostly) */ 
#define SCE_FONT_SUBSTYLE_VERTICALLAYOUT (0x0001) /* J fonts set vertical layout */ 
#define SCE_FONT_SUBSTYLE_PSEUDO_BOLD (0x0002) /* J bold were subjected to a quasi-processing */ 
#define SCE_FONT_SUBSTYLE_PSEUDO_SLANT (0x0004) /* J italics were subjected to a quasi-processing */ 

/* J maximum length of a string font name */ 
#define SCE_FONT_FONTNAME_LENGTH (64) 
/* J maximum length of the font file name */ 
#define SCE_FONT_FONTFILENAME_LENGTH (64) 

/* J access mode */ 
enum SceFontDataAccessMode { 
    SCE_FONT_FILEBASEDSTREAM = (0), 
    SCE_FONT_MEMORYBASEDSTREAM = (1) /* J Unsupported */ 
}; 


/* Variable type definitions --------------------------------------------- --*/ 
/* J-type general */ 
typedef unsigned long long SceFont_t_u64; /* J-type 64-bit unsigned */ 
typedef signed long long SceFont_t_s64; /* J signed 64 bit type */ 
typedef unsigned long SceFont_t_u32; /* J-type 32-bit unsigned */ 
typedef signed long SceFont_t_s32; /* J signed 32 bit type */ 
typedef unsigned short SceFont_t_u16; /* J-type 16-bit unsigned */ 
typedef signed short SceFont_t_s16; /* J signed 16 bit type */ 
typedef unsigned char SceFont_t_u8; /* J 8-bit unsigned type */ 
typedef signed char SceFont_t_s8; /* J signed 8 bit type */ 
typedef float SceFont_t_f32; /* J * 32-bit floating point */
typedef double SceFont_t_f64; /* J * 64-bit floating point */
typedef SceFont_t_u32 SceFont_t_bool; /* J boolean */ 

/* J-types handle */ 
typedef void * SceFont_t_libId; /* J sceFont library handle */ 
typedef void * SceFont_t_fontId; /* J sceFont font handle */ 
typedef void * SceFont_t_pointer; /* J a generic pointer type */ 
typedef void * SceFont_t_handle; /* J-type generic ID (SceFont_t_pointer synonym) */ 

/* J a special type */ 
typedef SceFont_t_s32 SceFont_t_error; /* J for error value */ 
typedef SceFont_t_s32 SceFont_t_int; /* J integer */ 
typedef SceFont_t_u16 SceFont_t_charCode; /* J type character code */ 
typedef SceFont_t_charCode * SceFont_t_string; /* J a string */ 
typedef SceFont_t_s32 SceFont_t_fontIndex; /* J decided that the number of system fonts */ 


/* Structure definitions ---------------------------------------------- -----*/ 
typedef struct SceFont_t_irect {/* J * Rectangular generic */
    SceFont_t_u16 width; /* J * 16-bit values */
    SceFont_t_u16 height; /* J * 16-bit values */
} SceFont_t_irect; 

typedef struct SceFont_t_rect {/* J * Rectangular generic */
    SceFont_t_u32 width; /* J 32-bit values */ 
    SceFont_t_u32 height; /* J 32-bit values */ 
} SceFont_t_rect; 

/* J ---- ---- Fontokyasshushisutemu interface to */ 
typedef struct SceFont_t_cacheSystemInterface { 
    SceFont_t_pointer * cacheInstance; /* J * value passed to the cache interface */
    SceFont_t_s32 (* lockFunc) /* J function to lock the cache */ 
( 
SceFont_t_pointer 
); 
    SceFont_t_s32 (* unlockFunc) /* J function to unlock the cache */ 
( 
SceFont_t_pointer 
); 
    SceFont_t_pointer (* findFunc) /* J determine whether a function exists in the cache */ 
( 
SceFont_t_pointer, 
SceFont_t_u32, 
SceFont_t_pointer, 
SceFont_t_bool * 
); 
    SceFont_t_s32 (* writeKeyValueToCacheFunc) /* J key function to write the value */ 
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer 
); 
    SceFont_t_s32 (* write0ToCacheFunc) /* J * writes to the cache function data0 */
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_int 
); 
    SceFont_t_s32 (* write1ToCacheFunc) /* J * writes to the cache function data1 */
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_int 
); 
    SceFont_t_s32 (* write2ToCacheFunc) /* J data2 to the cache write function */ 
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_int 
); 
    SceFont_t_s32 (* write3ToCacheFunc) /* J * writes to the cache function data3 */
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_int 
); 
    SceFont_t_s32 (* read0FromCacheFunc) /* J data0 from cache read function */ 
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer 
); 
    SceFont_t_s32 (* read1FromCacheFunc) /* J data1 from cache read function */ 
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer 
); 
    SceFont_t_s32 (* read2FromCacheFunc) /* J data2 from cache read function */ 
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer 
); 
    SceFont_t_s32 (* read3FromCacheFunc) /* J data3 from cache read function */ 
( 
SceFont_t_pointer, 
SceFont_t_pointer, 
SceFont_t_pointer 
); 
} SceFont_t_cacheSystemInterface; 

/* J ---- structure begin using information that you provide library ---- */ 
typedef struct SceFont_t_initRec { 
    /* J user data ? */ 
    SceFont_t_pointer userData; /* J to the user data pointer */ 

    /* J maximum number of simultaneous open fonts ? */ 
    SceFont_t_u32 maxNumFonts; /* J * The maximum number of simultaneous open fonts */

    /* J instance handle of the cache system ? */ 
    SceFont_t_cacheSystemInterface * cache; /* J caching system instance handle */ 

    /* J function pointer ? */ 
    /* J allocation of functions (allocFunc, freeFunc) */ 
    SceFont_t_pointer (* allocFunc) /* J memory allocation function */ 
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_u32 /* J request size */ 
); 
    void (* freeFunc) /* J memory free function */ 
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_pointer /* J * pointer to free area */
); 

    /* J-based file access functions (open, close, read, seek) */ 
    SceFont_t_handle (* openFunc) /* J open (read only) */ 
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_pointer, /* J file name */ 
SceFont_t_error */* J error value */ 
); 
    SceFont_t_error (* closeFunc) /* J * Close */
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_handle /* J identifier for accessing the file */ 
); 
    SceFont_t_u32 (* readFunc) /* J Read */ 
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_handle, /* J identifier for accessing the file */ 
SceFont_t_pointer, /* J buffer address */ 
SceFont_t_u32, /* J * 1 bytes per read */
SceFont_t_u32, /* J * Reads */
SceFont_t_error */* J error value */ 
); 
    SceFont_t_error (* seekFunc) /* J * Seek */
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_handle, /* J identifier for accessing the file */ 
SceFont_t_u32 /* J where seeking (absolute position from the beginning of the file) */ 
); 
    /* J callbacks when an error occurs (for future versions) */ 
    SceFont_t_s32 (* onErrorFunc) 
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_s32 /* J error value */ 
); 
    /* J callback at the end of loading (enabled in future versions) */ 
    SceFont_t_s32 (* whenDoneReadFunc) 
( 
SceFont_t_pointer, /* J to the user data pointer */ 
SceFont_t_s32 /* J status values */ 
); 
} SceFont_t_initRec; 

/* J ---- ---- font type used to search */ 
typedef struct SceFont_t_fontStyleInfo { 
    SceFont_t_f32 hSize; /* J points (0.0f means a standard value systems) */ 
    SceFont_t_f32 vSize; /* J points (0.0f means a standard value systems) */ 
    SceFont_t_f32 hResolution; /* J Resolution (0.0f means a standard value systems) */
    SceFont_t_f32 vResolution; /* J Resolution (0.0f means a standard value systems) */
    SceFont_t_f32 weight; /* J weights */ 

    SceFont_t_u16 familyCode; /* J Family Code */ 
    SceFont_t_u16 style; /* J * Style */
    SceFont_t_u16 subStyle; /* J * Sub-Style */

    SceFont_t_u16 languageCode; /* J code (0 is none) */ 
    SceFont_t_u16 regionCode; /* J region code (0 is none) */ 
    SceFont_t_u16 countryCode; /* J country code (0 is none) */ 
    SceFont_t_u8 fontName [SCE_FONT_FONTNAME_LENGTH]; /* J Font name string */ 
    SceFont_t_u8 fileName [SCE_FONT_FONTFILENAME_LENGTH]; /* J font file name string */ 
    SceFont_t_u32 extraAttributes; /* J additional attribute information */ 
    SceFont_t_u32 expireDate; /* J expiration date (0 if unlimited) */ 
} SceFont_t_fontStyleInfo; 

/* J ---- data types used to copy a glyph of the user memory area ---- */ 
typedef struct SceFont_t_userImageBufferRec { 
    SceFont_t_u32 pixelFormat; /* J * pixel format */
    SceFont_t_s32 xPos64; /* J * X position of the reference point to write */
    SceFont_t_s32 yPos64; /* J * Y position of a point to write */
    SceFont_t_irect rect; /* J horizontal and vertical size of buffer */ 
    SceFont_t_u16 bytesPerLine; /* J a line next to the number of bytes of buffer */ 
    SceFont_t_u16 reserved; /* J padding */ 
    SceFont_t_u8 * buffer; /* J Pointer to the buffer area */ 
} SceFont_t_userImageBufferRec; 

/* J ---- ---- letter Gurifumetorikusu information */ 
typedef struct SceFont_t_iGlyphMetricsInfo {/* J 26.6 fixed-point representation of the type */ 
    SceFont_t_u32 width64; /* J * width */
    SceFont_t_u32 height64; /* J * Height */
    SceFont_t_s32 ascender64; /* J ascender Note) For an individual character horizontalBearingY64 equivalent to */ 
    SceFont_t_s32 descender64; /* J descender Note) For an individual character horizontalBearingY64 - height64 */ 
    SceFont_t_s32 horizontalBearingX64; /* J for the layout of European writing Bearing X value */ 
    SceFont_t_s32 horizontalBearingY64; /* J for the layout of European writing Bearing Y value */ 
    SceFont_t_s32 verticalBearingX64; /* J for vertical text layout Bearing the X value */ 
    SceFont_t_s32 verticalBearingY64; /* J for vertical text layout of Bearing Y value */ 
    SceFont_t_s32 horizontalAdvance64; /* J X amount of escapement for the layout of European writing */ 
    SceFont_t_s32 verticalAdvance64; /* J for vertical text layout send X amount of characters */ 
} SceFont_t_iGlyphMetricsInfo; 

typedef struct SceFont_t_charInfo {/* J obtained from the header information of the character */ 
    SceFont_t_u32 bitmapWidth; /* J bitmap width */ 
    SceFont_t_u32 bitmapHeight; /* J height of the bitmap */ 
    SceFont_t_u32 bitmapLeft; /* J horizontal position of the origin of the baseline on the bitmap */ 
    SceFont_t_u32 bitmapTop; /* J vertical position of the origin of the baseline on the bitmap */ 
    SceFont_t_iGlyphMetricsInfo glyphMetrics; /* J-character metrics information (Fixed-point representation) */ 
    SceFont_t_u8 reserved0 [2]; /* J * reserved space */
    SceFont_t_u16 reserved1; /* J * reserved space */
} SceFont_t_charInfo; 

typedef struct SceFont_t_fGlyphMetricsInfo {/* J type of floating point representation */ 
    SceFont_t_f32 width; /* J * width */
    SceFont_t_f32 height; /* J * Height */
    SceFont_t_f32 ascender; /* J ascender */ 
    SceFont_t_f32 descender; /* J descender */ 
    SceFont_t_f32 horizontalBearingX; /* J for the layout of European writing Bearing X value */ 
    SceFont_t_f32 horizontalBearingY; /* J for the layout of European writing Bearing Y value */ 
    SceFont_t_f32 verticalBearingX; /* J for vertical text layout Bearing the X value */ 
    SceFont_t_f32 verticalBearingY; /* J for vertical text layout of Bearing Y value */ 
    SceFont_t_f32 horizontalAdvance; /* J X amount of escapement for the layout of European writing */ 
    SceFont_t_f32 verticalAdvance; /* J Y feed rate for vertical text layout character */ 
} SceFont_t_fGlyphMetricsInfo; 

/* J ---- ---- general information about the font data */ 
typedef struct SceFont_t_fontInfo { 
    SceFont_t_iGlyphMetricsInfo maxIGlyphMetrics; /* J maximum metric values (Fixed-point representation) */ 
    SceFont_t_fGlyphMetricsInfo maxFGlyphMetrics; /* J maximum metric values (Floating point representation) */ 
    SceFont_t_u16 maxGlyphBitmapWidth; /* J a maximum width of the bitmap */ 
    SceFont_t_u16 maxGlyphBitmapHeight; /* J a maximum height of the bitmap */ 
    SceFont_t_u32 numChars; /* J the number of character types that are included */ 
    SceFont_t_u32 numSubChars; /* J shape of the number of substrings that are included (such as data for the shadow) */ 
    SceFont_t_fontStyleInfo fontStyleInfo; /* J fonts are actually accessed Style information */ 
    SceFont_t_u8 pixelDepth; /* J Gurifuimejibittomappu number of bits per pixel */ 
    SceFont_t_u8 reserved [3]; /* J padding */ 
} SceFont_t_fontInfo; 

/* J ---- be used as a key structure in Fontokyasshushisutemu ---- */ 
typedef struct SceFontCacheKey {/* J a set of four keys only */ 
    int keyValue0; /* J 0 * Compare key */
    int keyValue1; /* J 1 compares the key */ 
    int keyValue2; /* J 2 compares the key */ 
    int keyValue3; /* J 3 compares key */ 
} SceFontCacheKey; 


/* Prototype definitions (public functions )---------------------------------*/ 
/* J initialize library */ 
extern SceFont_t_libId sceFontNewLib 
( 
 SceFont_t_initRec *, /* J and using a callback function pointer */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J end of library */ 
extern SceFont_t_error sceFontDoneLib 
( 
 SceFont_t_libId /* J Library ID */ 
 ); 
/* J a set of values of the resolution system */ 
extern SceFont_t_error sceFontSetResolution 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_f32, /* J horizontal resolution (float value) */ 
 SceFont_t_f32 /* J vertical resolution (float value) */ 
 ); 

/* J ---- The following is about the search function to handle font ---- */ 
/* J get a list of available fonts */ 
extern SceFont_t_int sceFontGetNumFontList 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J * Get the list of available fonts */
extern SceFont_t_error sceFontGetFontList 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_fontStyleInfo *, /* J * search for the font style */
 SceFont_t_int /* J limit on the number */ 
 ); 
/* J * Find the best fonts */
extern SceFont_t_fontIndex sceFontFindOptimumFont 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_fontStyleInfo *, /* J style of the font you request */ 
 SceFont_t_error */* J codes */ 
 ); 
extern SceFont_t_fontIndex sceFontFindFont 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_fontStyleInfo *, /* J style of the font you request */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J calculate the amount of memory required to handle the file */ 
extern SceFont_t_u32 sceFontCalcMemorySize 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_fontStyleInfo *, /* J style of the font you request */ 
 SceFont_t_error */* J codes */ 
 ); 
extern SceFont_t_error sceFontGetFontInfoByIndexNumber 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_fontStyleInfo *, /* J * search for the font style */
 SceFont_t_fontIndex /* J index value of the font to be examined */ 
 ); 

/* J ---- The following processing functions for a specific font ---- */ 
/* J a user-specified font file open (Memoribesudosutorimu, Fairubesudosutorimu) */ 
/* J Open Font (Memoribesudosutorimu, Fairubesudosutorimu) */ 
extern SceFont_t_fontId sceFontOpen 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_fontIndex, /* J fonts specified index value (0 Shisutemudeforutofonto) */ 
 SceFont_t_u32, /* J-mode value (SceFontDataAccessMode) */ 
 SceFont_t_error */* J error value */ 
 ); 
/* J a user-specified font file open (Memoribesudosutorimu, Fairubesudosutorimu) */ 
SceFont_t_fontId sceFontOpenUserFile 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_pointer, /* J file name (full path) */ 
 SceFont_t_u32, /* J-mode value (SceFontDataAccessMode) */ 
 SceFont_t_error */* J error value */ 
 ); 
/* J fonts in open user-specified memory address (Memoribesudosutorimu) */ 
SceFont_t_fontId sceFontOpenUserMemory 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_pointer, /* J Font data address */ 
 SceFont_t_u32, /* J Font data address */ 
 SceFont_t_error */* J error value */ 
 ); 
/* J fonts closed */ 
extern SceFont_t_error sceFontClose 
( 
 SceFont_t_fontId /* J font handle */ 
 ); 
/* J Get font information */ 
extern SceFont_t_error sceFontGetFontInfo 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_fontInfo */* J font information */ 
 ); 
/* J * Get the glyph info */
extern SceFont_t_error sceFontGetCharInfo 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_charCode, /* J * Character */
 SceFont_t_charInfo */* J * character information */
 ); 
/* J acquired the character of the rectangle */ 
extern SceFont_t_error sceFontGetCharImageRect 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_charCode, /* J * Character */
 SceFont_t_irect */* J * text rectangle */
 ); 
/* J glyph (character) get */ 
extern SceFont_t_error sceFontGetCharGlyphImage 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_charCode, /* J * Character */
 SceFont_t_userImageBufferRec */* J * Destination */
 ); 
/* J glyph (character) get (with a clipping rectangle) */ 
extern SceFont_t_error sceFontGetCharGlyphImage_Clip 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_charCode, /* J * Character */
 SceFont_t_userImageBufferRec *, /* J * Destination */
 SceFont_t_s32, /* J X position of the clip rectangle */ 
 SceFont_t_s32, /* J Y position of the clip rectangle */ 
 SceFont_t_u32, /* J the width of the clip rectangle */ 
 SceFont_t_u32 /* J the height of the clip rectangle */ 
 ); 
/* J rectangular get a string (addressed in future versions) */ 
extern SceFont_t_error sceFontGetStringImageRect 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_u32, /* J-length string */ 
 SceFont_t_string, /* J character code string */ 
 SceFont_t_rect */* J * text rectangle */
 ); 
/* J glyph (string) get (enabled in future versions) */ 
extern SceFont_t_error sceFontGetStringGlyphImage 
( 
 SceFont_t_fontId, /* J font handle */ 
 SceFont_t_u32, /* J-length string */ 
 SceFont_t_string, /* J-character code string */ 
 SceFont_t_userImageBufferRec */* J * Destination */
 ); 
/* J points to convert the units from pixels (horizontal) */ 
extern SceFont_t_f32 sceFontPixelToPointH 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_f32, /* J floating-point pixel values */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J to convert the units from pixels to points (vertical direction) */ 
extern SceFont_t_f32 sceFontPixelToPointV 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_f32, /* J floating-point pixel values */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J point to convert the units to pixels (horizontal) */ 
extern SceFont_t_f32 sceFontPointToPixelH 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_f32, /* J floating point values */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J point to convert the units to pixels (vertical direction) */ 
extern SceFont_t_f32 sceFontPointToPixelV 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_f32, /* J floating point values */ 
 SceFont_t_error */* J codes */ 
 ); 
/* J sets the character code proposal to replace the non-character code */ 
extern SceFont_t_error sceFontSetAltCharacterCode 
( 
 SceFont_t_libId, /* J Library ID */ 
 SceFont_t_charCode /* J * Alternate Character */
 ); 


/* External variables ---------------------------------------------- --------*/ 
/* Nothing */ 


#if defined (_LANGUAGE_C_PLUS_PLUS) || defined (__cplusplus) || defined (c_plusplus) 
} 
#endif /* defined (_LANGUAGE_C_PLUS_PLUS) || defined (__cplusplus) || defined (c_plusplus) */ 


#endif /* _SCE_LIBFONT_H_ */ 
/* DON'T ADD ANYTHING AFTER THIS # endif */ 


/* End of libfont.h */ 

/* 
  Local Variables: 
  tab-width: 8 
  End: 
*/