/* SCE CONFIDENTIAL
PSP(TM) Programmer Tool Runtime Library Release 2.0.0
*/
/* 
* "PSP" Graphic Library Samples
*
* Copyright (C) 2005 Sony Computer Entertainment Inc.
* All Rights Reserved.
*
*   model sample 2
*
*   Version     Date            Log
*  ----------------------------------------------------
*   2.0.0       2005-06-15      use sceGuStart() with LIST_CACHED mode
*                               use sceGmoModelSetMemoryManager3()
*                               use sceGmoModelLoadCacheEx() UnloadCacheEx()
*                               add freeze mode ( "cached packet mode" in old version )
*                               use sceGuBeginPacket(), sceGuEndPacket()
*                               use sceGmoModelAnimateEx(), UpdateEx(), DrawEx()
*                               change heap memory alignment
*/

//================================================================
//  model sample 2
//================================================================

#include <stdlib.h>
#include <malloc.h>
#include <psptypes.h>
#include <kernel.h>
#include <displaysvc.h>
#include <ctrlsvc.h>
#include <geman.h>
#include <libgu.h>
#include <libgum.h>
#include <gmo/gmo_model.h>
#include "my_heap.h"

SCE_MODULE_INFO( Model2, 0, 1, 1 ) ;
int sce_newlib_heap_kb_size = 1024 * 4 ;

#define MAX_OBJS 10
#define N_OBJS 10

#define ENABLE_HEAP_STAT 1

//----------------------------------------------------------------
//  declaration
//----------------------------------------------------------------

static void begin_scene( void ) ;
static void end_scene( void ) ;
static void swap_buffers( void ) ;
static void move_objects( void ) ;
static void draw_objects( void ) ;
static void draw_rect( int x, int y, int w, int h, int color ) ;

static bool load_objects( void ) ;
static void unload_objects( void ) ;
static bool load_file( const char *filename, int **buf, int *size ) ;
static bool glob_files( const char *ext ) ;
static int compare_names( const void *name_ptr1, const void *name_ptr2 ) ;

static void set_enable_bits( void ) ;
static void reload_cache( void ) ;

static void clear_freeze_packet( void ) ;
static void update_freeze_packet( void ) ;
static void draw_freeze_packet( void ) ;
static ScePspFMatrix4 *inverse_orthonormal( ScePspFMatrix4 *m0, const ScePspFMatrix4 *m1 ) ;
static ScePspFVector3 *rotate_vector3( ScePspFVector3 *v0, const ScePspFMatrix4 *m1, const ScePspFVector3 *v2 ) ;

static void init_heap() ;
static void term_heap() ;
static void cleanup_heap( int delay ) ;
static void *my_malloc( size_t size ) ;
static void my_free( void *addr ) ;
static void *my_malloc2( size_t size ) ;
static void my_free2( void *addr ) ;
static void *my_malloc3( size_t size ) ;
static void my_free3( void *addr ) ;

//----------------------------------------------------------------
//  buffers
//----------------------------------------------------------------

#define PACKET_BUFSIZE	(1024*1024)

static char g_packet_buf[ 2 ][ PACKET_BUFSIZE ] __attribute__((aligned(64))) ;
static int g_packet_idx = -1 ;

static ScePspFMatrix4 g_matrix_buf[ 16 ] ;

//----------------------------------------------------------------
//  heap
//----------------------------------------------------------------

#define HEAP2_BUFSIZE	(1024*1024)

static myHeap g_heap2 ;		// for DMA
static myHeap g_heap3 ;		// for eDRAM

//----------------------------------------------------------------
//  variables
//----------------------------------------------------------------

static int g_frame_count = 0 ;
static bool g_running = true ;
static bool g_reload = true ;

static SceGmoModel *g_model = 0 ;
static ScePspFVector3 g_translate = { 0, 0, 0 } ;
static ScePspFVector3 g_rotate = { 0, 0, 0 } ;
static ScePspFVector3 g_scale = { 1, 1, 1 } ;

static int g_vbl_count = -1 ;		// previous vblank count
static int g_vbl_step = 1 ;		// previous vblank step
static int g_vbl_time = 0 ;		// previous vblank time
static int g_cpu_load = 0 ;		// current cpu load
static int g_gpu_load = 0 ;		// current gpu load

//----------------------------------------------------------------
//  file and menu
//----------------------------------------------------------------

static int g_model_size = 0 ;
static int g_model_count = 0 ;
static const char **g_model_names = 0 ;

static const char *g_name_pointers[ 256 ] ;	// for glob
static char g_name_buffer[ 1024 * 64 ] ;	// for glob

enum {
	MENU_MODEL,
	MENU_LIST,
	MENU_DRAW,
	MENU_VERTEX,
	MENU_PIXEL,
	MENU_PRIMITIVE,
	MENU_TEXTURE,
	MENU_FREEZE,
	MENU_OBJECT
} ;

static int g_menu_select = MENU_MODEL ;	// selected menu
static int g_menu_model = 0 ;		// selected model ( 0 .. )
static int g_menu_list = 0 ;		// list mode ( IMMEDIATE / LIST_CACHED )
static int g_menu_draw = 0 ;		// draw mode ( CONCURRENTLY / AFTER CPU )
static int g_menu_vertex = 0 ;		// vertex buffer ( DDR / EDRAM )
static int g_menu_pixel = 0 ;		// pixel buffer ( DDR / EDRAM )
static int g_menu_primitive = 0 ;	// primitive packet ( NORMAL / PRESET )
static int g_menu_texture = 0 ;		// texture packet ( NORMAL / PRESET / OFF )
static int g_menu_freeze = 0 ;		// freeze packet ( 0 .. FREEZE_MAX )
static int g_menu_object = N_OBJS ;	// object count ( 0 .. N_OBJS )

