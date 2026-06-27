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
// Debug menu
// ============================================================

#define DEBUG_MENU_ITEMS 6

static const WCHAR g_rgDebugMenuLabels[DEBUG_MENU_ITEMS][3] = {
   { 0x5C0B, 0x8E64, 0 },  // 尋蹤
   { 0x6C23, 0x51DD, 0 },  // 氣凝 (pawn/sell)
   { 0x85CF, 0x771F, 0 },  // 藏真 (random shop)
   { 0x4FE0, 0x5F71, 0 },  // 俠影
   { 0x8A66, 0x7149, 0 },  // 試煉
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
            case 2: // 藏真 (random shop)
            {
               int nStores = gpGlobals->g.nStore;
               if (nStores > 0)
               {
                  int attempts = 0;
                  gpGlobals->dwCash += 10000;
                  while (attempts < 64)
                  {
                     int idx = RandomLong(0, nStores - 1);
                     if (gpGlobals->g.lprgStore[idx].rgwItems[0] != 0)
                     {
                        VIDEO_RestoreScreen(gpScreen);
                        VIDEO_UpdateScreen(NULL);
                        PAL_BuyMenu((WORD)idx);
                        break;
                     }
                     attempts++;
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
            }
            break;
         }
      }
   }

   VIDEO_RestoreScreen(gpScreen);
   VIDEO_UpdateScreen(NULL);
}
