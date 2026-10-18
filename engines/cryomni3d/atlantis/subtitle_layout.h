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

#ifndef CRYOMNI3D_ATLANTIS_SUBTITLE_LAYOUT_H
#define CRYOMNI3D_ATLANTIS_SUBTITLE_LAYOUT_H

#include "common/array.h"
#include "common/str.h"

namespace CryOmni3D {
namespace Atlantis {

// atlantis.exe FUN_0042ffe8 on the subtitle buffer FUN_0042844c fills: a space
// (the byte after ']') then the text.  Under 101 bytes it is one line;
// otherwise each line ends before the last space within the next 100 bytes
// and the following line starts at that space, until the remaining length
// is 0.  A window with no space makes the original loop forever; the port
// cuts it at 100 bytes.
void splitSubtitle(const Common::String &text, Common::Array<Common::String> &lines);

// atlantis.exe FUN_00430178: line i of n is drawn at x = 5,
// y = 0x1ce - (n - i + 1) * 14 (white, black shadow at +1, +1).
static const int kSubtitleX = 5;
int subtitleLineY(uint i, uint n);

// The English retail build of 1997 (atlantis-cd-en-688128, exe_build.h
// bitmapSubtitles; atlantis-re experiment 2026-10-18-01 item 1, decision
// 0018).  FUN_0042e970 breaks the buffer " " + text: up to 75 bytes, one
// row; otherwise, from the end p of a 75-byte window, it scans forward (f)
// and back (b) for a space (a CR stops the scan one byte further) and
// breaks at the nearer one, the forward one when f < b: the row is the 75
// + f or 75 - b bytes up to the space, the space starts the next row; the
// last row (75 bytes or fewer left) is the rest.  The buffer ends with a
// NUL then a CR, as the emulation of the original saw it (atlantis-re
// tools/emu/video_subtitles_en688128.py), which ends the forward scan at
// the text's end.  Rows of 85 bytes or more overrun their buffer (see the
// code).
void splitSubtitleBitmap(const Common::String &text, Common::Array<Common::String> &lines);

// FUN_0042eb3c: row i of n at y = 0x1ce - (n - i + 1) * 10, drawn with
// FUN_00410800 at (6, y + 1) in black, then at (5, y) in white.
static const int kSubtitleBitmapRowStep = 10;
int subtitleLineYBitmap(uint i, uint n);

// FUN_00410800 on a surface `width` pixels wide and `height` high, pixels
// of `bpp` bytes with no padding: the glyph of each byte of `row` is the
// 8 x 8 bytes at byte * 64 of DIALOG\FONTS.BIN (`font`, 16384 bytes), a
// non-zero byte a pixel in `color`; the pen advances 8 pixels.  There is no
// clipping: a pixel past the right edge lands on the next pixel row at the
// left (the offset is y * width + x).  Pixels outside the buffer are not
// written (the original writes them; the shipped texts never reach them).
void drawSubtitleBitmapRow(byte *pixels, uint bpp, int width, int height, const byte *font, uint32 fontSize,
                           int x, int y, const Common::String &row, uint32 color);

} // End of namespace Atlantis
} // End of namespace CryOmni3D

#endif
