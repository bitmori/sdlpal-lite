/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// hack.c - Text-based DSL hack system for sdlpal-lite
//
// File format (hack.txt):
//   +Name        — start a named hack group
//   #COMMAND ... — an instruction within the current group
//   ;comment     — ignored
//   -            — end current group
//

#include "main.h"
#include "hack.h"

#define MAX_HACKS        64
#define MAX_INSTRUCTIONS 256
#define MAX_NAME_LEN     64

typedef enum tagHACK_OP
{
   kHackOpAddCash,
   kHackOpChangeMagicData,
   kHackOpChangeObj,
   kHackOpEditScript,
} HACK_OP;

typedef struct tagHACK_INSTR
{
   HACK_OP  op;
   INT      args[5];
} HACK_INSTR;

#define MAX_DESC_LEN     128

typedef struct tagHACK_GROUP
{
   char         szName[MAX_NAME_LEN];
   char         szDesc[MAX_DESC_LEN];
   HACK_INSTR   rgInstr[MAX_INSTRUCTIONS];
   int          nInstr;
   BOOL         fAutoRun;
} HACK_GROUP;

static HACK_GROUP g_rgHacks[MAX_HACKS];
static int        g_nHacks = 0;
static BOOL       g_fLoaded = FALSE;

static BOOL
PAL_ParseInstruction(
   const char  *pszLine,
   HACK_INSTR  *pInstr
)
{
   char cmd[32];
   int n;

   if (sscanf(pszLine, "%31s%n", cmd, &n) < 1)
      return FALSE;

   pszLine += n;

   if (strcasecmp(cmd, "ADD_CASH") == 0)
   {
      pInstr->op = kHackOpAddCash;
      if (sscanf(pszLine, "%i", &pInstr->args[0]) < 1)
         return FALSE;
   }
   else if (strcasecmp(cmd, "CHANGE_MAGIC_DATA") == 0)
   {
      pInstr->op = kHackOpChangeMagicData;
      if (sscanf(pszLine, "%i %i %i", &pInstr->args[0], &pInstr->args[1], &pInstr->args[2]) < 3)
         return FALSE;
   }
   else if (strcasecmp(cmd, "CHANGE_OBJ") == 0)
   {
      pInstr->op = kHackOpChangeObj;
      if (sscanf(pszLine, "%i %i %i", &pInstr->args[0], &pInstr->args[1], &pInstr->args[2]) < 3)
         return FALSE;
   }
   else if (strcasecmp(cmd, "EDIT_SCRIPT") == 0)
   {
      pInstr->op = kHackOpEditScript;
      if (sscanf(pszLine, "%i %i %i %i %i",
         &pInstr->args[0], &pInstr->args[1], &pInstr->args[2],
         &pInstr->args[3], &pInstr->args[4]) < 5)
         return FALSE;
   }
   else
   {
      return FALSE;
   }

   return TRUE;
}

VOID
PAL_LoadHacks(
   VOID
)
{
   FILE *fp;
   char buf[512];
   int  iCurrent = -1;

   if (g_fLoaded) return;
   g_fLoaded = TRUE;
   g_nHacks = 0;

   fp = UTIL_OpenFileForMode("hack.txt", "r");
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

      if (*p == '+' || *p == '&')
      {
         BOOL fAuto = (*p == '&');
         if (g_nHacks >= MAX_HACKS) break;
         iCurrent = g_nHacks++;
         g_rgHacks[iCurrent].nInstr = 0;
         g_rgHacks[iCurrent].fAutoRun = fAuto;
         g_rgHacks[iCurrent].szDesc[0] = '\0';

         p++;
         while (*p == ' ' || *p == '\t') p++;

         // Truncate name to 6 visible characters (UTF-8 aware)
         {
            const char *src = p;
            int nChars = 0;
            size_t byteLen = 0;
            while (*src && nChars < 6)
            {
               unsigned char c = (unsigned char)*src;
               int n;
               if (c < 0x80) n = 1;
               else if ((c & 0xE0) == 0xC0) n = 2;
               else if ((c & 0xF0) == 0xE0) n = 3;
               else n = 4;
               src += n;
               byteLen += n;
               nChars++;
            }
            if (byteLen >= MAX_NAME_LEN) byteLen = MAX_NAME_LEN - 1;
            memcpy(g_rgHacks[iCurrent].szName, p, byteLen);
            g_rgHacks[iCurrent].szName[byteLen] = '\0';
         }
      }
      else if (*p == '!')
      {
         if (iCurrent < 0) continue;
         p++;
         while (*p == ' ' || *p == '\t') p++;
         strncpy(g_rgHacks[iCurrent].szDesc, p, MAX_DESC_LEN - 1);
         g_rgHacks[iCurrent].szDesc[MAX_DESC_LEN - 1] = '\0';
      }
      else if (*p == '-')
      {
         iCurrent = -1;
      }
      else if (*p == '#')
      {
         if (iCurrent < 0) continue;

         HACK_GROUP *pGroup = &g_rgHacks[iCurrent];
         if (pGroup->nInstr >= MAX_INSTRUCTIONS) continue;

         p++;
         while (*p == ' ' || *p == '\t') p++;

         if (PAL_ParseInstruction(p, &pGroup->rgInstr[pGroup->nInstr]))
            pGroup->nInstr++;
      }
   }

   fclose(fp);
}