//----------------------------------------------------------------
//  freeze packet
//----------------------------------------------------------------

#define FREEZE_BUFSIZE	(1024*1024)
#define FREEZE_MAX	(50)

static unsigned int g_freeze_buf[ FREEZE_BUFSIZE / 4 ] __attribute__((aligned(64))) ;
static unsigned int *g_freeze_end = g_freeze_buf + FREEZE_BUFSIZE / 4 ;
static unsigned int *g_freeze_top = g_freeze_buf ;
static unsigned int *g_freeze_packets[ FREEZE_MAX ] ;
static unsigned int *g_freeze_packet = 0 ;
static int g_freeze_count = 0 ;
static int g_freeze_dont_update = 0 ;

//----------------------------------------------------------------
//  main
//----------------------------------------------------------------

int main( int argc, const char **argv )
{
	sceKernelChangeCurrentThreadAttr( 0, SCE_KERNEL_TH_USE_VFPU ) ;	// enable VFPU
	sceCtrlSetSamplingMode( SCE_CTRL_MODE_DIGITALANALOG ) ;

	sceGuInit() ;
	sceGuStart( SCEGU_IMMEDIATE, g_packet_buf[ 0 ], PACKET_BUFSIZE ) ;
	sceGuDrawBuffer( SCEGU_PF5551, SCEGU_VRAM_BP_0, SCEGU_VRAM_WIDTH ) ;
	sceGuDispBuffer( SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT, SCEGU_VRAM_BP_1, SCEGU_VRAM_WIDTH ) ;
	sceGuDepthBuffer( SCEGU_VRAM_BP_2, SCEGU_VRAM_WIDTH ) ;
	sceGuOffset( SCEGU_SCR_OFFSETX, SCEGU_SCR_OFFSETY ) ;
	sceGuFinish() ;
	sceGuSync( SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT ) ;
	sceGumSetMatrixStack( g_matrix_buf, 4, 4, 8, 0 ) ;

	//  init heap

	init_heap() ;
	sceGimPictureSetMemoryManager( my_malloc, my_free ) ;	// align >= 4
	sceGimPictureSetMemoryManager2( my_malloc2, my_free2 ) ; // align >= 64 ( for DCache )
	sceGimPictureSetMemoryManager3( my_malloc3, my_free3 ) ; // align >= 64 ( for DCache )
	sceGmoModelSetMemoryManager( my_malloc, my_free ) ;	// align >= 16 ( for VFPU )
	sceGmoModelSetMemoryManager2( my_malloc2, my_free2 ) ;	// align >= 64 ( for DCache )
	sceGmoModelSetMemoryManager3( my_malloc3, my_free3 ) ;	// align >= 64 ( for DCache )

	if ( argc > 1 ) {
		g_model_count = argc - 1 ;
		g_model_names = argv + 1 ;
	} else {
		glob_files( ".gmo" ) ;
	}

	sceGuDisplay( SCEGU_DISPLAY_ON ) ;

	while ( g_running ) {

		//  reload objects

		if ( g_reload ) {
			unload_objects() ;	// unload all
			cleanup_heap( 0 ) ;	// cleanup heaps immediately
			g_packet_idx = -1 ;	// drop current packet buffer

			if ( !load_objects() ) break ;

			set_enable_bits() ;
			reload_cache() ;
			g_vbl_count = -1 ;
		}

		if ( g_menu_list == 0 ) {

			//  single packet buffer

			sceGuStart( SCEGU_IMMEDIATE, g_packet_buf[ 0 ], PACKET_BUFSIZE ) ;
			begin_scene() ;
			move_objects() ;
			draw_objects() ;
			end_scene() ;
			sceGuFinish() ;

		} else if ( g_menu_list == 1 && g_menu_draw == 0 ) {

			//  double packet buffer

			if ( g_packet_idx >= 0 ) {
				void *pkt = g_packet_buf[ g_packet_idx ] ;
				sceGuSendList( SCEGU_QUEUE_TAIL, pkt, 0, 0, 0 ) ;
			}

			g_packet_idx = ( g_packet_idx != 0 ) ? 0 : 1 ;
			void *pkt = g_packet_buf[ g_packet_idx ] ;
			void *buf = ( g_frame_count & 1 ) ? SCEGU_VRAM_BP_0 : SCEGU_VRAM_BP_1 ;

			sceGuStart( SCEGU_LIST_CACHED, pkt, PACKET_BUFSIZE ) ;
			sceGuDrawBufferList( SCEGU_PF5551, buf, SCEGU_VRAM_WIDTH ) ;
			begin_scene() ;
			move_objects() ;
			draw_objects() ;
			end_scene() ;
			int size = sceGuFinish() ;

			sceKernelDcacheWritebackRange( pkt, size ) ;

		} else if ( g_menu_list == 1 && g_menu_draw == 1 ) {

			//  double packet buffer ( draw after cpu )

			g_packet_idx = ( g_packet_idx != 0 ) ? 0 : 1 ;
			void *pkt = g_packet_buf[ g_packet_idx ] ;
			void *buf = ( g_frame_count & 1 ) ? SCEGU_VRAM_BP_1 : SCEGU_VRAM_BP_0 ;

			sceGuStart( SCEGU_LIST_CACHED, pkt, PACKET_BUFSIZE ) ;
			sceGuDrawBufferList( SCEGU_PF5551, buf, SCEGU_VRAM_WIDTH ) ;
			begin_scene() ;
			move_objects() ;
			draw_objects() ;
			end_scene() ;
			int size = sceGuFinish() ;

			sceKernelDcacheWritebackRange( pkt, size ) ;

			if ( g_packet_idx >= 0 ) {
				void *pkt = g_packet_buf[ g_packet_idx ] ;
				sceGuSendList( SCEGU_QUEUE_TAIL, pkt, 0, 0, 0 ) ;
			}

		}
		swap_buffers() ;
		cleanup_heap( 1 ) ;
	}
	unload_objects() ;
	term_heap() ;
	sceGuTerm() ;
	return 0 ;
}

