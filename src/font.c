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

#include "font.h"
#include "util.h"
#include "text.h"

#define _FONT_C

#include "ascii.h"

#define FONT_TABLE_SIZE  0x10000

static int _font_height = 16;

//
// Main font (wor16.fon, 16px)
//
static uint8_t (*unicode_font)[32] = NULL;
static uint8_t *font_width = NULL;
static int unicode_upper_base = 0;
static int unicode_lower_top = 0;
static int unicode_upper_top = 0;

//
// Small font (zpix.bdf, 12px)
//
#define SMALL_FONT_HEIGHT 12
static uint8_t (*small_font)[32] = NULL;
static uint8_t *small_font_width = NULL;

static uint8_t reverseBits(uint8_t x) {
    uint8_t y = 0;
    for (int i = 0 ; i < 8; i++){
        y <<= 1;
        y |= (x & 1);
        x >>= 1;
    }
    return y;
}

#ifdef DEBUG
void dump_font(BYTE *buf,int rows, int cols)
{
   for(int row=0;row<rows;row++)
   {
      for( int bit=0;bit<cols;bit++)
         if( buf[row] & (uint8_t)pow(2,bit) )
            printf("*");
         else
            printf(" ");
      printf("\n");
   }
}

static uint16_t reverseBits16(uint16_t x)
{
   //specified to unifont structure; not common means
   uint8_t l = reverseBits(x);
   uint8_t h = reverseBits(x >> 8);
   return h << 8 | l;
}

void dump_font16(WORD *buf,int rows, int cols)
{
   for(int row=0;row<rows;row++)
   {
      for( int bit=0;bit<cols;bit++)
         if( reverseBits16(buf[row]) & (uint16_t)pow(2,bit) )
            printf("*");
         else
            printf(" ");
      printf("\n");
   }
}
#endif

static void PAL_LoadISOFont(void)
{
    int         i, j;

    if (unicode_font == NULL || font_width == NULL)
    {
        return;
    }

    for (i = 0; i < (int)(sizeof(iso_font) / 15); i++)
    {
        for (j = 0; j < 15; j++)
        {
            unicode_font[i][j] = reverseBits(iso_font[i * 15 + j]);
        }

        unicode_font[i][15] = 0;
        font_width[i] = 16;
    }
}

static void PAL_LoadEmbeddedFont(void)
{
	FILE *fp;
	char *char_buf;
	wchar_t *wchar_buf;
	size_t nBytes;
	int nChars, i;

	if (unicode_font == NULL || font_width == NULL)
	{
		return;
	}

	//
	// Load the wor16.asc file.
	//
	if (NULL == (fp = UTIL_OpenFile("wor16.asc")))
	{
		return;
	}

	//
	// Get the size of wor16.asc file.
	//
	fseek(fp, 0, SEEK_END);
	nBytes = ftell(fp);

	//
	// Allocate buffer & read all the character codes.
	//
	if (NULL == (char_buf = (char *)malloc(nBytes)))
	{
		fclose(fp);
		return;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(char_buf, 1, nBytes, fp) < nBytes)
	{
		fclose(fp);
		return;
	}

	//
	// Close wor16.asc file.
	//
	fclose(fp);

	//
	// Detect the codepage of 'wor16.asc' and exit if not BIG5 or probability < 99
	// Note: 100% probability is impossible as the function does not recognize some special
	// characters such as bopomofo that may be used by 'wor16.asc'.
	//
	if (PAL_DetectCodePageForString(char_buf, nBytes, CP_BIG5, &i) != CP_BIG5 || i < 99)
	{
		free(char_buf);
		return;
	}

	//
	// Convert characters into unicode
	// Explictly specify BIG5 here for compatibility with codepage auto-detection
	//
	nChars = PAL_MultiByteToWideCharCP(CP_BIG5, char_buf, nBytes, NULL, 0);
	if (NULL == (wchar_buf = (wchar_t *)malloc(nChars * sizeof(wchar_t))))
	{
		free(char_buf);
		return;
	}
	PAL_MultiByteToWideCharCP(CP_BIG5, char_buf, nBytes, wchar_buf, nChars);
	free(char_buf);

	//
	// Read bitmaps from wor16.fon file.
	//
	fp = UTIL_OpenFile("wor16.fon");

	//
	// The font glyph data begins at offset 0x682 in wor16.fon.
	//
	fseek(fp, 0x682, SEEK_SET);

	//
	// Replace the original fonts
	//
	for (i = 0; i < nChars; i++)
	{
		wchar_t w = (wchar_buf[i] >= unicode_upper_base) ? (wchar_buf[i] - unicode_upper_base + unicode_lower_top) : wchar_buf[i];
		if (w < 0 || w >= FONT_TABLE_SIZE)
		{
			fseek(fp, 30, SEEK_CUR);
			continue;
		}
		if (fread(unicode_font[w], 30, 1, fp) == 1)
		{
			unicode_font[w][30] = 0;
			unicode_font[w][31] = 0;
		}
		font_width[w] = 32;
	}
	free(wchar_buf);

	fclose(fp);

	_font_height = 15;
}

