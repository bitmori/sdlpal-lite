/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2017, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"

BYTE
PAL_CalcShadowColor(
   BYTE bSourceColor
)
{
    return ((bSourceColor&0xF0)|((bSourceColor&0x0F)>>1));
}

INT
PAL_RLEBlitOne(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   UINT              flags,
   BYTE              bMonoColor,
   INT               iColorShift
)
{
   UINT          uiWidth, uiHeight, uiLen;
   INT           dx = PAL_X(pos), dy = PAL_Y(pos);

   if (lpBitmapRLE == NULL || lpDstSurface == NULL)
      return -1;

   if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
       lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
   {
      lpBitmapRLE += 4;
   }

   uiWidth = lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
   uiHeight = lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);
   uiLen = uiWidth * uiHeight;
   lpBitmapRLE += 4;

   if (flags & RLE_BLIT_MIRROR)
   {
      UINT i, k;
      INT  y;
      UINT uiSrcX = 0, uiSrcY = 0;
      BYTE T;
      LPBYTE p;

      if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
          (INT)uiHeight + dy <= 0 || dy >= lpDstSurface->h)
         return 0;

      bMonoColor &= 0xF0;

      for (i = 0; i < uiLen;)
      {
         T = *lpBitmapRLE++;
         if ((T & 0x80) && T <= 0x80 + uiWidth)
         {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            while (uiSrcX >= uiWidth) { uiSrcX -= uiWidth; uiSrcY++; }
         }
         else
         {
            UINT processed = 0;
            UINT curSrcX = uiSrcX;

            y = dy + uiSrcY;
            if (y < 0 || y >= lpDstSurface->h)
            {
               lpBitmapRLE += T;
               i += T;
               uiSrcX += T;
               while (uiSrcX >= uiWidth) { uiSrcX -= uiWidth; uiSrcY++; }
               continue;
            }

            p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;

            while (processed < T)
            {
               UINT pixelsInRow = T - processed;
               if (pixelsInRow > uiWidth - curSrcX)
                  pixelsInRow = uiWidth - curSrcX;

               {
                  INT dstXStart = dx + (INT)uiWidth - 1 - (INT)curSrcX;
                  INT dstXEnd = dx + (INT)uiWidth - 1 - (INT)(curSrcX + pixelsInRow - 1);
                  INT clipLeft = dstXEnd < 0 ? 0 : dstXEnd;
                  INT clipRight = dstXStart >= lpDstSurface->w ? lpDstSurface->w - 1 : dstXStart;

                  if (clipLeft <= clipRight)
                  {
                     INT pxClippedLeft = clipLeft - dstXEnd;
                     INT pxToDraw = clipRight - clipLeft + 1;
                     for (k = 0; (INT)k < pxToDraw; k++)
                     {
                        BYTE src = lpBitmapRLE[processed + pxClippedLeft + k];
                        if (bMonoColor)
                        {
                           BYTE b = src & 0x0F;
                           if ((INT)b + iColorShift > 0x0F) b = 0x0F;
                           else if ((INT)b + iColorShift < 0) b = 0;
                           else b += iColorShift;
                           p[clipRight - k] = b | bMonoColor;
                        }
                        else if (iColorShift)
                        {
                           BYTE b = src & 0x0F;
                           if ((INT)b + iColorShift > 0x0F) b = 0x0F;
                           else if ((INT)b + iColorShift < 0) b = 0;
                           else b += iColorShift;
                           p[clipRight - k] = b | (src & 0xF0);
                        }
                        else
                        {
                           p[clipRight - k] = src;
                        }
                     }
                  }
               }

               processed += pixelsInRow;
               curSrcX += pixelsInRow;
               if (curSrcX >= uiWidth)
               {
                  curSrcX = 0;
                  y++;
                  if (y >= lpDstSurface->h) break;
                  p = ((LPBYTE)lpDstSurface->pixels) + y * lpDstSurface->pitch;
               }
            }

            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth) { uiSrcX -= uiWidth; uiSrcY++; }
         }
      }
   }
   else
   {
      UINT i, j;
      INT  x, y;
      BYTE T;
      BOOL bShadow = (flags & RLE_BLIT_SHADOW) ? TRUE : FALSE;

      bMonoColor &= 0xF0;

      for (i = 0; i < uiLen;)
      {
         T = *lpBitmapRLE++;
         if ((T & 0x80) && T <= 0x80 + uiWidth)
         {
            i += T - 0x80;
         }
         else
         {
            for (j = 0; j < T; j++)
            {
               y = (i + j) / uiWidth + dy;
               x = (i + j) % uiWidth + dx;

               if (x < 0) { j += -x - 1; continue; }
               else if (x >= lpDstSurface->w) { j += x - lpDstSurface->w; continue; }
               if (y < 0) { j += -y * uiWidth - 1; continue; }
               else if (y >= lpDstSurface->h) { goto end; }

               LPBYTE dst = &((LPBYTE)lpDstSurface->pixels)[y * lpDstSurface->pitch + x];

               if (bShadow)
               {
                  *dst = PAL_CalcShadowColor(*dst);
               }
               else if (bMonoColor)
               {
                  BYTE b = lpBitmapRLE[j] & 0x0F;
                  if ((INT)b + iColorShift > 0x0F) b = 0x0F;
                  else if ((INT)b + iColorShift < 0) b = 0;
                  else b += iColorShift;
                  *dst = b | bMonoColor;
               }
               else if (iColorShift)
               {
                  BYTE b = lpBitmapRLE[j] & 0x0F;
                  if ((INT)b + iColorShift > 0x0F) b = 0x0F;
                  else if ((INT)b + iColorShift < 0) b = 0;
                  else b += iColorShift;
                  *dst = b | (lpBitmapRLE[j] & 0xF0);
               }
               else
               {
                  *dst = lpBitmapRLE[j];
               }
            }
            lpBitmapRLE += T;
            i += T;
         }
      }
   end:;
   }

   return 0;
}