//----------------------------------------------------------------
//  begin and end
//----------------------------------------------------------------

void begin_scene( void )
{
	//  viewport

	sceGuViewport( 2048, 2048, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT ) ;
	sceGuDepthRange( 0xe000, 0x1000 ) ;
	sceGuEnable( SCEGU_SCISSOR_TEST ) ;
	sceGuScissor( 0, 0, SCEGU_SCR_WIDTH, SCEGU_SCR_HEIGHT ) ;
	sceGuClearColor( 0xff800000 ) ;
	sceGuClearDepth( 0 ) ;
	sceGuClearStencil( 0 ) ;
	sceGuClear( SCEGU_CLEAR_ALL ) ;

	//  transform

	sceGumMatrixMode( SCEGU_MATRIX_VIEW ) ;
	sceGumLoadIdentity() ;
	sceGumLookAt( &(ScePspFVector3){ 0, 0, 50 }, &(ScePspFVector3){ 0, 0, 0 }, &(ScePspFVector3){ 0, 1, 0 } ) ;
	sceGumMatrixMode( SCEGU_MATRIX_PROJECTION ) ;
	sceGumLoadIdentity() ;
	sceGumPerspective( 30.0f * 3.14f / 180.0f, SCEGU_SCR_ASPECT, 10.0f, 1000000.0f ) ;
	sceGumMatrixMode( SCEGU_MATRIX_WORLD ) ;

	//  lighting

	sceGuEnable( SCEGU_FOG ) ;
	sceGuFog( 40, 100, 0xff800000 ) ;
	sceGuEnable( SCEGU_LIGHTING ) ;
	sceGuAmbient( 0xff404040 ) ;
	sceGuEnable( SCEGU_LIGHT0 ) ;
	sceGuLight( 0, SCEGU_LIGHT_DIRECTION, SCEGU_DIFFUSE_AND_SPECULAR, &(ScePspFVector3){ -5, 10, 5 } ) ;
	sceGuLightColor( 0, SCEGU_DIFFUSE, 0xffffffff ) ;
	sceGuLightColor( 0, SCEGU_SPECULAR, 0xffffffff ) ;
	sceGuLight( 2, SCEGU_LIGHT_DIRECTION, SCEGU_DIFFUSE, &(ScePspFVector3){ 1, 0, 0 } ) ;
	sceGuLight( 3, SCEGU_LIGHT_DIRECTION, SCEGU_DIFFUSE, &(ScePspFVector3){ 0, 1, 0 } ) ;
	sceGuColorMaterial( SCEGU_DIFFUSE | SCEGU_AMBIENT ) ;

	//  renderstate

	sceGuEnable( SCEGU_ALPHA_TEST ) ;
	sceGuEnable( SCEGU_DEPTH_TEST ) ;
	sceGuEnable( SCEGU_BLEND ) ;
	sceGuEnable( SCEGU_DITHER ) ;
	sceGuEnable( SCEGU_TEXTURE ) ;
	sceGuEnable( SCEGU_CULL_FACE ) ;
	sceGuEnable( SCEGU_CLIP_PLANE ) ;
	sceGuEnable( SCEGU_CULL_PATCH ) ;
	sceGuEnable( SCEGU_NORMAL_REVERSE_PATCH ) ;
	sceGuFrontFace( SCEGU_CCW ) ;
	sceGuShadeModel( SCEGU_SMOOTH ) ;
	sceGuAlphaFunc( SCEGU_GEQUAL, 0, 255 ) ;
	sceGuBlendFunc( SCEGU_ADD, SCEGU_SRC_ALPHA, SCEGU_ONE_MINUS_SRC_ALPHA, 0, 0 ) ;
	sceGuDepthFunc( SCEGU_GEQUAL ) ;
	sceGuTexFilter( SCEGU_LINEAR_MIPMAP_LINEAR, SCEGU_LINEAR ) ;
	sceGuTexFunc( SCEGU_TEX_MODULATE, SCEGU_RGBA ) ;
	sceGuTexWrap( SCEGU_REPEAT, SCEGU_REPEAT ) ;
	sceGuPatchDivide( 4, 4 ) ;
}