INT
PAL_LoadUserFont(
   LPCSTR      pszBdfFileName
)
/*++
  Purpose:

    Loads a BDF bitmap font file.

  Parameters:

    [IN]  pszBdfFileName - Name of BDF bitmap font file..

  Return value:

    0 = success, -1 = failure.

--*/
{
   char buf[4096];
   int state = 0;
   int codepage = -1;
   DWORD dwEncoding = 0;
   BYTE bFontGlyph[32] = {0};
   int iCurHeight = 0;
   int bbw = 0, bbh = 0, bbox, bboy;

   FILE *fp = UTIL_OpenFileForMode(pszBdfFileName, "r");

   if (fp == NULL)
   {
      return -1;
   }

   if (unicode_font == NULL || font_width == NULL)
   {
      fclose(fp);
      return -1;
   }


   while (fgets(buf, 4096, fp) != NULL)
   {
      if (state == 0)
      {
         if (strncmp(buf, "FONT ", 5) == 0)
         {
            if (strcasestr(buf, "iso10646") != NULL)
            {
               codepage = CP_UCS;
            }
         }
         else if (strncmp(buf, "CHARSET_REGISTRY", 16) == 0)
         {
            if (strcasestr(buf, "Big5") != NULL)
            {
               codepage = CP_BIG5;
            }
            else if (strcasestr(buf, "GBK") != NULL || strcasestr(buf, "GB2312") != NULL)
            {
               codepage = CP_GBK;
            }
            else if (strcasestr(buf, "ISO10646") != NULL)
            {
               codepage = CP_UCS;
            }
            //else if (strstr(buf, "JISX0208") != NULL)
            //
            //  codepage = CP_JISX0208;
            //}
         }
         else if (strncmp(buf, "ENCODING", 8) == 0)
         {
            dwEncoding = atoi(buf + 8);
         }
         else if (strncmp(buf, "SIZE", 3) == 0)
         {
         }
         else if (strncmp(buf, "BBX", 3) == 0)
         {
            int bytes_consumed = 0, bytes_now;
            char bbx[10];
            sscanf(buf+bytes_consumed,"%s%n",bbx,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbw,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbh,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbox,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bboy,&bytes_now);bytes_consumed += bytes_now;
         }
         else if (strncmp(buf, "BITMAP", 6) == 0)
         {
            state = 1;
            memset(bFontGlyph, 0, sizeof(bFontGlyph));
            iCurHeight = (_font_height - 2) - bbh - bboy;
            if (iCurHeight < 0) iCurHeight = 0;
            if (iCurHeight + bbh > 16) iCurHeight = 16 - bbh;
         }
      }
      else if (state == 1)
      {
         if (strncmp(buf, "ENDCHAR", 7) == 0)
         {
            //
            // Replace the original fonts
            //
            wchar_t w = 0;
            if (codepage == CP_UCS)
            {
               w = (wchar_t)dwEncoding;
            }
            else
            {
               BYTE szCp[3];
               szCp[0] = (dwEncoding >> 8) & 0xFF;
               szCp[1] = dwEncoding & 0xFF;
               szCp[2] = 0;

               if (codepage == CP_GBK && dwEncoding > 0xFF)
               {
                  szCp[0] |= 0x80;
                  szCp[1] |= 0x80;
               }

               wchar_t wc[2] = { 0 };
               PAL_MultiByteToWideCharCP(codepage, (LPCSTR)szCp, 2, wc, 1);
               w = wc[0];
            }

            if (w != 0)
            {
               wchar_t idx = (w >= unicode_upper_base) ? (w - unicode_upper_base + unicode_lower_top) : w;
               if (idx < FONT_TABLE_SIZE)
               {
                  memcpy(unicode_font[idx], bFontGlyph, sizeof(bFontGlyph));
                  font_width[idx] = (bbw <= 8) ? 16 : 32;
               }
            }

            state = 0;
         }
         else
         {
            if (iCurHeight < 16)
            {
               WORD wCode = strtoul(buf, NULL, 16);
               if (bbw <= 8)
               {
                  bFontGlyph[iCurHeight * 2] = wCode & 0xFF;
                  bFontGlyph[iCurHeight * 2 + 1] = 0;
               }
               else
               {
                  bFontGlyph[iCurHeight * 2] = (wCode >> 8) & 0xFF;
                  bFontGlyph[iCurHeight * 2 + 1] = wCode & 0xFF;
               }
               iCurHeight++;
            }
         }
      }
   }

   fclose(fp);
   return 0;
}

