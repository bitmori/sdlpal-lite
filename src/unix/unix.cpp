#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "util.h"
#include "palcfg.h"

#include <syslog.h>

BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
   return pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight;
}

BOOL
UTIL_IsAbsolutePath(
   LPCSTR lpszFileName
)
{
   return lpszFileName && *lpszFileName == '/';
}

INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
	openlog("sdlpal", LOG_PERROR | LOG_PID, LOG_USER);
	UTIL_LogAddOutputCallback([](LOGLEVEL level, const char* str, const char*)->void {
		const static int priorities[] = {
			LOG_DEBUG,
			LOG_DEBUG,
			LOG_INFO,
			LOG_WARNING,
			LOG_ERR,
			LOG_EMERG
		};
		syslog(priorities[level], "%s", str);
	}, PAL_DEFAULT_LOGLEVEL);

   return 0;
}

VOID
UTIL_Platform_Quit(
   VOID
)
{
	closelog();
}