void end_scene( void )
{
	static const char *const ListModes[] = { "IMMEDIATE", "LIST_CACHED" } ;
	static const char *const DrawModes[] = { "CONCURRENTLY", "AFTER CPU" } ;
	static const char *const VertexModes[] = { "DDR", "EDRAM" } ;
	static const char *const PixelModes[] = { "DDR", "EDRAM" } ;
	static const char *const PrimitiveModes[] = { "NORMAL", "PRESET" } ;
	static const char *const TextureModes[] = { "NORMAL", "PRESET", "OFF" } ;
	char buf[ 64 ] ;
	if ( g_frame_count * g_vbl_step % 30 < 15 ) {
		sceGuDebugPrint( 0, g_menu_select * 10 + 10, 0xffffffff, ">" ) ;
	}
	const char *name = ( g_model_count == 0 ) ? "none" : g_model_names[ g_menu_model ] ;
	sprintf( buf, "MODEL     %s %dKB", name, ( g_model_size + 1023 ) / 1024 ) ;
	sceGuDebugPrint( 10, 10, 0xffffffff, buf ) ;
	sprintf( buf, "LIST      %s", ListModes[ g_menu_list ] ) ;
	sceGuDebugPrint( 10, 20, 0xffffffff, buf ) ;
	sprintf( buf, "DRAW      %s", DrawModes[ g_menu_draw ] ) ;
	sceGuDebugPrint( 10, 30, 0xffffffff, buf ) ;
	sprintf( buf, "VERTEX    %s", VertexModes[ g_menu_vertex ] ) ;
	sceGuDebugPrint( 10, 40, 0xffffffff, buf ) ;
	sprintf( buf, "PIXEL     %s", PixelModes[ g_menu_pixel ] ) ;
	sceGuDebugPrint( 10, 50, 0xffffffff, buf ) ;
	sprintf( buf, "PRIMITIVE %s", PrimitiveModes[ g_menu_primitive ] ) ;
	sceGuDebugPrint( 10, 60, 0xffffffff, buf ) ;
	sprintf( buf, "TEXTURE   %s", TextureModes[ g_menu_texture ] ) ;
	sceGuDebugPrint( 10, 70, 0xffffffff, buf ) ;
	int size = ( ( g_freeze_top - g_freeze_buf ) * 4 + 1023 ) / 1024 ;
	sprintf( buf, "FREEZE    %d / %d KB", g_menu_freeze, size ) ;
	sceGuDebugPrint( 10, 80, 0xffffffff, buf ) ;
	sprintf( buf, "OBJECT    %d", g_menu_object ) ;
	sceGuDebugPrint( 10, 90, 0xffffffff, buf ) ;

	sprintf( buf, "CPU %d%%", g_cpu_load ) ;
	sceGuDebugPrint( 10, 240, 0xffffffff, buf ) ;
	sprintf( buf, "GPU %d%%", g_gpu_load ) ;
	sceGuDebugPrint( 10, 250, 0xffffffff, buf ) ;
	if ( g_reload ) {
		sceGuDebugPrint( 400, 10, 0xffffffff, "LOADING" ) ;
	}

	#if ( ENABLE_HEAP_STAT )
	sprintf( buf, "DMA %3d KB", ( myHeapGetAllocSize( &g_heap2 ) + 1023 ) / 1024 ) ;
	sceGuDebugPrint( 390, 240, 0xffffffff, buf ) ;
	sprintf( buf, "EDR %3d KB", ( myHeapGetAllocSize( &g_heap3 ) + 1023 ) / 1024 ) ;
	sceGuDebugPrint( 390, 250, 0xffffffff, buf ) ;
	#endif // ENABLE_HEAP_STAT

	sceGuDisable( SCEGU_TEXTURE ) ;
	sceGuDisable( SCEGU_DEPTH_TEST ) ;
	draw_rect( 82, 241, g_cpu_load, 7, 0x8f00 ) ;
	if ( g_menu_draw == 0 ) {
		draw_rect( 82, 251, g_gpu_load, 7, 0x800f ) ;
	} else {
		draw_rect( 82, 251, g_cpu_load, 7, 0x400f ) ;
		draw_rect( 82 + g_cpu_load, 251, g_gpu_load - g_cpu_load, 7, 0x800f ) ;
	}
}

void swap_buffers( void )
{
	//  swap buffers

	int cpu_time = sceKernelGetSystemTimeLow() ;
	sceGuSync( SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT ) ;
	int gpu_time = sceKernelGetSystemTimeLow() ;
	sceDisplayWaitVblankStart() ;
	sceGuSwapBuffers() ;
	g_frame_count ++ ;

	//  performance counter

	int vbl_time = sceKernelGetSystemTimeLow() ;
	int vbl_count = sceDisplayGetVcount() ;
	if ( g_vbl_count >= 0 ) {
		g_vbl_step = vbl_count - g_vbl_count ;
		if ( g_frame_count % 6 == 0 ) {
			g_cpu_load = ( cpu_time - g_vbl_time ) * 100 / 16667 ;
			g_gpu_load = ( gpu_time - g_vbl_time ) * 100 / 16667 ;
		}
	}
	g_vbl_time = vbl_time ;
	g_vbl_count = vbl_count ;
}

//----------------------------------------------------------------
//  move and draw
//----------------------------------------------------------------

