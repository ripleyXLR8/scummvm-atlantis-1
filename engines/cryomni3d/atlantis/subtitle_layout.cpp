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

int subtitleLineY(uint i, uint n, int step) {
	return 0x1ce - (int)(n - i + 1) * step;
}

void SubtitleTextBuffer::clear() {
	memset(_buf, 0, sizeof(_buf));
}

void SubtitleTextBuffer::store(const Common::String &bytes) {
	const uint n = MIN<uint>(bytes.size(), kSize - 1);
	memcpy(_buf, bytes.c_str(), n);
	_buf[n] = 0;
}

Common::String SubtitleTextBuffer::text() const {
	uint n = 0;
	while (n < kSize && _buf[n])
		n++;
	return Common::String((const char *)_buf, n);
}

void splitSubtitleBitmap(const Common::String &text, Common::Array<Common::String> &lines, bool backOnly) {
	SubtitleTextBuffer buf;
	buf.store(" " + text);
	splitSubtitleBitmap(buf, lines, backOnly);
}

void splitSubtitleBitmap(const SubtitleTextBuffer &buf, Common::Array<Common::String> &lines, bool backOnly) {
	lines.clear();
	// The text buffer (atlantis-cd-en-688128 0x00603d68; atlantis-cd-fr-688640
	// FUN_0042eab4, the same code, 0x00603ac0; atlantis-cd-nl-688128
	// FUN_0042e990, 0x00603f68): strlen @ 0x0042e9ce gives the length, and
	// the scans read the buffer itself, the bytes after the NUL included
	// (the zeros of the start-up clear or the tail of an earlier, longer
	// text: SubtitleTextBuffer).  A forward scan that leaves the 0x400-byte
	// buffer reads the object after it [UNKNOWN]; its count is then at least
	// 0x400 - p, more than 75 (so more than any b) for every text shorter than
	// 0x3b5 bytes, the backward cut whatever that memory holds (atlantis-re
	// tools/emu/video_subtitle_buffer_1997.py checks it with a space right
	// after the buffer).  The port stops the scan there and counts it as not
	// found; a text of 0x3b5 bytes or more is not shipped.
	const Common::String b = buf.text();
	const int len = (int)b.size();
	const int kNotFound = 0x10000;
	auto at = [&](int i) -> int {
		if (i < 0)
			return ' ';
		return buf.at((uint)i);
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
			if (p + f >= (int)SubtitleTextBuffer::kSize) {
				f = kNotFound;
				break;
			}
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
			// that depends on memory [UNKNOWN] (emulated on 150 'x', and on
			// 75 'x' with a zero tail: 10 rows, all empty).  The shipped
			// texts never reach this case (the longest space-free run of
			// the 2540 dialog lines is 16 bytes; atlantis-re experiment
			// 2026-10-18-01).  The port cuts at the forward space (75 + f;
			// with none in the buffer, the rest of the text), the choice
			// decision 0023 D2 made for the Swedish breaker.
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
	// overwrite the text itself in atlantis-cd-en-688128,
	// atlantis-cd-nl-688128 and atlantis-cd-sv-688128 (text 0x2a8 bytes
	// after the first row), or the table at 0x00604268 in
	// atlantis-cd-fr-688640 (rows after the 0x400-byte text and the object
	// at 0x00603ec0); at most 6 rows in the data, not reproduced.
	for (int i = (int)lines.size() - 2; i >= 0; i--)
		if (lines[i].size() > 0x54)
			lines[i] = lines[i].substr(0, 0x55) + lines[i + 1];
}

// FUN_0042f424's cut marks: whether the scan stops at the pair (lead,
// trail) (atlantis-cd-zh-tw-696832 @ 0x0042f510..0x0042f574,
// atlantis-cd-zh-cn-692736 @ 0x0042f510..0x0042f568).
static bool cjkCutAt(CjkBreaker breaker, byte lead, byte trail) {
	// A CR as the lead byte, in both (@ 0x0042f56c / 0x0042f55c).
	if (lead == '\r')
		return true;
	if (breaker == kCjkBreakerBig5)
		// Big5 A1 41 `，`, A1 43 `。`, A1 46 `；`, A1 48 `？`, A1 49 `！`,
		// A1 4B `…`.
		return lead == 0xa1 && (trail == 0x41 || trail == 0x43 || trail == 0x46 || trail == 0x48 ||
		                        trail == 0x49 || trail == 0x4b);
	// GB2312 A1 A1 (the ideographic space), A3 AC `，`, A1 A3 `。`, A3 BF `？`,
	// A3 A1 `！`.
	return (lead == 0xa1 && trail == 0xa1) || (lead == 0xa3 && trail == 0xac) ||
	       (lead == 0xa1 && trail == 0xa3) || (lead == 0xa3 && trail == 0xbf) ||
	       (lead == 0xa3 && trail == 0xa1);
}

void splitSubtitleCjk(const Common::String &buf, Common::Array<Common::String> &lines, CjkBreaker breaker,
                      bool minorBugFixes) {
	lines.clear();
	// The locals of FUN_0042f424 (atlantis-cd-zh-tw-696832; zh-cn has the
	// same frame): left [ebp-0x30] = strlen of the buffer, the space
	// included (@ 0x0042f47d), less each row's share; s [ebp-0x14], where the
	// row starts, first 0x00603cc1 (the text, without the space); e
	// [ebp-0x18], the scan's window end, first 0x00603d0b (buffer + 75); k
	// [ebp-0x28], how far back from e the scan is.  Indices here are from
	// 0x00603cc0.
	const int kWindow = 0x4b;
	const int len = (int)buf.size();
	// 0x0042f4a7: up to 75 bytes, strcpy of the whole buffer to the first row
	// (@ 0x0042f69a; zh-cn @ 0x0042f674).
	if (len <= kWindow) {
		lines.push_back(buf);
		return;
	}
	// Nothing past the NUL is read: while left is over 75 the window ends at
	// least two bytes before it (left falls by 76 - k a row, the start by 75
	// - k or 76 - k), and the checks below read e - k + 2 at most.
	auto at = [&](int i) -> byte {
		return i >= 0 && i < len ? (byte)buf[i] : 0;
	};
	// strncpy of `n` bytes from `s` (0x0042f635; zh-cn 0x0042f60f): it stops
	// at the NUL; the rows were zeroed (memset of 8 x 0x6e bytes @
	// 0x0042f4b1).
	auto row = [&](int s, int n) -> Common::String {
		s = CLIP(s, 0, len);
		n = CLIP(n, 0, len - s);
		return buf.substr(s, n);
	};
	int left = len, s = 1, e = kWindow;
	for (;;) {
		// 0x0042f4dd: 75 bytes or fewer left, strcpy of the rest from s
		// (@ 0x0042f660; zh-cn @ 0x0042f63a), the last row.
		if (left <= kWindow) {
			lines.push_back(buf.substr(MIN(s, len)));
			break;
		}
		// 0x0042f4f3..0x0042f578: k = 1, 2, ... until the pair (e - k,
		// e - k + 1) is a cut.  There is no lower bound: after a cut the scan
		// finds it again at k = 75 at the latest (a later window without a
		// mark gives rows of 0 or 1 byte and left one less each, until the
		// rest is the last row; past 8 rows the original writes beyond its
		// row buffers, effects [UNKNOWN]; no shipped text does it).
		int k = 1;
		while (k <= kWindow && !cjkCutAt(breaker, at(e - k), at(e - k + 1)))
			k++;
		int count, skip = 2;
		if (k <= kWindow) {
			// 0x0042f57e..0x0042f59c: the checks read c0 = s[75 - k] and c1 =
			// s[74 - k]: the mark's trail and lead bytes in the first row
			// (s = e - 74), but the byte after the mark and its trail byte in
			// the later ones (s = e - 73).  add [ebp-0x20] = 0, skip
			// [ebp-0x1c] = 2 (@ 0x0042f59f; zh-cn @ 0x0042f58b).
			const byte c0 = at(s + kWindow - k), c1 = at(s + kWindow - 1 - k);
			int add = 0;
			if (breaker == kCjkBreakerBig5) {
				// 0x0042f5ad..0x0042f615: add 1 when c0 is 41 43 46 48 49 4B.
				if (c0 == 0x41 || c0 == 0x43 || c0 == 0x46 || c0 == 0x48 || c0 == 0x49 || c0 == 0x4b)
					add = 1;
			} else {
				// zh-cn @ 0x0042f599..0x0042f5d3: add 1 when (c1, c0) is
				// A3 AC, A1 A3, A3 BF or A3 A1.
				if ((c1 == 0xa3 && c0 == 0xac) || (c1 == 0xa1 && c0 == 0xa3) || (c1 == 0xa3 && c0 == 0xbf) ||
				        (c1 == 0xa3 && c0 == 0xa1))
					add = 1;
				// zh-cn @ 0x0042f5da..0x0042f5ef: A1 A1 (the ideographic
				// space), the row ends before it (add -1) and the next starts
				// one byte past its lead byte (skip 1), on its second byte:
				// that row reads its pairs one byte out of step (72 rows of
				// 50 shipped lines, atlantis-re
				// evidence/golden/video_subtitles_zh_cn.txt).  With the minor
				// bug fixes the skip is the other marks' 2 (decision 0025 D2):
				// the 27 rows started on a space mended; the other 45 follow
				// a mark read across two characters (22 cuts: 20 the trail
				// byte of `。` and the lead byte of `　`, A3 A1, read as `！`;
				// 2 A3 BF read as `？`), as the original with the skip patched
				// to 2 (the golden's scan).
				if (c1 == 0xa1 && c0 == 0xa1) {
					add = -1;
					skip = minorBugFixes ? 2 : 1;
				}
			}
			// 0x0042f61c: a count of 75 - k + add (a count of -1, k = 75
			// with A1 A1, would be a strncpy of 4 GB; no text gives one).
			count = kWindow - k + add;
		} else {
			// No cut within 75 bytes: the original's scan goes on below the
			// buffer's start, into what lies before 0x00603cc0; under
			// emulation it never returns, what the game does is [UNKNOWN]
			// (zh-tw: one shipped line, CHAPI084's game-over narration, 80
			// bytes).  The port cuts the row at the last character boundary
			// within 75 bytes (lead bytes 0x81..0xfe in code pages 950 and
			// 936) and goes on as if a two-byte mark ended there (decision
			// 0025 D3, a deviation).
			int c = s;
			while (c < len) {
				const int w = at(c) >= 0x81 && at(c) <= 0xfe ? 2 : 1;
				if (c + w - s > kWindow)
					break;
				c = MIN(c + w, len);
			}
			count = c - s;
			k = e - (c - 2);
		}
		lines.push_back(row(s, count));
		// 0x0042f63a..0x0042f65a (zh-cn 0x0042f614..0x0042f634): left -= 76
		// - k; s = e - k + skip; e = e - k + 75; then 0x0042f68e: on while
		// left is positive.
		left -= kWindow - k + 1;
		s = e - k + skip;
		e = e - k + kWindow;
		// (left falls by at least one a row; the second test only bounds
		// the loop)
		if (left <= 0 || (int)lines.size() > len)
			break;
	}
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
