/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// debug.c - Debug menu for sdlpal-lite
//

#include "main.h"
#include "font.h"

BOOL g_fDebugShowEvents = FALSE;
BOOL g_fDebugShowStatus = FALSE;
BOOL g_fDebugShowGrid = FALSE;

// ============================================================
// Script OP descriptions (loaded from scriptor.txt)
// ============================================================

#define MAX_SCRIPTOR_ENTRIES 256
#define MAX_SCRIPTOR_DESC    256

static struct {
   WORD  wOP;
   char  szDesc[MAX_SCRIPTOR_DESC];
} g_rgScriptor[MAX_SCRIPTOR_ENTRIES];
static int g_nScriptor = 0;
static BOOL g_fScriptorLoaded = FALSE;

static void
DEBUG_LoadScriptor(
   VOID
)
{
   FILE *fp;
   char buf[256];

   if (g_fScriptorLoaded) return;
   g_fScriptorLoaded = TRUE;

   fp = UTIL_OpenFileForMode("scriptor.txt", "r");
   if (fp == NULL) return;

   while (fgets(buf, sizeof(buf), fp) != NULL && g_nScriptor < MAX_SCRIPTOR_ENTRIES)
   {
      char *p = buf;
      while (*p == ' ' || *p == '\t') p++;

      size_t len = strlen(p);
      while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r'))
         p[--len] = '\0';

      if (len == 0 || *p == ';') continue;

      // Parse: XXXX=description
      WORD wOP = (WORD)strtol(p, &p, 16);
      if (*p != '=') continue;
      p++;

      g_rgScriptor[g_nScriptor].wOP = wOP;
      strncpy(g_rgScriptor[g_nScriptor].szDesc, p, MAX_SCRIPTOR_DESC - 1);
      g_rgScriptor[g_nScriptor].szDesc[MAX_SCRIPTOR_DESC - 1] = '\0';
      g_nScriptor++;
   }

   fclose(fp);
}

static const char *
DEBUG_GetWord(
   WORD   wObjectID
)
{
   static char szBuf[64];
   LPCWSTR ws = PAL_GetWord(wObjectID);
   int i = 0;

   if (ws == NULL || ws[0] == 0)
   {
      szBuf[0] = '?';
      szBuf[1] = '\0';
      return szBuf;
   }

   while (*ws && i < (int)sizeof(szBuf) - 4)
   {
      wchar_t c = *ws++;
      if (c < 0x80) {
         szBuf[i++] = (char)c;
      } else if (c < 0x800) {
         szBuf[i++] = 0xC0 | (c >> 6);
         szBuf[i++] = 0x80 | (c & 0x3F);
      } else {
         szBuf[i++] = 0xE0 | (c >> 12);
         szBuf[i++] = 0x80 | ((c >> 6) & 0x3F);
         szBuf[i++] = 0x80 | (c & 0x3F);
      }
   }
   szBuf[i] = '\0';
   return szBuf;
}

static const char *
DEBUG_GetScriptorDesc(
   WORD   wOP
)
{
   int i;
   for (i = 0; i < g_nScriptor; i++)
   {
      if (g_rgScriptor[i].wOP == wOP)
         return g_rgScriptor[i].szDesc;
   }
   return NULL;
}

static const char *
DEBUG_FormatScriptDesc(
   const char *pszTemplate,
   WORD        rgwOperand[3]
)
{
   static char szResult[256];
   int ri = 0;
   const char *p = pszTemplate;

   while (*p && ri < (int)sizeof(szResult) - 16)
   {
      if (*p == '%' && p[1] >= '1' && p[1] <= '3' && p[2] != '\0')
      {
         int argIdx = p[1] - '1';
         char fmt = p[2];
         WORD val = rgwOperand[argIdx];
         p += 3;

         switch (fmt)
         {
         case 'd':
            ri += sprintf(szResult + ri, "%d", (SHORT)val);
            break;
         case 'X':
            ri += sprintf(szResult + ri, "%04X", val);
            break;
         case 'b':
            szResult[ri++] = val ? 'T' : 'F';
            break;
         case 'e':
         {
            // Inline enum: %1e{a,b,c} or %1e{0xB|a,b,c} — val selects by index
            while (*p == ' ') p++;
            if (*p == '{')
            {
               const char *start = ++p;
               const char *end = strchr(start, '}');
               if (end)
               {
                  int baseVal = 0;
                  const char *seg = start;

                  // Check for base offset: 0xNN| or NN|
                  const char *pipe = seg;
                  while (pipe < end && *pipe != '|' && *pipe != ',') pipe++;
                  if (pipe < end && *pipe == '|')
                  {
                     baseVal = (int)strtol(seg, NULL, 0);
                     seg = pipe + 1;
                  }

                  int ei = baseVal;
                  BOOL found = FALSE;
                  while (seg < end)
                  {
                     const char *comma = seg;
                     while (comma < end && *comma != ',') comma++;
                     if (ei == (int)val)
                     {
                        while (seg < comma && ri < (int)sizeof(szResult) - 2)
                           szResult[ri++] = *seg++;
                        found = TRUE;
                        break;
                     }
                     ei++;
                     seg = (comma < end) ? comma + 1 : end;
                  }
                  if (!found)
                     ri += sprintf(szResult + ri, "%d", (int)val);
                  p = end + 1;
               }
            }
            break;
         }
         case 'w':
         {
            const char *name = DEBUG_GetWord(val);
            while (*name && ri < (int)sizeof(szResult) - 2)
               szResult[ri++] = *name++;
            break;
         }
         case 'm':
         {
            LPCWSTR ws = PAL_GetMsg(val);
            int nch = 0;
            while (*ws && nch < 10 && ri < (int)sizeof(szResult) - 4)
            {
               wchar_t c = *ws++;
               if (c < 0x80) {
                  szResult[ri++] = (char)c;
               } else if (c < 0x800) {
                  szResult[ri++] = 0xC0 | (c >> 6);
                  szResult[ri++] = 0x80 | (c & 0x3F);
               } else {
                  szResult[ri++] = 0xE0 | (c >> 12);
                  szResult[ri++] = 0x80 | ((c >> 6) & 0x3F);
                  szResult[ri++] = 0x80 | (c & 0x3F);
               }
               nch++;
            }
            break;
         }
         default:
            szResult[ri++] = '%';
            szResult[ri++] = '0' + argIdx + 1;
            szResult[ri++] = fmt;
            break;
         }
      }
      else
      {
         szResult[ri++] = *p++;
      }
   }
   szResult[ri] = '\0';
   return szResult;
}

