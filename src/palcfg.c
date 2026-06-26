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

#include "global.h"
#include "palcfg.h"
#include "util.h"
#include "resampler.h"

//
// Mutable globals
//
INT       gMusicVolume = 100;
INT       gSoundVolume = 100;
BOOL      gFullScreen = FALSE;
LOGLEVEL  gLogLevel = PAL_DEFAULT_LOGLEVEL;
DWORD     gWordLength = 10;
BOOL      gUseCustomScreenLayout = FALSE;
char     *gpszGamePath = NULL;
char     *gpszSavePath = NULL;
char     *gpszMsgFile = NULL;
char     *gpszFontFile = NULL;
char     *gpszLogFile = NULL;

//
// Screen layout globals
//
SCREENLAYOUT gScreenLayout;
int gScreenLayoutFlag[sizeof(SCREENLAYOUT) / sizeof(PAL_POS)];

//
// Extra FM data (conditional)
//
#if USE_RIX_EXTRA_INIT
uint32_t *gpExtraFMRegs = NULL;
uint8_t  *gpExtraFMVals = NULL;
uint32_t  gExtraFMLength = 0;
#endif

void
PAL_InitConfig(
	void
)
{
	gpszGamePath = strdup(PAL_PREFIX);
	gpszSavePath = strdup(PAL_SAVE_PREFIX);
	gpszMsgFile = NULL;
	gpszFontFile = NULL;
	gpszLogFile = NULL;

	gWordLength = 10;
	gMusicVolume = 100;
	gSoundVolume = 100;
	gLogLevel = PAL_DEFAULT_LOGLEVEL;
	gFullScreen = FALSE;
	gUseCustomScreenLayout = FALSE;

	memset(&gScreenLayout, 0, sizeof(SCREENLAYOUT));
	memset(gScreenLayoutFlag, 0, sizeof(gScreenLayoutFlag));

	gScreenLayout = (SCREENLAYOUT){
		// Equipment Screen
		PAL_XY(8, 8), PAL_XY(2, 95), PAL_XY(5, 70), PAL_XY(51, 57),
		{ PAL_XY(92, 11), PAL_XY(92, 33), PAL_XY(92, 55), PAL_XY(92, 77), PAL_XY(92, 99), PAL_XY(92, 121) },
		{ PAL_XY(130, 11), PAL_XY(130, 33), PAL_XY(130, 55), PAL_XY(130, 77), PAL_XY(130, 99), PAL_XY(130, 121) },
		{ PAL_XY(226, 10), PAL_XY(226, 32), PAL_XY(226, 54), PAL_XY(226, 76), PAL_XY(226, 98) },
		{ PAL_XY(260, 14), PAL_XY(260, 36), PAL_XY(260, 58), PAL_XY(260, 80), PAL_XY(260, 102) },

		// Status Screen
		PAL_XY(110, 8), PAL_XY(110, 30), PAL_XY(6, 6),  PAL_XY(6, 32),  PAL_XY(6, 54),  PAL_XY(6, 76),
		{ PAL_XY(6, 98),   PAL_XY(6, 118),  PAL_XY(6, 138),  PAL_XY(6, 158),  PAL_XY(6, 178) },
		PAL_XY(58, 6), PAL_XY(58, 15), PAL_XY(0, 0), PAL_XY(54, 35), PAL_XY(42, 56),
		PAL_XY(63, 61), PAL_XY(65, 58), PAL_XY(42, 78), PAL_XY(63, 83), PAL_XY(65, 80),
		{ PAL_XY(42, 102), PAL_XY(42, 122), PAL_XY(42, 142), PAL_XY(42, 162), PAL_XY(42, 182) },
		{ PAL_XY(189, -1), PAL_XY(247, 39), PAL_XY(251, 101), PAL_XY(201, 133), PAL_XY(141, 141), PAL_XY(81, 125) },
		{ PAL_XY(195, 38), PAL_XY(253, 78), PAL_XY(257, 140), PAL_XY(207, 172), PAL_XY(147, 180), PAL_XY(87, 164) },
		{ PAL_XY(185, 58), PAL_XY(185, 76), PAL_XY(185, 94), PAL_XY(185, 112), PAL_XY(185, 130), PAL_XY(185, 148), PAL_XY(185, 166), PAL_XY(185, 184), PAL_XY(185, 184), PAL_XY(185, 184) },

		// Extra Lines
		PAL_XY(0, 0), PAL_XY(0, 0)
	};
}

void
PAL_FreeConfig(
	void
)
{
#if USE_RIX_EXTRA_INIT
	free(gpExtraFMRegs);
	free(gpExtraFMVals);
	gpExtraFMRegs = NULL;
	gpExtraFMVals = NULL;
	gExtraFMLength = 0;
#endif
	free(gpszMsgFile);
	free(gpszFontFile);
	free(gpszGamePath);
	free(gpszSavePath);
	free(gpszLogFile);

	gpszMsgFile = NULL;
	gpszFontFile = NULL;
	gpszGamePath = NULL;
	gpszSavePath = NULL;
	gpszLogFile = NULL;
}