INT
PAL_RLEBlitToSurface(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos
)
{
   return PAL_RLEBlitOne(lpBitmapRLE, lpDstSurface, pos, 0, 0, 0);
}

INT
PAL_RLEBlitToSurfaceWithShadow(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   BOOL              bShadow
)
{
   return PAL_RLEBlitOne(lpBitmapRLE, lpDstSurface, pos,
      bShadow ? RLE_BLIT_SHADOW : 0, 0, 0);
}

INT
PAL_RLEBlitWithColorShift(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   INT               iColorShift
)
{
   return PAL_RLEBlitOne(lpBitmapRLE, lpDstSurface, pos, 0, 0, iColorShift);
}

INT
PAL_RLEBlitMonoColor(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   BYTE              bColor,
   INT               iColorShift
)
{
   return PAL_RLEBlitOne(lpBitmapRLE, lpDstSurface, pos, 0, bColor, iColorShift);
}

INT
PAL_RLEBlitToSurfaceInMirror(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos
)
{
   return PAL_RLEBlitOne(lpBitmapRLE, lpDstSurface, pos, RLE_BLIT_MIRROR, 0, 0);
}

INT
PAL_FBPBlitToSurface(
   LPBYTE            lpBitmapFBP,
   SDL_Surface      *lpDstSurface
)
/*++
  Purpose:

    Blit an uncompressed bitmap in FBP.MKF to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit 320x200 one.

  Parameters:

    [IN]  lpBitmapFBP - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    0 = success, -1 = error.

--*/
{
   int       x, y;
   LPBYTE    p;

   if (lpBitmapFBP == NULL || lpDstSurface == NULL ||
      lpDstSurface->w != 320 || lpDstSurface->h != 200)
   {
      return -1;
   }

   //
   // simply copy everything to the surface
   //
   for (y = 0; y < 200; y++)
   {
      p = (LPBYTE)(lpDstSurface->pixels) + y * lpDstSurface->pitch;
      for (x = 0; x < 320; x++)
      {
         *(p++) = *(lpBitmapFBP++);
      }
   }

   return 0;
}

