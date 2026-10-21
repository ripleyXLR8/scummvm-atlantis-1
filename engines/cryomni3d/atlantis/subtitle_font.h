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

#ifndef CRYOMNI3D_ATLANTIS_SUBTITLE_FONT_H
#define CRYOMNI3D_ATLANTIS_SUBTITLE_FONT_H

#include "common/scummsys.h"
#include "common/str-enc.h"

namespace Graphics {
class Font;
}

namespace CryOmni3D {
namespace Atlantis {

// The subtitle font of the Chinese CDs (atlantis-re decision 0025 C).
// atlantis-cd-zh-tw-696832 and atlantis-cd-zh-cn-692736 FUN_0041ad5c @
// 0x0041ad77..0x0041ada0: CreateFontA(16, 0, 0, 0, FW_DONTCARE, 0, 0, 0,
// DEFAULT_CHARSET, 0, 0, 0, DEFAULT_QUALITY, "MingLi") (face 0x004ac2e8,
// height 16 from WinMain @ 0x00418f89), the handle in 0x006bd84c; the
// subtitle rows are written by FUN_0041ae20 with TextOutA of the CON bytes
// (@ 0x0041aedd, SetBkMode TRANSPARENT @ 0x0041aebb; no SetTextAlign is
// imported, so TA_TOP: the cell's top at the row's y), read in the system
// code page.  No Windows has a face "MingLi": a Traditional Chinese Windows
// (code page 950) gives the charset's default face, MingLiU (mingliu.ttc),
// a Simplified one (936) SimSun (simsun.ttc) [INFERRED]; the size it
// renders is [UNKNOWN], 16 pixels per em until a run on a Chinese Windows
// settles it (decision 0025 C3).
//
// The port looks for, in order:
//  1. the player's own font beside the game files (decision 0025 C1), its
//     face of that family in the collection (sfntFaceIndex);
//  2. Noto Serif TC / SC Regular (SIL OFL 1.1, the Ming / Song style of
//     MingLiU / SimSun, the owner's fallback of decision 0025 C2), a file
//     the player adds beside the game files (ScummVM does not ship it), or
//     from the fonts archives should they ever hold it;
//  3. Noto Sans TC / SC Regular from fonts-cjk.dat (ScummVM's CJK fonts,
//     shipped where engine_data_big.mk is used).
// All at 16 pixels per em, monochrome.
enum CjkSubtitleFontSource {
	kCjkSubtitleFontNone = 0,
	kCjkSubtitleFontPlayer,     // mingliu.ttc / simsun.ttc
	kCjkSubtitleFontSerif,      // Noto Serif TC / SC
	kCjkSubtitleFontSans        // Noto Sans TC / SC (fonts-cjk.dat)
};

struct CjkSubtitleFaces {
	const char *playerFile;     // beside the game files
	const char *playerFamily;   // its face in the collection
	const char *serifFiles[2];  // Noto Serif, OpenType / TrueType outlines
	const char *sansFile;       // in fonts-cjk.dat
};

// The faces of a double-byte code page (kWindows950, kWindows936), null
// for every other.
const CjkSubtitleFaces *cjkSubtitleFaces(Common::CodePage codePage);

// 16 pixels per em (decision 0025 C3).
static const int kCjkSubtitleEmPx = 16;
// The ideographs' em box top under the row's y, as the baseline's offset:
// MingLiU's and SimSun's em is their cell [INFERRED], so with TA_TOP they
// fill y .. y + 15; the Noto CJK faces' ideographs fill the em box from
// -120 to 880 of 1000 units (OS/2 sTypoDescender / sTypoAscender), so
// their baseline goes 880 x 16 / 1000 = 14 pixels under y.  The player's
// own face keeps its ascent (FreeType's, from its own metrics, as the
// port's Arial).
static const int kCjkSubtitleNotoAscent = 14;

// The face of the font file or TrueType collection `data` whose family
// (name ID 1, in any language, as CreateFontA matches a face name; ASCII
// case ignored) is `family`, or -1.
int sfntFaceIndex(const byte *data, uint32 size, const char *family);
// Face `index` of `data` as a font of its own (its table directory and
// tables), malloc'ed, its size in `outSize`; null if `data` is not a font
// or has no such face.  (Graphics::loadTTFFont reads the first face;
// Graphics::findTTFace, which picks a face, leaves the FT_StreamRec_ it
// passes to FT_Open_Face uninitialised, whose close callback
// FT_Done_Face then calls.)
byte *sfntFace(const byte *data, uint32 size, uint index, uint32 &outSize);

// The subtitle font of `codePage` (cjkSubtitleFaces) from the first of the
// sources above that loads, its source in `source`; null (source
// kCjkSubtitleFontNone) without FreeType or when none is found.
Graphics::Font *loadCjkSubtitleFont(Common::CodePage codePage, CjkSubtitleFontSource &source);

// The y offset drawSubtitleRows adds to a row's y for `font` of `source`:
// 0 for the player's face, kCjkSubtitleNotoAscent less the font's ascent
// for the Noto faces.
int cjkSubtitleYShift(const Graphics::Font &font, CjkSubtitleFontSource source);

} // End of namespace Atlantis
} // End of namespace CryOmni3D

#endif
