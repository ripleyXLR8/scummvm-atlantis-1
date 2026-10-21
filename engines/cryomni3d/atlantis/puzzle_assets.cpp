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

#include "cryomni3d/atlantis/puzzle_assets.h"

#include "cryomni3d/atlantis/engine.h"
#include "cryomni3d/atlantis/sprite_decode.h"

#include "common/debug.h"
#include "common/ptr.h"
#include "common/stream.h"
#include "common/system.h"
#include "graphics/managed_surface.h"
#include "graphics/pixelformat.h"

namespace CryOmni3D {
namespace Atlantis {

// The puzzles draw their SPR frames with the blitter of every other SPR,
// FUN_00410430 / FUN_0041047f, which writes colortable[index] unchanged and
// takes transparency from the run type: no colour is special.  The frames
// are decoded by readSprFile / decodeSprRows (transparency kept in the blend
// byte) and drawn by drawSprSprite.  This file used to keep its own decoder
// with RGB565 0x0001 as the transparency marker and drew a real 0x0001
// pixel as 0x0000 (CRABE, PUZHEXA, PUZMURIA, PUZPOUL0 and SERPNEW use it).
bool loadPuzzleSpr(Common::SeekableReadStream &stream, Common::Array<PuzzleSpr> &out) {
	// readSprFile refuses a count of 0 or above 256; FUN_00447398 reads the
	// file without a check.  The largest puzzle bank, PUZPOUL1.SPR, has 248.
	return readSprFile(stream, out);
}

bool loadPuzzleSpr(CryOmni3DEngine_Atlantis *engine, const char *fname,
                   Common::Array<PuzzleSpr> &out) {
	Common::ScopedPtr<Common::SeekableReadStream> stream(
	    engine->openBigFileStream(kFileTypePuzzles, fname));
	if (!stream) {
		warning("loadPuzzleSpr: cannot open PUZZLES\\%s", fname);
		return false;
	}
	if (!loadPuzzleSpr(*stream, out))
		return false;
	debugC(1, kDebugScript, "loadPuzzleSpr: loaded %u frames from PUZZLES\\%s", out.size(), fname);
	return true;
}

void blitFrame(Graphics::ManagedSurface &out, const PuzzleSpr &spr, int cx, int cy) {
	if (out.format.bytesPerPixel != 2) return;
	drawSprSprite(*out.surfacePtr(), spr, cx, cy);
}

bool loadPuzzleTGA(CryOmni3DEngine_Atlantis *engine, const char *fname,
                   Common::Array<uint16> &outPixels) {
	Common::ScopedPtr<Common::SeekableReadStream> stream(
	    engine->openBigFileStream(kFileTypePuzzles, fname));
	if (!stream) return false;
	byte idLen = stream->readByte();
	/* cmType = */ stream->readByte();
	byte imgType = stream->readByte();
	stream->skip(9);
	uint16 w = stream->readUint16LE();
	uint16 h = stream->readUint16LE();
	byte depth = stream->readByte();
	stream->readByte();
	stream->skip(idLen);
	// Puzzle backgrounds are full-screen 15-bit uncompressed TGAs.
	if (imgType != 2 || depth != 15 || w != 640 || h != 480) return false;
	const Graphics::PixelFormat fmt = g_system->getScreenFormat();
	outPixels.resize((uint)w * h);
	for (uint16 y = 0; y < h; y++) {
		uint16 *row = outPixels.begin() + (uint)(h - 1 - y) * w;
		for (uint16 x = 0; x < w; x++) {
			uint16 px = stream->readUint16LE();
			uint8 r = (uint8)(((px >> 10) & 0x1f) << 3);
			uint8 g = (uint8)(((px >>  5) & 0x1f) << 3);
			uint8 b = (uint8)(( px        & 0x1f) << 3);
			row[x] = (uint16)fmt.RGBToColor(r, g, b);
		}
	}
	return true;
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D