static INT
PAL_LoadSmallFont(
   LPCSTR      pszBdfFileName
)
{
   char buf[4096];
   int state = 0;
   int codepage = -1;
   DWORD dwEncoding = 0;
   BYTE bFontGlyph[32] = {0};
   int iCurHeight = 0;
   int bbw = 0, bbh = 0, bboy = 0;
   int dwidth = 0;

   FILE *fp = UTIL_OpenFileForMode(pszBdfFileName, "r");
   if (fp == NULL)
      return -1;

   if (small_font == NULL || small_font_width == NULL)
   {
      fclose(fp);
      return -1;
   }

   while (fgets(buf, 4096, fp) != NULL)
   {
      if (state == 0)
      {
         if (strncmp(buf, "FONT ", 5) == 0)
         {
            if (strcasestr(buf, "iso10646") != NULL)
               codepage = CP_UCS;
         }
         else if (strncmp(buf, "ENCODING", 8) == 0)
         {
            dwEncoding = atoi(buf + 8);
         }
         else if (strncmp(buf, "DWIDTH", 6) == 0)
         {
            dwidth = atoi(buf + 6);
         }
         else if (strncmp(buf, "BBX", 3) == 0)
         {
            int bytes_consumed = 0, bytes_now;
            char bbx_str[10];
            sscanf(buf+bytes_consumed,"%s%n",bbx_str,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbw,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bbh,&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%*d%n",&bytes_now);bytes_consumed += bytes_now;
            sscanf(buf+bytes_consumed,"%d%n",&bboy,&bytes_now);bytes_consumed += bytes_now;
         }
         else if (strncmp(buf, "BITMAP", 6) == 0)
         {
            state = 1;
            memset(bFontGlyph, 0, sizeof(bFontGlyph));
            iCurHeight = (SMALL_FONT_HEIGHT - 2) - bbh - bboy;
            if (iCurHeight < 0) iCurHeight = 0;
            if (iCurHeight + bbh > SMALL_FONT_HEIGHT) iCurHeight = SMALL_FONT_HEIGHT - bbh;
         }
      }
      else
      {
         if (strncmp(buf, "ENDCHAR", 7) == 0)
         {
            wchar_t w = 0;
            if (codepage == CP_UCS)
               w = (wchar_t)dwEncoding;

            if (w > 0 && w < FONT_TABLE_SIZE)
            {
               memcpy(small_font[w], bFontGlyph, sizeof(bFontGlyph));
               small_font_width[w] = dwidth > 0 ? dwidth : (bbw <= 7 ? 7 : 13);
            }
            state = 0;
         }
         else
         {
            if (iCurHeight < SMALL_FONT_HEIGHT)
            {
               WORD wCode = strtoul(buf, NULL, 16);
               if (bbw <= 8)
               {
                  bFontGlyph[iCurHeight * 2] = wCode & 0xFF;
                  bFontGlyph[iCurHeight * 2 + 1] = 0;
               }
               else
               {
                  bFontGlyph[iCurHeight * 2] = (wCode >> 8) & 0xFF;
                  bFontGlyph[iCurHeight * 2 + 1] = wCode & 0xFF;
               }
               iCurHeight++;
            }
         }
      }
   }

   fclose(fp);
   return 0;
}

int
PAL_InitFont(
   void
)
{
   //
   // Allocate the font tables dynamically.
   //
   unicode_font = (uint8_t (*)[32])calloc(FONT_TABLE_SIZE, 32);
   font_width = (uint8_t *)calloc(FONT_TABLE_SIZE, 1);

   if (unicode_font == NULL || font_width == NULL)
   {
      free(unicode_font);
      free(font_width);
      unicode_font = NULL;
      font_width = NULL;
      return -1;
   }

   //
   // Set up range mapping: use a flat table covering the full BMP.
   // All codepoints < FONT_TABLE_SIZE are stored directly at their index.
   //
   unicode_lower_top = FONT_TABLE_SIZE;
   unicode_upper_base = FONT_TABLE_SIZE;
   unicode_upper_top = FONT_TABLE_SIZE;

   if (!gpszMsgFile)
   {
      PAL_LoadEmbeddedFont();
   }

   if (g_TextLib.fUseISOFont)
   {
      PAL_LoadISOFont();
   }

   if (gpszFontFile)
   {
      PAL_LoadUserFont(gpszFontFile);
   }

   //
   // Allocate and load the small font (zpix.bdf)
   //
   small_font = (uint8_t (*)[32])calloc(FONT_TABLE_SIZE, 32);
   small_font_width = (uint8_t *)calloc(FONT_TABLE_SIZE, 1);
   if (small_font != NULL && small_font_width != NULL)
   {
      PAL_LoadSmallFont("zpix.bdf");
   }

   return 0;
}

void
PAL_FreeFont(
	void
)
{
	if (unicode_font != NULL)
	{
		free(unicode_font);
		unicode_font = NULL;
	}
	if (font_width != NULL)
	{
		free(font_width);
		font_width = NULL;
	}
	if (small_font != NULL)
	{
		free(small_font);
		small_font = NULL;
	}
	if (small_font_width != NULL)
	{
		free(small_font_width);
		small_font_width = NULL;
	}
}

void
PAL_DrawCharOnSurface(
	uint16_t                 wChar,
	SDL_Surface             *lpSurface,
	PAL_POS                  pos,
	uint8_t                  bColor,
	BOOL                     fUse8x8Font
)
{
	int       i, j;
	int       x = PAL_X(pos), y = PAL_Y(pos);

	//
	// Check for NULL pointer & invalid char code.
	//
	if (lpSurface == NULL || unicode_font == NULL || font_width == NULL)
	{
		return;
	}
	if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) ||
		wChar >= unicode_upper_top || (_font_height == 8 && wChar >= 0x100))
	{
		return;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - unicode_lower_top);
	}

	//
	// Draw the character to the surface.
	//
	LPBYTE dest = (LPBYTE)lpSurface->pixels + y * lpSurface->pitch + x;
	LPBYTE top = (LPBYTE)lpSurface->pixels + lpSurface->h * lpSurface->pitch;
	if (fUse8x8Font)
	{
		for (i = 0; i < 8 && dest < top; i++, dest += lpSurface->pitch)
		{
			for (j = 0; j < 8 && x + j < lpSurface->w; j++)
			{
				if (iso_font_8x8[wChar][i] & (1 << j))
				{
					dest[j] = bColor;
				}
			}
		}
	}
	else
	{
		if (font_width[wChar] == 32)
		{
			for (i = 0; i < _font_height * 2 && dest < top; i += 2, dest += lpSurface->pitch)
			{
				for (j = 0; j < 8 && x + j < lpSurface->w; j++)
				{
					if (unicode_font[wChar][i] & (1 << (7 - j)))
					{
						dest[j] = bColor;
					}
				}
				for (j = 0; j < 8 && x + j + 8 < lpSurface->w; j++)
				{
					if (unicode_font[wChar][i + 1] & (1 << (7 - j)))
					{
						dest[j + 8] = bColor;
					}
				}
			}
		}
		else
		{
			for (i = 0; i < _font_height && dest < top; i++, dest += lpSurface->pitch)
			{
				for (j = 0; j < 8 && x + j < lpSurface->w; j++)
				{
					if (unicode_font[wChar][i] & (1 << (7 - j)))
					{
						dest[j] = bColor;
					}
				}
			}
		}
	}
}