void move_objects( void )
{
	float move = g_vbl_step * 0.5f ;
	float turn = g_vbl_step * ( 2.0f * 3.14f / 180.0f ) ;

	SceCtrlData buf ;
	sceCtrlReadBufferPositive( &buf, 1 ) ;

	static int prev = 0 ;
	int state = buf.Buttons ;
	int press = state & ~prev ;
	prev = state ;

	if ( press & SCE_CTRL_SELECT ) {
		g_running = 0 ;
		return ;
	}
	if ( press & SCE_CTRL_START ) {
		g_translate = (ScePspFVector3){ 0, 0, 0 } ;
		g_rotate = (ScePspFVector3){ 0, 0, 0 } ;
		g_menu_model = 0 ;
		g_menu_list = 0 ;
		g_menu_draw = 0 ;
		g_menu_vertex = 0 ;
		g_menu_pixel = 0 ;
		g_menu_primitive = 0 ;
		g_menu_texture = 0 ;
		g_menu_freeze = 0 ;
		g_menu_object = N_OBJS ;

		g_reload = true ;
		glob_files( ".gmo" ) ;
		return ;
	}
	if ( state & SCE_CTRL_LEFT ) g_rotate.y -= turn ;
	if ( state & SCE_CTRL_RIGHT ) g_rotate.y += turn ;
	if ( state & SCE_CTRL_L1 ) g_translate.z -= move ;
	if ( state & SCE_CTRL_R1 ) g_translate.z += move ;
	float dx = ( buf.Lx - 128 ) / 128.0f ;
	float dy = ( buf.Ly - 128 ) / 128.0f ;
	if ( dx > -0.75f && dx < 0.75f ) dx = 0.0f ;
	if ( dy > -0.75f && dy < 0.75f ) dy = 0.0f ;
	g_translate.x += move * dx ;
	g_translate.y -= move * dy ;

	//  menu

	if ( press & SCE_CTRL_UP ) {
		if ( g_menu_select > 0 ) g_menu_select -= 1 ;
	}
	if ( press & SCE_CTRL_DOWN ) {
		if ( g_menu_select < MENU_OBJECT ) g_menu_select += 1 ;
	}

	switch ( g_menu_select ) {
	    case MENU_MODEL : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			int max = g_model_count - 1 ;
			if ( press & SCE_CTRL_CIRCLE ) g_menu_model ++ ;
			if ( press & SCE_CTRL_CROSS ) g_menu_model -- ;
			if ( g_menu_model < 0 ) g_menu_model = 0 ;
			if ( g_menu_model > max ) g_menu_model = max ;
			g_reload = true ;
		}
		break ;
	    }
	    case MENU_LIST : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			g_menu_list = !g_menu_list ;

			g_packet_idx = -1 ;	// drop current packet buffer
			g_frame_count %= 2 ;
		}
		break ;
	    }
	    case MENU_DRAW : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			g_menu_draw = !g_menu_draw ;
		}
		break ;
	    }
	    case MENU_VERTEX : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			g_menu_vertex = !g_menu_vertex ;
			reload_cache() ;
		}
		break ;
	    }
	    case MENU_PIXEL : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			g_menu_pixel = !g_menu_pixel ;
			reload_cache() ;
		}
		break ;
	    }
	    case MENU_PRIMITIVE : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			g_menu_primitive = !g_menu_primitive ;
			reload_cache() ;
		}
		break ;
	    }
	    case MENU_TEXTURE : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			if ( press & SCE_CTRL_CIRCLE ) g_menu_texture ++ ;
			if ( press & SCE_CTRL_CROSS ) g_menu_texture -- ;
			if ( g_menu_texture < 0 ) g_menu_texture = 0 ;
			if ( g_menu_texture > 2 ) g_menu_texture = 2 ;
			set_enable_bits() ;
			reload_cache() ;
		}
		break ;
	    }
	    case MENU_FREEZE : {
		if ( state & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			if ( state & SCE_CTRL_CIRCLE ) g_menu_freeze ++ ;
			if ( state & SCE_CTRL_CROSS ) g_menu_freeze -- ;
			if ( g_menu_freeze < 0 ) g_menu_freeze = 0 ;
			if ( g_menu_freeze > FREEZE_MAX ) g_menu_freeze = FREEZE_MAX ;
			clear_freeze_packet() ;
		}
		break ;
	    }
	    case MENU_OBJECT : {
		if ( press & ( SCE_CTRL_CIRCLE | SCE_CTRL_CROSS ) ) {
			if ( press & SCE_CTRL_CIRCLE ) g_menu_object += 1 ;
			if ( press & SCE_CTRL_CROSS ) g_menu_object -= 1 ;
			if ( g_menu_object < 0 ) g_menu_object = 0 ;
			if ( g_menu_object > MAX_OBJS ) g_menu_object = MAX_OBJS ;
		}
		break ;
	    }
	}
}

