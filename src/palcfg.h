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

#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"
#include "palcommon.h"

typedef struct tagSCREENLAYOUT
{
	PAL_POS          EquipImageBox;
	PAL_POS          EquipRoleListBox;
	PAL_POS          EquipItemName;
	PAL_POS          EquipItemAmount;
	PAL_POS          EquipLabels[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          EquipNames[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          EquipStatusLabels[5];
	PAL_POS          EquipStatusValues[5];

	PAL_POS          RoleName;
	PAL_POS          RoleImage;
	PAL_POS          RoleExpLabel;
	PAL_POS          RoleLevelLabel;
	PAL_POS          RoleHPLabel;
	PAL_POS          RoleMPLabel;
	PAL_POS          RoleStatusLabels[5];
	PAL_POS          RoleCurrExp;
	PAL_POS          RoleNextExp;
	PAL_POS          RoleExpSlash;
	PAL_POS          RoleLevel;
	PAL_POS          RoleCurHP;
	PAL_POS          RoleMaxHP;
	PAL_POS          RoleHPSlash;
	PAL_POS          RoleCurMP;
	PAL_POS          RoleMaxMP;
	PAL_POS          RoleMPSlash;
	PAL_POS          RoleStatusValues[5];
	PAL_POS          RoleEquipImageBoxes[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          RoleEquipNames[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          RolePoisonNames[MAX_POISONS];

	PAL_POS          ExtraItemDescLines;
	PAL_POS          ExtraMagicDescLines;
} SCREENLAYOUT;

//
// Constants (hardcoded, never change at runtime)
//
#define PAL_SAMPLE_RATE              44100
#define PAL_MAX_SAMPLERATE           49716
#define PAL_OPL_SAMPLE_RATE          49716
#define PAL_AUDIO_CHANNELS           2
#define PAL_RESAMPLE_QUALITY         4
#define PAL_SURROUND_OPL_OFFSET      384
#define PAL_USE_SURROUND_OPL         TRUE
#define PAL_OPL_TYPE                 OPL_DOSBOX
#define PAL_KEEP_ASPECT_RATIO        TRUE
#define PAL_ENABLE_KEY_REPEAT        FALSE
#define PAL_MAX_VOLUME               100

//
// Screen layout flag values
//
enum {
	USE_8x8_FONT = 1,
	DISABLE_SHADOW = 2,
};

PAL_C_LINKAGE_BEGIN

//
// Mutable globals
//
extern INT       gMusicVolume;
extern INT       gSoundVolume;
extern BOOL      gFullScreen;
extern LOGLEVEL  gLogLevel;
extern DWORD     gWordLength;
extern BOOL      gUseCustomScreenLayout;
extern char     *gpszGamePath;
extern char     *gpszSavePath;
extern char     *gpszMsgFile;
extern char     *gpszFontFile;
extern char     *gpszLogFile;

//
// Screen layout globals
//
extern SCREENLAYOUT gScreenLayout;
extern int gScreenLayoutFlag[sizeof(SCREENLAYOUT) / sizeof(PAL_POS)];
#define gScreenLayoutArray ((PAL_POS*)&gScreenLayout)

//
// Extra FM data (conditional)
//
#if USE_RIX_EXTRA_INIT
extern uint32_t *gpExtraFMRegs;
extern uint8_t  *gpExtraFMVals;
extern uint32_t  gExtraFMLength;
#endif

void
PAL_InitConfig(
	void
);

void
PAL_FreeConfig(
	void
);

PAL_C_LINKAGE_END

#endif
