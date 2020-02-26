/*************************************************

  File name:     xdw_std.h

  Description:

  Others:

*************************************************/

#ifndef XDW_STD_H_
#define XDW_STD_H_

#ifndef NULL
	#ifdef  __cplusplus
		#define NULL    0
	#else
		#define NULL    ((void *)0)
	#endif
#endif

#ifndef XDW_IMPORT
#define XDW_IMPORT
#endif

#ifndef XDW_EXPORT
#define XDW_EXPORT
#endif

#define XDW_CONST const
#define XDW_NULL NULL

#define XDW_SUCCESS	0
#define XDW_FAILED	-1

typedef int XDW_BOOL;
#define XDW_TRUE     1
#define XDW_FALSE    0

#define IN      // 表示传入，默认，可以不用加
#define INOUT   // 表示传入、传出参数
#define OUT     // 表示传出参数

#ifndef XDW_UNREFERENCED_PARAMETER
//#define XDW_UNREFERENCED_PARAMETER(P){(P) = (P);}
#define XDW_UNREFERENCED_PARAMETER(P){(P);}
#endif

#define XDW_MIN(x,y)			(((x)<(y))?(x):(y))

typedef unsigned int    XDW_SIZE;
typedef char            XDW_CHAR;
typedef int             XDW_INT;
typedef unsigned int    XDW_UINT;
typedef long            XDW_LONG;
typedef short           XDW_SHORT;
typedef unsigned long   XDW_DWORD;
typedef unsigned short  XDW_WORD;
typedef unsigned char   XDW_BYTE;
typedef unsigned char   XDW_UTF8;
typedef unsigned short  XDW_WCHAR;
typedef float           XDW_FLOAT;
typedef double          XDW_DOUBLE;
typedef void            XDW_VOID;

#if defined(WIN32)
typedef __int64			XDW_64;
#else
typedef long long       XDW_64;
#endif

#define XDW_MAKEDWORD(a, b)  ((XDW_DWORD)(((XDW_WORD)(a)) | ((XDW_DWORD)((XDW_WORD)(b))) << 16))
#define XDW_MAKEWORD(a, b)   ((XDW_WORD)(((XDW_BYTE)(a)) | ((XDW_WORD)((XDW_BYTE)(b))) << 8))
#define XDW_LOWORD(l)        ((XDW_WORD)(l))
#define XDW_HIWORD(l)        ((XDW_WORD)(((XDW_DWORD)(l) >> 16) & 0xFFFF))
#define XDW_LOBYTE(w)        ((XDW_BYTE)(w))
#define XDW_HIBYTE(w)        ((XDW_BYTE)(((XDW_WORD)(w) >> 8) & 0xFF))




#endif