void draw_objects( void )
{
	if ( g_menu_freeze > 0 ) {	// freeze packet mode
		update_freeze_packet() ;
		draw_freeze_packet() ;
		return ;
	}

	float step = g_vbl_step * ( 1.0f / 60.0f ) ;

	sceGumMatrixMode( SCEGU_MATRIX_WORLD ) ;
	sceGumLoadIdentity() ;
	sceGumTranslate( &g_translate ) ;
	sceGumRotateY( g_rotate.y ) ;
	sceGumRotateX( g_rotate.x ) ;

	ScePspFMatrix4 m ;
	for ( int i = 0 ; i < g_menu_object ; i ++ ) {
		ScePspFVector3 move = { ( ( i % 2 == 0 ) ? i : -i ) * 10, 0, 0 } ;
		sceGumTranslate( &move ) ;
		sceGumPushMatrix() ;
		sceGumScale( &g_scale ) ;
		sceGumStoreMatrix( &m ) ;
		sceGumPopMatrix() ;

		SceGmoModel *model = g_model ;
		sceGmoModelSetWorldMatrix( model, &m ) ;
		sceGmoModelAnimate( model, step ) ;
		sceGmoModelUpdate( model ) ;
		sceGmoModelDraw( model ) ;

		//  stall control is optional

		if ( g_menu_list == 0 && g_menu_draw == 0 ) {
			sceGuFlush() ;
		}

		step = 0.0f ;
	}
}

void draw_rect( int x, int y, int w, int h, int color )
{
	int format = SCEGU_COLOR_PF4444 | SCEGU_VERTEX_SHORT | SCEGU_THROUGH ;
	short *vertex = (short *)sceGuGetMemory( sizeof( short ) * 8 ) ;
	vertex[ 0 ] = color ;
	vertex[ 1 ] = x ;
	vertex[ 2 ] = y ;
	vertex[ 3 ] = 0 ;
	vertex[ 4 ] = color ;
	vertex[ 5 ] = x + w ;
	vertex[ 6 ] = y + h ;
	vertex[ 7 ] = 0 ;
	sceGuDrawArray( SCEGU_PRIM_RECTANGLES, format, 2, 0, vertex ) ;
}

//----------------------------------------------------------------
//  load and unload
//----------------------------------------------------------------

bool load_objects( void )
{
	unload_objects() ;

	//  load file

	g_model_size = 0 ;
	if ( g_model_count == 0 ) return true ;

	int *data, size ;
	const char *filename = g_model_names[ g_menu_model ] ;
	if ( !load_file( filename, &data, &size ) ) return false ;
	g_model_size = size ;

	//  create model

	g_model = sceGmoModelCreate( 0 ) ;
	sceGmoModelLoadFile( g_model, data, size, 0 ) ;
	sceGmoModelSetCurrentMotion( g_model, 0, 0.0f ) ;

	//  init scaling

	g_scale = (ScePspFVector3){ 1, 1, 1 } ;

	const ScePspFVector4 *box = sceGmoModelGetBoundingBox( g_model ) ;
	if ( box != 0 ) {
		float s = box[ 1 ].x - box[ 0 ].x ;
		float s2 = box[ 1 ].y - box[ 0 ].y ;
		if ( s2 > s ) s = s2 ;
		s2 = box[ 1 ].z - box[ 0 ].z ;
		if ( s2 > s ) s = s2 ;
		if ( s > 0.0f ) {
			s = 10.0f / s ;
			g_scale = (ScePspFVector3){ s, s, s } ;
		}
	}

	free( data ) ;
	g_reload = false ;
	return ( g_model != 0 ) ;
}

void unload_objects( void )
{
	sceGmoModelDelete( g_model ) ;
	g_model = 0 ;

	clear_freeze_packet() ;		// freeze packet is invalid
}

bool load_file( const char *filename, int **buf, int *size )
{
	char fullpath[ 256 ] ;
	sprintf( fullpath, "host0:%s", filename ) ;

	SceUID fd = sceIoOpen( fullpath, SCE_O_RDONLY, 0777 ) ;
	if ( fd < 0 ) return false ;
	*size = sceIoLseek( fd, 0, SCE_SEEK_END ) ;
	if ( *size <= 0 ) {
		sceIoClose( fd ) ;
		return false ;
	}
	sceIoLseek( fd, 0, SCE_SEEK_SET ) ;
	*buf = (int *)malloc( *size ) ;
	if ( *buf == 0 ) {
		sceIoClose( fd ) ;
		return false ;
	}
	sceIoRead( fd, (void *)*buf, (unsigned int)*size ) ;
	sceIoClose( fd ) ;
	return true ;
}

bool glob_files( const char *ext )
{
	g_model_count = 0 ;
	g_model_names = g_name_pointers ;

	SceIoDirent de ;
	SceUID fd = sceIoDopen( "host0:." ) ;
	if ( fd < 0 ) return false ;

	char *name_ptr = g_name_buffer ;
	while ( sceIoDread( fd, &de ) > 0 ) {
		if ( strstr( de.d_name, ext ) != 0 ) {
			g_model_names[ g_model_count ++ ] = name_ptr ;
			strcpy( name_ptr, de.d_name ) ;
			name_ptr += strlen( name_ptr ) + 1 ;
		}
	}
	sceIoDclose( fd ) ;

	if ( g_model_count > 0 ) {
		qsort( g_model_names, g_model_count, sizeof( char * ), compare_names ) ;
	}
	return true ;
}

int compare_names( const void *name_ptr1, const void *name_ptr2 )
{
	return strcmp( *(const char **)name_ptr1, *(const char **)name_ptr2 ) ;
}

//----------------------------------------------------------------
//  set enable bits
//----------------------------------------------------------------

