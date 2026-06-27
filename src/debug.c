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
   WORD wStart = gpGlobals->g.rgScene[wSceneIndex].wEventObjectIndex + 1;
   WORD wEnd = gpGlobals->g.rgScene[wSceneIndex + 1].wEventObjectIndex;
   WORD i;

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
      PAL_DrawSmallText(buf, gpScreen, PAL_XY(sx - 5, sy - 19), 0x00);
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
   PAL_DrawSmallText(buf, gpScreen, PAL_XY(3, 3), 0x00);
   PAL_DrawSmallText(buf, gpScreen, PAL_XY(2, 2), 0xFF);

   sprintf(buf, "TILE=(%d,%d)", wx / 32, wy / 16);
   PAL_DrawSmallText(buf, gpScreen, PAL_XY(3, 15), 0x00);
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
// Debug menu
// ============================================================

#define DEBUG_MENU_ITEMS 6

static const WCHAR g_rgDebugMenuLabels[DEBUG_MENU_ITEMS][3] = {
   { 0x5C0B, 0x8E64, 0 },  // 尋蹤
   { 0x9032, 0x5BF6, 0 },  // 進寶
   { 0x795E, 0x6388, 0 },  // 神授
   { 0x4FE0, 0x5F71, 0 },  // 俠影
   { 0x7149, 0x6230, 0 },  // 煉戰
   { 0x5916, 0x5178, 0 },  // 外典
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
            case 1: // 進寶
               gpGlobals->dwCash += 10000;
               break;
            case 2: // 神授
               break;
            case 3: // 俠影
               break;
            case 4: // 煉戰
               break;
            case 5: // 外典
               break;
            }
            break;
         }
      }
   }

   VIDEO_RestoreScreen(gpScreen);
   VIDEO_UpdateScreen(NULL);
}