// ============================================================
// Debug overlay drawing helpers
// ============================================================

#define COLOR_BLOCKED       0x1A  // red-ish
#define COLOR_EVENT_TOUCH   0x9F  // yellow
#define COLOR_EVENT_SEARCH  0x8D  // cyan
#define COLOR_PARTY         0xFF  // bright

static void
DEBUG_DrawPixel(
   int    x,
   int    y,
   BYTE   color
)
{
   if (x < 0 || x >= gpScreen->w || y < 0 || y >= gpScreen->h) return;
   ((LPBYTE)gpScreen->pixels)[y * gpScreen->pitch + x] = color;
}

static void
DEBUG_DrawRect(
   int    x,
   int    y,
   int    w,
   int    h,
   BYTE   color
)
{
   int i;
   for (i = 0; i < w; i++)
   {
      DEBUG_DrawPixel(x + i, y, color);
      DEBUG_DrawPixel(x + i, y + h - 1, color);
   }
   for (i = 0; i < h; i++)
   {
      DEBUG_DrawPixel(x, y + i, color);
      DEBUG_DrawPixel(x + w - 1, y + i, color);
   }
}

static void
DEBUG_DrawTileDiamond(
   int    sx,
   int    sy,
   BYTE   color
)
{
   int dx;
   for (dx = 0; dx <= 16; dx++)
   {
      int dy = dx / 2;
      DEBUG_DrawPixel(sx + 16 - dx, sy + dy, color);
      DEBUG_DrawPixel(sx + 16 + dx, sy + dy, color);
      DEBUG_DrawPixel(sx + 16 - dx, sy + 16 - dy, color);
      DEBUG_DrawPixel(sx + 16 + dx, sy + 16 - dy, color);
   }
}

static void
DEBUG_DrawGrid(
   void
)
{
   int view_x = PAL_X(gpGlobals->viewport);
   int view_y = PAL_Y(gpGlobals->viewport);
   int x0 = view_x / 32 - 1;
   int x1 = (view_x + 320) / 32 + 2;
   int y0 = view_y / 16 - 1;
   int y1 = (view_y + 200) / 16 + 2;
   int tx, ty, h;

   for (ty = y0; ty < y1; ty++)
   {
      if (ty < 0 || ty >= 128) continue;
      for (tx = x0; tx < x1; tx++)
      {
         if (tx < 0 || tx >= 64) continue;
         for (h = 0; h < 2; h++)
         {
            int wx = tx * 32 + h * 16 - 16;
            int wy = ty * 16 + h * 8 - 8;
            int sx = wx - view_x;
            int sy = wy - view_y;

            if (PAL_CheckObstacle(PAL_XY(tx * 32 + h * 16, ty * 16 + h * 8), TRUE, 0))
            {
               DEBUG_DrawTileDiamond(sx, sy, COLOR_BLOCKED);
            }
         }
      }
   }
}

static void
DEBUG_DrawEventObjects(
   void
)
{
   int view_x = PAL_X(gpGlobals->viewport);
   int view_y = PAL_Y(gpGlobals->viewport);
   WORD wSceneIndex = gpGlobals->wNumScene - 1;
   WORD wStart, wEnd, i;

   if (wSceneIndex >= MAX_SCENES - 1 || gpGlobals->g.lprgEventObject == NULL)
      return;

   wStart = gpGlobals->g.rgScene[wSceneIndex].wEventObjectIndex + 1;
   wEnd = gpGlobals->g.rgScene[wSceneIndex + 1].wEventObjectIndex;

   if (wEnd >= gpGlobals->g.nEventObject) wEnd = gpGlobals->g.nEventObject - 1;
   if (wStart > wEnd) return;

   for (i = wStart; i <= wEnd; i++)
   {
      LPEVENTOBJECT pEvt = &gpGlobals->g.lprgEventObject[i - 1];
      int sx, sy;
      BYTE color;
      char buf[8];

      if (pEvt->sState == 0) continue;

      sx = pEvt->x - view_x;
      sy = pEvt->y - view_y;
      if (sx < -16 || sx >= 336 || sy < -16 || sy >= 216) continue;

      color = (pEvt->wTriggerMode >= kTriggerTouchNear) ? COLOR_EVENT_TOUCH : COLOR_EVENT_SEARCH;
      DEBUG_DrawRect(sx - 6, sy - 8, 13, 13, color);

      sprintf(buf, "%d", i);
      PAL_DrawSmallText(buf, gpScreen, PAL_XY(sx - 6, sy - 20), 0xFF);
   }

   // Party marker
   {
      int px = PAL_X(gpGlobals->partyoffset);
      int py = PAL_Y(gpGlobals->partyoffset);
      DEBUG_DrawRect(px - 4, py - 6, 9, 9, COLOR_PARTY);
   }
}

static void
DEBUG_DrawStatusLine(
   void
)
{
   char buf[128];
   WORD wSceneIndex = gpGlobals->wNumScene - 1;
   WORD wMapNum = gpGlobals->g.rgScene[wSceneIndex].wMapNum;
   int wx = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   int wy = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
   const char *dirs[] = {"S", "W", "N", "E", "?"};
   int dir = gpGlobals->wPartyDirection;
   if (dir > 3) dir = 4;

   sprintf(buf, "SCN=%d MAP=%d XY=(%d,%d) D=%s", gpGlobals->wNumScene, wMapNum, wx, wy, dirs[dir]);
   PAL_DrawSmallText(buf, gpScreen, PAL_XY(2, 2), 0xFF);

   sprintf(buf, "TILE=(%d,%d)", wx / 32, wy / 16);
   PAL_DrawSmallText(buf, gpScreen, PAL_XY(2, 14), 0xFF);
}

