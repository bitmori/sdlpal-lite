/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// objectdesc.c — Load desc.txt and render UTF-8 object descriptions.
//
// Format:
//   +<hex_id>=<name>=<type>    start entry (type: M=Magic, I=Item, ?=Other)
//   description lines...       (multi-line, joined with \n)
//   -                          end entry
//   ;comment                   ignored
//

#include "main.h"
#include "font.h"

#define MAX_DESC_ENTRIES 1024
#define MAX_DESC_TEXT    256

typedef struct tagDESC_ENTRY
{
   WORD             wID;
   OBJECT_DESC_TYPE type;
   char             szText[MAX_DESC_TEXT];
} DESC_ENTRY;

static DESC_ENTRY  g_rgDescEntries[MAX_DESC_ENTRIES];
static int         g_nDescEntries = 0;
static BOOL        g_fDescLoaded = FALSE;

VOID
PAL_LoadObjectDesc(
   VOID
)
{
   FILE *fp;
   char buf[512];
   int  iCurrent = -1;

   if (g_fDescLoaded) return;
   g_fDescLoaded = TRUE;

   fp = UTIL_OpenFileForMode("desc.txt", "r");
   if (fp == NULL) return;

   while (fgets(buf, sizeof(buf), fp) != NULL)
   {
      char *p = buf;
      while (*p == ' ' || *p == '\t') p++;

      size_t len = strlen(p);
      while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r'))
         p[--len] = '\0';

      if (len == 0 || *p == ';')
         continue;

      if (*p == '+')
      {
         if (g_nDescEntries >= MAX_DESC_ENTRIES) break;
         iCurrent = g_nDescEntries++;

         p++;
         WORD wID = (WORD)strtol(p, &p, 16);
         g_rgDescEntries[iCurrent].wID = wID;
         g_rgDescEntries[iCurrent].szText[0] = '\0';
         g_rgDescEntries[iCurrent].type = kObjDescOther;

         // skip to type field (after second '=')
         char *pEq = strchr(p, '=');
         if (pEq != NULL)
         {
            pEq = strchr(pEq + 1, '=');
            if (pEq != NULL && pEq[1] != '\0')
            {
               switch (pEq[1])
               {
               case 'M': case 'm': g_rgDescEntries[iCurrent].type = kObjDescMagic; break;
               case 'I': case 'i': g_rgDescEntries[iCurrent].type = kObjDescItem; break;
               }
            }
         }
      }
      else if (*p == '-')
      {
         iCurrent = -1;
      }
      else if (iCurrent >= 0)
      {
         DESC_ENTRY *pEntry = &g_rgDescEntries[iCurrent];
         size_t cur = strlen(pEntry->szText);

         if (cur > 0 && cur < MAX_DESC_TEXT - 1)
            pEntry->szText[cur++] = '\n';

         size_t remain = MAX_DESC_TEXT - 1 - cur;
         if (len > remain) len = remain;
         memcpy(pEntry->szText + cur, p, len);
         pEntry->szText[cur + len] = '\0';
      }
   }

   fclose(fp);
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
         return g_rgDescEntries[i].szText;
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
