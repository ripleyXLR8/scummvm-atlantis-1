/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CRYOMNI3D_ATLANTIS_EXE_BUILD_H
#define CRYOMNI3D_ATLANTIS_EXE_BUILD_H

#include "common/scummsys.h"

namespace CryOmni3D {
namespace Atlantis {

// What the port needs to know about one build of atlantis.exe: where the
// tables it reads sit, and the code differences between builds that change
// behaviour.  Behaviour is established on the reference build
// (atlantis-cd-en-714240); another build is described by its differences
// (atlantis-re binaries/, decisions 0017 and 0018).
struct AtlantisExeBuild {
	const char *name;        // atlantis-re binaries/ name
	uint32 size;             // file size of atlantis.exe
	// The DGROUP section: virtual address and file offset (pefile).
	uint32 dgroupVA, dgroupRawPtr;
	uint32 inventoryVA;      // FUN_0042b328's object table
	uint32 guestVA;          // FUN_0042b480's guest sprite table
	uint32 creditWidthsVA;   // credits font advance table
	// The WAM + NEWSOUND name pointer table (17 WAM names, then the APC
	// names up to "FIN"), and the range its string pointers fall in.
	uint32 newSoundTableVA, newSoundPtrMin, newSoundPtrMax;
	// The player-row string: the marker players.atl holds after the last
	// name; the label shown follows its NUL when playerLabelAfterMarker,
	// else the marker itself is the label.
	uint32 playerRowVA;
	bool playerLabelAfterMarker;
	// FUN_00422fcc @ 0x0042313d: ids 0x219 / 0x21a get the navigate cursor.
	bool cursorIdRule;
	// FUN_0042844c @ 0x0042892e: perso 0x230 (Rhea) always on camera 2.
	bool rheaCamera2;
	// FUN_00426590: a second death in the same section plays the next
	// narrator line (per-section counter 0x6cccac; the 2001 build resets it
	// before the test).
	bool altGameOverNarration;

	// The 1997 build's differences (atlantis-re experiment 2026-10-18-01,
	// numbered as its list).  The reference's behaviour when false / as in
	// the reference entry.
	// 1: subtitles in the 8x8 font DIALOG\FONTS.BIN, 75-byte rows, 10 px
	// apart (FUN_0042e970, FUN_0042eb3c), not GDI Arial (decision 0018).
	bool bitmapSubtitles;
	// 5: the SUBTITLES option's three-click cycle (FUN_0041d5f4 @
	// 0x0041d73b); the 1997 build has a plain toggle.
	bool subtitleClickCycle;
	// 6: name entry takes 0x20..0x7a and stores a..z as A..Z; the player
	// list draws '#' like any glyph (no '#' rule in FUN_0041fff4).
	bool upperCaseNames;
	// 4: the game-over voice is started at the screen's entry, before the
	// image (FUN_004256a4 @ 0x004256ee..0x0042587b).
	bool gameOverVoiceAtEntry;
	// 7: the LOAD picker's knob starts at y 0x7b and is clamped there; a
	// row click tests row + offset < count (FUN_0041cf78).
	bool loadPickerKnobClamp;
	// 8: the hexagon puzzle's entry forces OBJETENMAIN to 402 (FUN_004406fc
	// @ 0x0044076e), so its pieces are always there.
	bool hexaForcesObject402;
	// 9: the ogre puzzle's queen reveal: frames and ticks per frame.
	uint8 ogreRevealFrames;
	uint16 ogreRevealTicks;
	// 10: the rat puzzle's frame waits 5 ticks after each frame (reference)
	// or until a deadline set 5 ticks after the previous wait; T0023's
	// search restarts at point 0 (reference) or goes on from where the
	// trajectory was left.
	bool ratFrameDeadline;
	bool ratT0023Restart;
	// 12: LOADMUSIK / DELMUSIK do nothing when their first value is the
	// playing track, and DelMusik deletes the playing track too.
	bool musicFirstValueRule;
	// 13: frames drawn by the Muria puzzle before its piece-on-slot hook.
	uint8 muriaPlacedFrames;

	uint32 fileOffset(uint32 va) const { return va - dgroupVA + dgroupRawPtr; }
};

// atlantis-re experiments 2026-10-17-06 / -07 / -08: the GOG executable
// (the same code for its five languages) is a 1999 build; its three tables
// are 4 bytes after the reference's; it lacks the 2001 cursor and camera
// rules and keeps the alternative game-over narrations.
// atlantis-re experiment 2026-10-18-01 (binaries/atlantis-cd-en-688128.md):
// the English retail CD's executable (May 1997), DGROUP 0x10000 lower, the
// tables at new addresses with the same bytes but the player row; it lacks
// the 2001 cursor and camera rules and the alternative narrations, and has
// the differences of its list.
static const AtlantisExeBuild kAtlantisExeBuilds[] = {
	{ "atlantis-cd-en-714240", 714240, 0x00490000, 0x71200,
	  0x004967b0, 0x00496884, 0x004966fc, 0x004961e0, 0x004b0000, 0x004d0000,
	  0x00496570, true,
	  true, true, false,
	  false, true, false, false, true, true, 3, 200, false, true, false, 2 },
	{ "atlantis-gog-714752", 714752, 0x00490000, 0x71200,
	  0x004967b4, 0x00496888, 0x00496700, 0x004961e0, 0x004b0000, 0x004d0000,
	  0x00496570, true,
	  false, false, true,
	  false, true, false, false, true, true, 3, 200, false, true, false, 2 },
	{ "atlantis-cd-en-688128", 688128, 0x00480000, 0x6c400,
	  0x00486788, 0x0048685c, 0x004866d5, 0x004861d0, 0x004a0000, 0x004b0000,
	  0x0048654a, false,
	  false, false, false,
	  true, false, true, true, false, false, 5, 75, true, false, true, 1 },
};

// The build of an executable of `size` bytes; the reference when unknown.
inline const AtlantisExeBuild &atlantisExeBuild(uint32 size) {
	for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
		if (kAtlantisExeBuilds[i].size == size)
			return kAtlantisExeBuilds[i];
	return kAtlantisExeBuilds[0];
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D

#endif
