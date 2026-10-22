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

#include "cryomni3d/atlantis/exe_build.h"

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
// y = 0x1ce - (n - i + 1) * 14 (white, black shadow at +1, +1); `step` 18
// in the Chinese builds (exe_build.h subtitleRowStep).
static const int kSubtitleX = 5;
int subtitleLineY(uint i, uint n, int step = 14);

// The subtitle text buffer of the bitmap-subtitle builds of 1997
// (exe_build.h bitmapSubtitles): atlantis-cd-en-688128 0x00603d68,
// atlantis-cd-fr-688640 0x00603ac0, atlantis-cd-nl-688128 and
// atlantis-cd-sv-688128 0x00603f68, 0x400 bytes in each (the object at
// 0x00604168 / 0x00603ec0 / 0x00604368 follows it).  It is zeroed once, at
// start-up (FUN_00422bd0 @ 0x00422c58, called from the WinMain start-up code
// @ 0x00419499), and written only by the section executor for every `[`
// line it runs (FUN_00427420 @ 0x00427707..0x00427722) and by the game-over
// screen for its narrator line (FUN_004256a4 @ 0x00425a31..0x00425a50):
// the bytes up to the line's CR, then a NUL, the rest left as it was.  So
// after the NUL it holds zeros, or the tail of an earlier, longer text
// (atlantis-re experiment 2026-10-21-01, decision 0016 row 34).
class SubtitleTextBuffer {
public:
	static const uint kSize = 0x400;

	SubtitleTextBuffer() { clear(); }

	// FUN_00422bd0's memset.
	void clear();
	// A copy loop: `bytes`, then a NUL.  A text of 0x400 bytes or more would
	// run on into the object after the buffer; none is shipped (the longest
	// dialog line has 435 bytes, atlantis-cd-fr-688640), the port cuts it.
	void store(const Common::String &bytes);
	// The bytes up to the NUL.
	Common::String text() const;
	byte at(uint i) const { return i < kSize ? _buf[i] : 0; }

private:
	byte _buf[kSize];
};

// The English retail build of 1997 (atlantis-cd-en-688128, exe_build.h
// bitmapSubtitles; atlantis-re experiment 2026-10-18-01 item 1, decision
// 0018; also the French build of May 1997, atlantis-cd-fr-688640
// FUN_0042eab4 / FUN_0042ec80, experiment 2026-10-18-04, decision 0020, and
// the Dutch build's FUN_0042e990, decision 0022).  FUN_0042e970 breaks the
// text buffer (" " + text, the byte after ']' first): up to 75 bytes, one
// row; otherwise, from the end p of a 75-byte window, it scans forward (f)
// and back (b) for a space (a CR stops the scan one byte further) and
// breaks at the nearer one, the forward one when f < b: the row is the 75
// + f or 75 - b bytes up to the space, the space starts the next row; the
// last row (75 bytes or fewer left) is the rest.  The forward scan does not
// stop at the NUL: when the text's last word crosses the window end it
// reads the buffer's tail, zeros (then b wins) or the tail of an earlier,
// longer text, whose spaces can make the forward cut win (the rows then
// depend on what was shown before, as in the original: about 32 of the
// 2540 English lines change rows between a zero tail and the planted CR the
// harnesses assumed before 2026-10-21).  Rows of 85 bytes or more overrun
// their buffer (see the code).
// backOnly: the Swedish build's FUN_0042ea7c (atlantis-cd-sv-688128,
// exe_build.h subtitleBreakBackOnly, decision 0023 D): every row ends at
// the last space at or before the window end (75 - b bytes), so no row is
// longer than 75 bytes; a window without a space after the row's first
// byte (the original faults there) is cut at 75 bytes (decision 0023 D2).
// The forward scan still runs there, its count unused: the tail does not
// change the rows.
void splitSubtitleBitmap(const SubtitleTextBuffer &buf, Common::Array<Common::String> &lines,
                         bool backOnly = false);
// The same on the buffer as the first line after start-up leaves it: " " +
// text, a NUL, zeros.
void splitSubtitleBitmap(const Common::String &text, Common::Array<Common::String> &lines,
                         bool backOnly = false);

// The Chinese CDs' breaker FUN_0042f424 (atlantis-cd-zh-tw-696832, Big5
// marks; atlantis-cd-zh-cn-692736, GB2312 marks: exe_build.h cjkBreaker;
// atlantis-re experiment 2026-10-19-01, decision 0025 D) on the subtitle
// text buffer 0x00603cc0, `buf` = its bytes up to the NUL: the byte after
// the line's ']' (a space), the text and its final blanks, as the dialog
// code (FUN_00427cc0 @ 0x00427ff9..0x00428014) and the game over
// (FUN_00425e80 @ 0x004262d4..0x004262ef) copy it up to the CR in both
// builds.  Up to 75 bytes the buffer is one row, its space included;
// otherwise each 75-byte window is cut backwards after a double-byte
// punctuation mark (see the code for the original's arithmetic, kept as it
// is, and zh-cn's row started one byte out of step after an ideographic
// space).  minorBugFixes (kEnhMinorBugFixes, decision 0025 D2 and decision
// 0016 row 39): zh-cn's row after an ideographic space starts past the whole
// space, and zh-cn reads its marks at character boundaries only, so every
// row decodes.  A window with no
// mark within 75 bytes is cut at the last character boundary (decision 0025
// D3, a deviation: the original's scan runs on below the buffer).
void splitSubtitleCjk(const Common::String &buf, Common::Array<Common::String> &lines, CjkBreaker breaker,
                      bool minorBugFixes);

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