int
PAL_CharWidth(
	uint16_t                 wChar
)
{
	if (font_width == NULL)
	{
		return 0;
	}

	if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || wChar >= unicode_upper_top)
	{
		return 0;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - unicode_lower_top);
	}

	return font_width[wChar] >> 1;
}

int
PAL_FontHeight(
	void
)
{
	return _font_height;
}

void
PAL_DrawSmallCharOnSurface(
	uint16_t                 wChar,
	SDL_Surface             *lpSurface,
	PAL_POS                  pos,
	uint8_t                  bColor
)
{
	int       i, j;
	int       x = PAL_X(pos), y = PAL_Y(pos);

	if (lpSurface == NULL || small_font == NULL || small_font_width == NULL)
		return;
	if (small_font_width[wChar] == 0)
		return;

	LPBYTE dest = (LPBYTE)lpSurface->pixels + y * lpSurface->pitch + x;
	LPBYTE top = (LPBYTE)lpSurface->pixels + lpSurface->h * lpSurface->pitch;

	for (i = 0; i < SMALL_FONT_HEIGHT && dest < top; i++, dest += lpSurface->pitch)
	{
		for (j = 0; j < 8 && x + j < lpSurface->w; j++)
		{
			if (small_font[wChar][i * 2] & (1 << (7 - j)))
				dest[j] = bColor;
		}
		for (j = 0; j < 8 && x + j + 8 < lpSurface->w; j++)
		{
			if (small_font[wChar][i * 2 + 1] & (1 << (7 - j)))
				dest[j + 8] = bColor;
		}
	}
}