INT
PAL_RLEGetWidth(
   LPCBITMAPRLE    lpBitmapRLE
)
/*++
  Purpose:

    Get the width of an RLE-compressed bitmap.

  Parameters:

    [IN]  lpBitmapRLE - pointer to an RLE-compressed bitmap.

  Return value:

    Integer value which indicates the height of the bitmap.

--*/
{
   if (lpBitmapRLE == NULL)
   {
      return 0;
   }

   //
   // Skip the 0x00000002 in the file header.
   //
   if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
      lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
   {
      lpBitmapRLE += 4;
   }

   //
   // Return the width of the bitmap.
   //
   return lpBitmapRLE[0] | (lpBitmapRLE[1] << 8);
}

INT
PAL_RLEGetHeight(
   LPCBITMAPRLE       lpBitmapRLE
)
/*++
  Purpose:

    Get the height of an RLE-compressed bitmap.

  Parameters:

    [IN]  lpBitmapRLE - pointer of an RLE-compressed bitmap.

  Return value:

    Integer value which indicates the height of the bitmap.

--*/
{
   if (lpBitmapRLE == NULL)
   {
      return 0;
   }

   //
   // Skip the 0x00000002 in the file header.
   //
   if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
      lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
   {
      lpBitmapRLE += 4;
   }

   //
   // Return the height of the bitmap.
   //
   return lpBitmapRLE[2] | (lpBitmapRLE[3] << 8);
}

WORD
PAL_SpriteGetNumFrames(
   LPCSPRITE       lpSprite
)
/*++
  Purpose:

    Get the total number of frames of a sprite.

  Parameters:

    [IN]  lpSprite - pointer to the sprite.

  Return value:

    Number of frames of the sprite.

--*/
{
   if (lpSprite == NULL)
   {
      return 0;
   }

   return (lpSprite[0] | (lpSprite[1] << 8)) - 1;
}

LPCBITMAPRLE
PAL_SpriteGetFrame(
   LPCSPRITE       lpSprite,
   INT             iFrameNum
)
/*++
  Purpose:

    Get the pointer to the specified frame from a sprite.

  Parameters:

    [IN]  lpSprite - pointer to the sprite.

    [IN]  iFrameNum - number of the frame.

  Return value:

    Pointer to the specified frame. NULL if the frame does not exist.

--*/
{
   int imagecount, offset;

   if (lpSprite == NULL)
   {
      return NULL;
   }

   //
   // Hack for broken sprites like the Bloody-Mouth Bug
   //
//   imagecount = (lpSprite[0] | (lpSprite[1] << 8)) - 1;
   imagecount = (lpSprite[0] | (lpSprite[1] << 8));

   if (iFrameNum < 0 || iFrameNum >= imagecount)
   {
      //
      // The frame does not exist
      //
      return NULL;
   }

   //
   // Get the offset of the frame
   //
   iFrameNum <<= 1;
   offset = ((lpSprite[iFrameNum] | (lpSprite[iFrameNum + 1] << 8)) << 1);
   if (offset == 0x18444) offset = (WORD)offset;
   return &lpSprite[offset];
}

INT
PAL_MKFGetChunkCount(
   FILE *fp
)
/*++
  Purpose:

    Get the number of chunks in an MKF archive.

  Parameters:

    [IN]  fp - pointer to an fopen'ed MKF file.

  Return value:

    Integer value which indicates the number of chunks in the specified MKF file.

--*/
{
   INT iNumChunk;
   if (fp == NULL)
   {
      return 0;
   }

   fseek(fp, 0, SEEK_SET);
   if (fread(&iNumChunk, sizeof(INT), 1, fp) == 1)
      return (SDL_SwapLE32(iNumChunk) - 4) >> 2;
   else
      return 0;
}

INT
PAL_MKFGetChunkSize(
   UINT    uiChunkNum,
   FILE   *fp
)
/*++
  Purpose:

    Get the size of a chunk in an MKF archive.

  Parameters:

    [IN]  uiChunkNum - the number of the chunk in the MKF archive.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if the chunk does not exist.

--*/
{
   UINT    uiOffset       = 0;
   UINT    uiNextOffset   = 0;
   UINT    uiChunkCount   = 0;

   //
   // Get the total number of chunks.
   //
   uiChunkCount = PAL_MKFGetChunkCount(fp);
   if (uiChunkNum >= uiChunkCount)
   {
      return -1;
   }

   //
   // Get the offset of the specified chunk and the next chunk.
   //
   fseek(fp, 4 * uiChunkNum, SEEK_SET);
   PAL_fread(&uiOffset, sizeof(UINT), 1, fp);
   PAL_fread(&uiNextOffset, sizeof(UINT), 1, fp);

   uiOffset = SDL_SwapLE32(uiOffset);
   uiNextOffset = SDL_SwapLE32(uiNextOffset);

   //
   // Return the length of the chunk.
   //
   return uiNextOffset - uiOffset;
}

