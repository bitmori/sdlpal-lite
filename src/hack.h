/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// hack.h - Text-based DSL hack system for sdlpal-lite
//

#ifndef HACK_H
#define HACK_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

VOID
PAL_LoadHacks(
   VOID
);

VOID
PAL_ExecuteAutoHacks(
   VOID
);

VOID
PAL_FreeHacks(
   VOID
);

INT
PAL_GetHackCount(
   VOID
);

LPCSTR
PAL_GetHackName(
   INT iIndex
);

LPCSTR
PAL_GetHackDesc(
   INT iIndex
);

VOID
PAL_ExecuteHack(
   INT iIndex
);

PAL_C_LINKAGE_END

#endif
