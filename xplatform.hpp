#if defined (__WIN32__) || defined(WIN32) /* WINDOWS */

#include <WinSock2.h>
#include <Windows.h>
#include <Sddl.h>

#define _WINDOWS

#if !defined(_TYPES_)
#define _TYPES_
typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long long	uint64_t;

typedef			char		int8_t;
typedef			short		int16_t;
typedef			int			int32_t;
typedef			long long	int64_t;
#endif

#else /* LINUX */

#define _LINUX

#endif