INT
PAL_MKFReadChunk(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiChunkNum,
   FILE           *fp
)
/*++
  Purpose:

    Read a chunk from an MKF archive into lpBuffer.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if there are error in parameters.
    -2 if buffer size is not enough.

--*/
{
   UINT     uiOffset       = 0;
   UINT     uiNextOffset   = 0;
   UINT     uiChunkCount;
   UINT     uiChunkLen;

   if (lpBuffer == NULL || fp == NULL || uiBufferSize == 0)
   {
      return -1;
   }

   //
   // Get the total number of chunks.
   //
   uiChunkCount = PAL_MKFGetChunkCount(fp);
   if (uiChunkNum >= uiChunkCount)
   {
      return -1;
   }

   //
   // Get the offset of the chunk.
   //
   fseek(fp, 4 * uiChunkNum, SEEK_SET);
   PAL_fread(&uiOffset, 4, 1, fp);
   PAL_fread(&uiNextOffset, 4, 1, fp);
   uiOffset = SDL_SwapLE32(uiOffset);
   uiNextOffset = SDL_SwapLE32(uiNextOffset);

   //
   // Get the length of the chunk.
   //
   uiChunkLen = uiNextOffset - uiOffset;

   if (uiChunkLen > uiBufferSize)
   {
      return -2;
   }

   if (uiChunkLen != 0)
   {
      fseek(fp, uiOffset, SEEK_SET);
      return (int)fread(lpBuffer, 1, uiChunkLen, fp);
   }

   return -1;
}

INT
PAL_MKFGetDecompressedSize(
   UINT    uiChunkNum,
   FILE   *fp
)
/*++
  Purpose:

    Get the decompressed size of a compressed chunk in an MKF archive.

  Parameters:

    [IN]  uiChunkNum - the number of the chunk in the MKF archive.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if the chunk does not exist.

--*/
{
   DWORD         buf[2];
   UINT          uiOffset;
   UINT          uiChunkCount;

   if (fp == NULL)
   {
      return -1;
   }

   //
   // Get the total number of chunks.
   //
   uiChunkCount = PAL_MKFGetChunkCount(fp);
   if (uiChunkNum >= uiChunkCount)
   {
      return -1;
   }

   //
   // Get the offset of the chunk.
   //
   fseek(fp, 4 * uiChunkNum, SEEK_SET);
   PAL_fread(&uiOffset, 4, 1, fp);
   uiOffset = SDL_SwapLE32(uiOffset);

   //
   // Read the header.
   //
   fseek(fp, uiOffset, SEEK_SET);
   PAL_fread(buf, sizeof(DWORD), 2, fp);
   buf[0] = SDL_SwapLE32(buf[0]);
   buf[1] = SDL_SwapLE32(buf[1]);

   return (buf[0] != 0x315f4a59) ? -1 : (INT)buf[1];
}

INT
PAL_MKFDecompressChunk(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiChunkNum,
   FILE           *fp
)
/*++
  Purpose:

    Decompress a compressed chunk from an MKF archive into lpBuffer.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if there are error in parameters, or buffer size is not enough.
    -3 if cannot allocate memory for decompression.

--*/
{
   LPBYTE          buf;
   int             len;

   len = PAL_MKFGetChunkSize(uiChunkNum, fp);

   if (len <= 0)
   {
      return len;
   }

   buf = (LPBYTE)malloc(len);
   if (buf == NULL)
   {
      return -3;
   }

   PAL_MKFReadChunk(buf, len, uiChunkNum, fp);

   len = Decompress(buf, lpBuffer, uiBufferSize);
   free(buf);

   return len;
}
