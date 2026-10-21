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

#include "cryomni3d/atlantis/subtitle_layout.h"

#include "common/endian.h"
#include "common/util.h"

namespace CryOmni3D {
namespace Atlantis {

void splitSubtitle(const Common::String &text, Common::Array<Common::String> &lines) {
	lines.clear();
	const Common::String b = " " + text;
	int remaining = (int)b.size();
	if (remaining < 0x65) {
		lines.push_back(b);
		return;
	}
	uint start = 0;
	uint p = 100;
	do {
		if (remaining < 0x65) {
			lines.push_back(b.substr(start));
			break;
		}
		// Back from p to the first space (a CR also stops it; the buffer
		// holds none).
		uint back = 0;
		while (back < 100 && b[p - back] != ' ')
			back++;
		// No space in 100 characters: FUN_0042ffe8 copies nothing and
		// stores a NUL every 0x6e bytes past the line buffer until a write
		// faults; not met by the shipped texts.  Not reproduced:
		// kEnhGameBreakingBugFixes (atlantis-re decisions/0016, row 4).
		if (back == 100)
			back = 0;
		lines.push_back(b.substr(start, 100 - back));
		remaining -= (int)(100 - back);
		start = p - back;
		p = start + 100;
	} while (remaining > 0);
}

int subtitleLineY(uint i, uint n) {
	return 0x1ce - (int)(n - i + 1) * 14;
}

void splitSubtitleBitmap(const Common::String &text, Common::Array<Common::String> &lines, bool backOnly) {
	lines.clear();
	// The buffer at 0x00603d68 (atlantis-cd-en-688128; atlantis-cd-fr-688640
	// FUN_0042eab4, the same code, 0x00603ac0, emulated equal on its 2540
	// French lines): the byte after ']' (a space), the text, then a NUL.
	// The original stores only the NUL (atlantis-cd-en-688128 @
	// 0x00427707..0x00427722, the executor; 0x00425a35..0x00425a50, the
	// game-over text); the bytes after it are zeros (FUN_00422bd0's clear @
	// 0x00422c58) or stale text of earlier, longer lines, never a CR.  So a
	// forward scan that runs past the NUL depends on what was shown before
	// [UNKNOWN] per line (about 32 of the 2540 English lines change rows).
	// The port stops it as if a CR followed the NUL, as the emulation
	// harnesses planted it (atlantis-re tools/emu/video_subtitles_en688128.py,
	// video_subtitles_fr688640.py), and the goldens assume the same: a named
	// difference (atlantis-re experiments 2026-10-18-01 and 2026-10-18-04).
	const Common::String b = " " + text;
	const int len = (int)b.size();
	auto at = [&](int i) -> int {
		if (i < 0)
			return ' ';
		if (i < len)
			return (byte)b[i];
		return i == len ? 0 : '\r';
	};
	// 0x0042e9dd: up to 75 bytes, the whole text (strcpy @ 0x0042eb1f).
	if (len <= 0x4b) {
		lines.push_back(b);
		return;
	}
	int remaining = len, start = 0, p = 0x4b;
	while (remaining > 0) {
		// 0x0042e9fc: 75 bytes or fewer left, the last row (0x0042eadb).
		if (remaining <= 0x4b) {
			lines.push_back(b.substr(start));
			break;
		}
		// 0x0042ea06..0x0042ea21: forward from p to a space; a CR ends the
		// scan one byte further.
		int f = 0;
		while (at(p + f) != ' ') {
			if (at(p + f++) == '\r')
				break;
		}
		// 0x0042ea23..0x0042ea42: back from p, the same way.
		int k = 0;
		while (at(p - k) != ' ') {
			if (at(p - k++) == '\r')
				break;
		}
		int n;
		if (backOnly) {
			// atlantis-cd-sv-688128 FUN_0042ea7c @ 0x0042eb50: always the
			// backward cut, strncpy of 75 - b bytes and a NUL @ 0x0042eb78;
			// the next row starts at the space (0x0042eb86..0x0042eb95).
			n = 0x4b - k;
			if (n <= 0) {
				// No space after the row's first byte within the window:
				// the original copies 0 bytes, keeps the remaining length
				// and loops, writing a NUL at each next row's start (0x55
				// bytes apart).  Under emulation (atlantis-re
				// evidence/golden/video_subtitles_sv688128.txt) it faults.
				// When the row starts at the text's leading space (len 75,
				// 81), the 9th NUL (row 8 = 0x00603f68) erases it, the
				// backward scan runs below the text and strncpy gets a
				// count 75 - b < 0 (fault in the harness's strncpy stub).
				// When the row starts at a later space (len 86), the NULs
				// go on until the write @ 0x0042eb78 faults.  What a real
				// machine does is [UNKNOWN].  Not met by the shipped texts
				// (the 2539 Swedish dialog lines).  The port cuts the run
				// at the window end: kEnhGameBreakingBugFixes (undefined
				// behaviour, decision 0016 R3 and row 26; decision 0023 D2).
				n = 0x4b;
				lines.push_back(b.substr(start, n));
				start = p;
				p = start + 0x4b;
			} else {
				lines.push_back(b.substr(start, n));
				start = p - k;
				p = start + 0x4b;
			}
		} else if (f < k || 0x4b - k <= 0) {
			// 0x0042ea4c: strncpy of 75 + f bytes (it stops at the NUL).
			// The second test is the port's: with b = 75 and f >= 75 the
			// original copies 0 bytes and keeps start and the remaining
			// length, but the row index grows (atlantis-cd-en-688128 @
			// 0x0042eb06), so each pass stores a NUL at 0x00603ac0 + i *
			// 0x55; the ninth pass NULs the text's first byte (0x00603d68)
			// and the backward scan then runs below the buffer, an outcome
			// that depends on memory [UNKNOWN] (emulated on 150 'x': 10
			// rows, all empty).  The shipped texts never reach this case
			// (the longest space-free run of the 2540 dialog lines is 16
			// bytes; atlantis-re experiment 2026-10-18-01).  The port cuts
			// at the forward space (75 + f), the choice decision 0023 D2
			// made for the Swedish breaker.
			n = 0x4b + f;
			lines.push_back(b.substr(start, MIN(n, len - start)));
			start = p + f;
			p += n;
		} else {
			// 0x0042ea90: 75 - b bytes.
			n = 0x4b - k;
			lines.push_back(b.substr(start, n));
			start = p - k;
			p = start + 0x4b;
		}
		remaining -= n;
	}
	// The rows are stored 0x55 bytes apart (atlantis-cd-en-688128 @
	// 0x00603ac0 + i * 0x55, atlantis-cd-fr-688640 @ 0x00603fc0 + i * 0x55)
	// with no bound on their length: a row of 85 bytes or more runs into the
	// next row's buffer, which the next row then overwrites, so it is drawn
	// (up to the NUL) as its first 85 bytes followed by the next row.  The
	// shipped rows are at most 81 bytes long.  Past 8 rows the buffers would
	// overwrite the text itself in atlantis-cd-en-688128 and
	// atlantis-cd-sv-688128 (text 0x2a8 bytes after the first row), or run
	// past the end of the 0x500-byte text buffer in atlantis-cd-fr-688640
	// (rows after the text); at most 5 rows in the data, not reproduced.
	for (int i = (int)lines.size() - 2; i >= 0; i--)
		if (lines[i].size() > 0x54)
			lines[i] = lines[i].substr(0, 0x55) + lines[i + 1];
}

int subtitleLineYBitmap(uint i, uint n) {
	return 0x1ce - (int)(n - i + 1) * kSubtitleBitmapRowStep;
}

void drawSubtitleBitmapRow(byte *pixels, uint bpp, int width, int height, const byte *font, uint32 fontSize,
                           int x, int y, const Common::String &row, uint32 color) {
	const int64 total = (int64)width * height;
	// 0x00410822..0x0041082c: the destination is y * 640 + x pixels.
	int64 base = (int64)y * width + x;
	for (uint c = 0; c < row.size(); c++, base += 8) {
		// 0x00410837: glyph = byte * 64.
		const uint32 glyph = (uint32)(byte)row[c] << 6;
		if (glyph + 64 > fontSize)
			continue;
		for (int gy = 0; gy < 8; gy++) {
			for (int gx = 0; gx < 8; gx++) {
				if (!font[glyph + gy * 8 + gx])
					continue;
				const int64 o = base + (int64)gy * width + gx;
				if (o < 0 || o >= total)
					continue;
				byte *d = pixels + o * bpp;
				if (bpp == 2)
					WRITE_UINT16(d, (uint16)color);
				else if (bpp == 4)
					WRITE_UINT32(d, color);
				else
					*d = (byte)color;
			}
		}
	}
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D
