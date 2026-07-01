/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// debug.h - Debug menu for sdlpal-lite
//

#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

VOID
PAL_DebugMenu(
   VOID
);

VOID
PAL_DebugOverlay(
   VOID
);

extern BOOL g_fDebugShowEvents;
extern BOOL g_fDebugShowStatus;
extern BOOL g_fDebugShowGrid;
extern BOOL g_fDebugLockParty;

PAL_C_LINKAGE_END

#endif