static VOID
PAL_ExecuteInstruction(
   const HACK_INSTR *pInstr
)
{
   switch (pInstr->op)
   {
   case kHackOpAddCash:
      gpGlobals->dwCash += pInstr->args[0];
      break;

   case kHackOpChangeMagicData:
   {
      int id = pInstr->args[0];
      int off = pInstr->args[1];
      int val = pInstr->args[2];
      if (id >= 0 && id < gpGlobals->g.nMagic &&
          off >= 0 && off < (int)(sizeof(MAGIC) / sizeof(WORD)))
      {
         ((WORD *)&gpGlobals->g.lprgMagic[id])[off] = (WORD)val;
      }
      break;
   }

   case kHackOpChangeObj:
   {
      int id = pInstr->args[0];
      int off = pInstr->args[1];
      int val = pInstr->args[2];
      if (id >= 0 && id < MAX_OBJECTS && off >= 0 && off < 7)
      {
         gpGlobals->g.rgObject[id].rgwData[off] = (WORD)val;
      }
      break;
   }

   case kHackOpEditScript:
   {
      int entry = pInstr->args[0];
      if (entry >= 0 && entry < gpGlobals->g.nScriptEntry)
      {
         gpGlobals->g.lprgScriptEntry[entry].wOperation = (WORD)pInstr->args[1];
         gpGlobals->g.lprgScriptEntry[entry].rgwOperand[0] = (WORD)pInstr->args[2];
         gpGlobals->g.lprgScriptEntry[entry].rgwOperand[1] = (WORD)pInstr->args[3];
         gpGlobals->g.lprgScriptEntry[entry].rgwOperand[2] = (WORD)pInstr->args[4];
      }
      break;
   }
   }
}

VOID
PAL_FreeHacks(
   VOID
)
{
   g_nHacks = 0;
   g_fLoaded = FALSE;
}

static INT
PAL_FindManualHack(
   INT iMenuIndex
)
{
   int i, n = 0;
   for (i = 0; i < g_nHacks; i++)
   {
      if (g_rgHacks[i].fAutoRun) continue;
      if (n == iMenuIndex) return i;
      n++;
   }
   return -1;
}

INT
PAL_GetHackCount(
   VOID
)
{
   int i, n = 0;
   for (i = 0; i < g_nHacks; i++)
   {
      if (!g_rgHacks[i].fAutoRun) n++;
   }
   return n;
}

LPCSTR
PAL_GetHackName(
   INT iIndex
)
{
   int i = PAL_FindManualHack(iIndex);
   if (i < 0) return NULL;
   return g_rgHacks[i].szName;
}

LPCSTR
PAL_GetHackDesc(
   INT iIndex
)
{
   int i = PAL_FindManualHack(iIndex);
   if (i < 0 || g_rgHacks[i].szDesc[0] == '\0') return NULL;
   return g_rgHacks[i].szDesc;
}

VOID
PAL_ExecuteHack(
   INT iIndex
)
{
   int i = PAL_FindManualHack(iIndex);
   int j;
   if (i < 0) return;
   for (j = 0; j < g_rgHacks[i].nInstr; j++)
   {
      PAL_ExecuteInstruction(&g_rgHacks[i].rgInstr[j]);
   }
}

VOID
PAL_ExecuteAutoHacks(
   VOID
)
{
   int i, j;
   for (i = 0; i < g_nHacks; i++)
   {
      if (!g_rgHacks[i].fAutoRun) continue;
      for (j = 0; j < g_rgHacks[i].nInstr; j++)
      {
         PAL_ExecuteInstruction(&g_rgHacks[i].rgInstr[j]);
      }
   }
}