int
PAL_SmallCharWidth(
	uint16_t                 wChar
)
{
	if (small_font_width == NULL || small_font_width[wChar] == 0)
		return 0;
	return small_font_width[wChar];
}

int
PAL_SmallFontHeight(
	void
)
{
	return SMALL_FONT_HEIGHT;
}

static uint32_t
PAL_DecodeUTF8(
	const char **pp
)
{
	const unsigned char *p = (const unsigned char *)*pp;
	uint32_t cp;
	if (*p < 0x80) {
		cp = *p++;
	} else if ((*p & 0xE0) == 0xC0) {
		cp = (*p++ & 0x1F) << 6;
		cp |= (*p++ & 0x3F);
	} else if ((*p & 0xF0) == 0xE0) {
		cp = (*p++ & 0x0F) << 12;
		cp |= (*p++ & 0x3F) << 6;
		cp |= (*p++ & 0x3F);
	} else {
		cp = '?';
		p++;
	}
	*pp = (const char *)p;
	return cp;
}

void
PAL_DrawSmallText(
	const char          *pszText,
	SDL_Surface         *lpSurface,
	PAL_POS              pos,
	uint8_t              bColor
)
{
	int x = PAL_X(pos), y = PAL_Y(pos);
	while (*pszText)
	{
		uint32_t cp = PAL_DecodeUTF8(&pszText);
		if (cp >= FONT_TABLE_SIZE) continue;
		PAL_DrawSmallCharOnSurface((uint16_t)cp, lpSurface, PAL_XY(x, y), bColor);
		x += PAL_SmallCharWidth((uint16_t)cp);
	}
}