VOID
PAL_DebugOverlay(
   VOID
)
/*++
  Purpose:

    Draw debug overlay layers on top of the scene.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (!g_fDebugShowEvents && !g_fDebugShowStatus && !g_fDebugShowGrid) return;
   if (g_fDebugShowGrid) DEBUG_DrawGrid();
   if (g_fDebugShowEvents) DEBUG_DrawEventObjects();
   if (g_fDebugShowStatus) DEBUG_DrawStatusLine();
}

// ============================================================
// Hex drawing / input / script viewer
// ============================================================

static void
DEBUG_DrawHexChar(
   int     x,
   int     y,
   char    c,
   BYTE    bColor
)
{
   PAL_DrawCharOnSurface((uint16_t)c, gpScreen, PAL_XY(x + 1, y + 1), 0, TRUE);
   PAL_DrawCharOnSurface((uint16_t)c, gpScreen, PAL_XY(x, y), bColor, TRUE);
}

static void
DEBUG_DrawHex4(
   int     x,
   int     y,
   WORD    val,
   BYTE    bColor
)
{
   const char hex[] = "0123456789ABCDEF";
   DEBUG_DrawHexChar(x,      y, hex[(val >> 12) & 0xF], bColor);
   DEBUG_DrawHexChar(x + 8,  y, hex[(val >> 8) & 0xF], bColor);
   DEBUG_DrawHexChar(x + 16, y, hex[(val >> 4) & 0xF], bColor);
   DEBUG_DrawHexChar(x + 24, y, hex[val & 0xF], bColor);
}

static INT
DEBUG_NumInput(
   WORD        wDefault,
   const char *pszHint,
   int         nDigits,
   int         iBase,
   const char *pszSuffix
)
{
   int iDigit = 0;
   BYTE rgDigits[4];
   DWORD dwTime;
   const char hex[] = "0123456789ABCDEF";
   int i;
   int xStart = 152;

   for (i = nDigits - 1; i >= 0; i--)
   {
      rgDigits[i] = wDefault % iBase;
      wDefault /= iBase;
   }

   VIDEO_BackupScreen(gpScreen);
   PAL_ClearKeyState();
   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      VIDEO_RestoreScreen(gpScreen);
      PAL_CreateSingleLineBox(PAL_XY(100, 80), pszSuffix ? 6 : 5, FALSE);

      if (pszHint != NULL)
      {
         PAL_DrawSmallText(pszHint, gpScreen, PAL_XY(108, 90), 0x4E);
      }

      for (i = 0; i < nDigits; i++)
      {
         BYTE color = (i == iDigit) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
         DEBUG_DrawHexChar(xStart + i * 10, 94, hex[rgDigits[i]], color);
      }

      if (pszSuffix != NULL)
      {
         PAL_DrawSmallText(pszSuffix, gpScreen, PAL_XY(xStart + nDigits * 10 + 4, 90), 0x4E);
      }

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
         gpScreen, PAL_XY(xStart + iDigit * 10, 103));

      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();

      while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
      {
         PAL_ProcessEvent();
         if (g_InputState.dwKeyPress != 0) break;
         SDL_Delay(5);
      }
      dwTime = SDL_GetTicks() + FRAME_TIME;

      if (g_InputState.dwKeyPress & kKeyLeft)
      {
         iDigit = (iDigit > 0) ? iDigit - 1 : nDigits - 1;
      }
      else if (g_InputState.dwKeyPress & kKeyRight)
      {
         iDigit = (iDigit < nDigits - 1) ? iDigit + 1 : 0;
      }
      else if (g_InputState.dwKeyPress & kKeyUp)
      {
         rgDigits[iDigit] = (rgDigits[iDigit] + 1) % iBase;
      }
      else if (g_InputState.dwKeyPress & kKeyDown)
      {
         rgDigits[iDigit] = (rgDigits[iDigit] + iBase - 1) % iBase;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         INT result = 0;
         for (i = 0; i < nDigits; i++)
            result = result * iBase + rgDigits[i];
         VIDEO_RestoreScreen(gpScreen);
         return result;
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         VIDEO_RestoreScreen(gpScreen);
         return -1;
      }
   }
}

#define DEBUG_HexInput(def, hint) DEBUG_NumInput((def), (hint), 4, 16, NULL)
#define DEBUG_DecInput(def, hint, suffix) DEBUG_NumInput((def), (hint), 2, 10, (suffix))

#define SCRIPT_LINES_PER_PAGE 14

static void
DEBUG_ScriptBrowse(
   INT    *rgIndices,
   int     nCount,
   BOOL    fReadOnly
)
{
   int iCurrent = 0, iTop = 0;
   DWORD dwTime;

   if (nCount <= 0) return;

   DEBUG_LoadScriptor();

   VIDEO_BackupScreen(gpScreen);
   PAL_ClearKeyState();
   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      int i;

      VIDEO_RestoreScreen(gpScreen);
      PAL_CreateBox(PAL_XY(2, 2), (SCRIPT_LINES_PER_PAGE * 13 + 16) / 16 - 1, 17, 0, FALSE);

      for (i = 0; i < SCRIPT_LINES_PER_PAGE; i++)
      {
         int ii = iTop + i;
         int idx, y;
         BYTE color;

         if (ii >= nCount) break;
         idx = rgIndices[ii];
         y = 14 + i * 13;

         if (gpGlobals->g.lprgScriptEntry[idx].wOperation == 0)
            color = 0x1A;
         else
            color = (ii == iCurrent) ? 0x2D : 0x4F;

         DEBUG_DrawHex4(8, y, (WORD)idx, (ii == iCurrent) ? 0xFF : 0x8D);

         {
            static const int colX[] = { 44, 80, 116, 152 };
            WORD *pEntry = &gpGlobals->g.lprgScriptEntry[idx].wOperation;
            int c;
            for (c = 0; c < 4; c++)
               DEBUG_DrawHex4(colX[c], y, pEntry[c], color);
         }
      }

      // Description panel on the right
      {
         int idx = rgIndices[iCurrent];
         LPSCRIPTENTRY pE = &gpGlobals->g.lprgScriptEntry[idx];
         const char *desc = DEBUG_GetScriptorDesc(pE->wOperation);
         const char *text;
         BYTE textColor;

         if (desc)
         {
            text = DEBUG_FormatScriptDesc(desc, pE->rgwOperand);
            textColor = 0x3C;
         }
         else
         {
            text = "\xe6\x9c\xaa\xe7\x9f\xa5\xe8\x84\x9a\xe6\x9c\xac";
            textColor = 0x1A;
         }

         PAL_CreateBoxWithShadow(PAL_XY(188, 2), (SCRIPT_LINES_PER_PAGE * 13 + 16) / 16 - 1, 6, 1, FALSE, 0);

         // Word-wrap into buffer, max width ~118px
         {
            char wrapped[256];
            const char *p = text;
            int wi = 0, lineW = 0;
            const int maxW = 118;

            while (*p && wi < (int)sizeof(wrapped) - 2)
            {
               const unsigned char *u = (const unsigned char *)p;
               int n;
               uint32_t cp;
               if (*u < 0x80) { cp = *u; n = 1; }
               else if ((*u & 0xE0) == 0xC0) { cp = (*u & 0x1F) << 6 | (u[1] & 0x3F); n = 2; }
               else if ((*u & 0xF0) == 0xE0) { cp = (*u & 0x0F) << 12 | (u[1] & 0x3F) << 6 | (u[2] & 0x3F); n = 3; }
               else { cp = '?'; n = 1; }

               int cw = PAL_SmallCharWidth((uint16_t)cp);
               if (lineW + cw > maxW && lineW > 0)
               {
                  wrapped[wi++] = '\n';
                  lineW = 0;
               }
               memcpy(wrapped + wi, p, n);
               wi += n;
               lineW += cw;
               p += n;
            }
            wrapped[wi] = '\0';
            PAL_DrawObjectDesc(wrapped, gpScreen, 195, 14, textColor);
         }
      }

      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();

      while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
      {
         PAL_ProcessEvent();
         if (g_InputState.dwKeyPress != 0) break;
         SDL_Delay(5);
      }
      dwTime = SDL_GetTicks() + FRAME_TIME;

      if (g_InputState.dwKeyPress & kKeyUp)
      {
         if (iCurrent > 0) iCurrent--;
         if (iCurrent < iTop) iTop = iCurrent;
      }
      else if (g_InputState.dwKeyPress & kKeyDown)
      {
         if (iCurrent < nCount - 1) iCurrent++;
         if (iCurrent >= iTop + SCRIPT_LINES_PER_PAGE)
            iTop = iCurrent - SCRIPT_LINES_PER_PAGE + 1;
      }
      else if (g_InputState.dwKeyPress & kKeyPgUp)
      {
         iCurrent -= SCRIPT_LINES_PER_PAGE;
         if (iCurrent < 0) iCurrent = 0;
         iTop = iCurrent - SCRIPT_LINES_PER_PAGE / 2;
         if (iTop < 0) iTop = 0;
      }
      else if (g_InputState.dwKeyPress & kKeyPgDn)
      {
         iCurrent += SCRIPT_LINES_PER_PAGE;
         if (iCurrent >= nCount) iCurrent = nCount - 1;
         iTop = iCurrent - SCRIPT_LINES_PER_PAGE / 2;
         if (iTop < 0) iTop = 0;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         int idx = rgIndices[iCurrent];
         LPSCRIPTENTRY e = &gpGlobals->g.lprgScriptEntry[idx];
         UTIL_LogOutput(LOGLEVEL_INFO,
            "#FIND_SCRIPT 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X\n",
            idx, e->wOperation, e->rgwOperand[0], e->rgwOperand[1], e->rgwOperand[2]);
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         break;
      }
   }

   VIDEO_RestoreScreen(gpScreen);
   VIDEO_UpdateScreen(NULL);
}

static void
DEBUG_ScriptView(
   VOID
)
{
   INT iEntry;
   int nTotal, i;
   INT *rgIndices;

   iEntry = DEBUG_HexInput(0, "\xe5\x9c\xb0\xe5\x9d\x80");
   if (iEntry < 0) return;

   if (iEntry >= gpGlobals->g.nScriptEntry)
      iEntry = gpGlobals->g.nScriptEntry - 1;

   nTotal = gpGlobals->g.nScriptEntry - iEntry;
   rgIndices = (INT *)malloc(nTotal * sizeof(INT));
   if (rgIndices == NULL) return;

   for (i = 0; i < nTotal; i++)
      rgIndices[i] = iEntry + i;

   DEBUG_ScriptBrowse(rgIndices, nTotal, TRUE);
   free(rgIndices);
}

static void
DEBUG_ScriptSearch(
   VOID
)
{
   INT opVal;
   int i, nFound;
   INT *rgIndices;

   opVal = DEBUG_HexInput(0, "\xe6\x8c\x87\xe4\xbb\xa4");
   if (opVal < 0) return;

   rgIndices = (INT *)malloc(gpGlobals->g.nScriptEntry * sizeof(INT));
   if (rgIndices == NULL) return;

   nFound = 0;
   for (i = 0; i < gpGlobals->g.nScriptEntry; i++)
   {
      if (gpGlobals->g.lprgScriptEntry[i].wOperation == (WORD)opVal)
         rgIndices[nFound++] = i;
   }

   if (nFound > 0)
      DEBUG_ScriptBrowse(rgIndices, nFound, TRUE);

   free(rgIndices);
}

static void
DEBUG_LevelUpMagicView(
   VOID
)
{
   static const int rgColRole[] = { 0, 1, 2, 4 };
   int iRow = 0, iCol = 0, iTop = 0;
   int nRows = gpGlobals->g.nLevelUpMagic;
   const int nCols = 4;
   const int iColWidth = 80;
   const int iFaceY = 4;
   const int iHeaderH = 38;
   const int iRowH = 12;
   const int iVisibleRows = (200 - iHeaderH) / iRowH;
   DWORD dwTime;

   if (nRows <= 0) return;

   VIDEO_BackupScreen(gpScreen);
   PAL_ClearKeyState();
   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      int i, r;

      if (iRow < iTop) iTop = iRow;
      if (iRow >= iTop + iVisibleRows) iTop = iRow - iVisibleRows + 1;

      VIDEO_RestoreScreen(gpScreen);

      PAL_CreateBoxWithShadow(PAL_XY(0, 0), 11, 18, 1, FALSE, 6);

      for (r = 0; r < nCols; r++)
      {
         int role = rgColRole[r];
         int x = r * iColWidth + (iColWidth - 32) / 2;
         PAL_RLEBlitToSurface(
            PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + role),
            gpScreen, PAL_XY(x, iFaceY));

         BYTE nameColor = (r == iCol) ? 0x8D : 0x1B;
         const char *name = DEBUG_GetWord(gpGlobals->g.PlayerRoles.rgwName[role]);
         PAL_DrawSmallText(name, gpScreen, PAL_XY(r * iColWidth + 4, 28), nameColor);
      }

      for (i = 0; i < iVisibleRows && (iTop + i) < nRows; i++)
      {
         int row = iTop + i;
         int y = iHeaderH + i * iRowH;
         BOOL bSelectedRow = (row == iRow);

         for (r = 0; r < nCols; r++)
         {
            int role = rgColRole[r];
            WORD wLevel = gpGlobals->g.lprgLevelUpMagic[row].m[role].wLevel;
            WORD wMagic = gpGlobals->g.lprgLevelUpMagic[row].m[role].wMagic;
            int x = r * iColWidth + 2;
            char szBuf[64];
            BOOL bSelected = (bSelectedRow && r == iCol);

            if (wMagic == 0)
            {
               PAL_DrawSmallText("--", gpScreen, PAL_XY(x, y),
                  bSelected ? 0x8D : 0x4E);
               PAL_DrawSmallText(" " "\xe7\xa9\xba" " --", gpScreen, PAL_XY(x + 14, y),
                  bSelected ? 0x8D : 0x4E);
            }
            else
            {
               sprintf(szBuf, "%02d", wLevel);
               PAL_DrawSmallText(szBuf, gpScreen, PAL_XY(x, y),
                  bSelected ? 0x8D : 0x2E);
               const char *mname = DEBUG_GetWord(wMagic);
               PAL_DrawSmallText(mname, gpScreen, PAL_XY(x + 14, y),
                  bSelected ? 0x8D : 0x7D);
            }
         }
      }

      {
         char szStatus[64];
         sprintf(szStatus, "[%d/%d] R%d", iRow, nRows - 1, iCol);
         PAL_DrawSmallText(szStatus, gpScreen, PAL_XY(270, 2), 0x4E);
      }

      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();

      while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
      {
         PAL_ProcessEvent();
         if (g_InputState.dwKeyPress != 0) break;
         SDL_Delay(5);
      }
      dwTime = SDL_GetTicks() + FRAME_TIME;

      if (g_InputState.dwKeyPress & kKeyMenu)
         break;
      else if (g_InputState.dwKeyPress & kKeyUp)
         iRow = (iRow > 0) ? iRow - 1 : 0;
      else if (g_InputState.dwKeyPress & kKeyDown)
         iRow = (iRow < nRows - 1) ? iRow + 1 : iRow;
      else if (g_InputState.dwKeyPress & kKeyLeft)
         iCol = (iCol > 0) ? iCol - 1 : nCols - 1;
      else if (g_InputState.dwKeyPress & kKeyRight)
         iCol = (iCol < nCols - 1) ? iCol + 1 : 0;
      else if (g_InputState.dwKeyPress & kKeyPgUp)
         iRow = (iRow > iVisibleRows) ? iRow - iVisibleRows : 0;
      else if (g_InputState.dwKeyPress & kKeyPgDn)
      {
         iRow += iVisibleRows;
         if (iRow >= nRows) iRow = nRows - 1;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         int role = rgColRole[iCol];
         LPLEVELUPMAGIC_ALL pRow = &gpGlobals->g.lprgLevelUpMagic[iRow];
         WORD wOldLevel = pRow->m[role].wLevel;
         WORD wOldMagic = pRow->m[role].wMagic;
         INT val;

         val = DEBUG_HexInput(wOldMagic, "\xe4\xbb\x99\xe8\xa1\x93" "ID");
         if (val >= 0)
         {
            pRow->m[role].wMagic = (WORD)val;

            if (val != 0)
            {
               val = DEBUG_DecInput(wOldLevel, "\xe7\xad\x89\xe7\xb4\x9a", NULL);
               if (val >= 0)
                  pRow->m[role].wLevel = (WORD)val;
            }
            else
            {
               pRow->m[role].wLevel = 0;
            }

            if (pRow->m[role].wMagic != wOldMagic || pRow->m[role].wLevel != wOldLevel)
            {
               UTIL_LogOutput(LOGLEVEL_DEBUG,
                  "#CHANGE_LEVELUP_MAGIC %d %d %d 0x%X\n",
                  iRow, role, pRow->m[role].wLevel, pRow->m[role].wMagic);
            }
         }

         VIDEO_RestoreScreen(gpScreen);
      }
   }

   VIDEO_RestoreScreen(gpScreen);
   VIDEO_UpdateScreen(NULL);
}

static const WCHAR g_rgScriptSubLabels[3][3] = {
   { 0x67E5, 0x770B, 0 },  // 查看
   { 0x641C, 0x7D22, 0 },  // 搜索
   { 0x7CBE, 0x9032, 0 },  // 精進
};

static void
PAL_DebugScriptViewer(
   VOID
)
{
   int iSubItem = 0;
   const int nItems = 3;
   const int iBoxX = 55;
   const int iBoxY = 100;
   LPBOX lpMenuBox;

   lpMenuBox = PAL_CreateBox(PAL_XY(iBoxX, iBoxY), nItems - 1, 1, 0, TRUE);

   while (TRUE)
   {
      int k;

      for (k = 0; k < nItems; k++)
      {
         BYTE bColor = (k == iSubItem) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
         PAL_DrawText(g_rgScriptSubLabels[k],
            PAL_XY(iBoxX + 13, iBoxY + 13 + k * 18), bColor, TRUE, FALSE, FALSE);
      }

      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();
      while (TRUE)
      {
         UTIL_Delay(1);
         if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
         {
            iSubItem = (iSubItem > 0) ? iSubItem - 1 : nItems - 1;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
         {
            iSubItem = (iSubItem < nItems - 1) ? iSubItem + 1 : 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyMenu)
         {
            PAL_DeleteBox(lpMenuBox);
            VIDEO_UpdateScreen(NULL);
            return;
         }
         else if (g_InputState.dwKeyPress & kKeySearch)
         {
            PAL_DeleteBox(lpMenuBox);
            VIDEO_UpdateScreen(NULL);
            switch (iSubItem)
            {
            case 0: DEBUG_ScriptView(); break;
            case 1: DEBUG_ScriptSearch(); break;
            case 2: DEBUG_LevelUpMagicView(); break;
            }
            return;
         }
      }
   }
}

// ============================================================
// Debug menu
// ============================================================

#define DEBUG_MENU_ITEMS 7

static const WCHAR g_rgDebugMenuLabels[DEBUG_MENU_ITEMS][3] = {
   { 0x5C0B, 0x8E64, 0 },  // 尋蹤
   { 0x6C23, 0x51DD, 0 },  // 氣凝 (pawn/sell)
   { 0x85CF, 0x771F, 0 },  // 藏真 (random shop)
   { 0x4FE0, 0x5F71, 0 },  // 俠影
   { 0x8A66, 0x7149, 0 },  // 試煉
   { 0x5916, 0x5178, 0 },  // 外典
   { 0x5929, 0x6A5F, 0 },  // 天機 (script viewer)
};

static const WCHAR g_rgSubLabels[3][3] = {
   { 0x7269, 0x4EF6, 0 },  // 物件
   { 0x5750, 0x6A19, 0 },  // 坐標
   { 0x901A, 0x884C, 0 },  // 通行
};

#define DEBUG_BOX_X    16
#define DEBUG_BOX_Y    28
#define DEBUG_PAD_X    13
#define DEBUG_PAD_Y    13
#define DEBUG_ROW_H    18

static void
PAL_DebugTraceSubmenu(
   VOID
)
{
   int iSubItem = 0;

   VIDEO_BackupScreen(gpScreen);

   while (TRUE)
   {
      BOOL *rgFlags[] = { &g_fDebugShowEvents, &g_fDebugShowStatus, &g_fDebugShowGrid };
      int k;

      VIDEO_RestoreScreen(gpScreen);
      PAL_CreateBox(PAL_XY(55, 45), 2, 1, 0, FALSE);

      for (k = 0; k < 3; k++)
      {
         BYTE bColor;
         if (*rgFlags[k])
            bColor = MENUITEM_COLOR_CONFIRMED;
         else
            bColor = MENUITEM_COLOR;
         PAL_DrawText(g_rgSubLabels[k], PAL_XY(68, 58 + k * DEBUG_ROW_H), bColor, TRUE, FALSE, FALSE);
      }

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
         gpScreen, PAL_XY(80, 72 + iSubItem * DEBUG_ROW_H));

      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();
      while (TRUE)
      {
         UTIL_Delay(1);
         if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
         {
            iSubItem--;
            if (iSubItem < 0) iSubItem = 2;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
         {
            iSubItem++;
            if (iSubItem > 2) iSubItem = 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyMenu)
         {
            VIDEO_RestoreScreen(gpScreen);
            return;
         }
         else if (g_InputState.dwKeyPress & kKeySearch)
         {
            *rgFlags[iSubItem] = !(*rgFlags[iSubItem]);
            break;
         }
      }
   }
}

static INT
DEBUG_PlayerSlot(
   WORD   wRole
)
{
   int i;
   for (i = 0; i <= (int)gpGlobals->wMaxPartyMemberIndex; i++)
   {
      if (gpGlobals->rgParty[i].wPlayerRole == wRole) return i;
   }
   return -1;
}

static void
DEBUG_TogglePartyMember(
   WORD   wRole
)
{
   INT slot = DEBUG_PlayerSlot(wRole);
   if (slot >= 0)
   {
      if (gpGlobals->wMaxPartyMemberIndex == 0) return;
      for (int i = slot; i < (int)gpGlobals->wMaxPartyMemberIndex; i++)
      {
         gpGlobals->rgParty[i] = gpGlobals->rgParty[i + 1];
      }
      gpGlobals->wMaxPartyMemberIndex--;
   }
   else
   {
      if (gpGlobals->wMaxPartyMemberIndex + 1 >= MAX_PLAYERS_IN_PARTY) return;
      gpGlobals->wMaxPartyMemberIndex++;
      memset(&gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex], 0, sizeof(gpGlobals->rgParty[0]));
      gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex].wPlayerRole = wRole;
   }
}

static void
PAL_DebugPartyEdit(
   VOID
)
{
   int iCurrent = 0;

   VIDEO_BackupScreen(gpScreen);

   while (TRUE)
   {
      int i;
      VIDEO_RestoreScreen(gpScreen);

      PAL_CreateBox(PAL_XY(20, 30), MAX_PLAYER_ROLES - 1, 11, 1, FALSE);

      for (i = 0; i < MAX_PLAYER_ROLES; i++)
      {
         int x = 40, y = 48 + i * 18;
         BYTE bColor = (i == iCurrent) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
         INT slot = DEBUG_PlayerSlot((WORD)i);

         PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[i]),
            PAL_XY(x, y), bColor, TRUE, FALSE, FALSE);

         if (slot >= 0)
         {
            PAL_DrawNumber((UINT)(slot + 1), 1, PAL_XY(x + 80, y + 4), kNumColorYellow, kNumAlignRight);
         }

         PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwSpriteNum[i], 4,
            PAL_XY(x + 145, y + 4), kNumColorCyan, kNumAlignRight);
      }

      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();
      while (TRUE)
      {
         UTIL_Delay(1);
         if (g_InputState.dwKeyPress & kKeyUp)
         {
            iCurrent--;
            if (iCurrent < 0) iCurrent = MAX_PLAYER_ROLES - 1;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyDown)
         {
            iCurrent++;
            if (iCurrent >= MAX_PLAYER_ROLES) iCurrent = 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyLeft)
         {
            gpGlobals->g.PlayerRoles.rgwSpriteNum[iCurrent]--;
            PAL_SetLoadFlags(kLoadPlayerSprite);
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyRight)
         {
            gpGlobals->g.PlayerRoles.rgwSpriteNum[iCurrent]++;
            PAL_SetLoadFlags(kLoadPlayerSprite);
            break;
         }
         else if (g_InputState.dwKeyPress & kKeySearch)
         {
            DEBUG_TogglePartyMember((WORD)iCurrent);
            PAL_SetLoadFlags(kLoadPlayerSprite);
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyMenu)
         {
            VIDEO_RestoreScreen(gpScreen);
            VIDEO_UpdateScreen(NULL);
            PAL_SetLoadFlags(kLoadPlayerSprite);
            return;
         }
      }
   }
}

static void
PAL_DebugStartBattle(
   VOID
)
{
   #define TEAMS_PER_LINE   3
   #define LINES_PER_PAGE   7
   #define TEAM_TEXT_WIDTH  100

   WORD rgTeamIds[4096];
   int nTeams = 0;
   int iCurrent = 0;
   int i, j, k;

   for (i = 1; i < gpGlobals->g.nEnemyTeam; i++)
   {
      if (gpGlobals->g.lprgEnemyTeam[i].rgwEnemy[0] != 0 &&
          gpGlobals->g.lprgEnemyTeam[i].rgwEnemy[0] != 0xFFFF)
      {
         if (nTeams < 4096) rgTeamIds[nTeams++] = (WORD)i;
      }
   }

   if (nTeams == 0) return;

   VIDEO_BackupScreen(gpScreen);

   while (TRUE)
   {
      VIDEO_RestoreScreen(gpScreen);

      PAL_CreateBox(PAL_XY(2, 0), LINES_PER_PAGE - 1, 17, 1, FALSE);

      int iStart = (iCurrent / TEAMS_PER_LINE) * TEAMS_PER_LINE - TEAMS_PER_LINE * (LINES_PER_PAGE / 2);
      if (iStart < 0) iStart = 0;

      i = iStart;
      for (j = 0; j < LINES_PER_PAGE && i < nTeams; j++)
      {
         for (k = 0; k < TEAMS_PER_LINE && i < nTeams; k++, i++)
         {
            WORD wTeamId = rgTeamIds[i];
            BYTE bColor = (i == iCurrent) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
            int x = 15 + k * TEAM_TEXT_WIDTH;
            int y = 12 + j * 18;

            WORD wLeader = gpGlobals->g.lprgEnemyTeam[wTeamId].rgwEnemy[0];
            if (wLeader != 0)
            {
               PAL_DrawText(PAL_GetWord(wLeader), PAL_XY(x, y), bColor, TRUE, FALSE, FALSE);
            }
            PAL_DrawNumber(wTeamId, 3, PAL_XY(x + 72, y + 4), kNumColorCyan, kNumAlignRight);

            if (i == iCurrent)
            {
               PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
                  gpScreen, PAL_XY(x + 20, y + 10));
            }
         }
      }

      //
      // Show composition of the highlighted team at the bottom
      //
      {
         WORD wTeamId = rgTeamIds[iCurrent];
         int slot, col = 0, row = 0;
         static const WCHAR wszSummon[] = { 0x53EC, 0x559A, 0 }; // 召喚
         for (slot = 0; slot < MAX_ENEMIES_IN_TEAM; slot++)
         {
            WORD wEnemy = gpGlobals->g.lprgEnemyTeam[wTeamId].rgwEnemy[slot];
            if (wEnemy == 0xFFFF) continue;
            int sx = 15 + col * 96;
            int sy = 158 + row * 18;
            if (wEnemy == 0)
               PAL_DrawText(wszSummon, PAL_XY(sx, sy), MENUITEM_COLOR, TRUE, FALSE, FALSE);
            else
               PAL_DrawText(PAL_GetWord(wEnemy), PAL_XY(sx, sy), 0x3C, TRUE, FALSE, FALSE);
            col++;
            if (col >= 3) { col = 0; row++; }
         }
      }

      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();
      while (TRUE)
      {
         UTIL_Delay(1);
         if (g_InputState.dwKeyPress & kKeyUp)
         {
            iCurrent -= TEAMS_PER_LINE;
            if (iCurrent < 0) iCurrent = 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyDown)
         {
            iCurrent += TEAMS_PER_LINE;
            if (iCurrent >= nTeams) iCurrent = nTeams - 1;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyLeft)
         {
            iCurrent--;
            if (iCurrent < 0) iCurrent = 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyRight)
         {
            iCurrent++;
            if (iCurrent >= nTeams) iCurrent = nTeams - 1;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyPgUp)
         {
            iCurrent -= TEAMS_PER_LINE * LINES_PER_PAGE;
            if (iCurrent < 0) iCurrent = 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyPgDn)
         {
            iCurrent += TEAMS_PER_LINE * LINES_PER_PAGE;
            if (iCurrent >= nTeams) iCurrent = nTeams - 1;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeySearch)
         {
            VIDEO_RestoreScreen(gpScreen);
            VIDEO_UpdateScreen(NULL);
            PAL_StartBattle(rgTeamIds[iCurrent], FALSE);
            return;
         }
         else if (g_InputState.dwKeyPress & kKeyMenu)
         {
            VIDEO_RestoreScreen(gpScreen);
            VIDEO_UpdateScreen(NULL);
            return;
         }
      }
   }

   #undef TEAMS_PER_LINE
   #undef LINES_PER_PAGE
   #undef TEAM_TEXT_WIDTH
}

static VOID
PAL_DebugHackMenu(
   VOID
)
{
   #define HACK_ITEMS_PER_LINE  3
   #define HACK_LINES_PER_PAGE  5
   #define HACK_ITEM_WIDTH      87
   #define HACK_BOX_X           10
   #define HACK_BOX_Y           42
   #define HACK_ITEM_X0         35
   #define HACK_ITEM_Y0         54
   #define HACK_ROW_H           18

   int nHacks = PAL_GetHackCount();
   int iCurrent = 0;
   int i, j, k, item_delta;
   DWORD dwTime;

   if (nHacks == 0) return;

   VIDEO_BackupScreen(gpScreen);
   PAL_ClearKeyState();
   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      item_delta = 0;

      if (g_InputState.dwKeyPress & kKeyUp)
         item_delta = -HACK_ITEMS_PER_LINE;
      else if (g_InputState.dwKeyPress & kKeyDown)
         item_delta = HACK_ITEMS_PER_LINE;
      else if (g_InputState.dwKeyPress & kKeyLeft)
         item_delta = -1;
      else if (g_InputState.dwKeyPress & kKeyRight)
         item_delta = 1;
      else if (g_InputState.dwKeyPress & kKeyPgUp)
         item_delta = -(HACK_ITEMS_PER_LINE * HACK_LINES_PER_PAGE);
      else if (g_InputState.dwKeyPress & kKeyPgDn)
         item_delta = HACK_ITEMS_PER_LINE * HACK_LINES_PER_PAGE;
      else if (g_InputState.dwKeyPress & kKeyMenu)
         break;
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         PAL_ExecuteHack(iCurrent);
         break;
      }

      if (iCurrent + item_delta >= 0 && iCurrent + item_delta < nHacks)
         iCurrent += item_delta;

      VIDEO_RestoreScreen(gpScreen);

      PAL_CreateBoxWithShadow(PAL_XY(HACK_BOX_X, HACK_BOX_Y),
         HACK_LINES_PER_PAGE - 1, 16, 1, FALSE, 0);

      // Cash box (top-left)
      PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
      PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE, FALSE);
      PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

      // Grid of hack names
      i = iCurrent / HACK_ITEMS_PER_LINE * HACK_ITEMS_PER_LINE
         - HACK_ITEMS_PER_LINE * (HACK_LINES_PER_PAGE / 2);
      if (i < 0) i = 0;

      for (j = 0; j < HACK_LINES_PER_PAGE; j++)
      {
         for (k = 0; k < HACK_ITEMS_PER_LINE; k++)
         {
            if (i >= nHacks) { j = HACK_LINES_PER_PAGE; break; }

            BYTE bColor = (i == iCurrent) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
            int px = HACK_ITEM_X0 + k * HACK_ITEM_WIDTH;
            int py = HACK_ITEM_Y0 + j * HACK_ROW_H;

            PAL_DrawSmallText(PAL_GetHackName(i), gpScreen, PAL_XY(px, py), bColor);

            if (i == iCurrent)
            {
               PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
                  gpScreen, PAL_XY(px + 25, py + 10));
            }
            i++;
         }
      }

      // Description panel (top-right area)
      {
         LPCSTR pszDesc = PAL_GetHackDesc(iCurrent);
         if (pszDesc != NULL)
         {
            PAL_DrawObjectDesc(pszDesc, gpScreen, 102, 3, 0x3C);
         }
      }

      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();

      while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
      {
         PAL_ProcessEvent();
         if (g_InputState.dwKeyPress != 0) break;
         SDL_Delay(5);
      }
      dwTime = SDL_GetTicks() + FRAME_TIME;
   }

   VIDEO_RestoreScreen(gpScreen);
   VIDEO_UpdateScreen(NULL);

   #undef HACK_ITEMS_PER_LINE
   #undef HACK_LINES_PER_PAGE
   #undef HACK_ITEM_WIDTH
   #undef HACK_BOX_X
   #undef HACK_BOX_Y
   #undef HACK_ITEM_X0
   #undef HACK_ITEM_Y0
   #undef HACK_ROW_H
}

VOID
PAL_DebugMenu(
   VOID
)
{
   int iCurrentItem = 0;
   BOOL fDone = FALSE;

   VIDEO_BackupScreen(gpScreen);

   PAL_CreateBox(PAL_XY(DEBUG_BOX_X, DEBUG_BOX_Y), DEBUG_MENU_ITEMS - 1, 1, 0, FALSE);

   while (!fDone)
   {
      for (int i = 0; i < DEBUG_MENU_ITEMS; i++)
      {
         BYTE bColor = (i == iCurrentItem) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
         PAL_DrawText(g_rgDebugMenuLabels[i],
            PAL_XY(DEBUG_BOX_X + DEBUG_PAD_X, DEBUG_BOX_Y + DEBUG_PAD_Y + i * DEBUG_ROW_H),
            bColor, TRUE, FALSE, FALSE);
      }

      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();
      while (TRUE)
      {
         UTIL_Delay(1);
         if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
         {
            iCurrentItem--;
            if (iCurrentItem < 0) iCurrentItem = DEBUG_MENU_ITEMS - 1;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
         {
            iCurrentItem++;
            if (iCurrentItem >= DEBUG_MENU_ITEMS) iCurrentItem = 0;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeyMenu)
         {
            fDone = TRUE;
            break;
         }
         else if (g_InputState.dwKeyPress & kKeySearch)
         {
            switch (iCurrentItem)
            {
            case 0: // 尋蹤
               PAL_DebugTraceSubmenu();
               fDone = TRUE;
               break;
            case 1: // 氣凝 (pawn/sell)
               VIDEO_RestoreScreen(gpScreen);
               VIDEO_UpdateScreen(NULL);
               PAL_SellMenu();
               fDone = TRUE;
               break;
            case 2: // 藏真 (shop by ID)
            {
               int nStores = gpGlobals->g.nStore;
               if (nStores > 0)
               {
                  char szSuffix[16];
                  sprintf(szSuffix, "/%d", nStores - 1);
                  INT idx = DEBUG_DecInput(0, "\xe5\x95\x86\xe5\xba\x97", szSuffix);
                  if (idx >= 0 && idx < nStores && gpGlobals->g.lprgStore[idx].rgwItems[0] != 0)
                  {
                     gpGlobals->dwCash += 10000;
                     VIDEO_RestoreScreen(gpScreen);
                     VIDEO_UpdateScreen(NULL);
                     PAL_BuyMenu((WORD)idx);
                  }
               }
               fDone = TRUE;
            }
            break;
            case 3: // 俠影 (party edit)
            {
               VIDEO_RestoreScreen(gpScreen);
               VIDEO_UpdateScreen(NULL);
               PAL_DebugPartyEdit();
               fDone = TRUE;
            }
            break;
            case 4: // 試煉 (start battle)
            {
               VIDEO_RestoreScreen(gpScreen);
               VIDEO_UpdateScreen(NULL);
               PAL_DebugStartBattle();
               fDone = TRUE;
            }
            break;
            case 5: // 外典
            {
               VIDEO_RestoreScreen(gpScreen);
               VIDEO_UpdateScreen(NULL);
               PAL_DebugHackMenu();
               fDone = TRUE;
            }
            break;
            case 6: // 經卷 (script viewer)
            {
               PAL_DebugScriptViewer();
               fDone = TRUE;
            }
            break;
            }
            break;
         }
      }
   }

   VIDEO_RestoreScreen(gpScreen);
   VIDEO_UpdateScreen(NULL);
}
