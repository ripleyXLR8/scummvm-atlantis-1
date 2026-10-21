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
#ifndef CRYOMNI3D_ATLANTIS_LOAD_LIST_H
#define CRYOMNI3D_ATLANTIS_LOAD_LIST_H

#include "common/array.h"
#include "common/scummsys.h"

#include "cryomni3d/atlantis/exe_build.h"

namespace CryOmni3D {
namespace Atlantis {

// The LOAD list of the sprite-row builds (AtlantisExeBuild::loadList
// kLoadListSprites: atlantis-cd-zh-tw-696832 and atlantis-cd-zh-cn-692736,
// the same code at the same addresses).  atlantis-re
// binaries/atlantis-cd-zh-tw-696832.md Z6; the values below are those of
// tools/emu/ui_load_list_zh.py (evidence/golden/ui_load_list_zh_tw.txt,
// ui_load_list_zh_cn.txt).

// One call of the sprite blit FUN_00410430: frame `frame` of bank 0
// (EPICHI1) or 1 (EPICHI2), hotspot at (x, y).  The blit draws nothing for a
// frame past the bank's count (`cmp ax,[esi]; jae` @ 0x00410462).
struct LoadListBlit {
	uint bank;
	int x, y;
	uint frame;
};

// The row drawer FUN_004205a4 (AL = file count, EDX = the hovered row of the
// hit grid, list offset 0x004866e1): min(count, 11) rows from the list
// offset (`cmp byte [ebp-8],0xb; jbe` @ 0x004205d6), row r = entry offset +
// r drawn as that frame (the entry, not its checkpoint), from EPICHI2 when r
// is the hovered grid row (@ 0x0042061a..0x0042062c), else EPICHI1, at
// (0x37, 0x6e + 22 r) (@ 0x004205c8..0x004205cf, 0x004206d5).
inline void loadListRowBlits(uint count, uint offset, int hoverRow,
                             Common::Array<LoadListBlit> &out) {
	out.clear();
	const uint rows = count > 11 ? 11 : count;
	for (uint r = 0; r < rows; r++) {
		LoadListBlit b;
		b.bank = (int)r == hoverRow ? 1 : 0;
		b.x = 0x37;
		b.y = 0x6e + 22 * (int)r;
		b.frame = (offset + r) & 0xffff;
		out.push_back(b);
	}
}

// The chapter a pick of list entry `entry` (grid row + offset) stores in the
// player's chapter byte 0x006be142[player] (`mov al,[eax+0x00486619]` @
// 0x0041d432, over the chapter parsed from the file name @ 0x0041d413) and
// in 0x006bd7dc (@ 0x0041d442..0x0041d44d, which nothing reads): the chapter
// table's byte `entry`.  The width table follows the chapter table (0x00486619
// + 74 = 0x00486663), so an entry past the rows reads a width (entry 74 of
// 75 files and more: 80, emulated); past both tables nothing is known, 0
// here.
inline uint8 loadListChapter(uint entry, const Common::Array<byte> &chapters,
                             const Common::Array<byte> &widths) {
	if (entry < chapters.size())
		return chapters[entry];
	if (entry - chapters.size() < widths.size())
		return widths[entry - chapters.size()];
	return 0;
}

// The chosen row's title: FUN_0041ce00 centres it by the width table, x =
// (0x280 - width) / 2 (@ 0x0041d3d0..0x0041d3ee, after the EPI.TXT width it
// overwrites), and FUN_004208bc mode 1 (EBX = 1 @ 0x0041d469) draws frame
// `entry` of EPICHI2 there at y 0x3c (`mov [ebp-0x14],0x3c` @ 0x00420a22,
// the passed 0x50 unused; the blit @ 0x00420a87).  False past the width
// table (no frame either: the banks have the table's 74 frames).
inline bool loadListTitleBlit(uint entry, const Common::Array<byte> &widths, LoadListBlit &out) {
	if (entry >= widths.size())
		return false;
	out.bank = 1;
	out.x = (0x280 - (int)widths[entry]) / 2;
	out.y = 0x3c;
	out.frame = entry;
	return true;
}

// The CINEMA captions (AtlantisCinemaCaptions) of one presented frame: the
// frame presenter atlantis-cd-zh-tw-696832 FUN_0041afa0 @
// 0x0041b105..0x0041b2bc, on its 16-bit path after the frame's copy to the
// surface.  `t` is the 200 Hz ticks since the video's start (0x006be014 -
// 0x004865a4, the start stored by the video loop @ 0x0041bdc0); `frame` is
// 0x004865a0, kept from frame to frame (0 at the CINEMA= command @
// 0x00429fbe).  Returns whether frame `drawn` is drawn at (c.x, c.y): when
// after < t < before (unsigned, @ 0x0041b11d / 0x0041b12f).  Then each step
// k = 0..9 in turn moves the frame from k to k + 1 when t > steps[k], so a
// late frame catches up in one call (emulated: the first frame at t 9000
// draws 0 and leaves 5).
inline bool cinemaCaptionFrame(const AtlantisCinemaCaptions &c, uint32 t, uint8 &frame, uint &drawn) {
	if (!(t > c.after && t < c.before))
		return false;
	drawn = frame;
	for (uint k = 0; k < ARRAYSIZE(c.steps); k++) {
		if (t > c.steps[k] && frame == k)
			frame++;
	}
	return true;
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D

#endif
