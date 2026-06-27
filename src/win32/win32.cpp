#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "global.h"
#include "util.h"
#include "palcfg.h"

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
   if (!lpszFileName || *lpszFileName == '\0') return FALSE;
   if (lpszFileName[0] == '/' || lpszFileName[0] == '\\') return TRUE;
   if (lpszFileName[1] == ':' && (lpszFileName[2] == '/' || lpszFileName[2] == '\\')) return TRUE;
   return FALSE;
}

INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
   return 0;
}

VOID
UTIL_Platform_Quit(
   VOID
)
{
}