void set_enable_bits( void )
{
	sceGmoModelSetEnableBits( g_model, SCEGMO_ENABLE_FOG, 0 ) ;

	if ( g_menu_texture >= 2 ) {
		sceGmoModelSetEnableMask( g_model, SCEGMO_ENABLE_TEXTURE, ~0 ) ;
		sceGmoModelSetEnableBits( g_model, SCEGMO_ENABLE_TEXTURE, 0 ) ;
	} else {
		sceGmoModelSetEnableMask( g_model, SCEGMO_ENABLE_TEXTURE, 0 ) ;
		sceGmoModelSetEnableBits( g_model, SCEGMO_ENABLE_TEXTURE, ~0 ) ;
	}
}

//----------------------------------------------------------------
//  reload cache
//----------------------------------------------------------------

void reload_cache( void )
{
	//  vertex ( allocate eDRAM )

	if ( g_menu_vertex != 0 ) {
		sceGmoModelLoadCacheEx( g_model, SCEGMO_CACHE_VERTEX ) ;
		sceGmoModelUnloadCacheEx( g_model, SCEGMO_CACHE_PRIMITIVE ) ;
	} else {
		sceGmoModelUnloadCacheEx( g_model, SCEGMO_CACHE_VERTEX ) ;
	}

	//  primitive ( create preset packet )

	if ( g_menu_primitive != 0 ) {
		sceGmoModelLoadCacheEx( g_model, SCEGMO_CACHE_PRIMITIVE ) ;
	} else {
		sceGmoModelUnloadCacheEx( g_model, SCEGMO_CACHE_PRIMITIVE ) ;
	}

	//  pixel ( allocate eDRAM )

	if ( g_menu_pixel != 0 ) {
		sceGmoModelLoadCacheEx( g_model, SCEGMO_CACHE_PIXEL ) ;
		sceGmoModelUnloadCacheEx( g_model, SCEGMO_CACHE_TEXTURE ) ;
	} else {
		sceGmoModelUnloadCacheEx( g_model, SCEGMO_CACHE_PIXEL ) ;
	}

	//  texture ( create preset packet )

	if ( g_menu_texture != 0 ) {
		sceGmoModelLoadCacheEx( g_model, SCEGMO_CACHE_TEXTURE ) ;
	} else {
		sceGmoModelUnloadCacheEx( g_model, SCEGMO_CACHE_TEXTURE ) ;
	}

	clear_freeze_packet() ;		// freeze packet is invalid
}

//----------------------------------------------------------------
//  freeze packet mode
//----------------------------------------------------------------

void clear_freeze_packet( void )
{
	if ( g_menu_freeze == 0 ) return ;

	memset( g_freeze_packets, 0, sizeof( g_freeze_packets ) ) ;
	g_freeze_top = g_freeze_buf ;
	g_freeze_packet = 0 ;
	g_freeze_count = 0 ;

	g_freeze_dont_update = 2 ;	// wait 2 frames
}

void update_freeze_packet( void )
{
	if ( g_menu_freeze == 0 ) return ;

	//  update frame

	float step = g_vbl_step * ( 1.0f / 60.0f ) ;
	sceGmoModelAnimateEx( g_model, step, SCEGMO_ANIMATE_FRAME ) ;

	//  select packet

	int cur = sceGmoModelGetCurrentMotion( g_model ) ;
	SceGmoMotion *motion = sceGmoModelGetMotion( g_model, cur ) ;
	float f = sceGmoMotionGetFrame( motion ) ;
	const float *loop = sceGmoMotionGetFrameLoop( motion ) ;
	float f0 = ( loop == 0 ) ? 0.0f : loop[ 0 ] ;
	float f1 = ( loop == 0 ) ? 0.0f : loop[ 1 ] ;
	f = ( f0 == f1 ) ? 0.0f : ( f - f0 ) / ( f1 - f0 ) ;
	int index = (int)( ( g_menu_freeze - 1 ) * f + 0.5f ) ;
	g_freeze_packet = g_freeze_packets[ index ] ;

	//  update packet

	if ( g_freeze_packet == 0 ) {
		if ( g_freeze_dont_update > 0 ) {	// don't update
			g_freeze_dont_update -- ;
			return ;
		}

		unsigned int *pkt = g_freeze_top ;
		unsigned int size = ( g_freeze_end - g_freeze_top ) * 4 ;

		SceGmoModel *model = g_model ;
		sceGmoModelSetWorldMatrix( model, 0 ) ;
		sceGmoModelAnimateEx( model, 0.0f, SCEGMO_ANIMATE_PARAM ) ;
		sceGmoModelUpdateEx( model, SCEGMO_UPDATE_ALL ) ;
		sceGmoModelDrawEx( model, &pkt, &size, SCEGMO_DRAW_ALL ) ;

		*( pkt ++ ) = SCE_GE_SET_RET() ;		// normal call
		// *( pkt ++ ) = SCE_GE_SET_SIGNAL_RET());	// signal call
		// *( pkt ++ ) = SCE_GE_SET_END_RET());

		size = ( pkt - g_freeze_top ) * 4 ;
		size = ( size + 63 ) / 64 * 64 ;
		sceKernelDcacheWritebackRange( g_freeze_top, size ) ;

		g_freeze_packets[ index ] = g_freeze_top ;
		g_freeze_packet = g_freeze_top ;
		g_freeze_top = g_freeze_top + size / 4 ;
		g_freeze_count ++ ;
	}
}

