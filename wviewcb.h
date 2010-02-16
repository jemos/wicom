
#ifndef _WVIEWCB_H
#define _WVIEWCB_H

#include "posh.h"
#include "wstatus.h"

typedef union _wvkey_t
{
	unsigned int key;
	unsigned int flags;
	signed int x,y;
} wvkey_t;

typedef enum _wvkey_mode_t
{
	WV_KEY_DOWN,
	WV_KEY_UP
} wvkey_mode_t;

typedef enum _wvmouse_button_t
{
	WV_MOUSE_LEFT,
	WV_MOUSE_RIGHT,
	WV_MOUSE_MIDDLE
} wvmouse_button_t;

typedef enum _wvmouse_mode_t
{
	WV_MOUSE_DOWN,
	WV_MOUSE_UP,
	WV_MOUSE_DOUBLE
} wvmouse_mode_t;

typedef struct _wvmouse_t
{
	unsigned int x,y;
	wvmouse_button_t button;
	wvmouse_mode_t mode;
} wvmouse_t;

#define WVDRAW_FLAG_ORTHOGONAL 1
#define WVDRAW_FLAG_PERSPECTIVE 2
typedef struct _wvdraw_t
{
	int flags;
} wvdraw_t;


typedef wstatus (POSH_CDECL *wvkeyboard_cb)(wvkey_t key,wvkey_mode_t key_mode);
typedef wstatus (POSH_CDECL *wvmouse_cb)(wvmouse_t mouse);
typedef wstatus (POSH_CDECL *wvdraw_cb)(wvdraw_t draw);

#endif

