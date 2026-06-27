// objectdesc.c — Load desc.json and render UTF-8 object descriptions.
//
// Format: {"<hex_id>|<type>": "description line1\nline2", ...}
// type: M = Magic, I = Item, else = Other
// Descriptions are UTF-8, rendered with the zpix small font.

#include "main.h"
#include "font.h"
#include "cJSON.h"

#define MAX_DESC_ENTRIES 1024

typedef struct tagDESC_ENTRY
{
   WORD             wID;
   OBJECT_DESC_TYPE type;
   const char      *pszText;
} DESC_ENTRY;

static DESC_ENTRY  g_rgDescEntries[MAX_DESC_ENTRIES];
static int         g_nDescEntries = 0;
static BOOL        g_fDescLoaded = FALSE;
static char       *g_pszJsonBuf = NULL;

VOID
PAL_LoadObjectDesc(
   VOID
)
{
   FILE *fp;
   long lSize;
   cJSON *pRoot, *pItem;
   char *pKey;

   if (g_fDescLoaded) return;
   g_fDescLoaded = TRUE;

   fp = UTIL_OpenFileForMode("desc.json", "r");
   if (fp == NULL) return;

   fseek(fp, 0, SEEK_END);
   lSize = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   if (lSize <= 0 || lSize > 1024 * 1024)
   {
      fclose(fp);
      return;
   }

   g_pszJsonBuf = (char *)malloc(lSize + 1);
   if (g_pszJsonBuf == NULL)
   {
      fclose(fp);
      return;
   }

   fread(g_pszJsonBuf, 1, lSize, fp);
   g_pszJsonBuf[lSize] = '\0';
   fclose(fp);

   pRoot = cJSON_Parse(g_pszJsonBuf);
   if (pRoot == NULL || !cJSON_IsObject(pRoot))
   {
      if (pRoot) cJSON_Delete(pRoot);
      free(g_pszJsonBuf);
      g_pszJsonBuf = NULL;
      return;
   }

   cJSON_ArrayForEach(pItem, pRoot)
   {
      if (g_nDescEntries >= MAX_DESC_ENTRIES) break;
      if (!cJSON_IsString(pItem)) continue;

      pKey = pItem->string;
      if (pKey == NULL) continue;

      WORD wID = (WORD)strtol(pKey, NULL, 16);
      OBJECT_DESC_TYPE type = kObjDescOther;

      char *pBar = strchr(pKey, '|');
      if (pBar != NULL && pBar[1] != '\0')
      {
         switch (pBar[1])
         {
         case 'M': case 'm': type = kObjDescMagic; break;
         case 'I': case 'i': type = kObjDescItem; break;
         }
      }

      g_rgDescEntries[g_nDescEntries].wID = wID;
      g_rgDescEntries[g_nDescEntries].type = type;
      g_rgDescEntries[g_nDescEntries].pszText = pItem->valuestring;
      g_nDescEntries++;
   }

   // NOTE: we intentionally do NOT call cJSON_Delete(pRoot) because
   // pItem->valuestring pointers are used directly as description text.
   // The JSON tree and g_pszJsonBuf live for the lifetime of the program.
}

LPCSTR
PAL_GetObjectDesc(
   WORD   wObjectID
)
{
   int i;
   for (i = 0; i < g_nDescEntries; i++)
   {
      if (g_rgDescEntries[i].wID == wObjectID)
         return g_rgDescEntries[i].pszText;
   }
   return NULL;
}

OBJECT_DESC_TYPE
PAL_GetObjectDescType(
   WORD   wObjectID
)
{
   int i;
   for (i = 0; i < g_nDescEntries; i++)
   {
      if (g_rgDescEntries[i].wID == wObjectID)
         return g_rgDescEntries[i].type;
   }
   return kObjDescOther;
}

BOOL
PAL_HasObjectDesc(
   VOID
)
{
   return g_nDescEntries > 0;
}

VOID
PAL_DrawObjectDesc(
   LPCSTR       pszText,
   SDL_Surface *lpSurface,
   int          x,
   int          y,
   BYTE         bColor
)
{
   int cx = x, cy = y;

   if (pszText == NULL) return;

   while (*pszText)
   {
      if (*pszText == '\n')
      {
         cx = x;
         cy += PAL_SmallFontHeight();
         pszText++;
         continue;
      }

      // Decode one UTF-8 codepoint
      const unsigned char *p = (const unsigned char *)pszText;
      uint32_t cp;
      int n;

      if (*p < 0x80) {
         cp = *p; n = 1;
      } else if ((*p & 0xE0) == 0xC0) {
         cp = (*p & 0x1F) << 6; cp |= (p[1] & 0x3F); n = 2;
      } else if ((*p & 0xF0) == 0xE0) {
         cp = (*p & 0x0F) << 12; cp |= (p[1] & 0x3F) << 6; cp |= (p[2] & 0x3F); n = 3;
      } else {
         cp = '?'; n = 1;
      }

      if (cp < 0x10000)
      {
         PAL_DrawSmallCharOnSurface((uint16_t)cp, lpSurface, PAL_XY(cx + 1, cy + 1), 0);
         PAL_DrawSmallCharOnSurface((uint16_t)cp, lpSurface, PAL_XY(cx, cy), bColor);
         cx += PAL_SmallCharWidth((uint16_t)cp);
      }

      pszText += n;
   }
}