void draw_freeze_packet( void )
{
	float step = g_vbl_step * ( 1.0f / 60.0f ) ;

	sceGumMatrixMode( SCEGU_MATRIX_WORLD ) ;
	sceGumLoadIdentity() ;
	sceGumTranslate( &g_translate ) ;
	sceGumRotateY( g_rotate.y ) ;
	sceGumRotateX( g_rotate.x ) ;

	ScePspFMatrix4 m ;
	for ( int i = 0 ; i < g_menu_object ; i ++ ) {
		ScePspFVector3 move = { ( ( i % 2 == 0 ) ? i : -i ) * 10, 0, 0 } ;
		sceGumTranslate( &move ) ;
		sceGumPushMatrix() ;
		sceGumScale( &g_scale ) ;
		sceGumStoreMatrix( &m ) ;
		sceGumPopMatrix() ;

		sceGumMatrixMode( SCEGU_MATRIX_VIEW ) ;
		sceGumPushMatrix() ;
		sceGumMultMatrix( &m ) ;
		sceGumMatrixMode( SCEGU_MATRIX_WORLD ) ;

		ScePspFMatrix4 m2 ;
		ScePspFVector3 v2 ;
		inverse_orthonormal( &m2, &m ) ;
		rotate_vector3( &v2, &m2, &(ScePspFVector3){ -5, 10, 5 } ) ;
		sceGuLight( 0, SCEGU_LIGHT_DIRECTION, SCEGU_DIFFUSE_AND_SPECULAR, &v2 ) ;

		if ( g_freeze_packet != 0 ) {
			sceGumUpdateMatrix() ;
			sceGuCallList( g_freeze_packet ) ;
		} else {
			SceGmoModel *model = g_model ;
			sceGmoModelSetWorldMatrix( model, 0 ) ;
			sceGmoModelAnimateEx( model, 0.0f, SCEGMO_ANIMATE_PARAM ) ;
			sceGmoModelUpdate( model ) ;
			sceGmoModelDraw( model ) ;
		}

		sceGumMatrixMode( SCEGU_MATRIX_VIEW ) ;
		sceGumPopMatrix() ;
		sceGumMatrixMode( SCEGU_MATRIX_WORLD ) ;
		step = 0.0f ;
	}
}

ScePspFMatrix4 *inverse_orthonormal( ScePspFMatrix4 *m0, const ScePspFMatrix4 *m1 )
{
	float x = - m1->w.x ;
	float y = - m1->w.y ;
	float z = - m1->w.z ;
	m0->x = (ScePspFVector4){ m1->x.x, m1->y.x, m1->z.x, 0 } ;
	m0->y = (ScePspFVector4){ m1->x.y, m1->y.y, m1->z.y, 0 } ;
	m0->z = (ScePspFVector4){ m1->x.z, m1->y.z, m1->z.z, 0 } ;
	m0->w = (ScePspFVector4){ m1->x.x * x + m1->x.y * y + m1->x.z * z,
	                          m1->y.x * x + m1->y.y * y + m1->y.z * z,
	                          m1->z.x * x + m1->z.y * y + m1->z.z * z, 1 } ;
	return m0 ;
}

ScePspFVector3 *rotate_vector3( ScePspFVector3 *v0, const ScePspFMatrix4 *m1, const ScePspFVector3 *v2 )
{
	float x = v2->x ;
	float y = v2->y ;
	float z = v2->z ;
	v0->x = m1->x.x * x + m1->y.x * y + m1->z.x * z ;
	v0->y = m1->x.y * x + m1->y.y * y + m1->z.y * z ;
	v0->z = m1->x.z * x + m1->y.z * y + m1->z.z * z ;
	return v0 ;
}

//----------------------------------------------------------------
//  heap functions
//----------------------------------------------------------------

void init_heap()
{
	//  for DMA

	void *heap2_addr = memalign( 64, HEAP2_BUFSIZE ) ;
	myHeapInit( &g_heap2, heap2_addr, HEAP2_BUFSIZE ) ;

	//  for eDRAM

	int edram_usage = SCEGU_VRAM_BUFSIZE * 3 ;
	int edram_size = sceGeEdramGetSize() ;
	char *edram_base = (char *)sceGeEdramGetAddr() ;
	myHeapInit( &g_heap3, edram_base + edram_usage, edram_size - edram_usage ) ;
}

void term_heap()
{
	free( myHeapGetBufAddr( &g_heap2 ) ) ;

	myHeapTerm( &g_heap2 ) ;
	myHeapTerm( &g_heap3 ) ;
}

void cleanup_heap( int delay )
{
	myHeapCleanup( &g_heap2, delay ) ;
	myHeapCleanup( &g_heap3, delay ) ;
}

void *my_malloc( size_t size )
{
	return memalign( 16, size ) ;
}

void my_free( void *addr )
{
	return free( addr ) ;
}

void *my_malloc2( size_t size )
{
	return myHeapAlloc( &g_heap2, 64, ( size + 63 ) / 64 * 64 ) ;
}

void my_free2( void *addr )
{
	myHeapTrash( &g_heap2, addr ) ;		// delayed release
}

void *my_malloc3( size_t size )
{
	return myHeapAlloc( &g_heap3, 64, ( size + 63 ) / 64 * 64 ) ;
}

void my_free3( void *addr )
{
	myHeapTrash( &g_heap3, addr ) ;		// delayed release
}