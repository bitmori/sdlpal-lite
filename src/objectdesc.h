#ifndef OBJECTDESC_H
#define OBJECTDESC_H

#include "common.h"

typedef enum tagOBJECT_DESC_TYPE
{
   kObjDescMagic = 0,
   kObjDescItem,
   kObjDescOther
} OBJECT_DESC_TYPE;

PAL_C_LINKAGE_BEGIN

VOID
PAL_LoadObjectDesc(
   VOID
);

LPCSTR
PAL_GetObjectDesc(
   WORD   wObjectID
);

OBJECT_DESC_TYPE
PAL_GetObjectDescType(
   WORD   wObjectID
);

BOOL
PAL_HasObjectDesc(
   VOID
);

VOID
PAL_DrawObjectDesc(
   LPCSTR       pszText,
   SDL_Surface *lpSurface,
   int          x,
   int          y,
   BYTE         bColor
);

PAL_C_LINKAGE_END

#endif
