#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_SUBTITLES_EN1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_SUBTITLES_EN1997_H

// Verifier "video": the subtitles of the English retail build of 1997.
//
// Original code (atlantis-cd-en-688128, atlantis-re experiment
// 2026-10-18-01 item 1, decision 0018):
//   FUN_0042e970  row breaker (75-byte window, nearer space forward or back)
//   FUN_0042eb3c  row drawer (10 px rows, shadow at (6, y + 1), white at (5, y))
//   FUN_00410800  8x8 glyphs of DIALOG\FONTS.BIN, no clipping
// Harness: atlantis-re/tools/emu/video_subtitles_en688128.py, golden
// evidence/golden/video_subtitles_en688128.txt / .png (the original code run
// under Unicorn on synthetic texts, on the 2540 dialog lines of the CON
// files, and one subtitle rendered on a grey screen).  The harness writes
// " " + text + NUL + CR; the original stores only the NUL, the bytes after it
// being zeros or stale text (subtitle_layout.cpp), so these goldens hold the
// rows for that CR input, the port's model, not the history-dependent rows
// of a played game (Fable pass 2 opus-gap3-builds-1997-en-fr#0).
//
// Port code exercised directly: splitSubtitleBitmap, subtitleLineYBitmap,
// drawSubtitleBitmapRow (subtitle_layout.cpp).

#include "atlantis_test_helper.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/array.h"
#include "common/fs.h"
#include "common/ptr.h"
#include "graphics/surface.h"
#include "image/png.h"

#include "cryomni3d/atlantis/subtitle_layout.h"

namespace VideoSubEn1997 {

// The inputs of the harness's SAMPLES, in its order (the golden gives the
// rows the original made of them).
inline Common::Array<Common::String> samples() {
	Common::Array<Common::String> s;
	s.push_back("Short line.");
	Common::String t;
	for (int i = 0; i < 2; i++)
		t += Common::String((size_t)49, 'y') + " ";
	s.push_back(t);
	s.push_back(Common::String((size_t)74, 'x'));
	s.push_back(Common::String((size_t)75, 'x'));
	s.push_back(Common::String((size_t)76, 'x') + " tail");
	t.clear();
	for (int i = 0; i < 3; i++)
		t += Common::String((size_t)60, 'x') + " ";
	s.push_back(t);
	t.clear();
	for (int i = 0; i < 30; i++)
		t += "word ";
	t.trim();
	s.push_back(t);
	t.clear();
	for (int i = 0; i < 25; i++)
		t += "abcdefghi ";
	t.trim();
	s.push_back(t);
	s.push_back("The quick brown fox jumps over the lazy dog, then the lazy dog wakes up and chases the fox "
	            "around the field.");
	return s;
}

// More texts broken by the original FUN_0042e970 under Unicorn (the same
// harness's split(), 2026-10-18): rows of 85 bytes or more running into
// the next row's buffer, a 76-byte text kept whole, a leading space.
struct Case { const char *input; int n; const char *rows[3]; };
static const Case kCases[] = {
	{ "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyy to ssssssssssssssssssss zzzzzzzzzzzzzzzzzzzzzzzzzzzzzz q end end end end end end end end end end end end end end end end end end end end", 3, { " yyyyyyyyyyyyyyyyyyyyyyyyyyyyyy to ssssssssssssssssssss zzzzzzzzzzzzzzzzzzzzzzzzzzzzz q end end end end end end end end end end end end end end end end end end", " q end end end end end end end end end end end end end end end end end end", " end end" } },
	{ "blood blood yyyyyyyyyyyyyyyyyyyyyyyyyyyyyy xxxxxxxxxxxx yyyyyyyyyyyyyyyyyyyyyyyyyyyyyy the xxxxxxxxxxxx ", 2, { " blood blood yyyyyyyyyyyyyyyyyyyyyyyyyyyyyy xxxxxxxxxxxx yyyyyyyyyyyyyyyyyyyyyyyyyyyy the xxxxxxxxxxxx ", " the xxxxxxxxxxxx " } },
	{ "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa bbb", 2, { " aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", " bbb" } },
	{ "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 1, { " aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" } },
	{ "x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x ", 2, { " x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x", " x x x x x x x x x x x x x x x x x x x x x x x " } },
	{ " lead w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w", 2, { "  lead w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w w", " w w w w w w w w w w w w w w w w" } },
	{ "one two three four five six seven eight nine ten eleven twelve thirteen fourteen", 2, { " one two three four five six seven eight nine ten eleven twelve thirteen", " fourteen" } },
};

// "len  11 -> 1 lines: 12:' Short line.'" -> the rows.
inline bool parseGoldenRows(const Common::String &line, Common::Array<Common::String> &rows) {
	rows.clear();
	const size_t colon = line.find("lines: ");
	if (colon == Common::String::npos)
		return false;
	Common::String rest = line.substr(colon + 7);
	while (!rest.empty()) {
		const size_t q1 = rest.find('\'');
		if (q1 == Common::String::npos)
			return false;
		// a row ends at the quote followed by " | " or the end of the line
		size_t q2 = rest.find("' | ", q1 + 1);
		const bool last = q2 == Common::String::npos;
		if (last)
			q2 = rest.size() - 1;
		rows.push_back(rest.substr(q1 + 1, q2 - q1 - 1));
		if (last)
			break;
		rest = rest.substr(q2 + 4);
	}
	return true;
}

// Dialog lines of a CON file as the harness takes them: "[...]" then a
// non-blank text; the text without its first byte when it is a space.
inline void conDialogTexts(const byte *b, uint32 size, Common::Array<Common::String> &out) {
	uint32 i = 0;
	while (i < size) {
		uint32 j = i;
		while (j < size && !(b[j] == '\r' && j + 1 < size && b[j + 1] == '\n'))
			j++;
		Common::String line((const char *)b + i, j - i);
		i = j + 2;
		if (line.empty() || line[0] != '[')
			continue;
		const size_t close = line.find(']');
		if (close == Common::String::npos)
			continue;
		Common::String text = line.substr(close + 1);
		Common::String stripped = text;
		stripped.trim();
		if (stripped.empty())
			continue;
		if (text[0] == ' ')
			text = text.substr(1);
		out.push_back(text);
	}
}

} // End of namespace VideoSubEn1997

class VideoSubtitlesEn1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// The harness's samples: the port's rows are the ones the golden lists.
	void test_rows_vs_golden() {
		if (!AtlantisTest::haveData()) { TS_WARN("data not configured; skipped"); return; }
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_en688128.txt", lines));
		const Common::Array<Common::String> in = VideoSubEn1997::samples();
		uint k = 0;
		for (uint i = 0; i < lines.size(); i++) {
			if (!lines[i].hasPrefix("len "))
				continue;
			TS_ASSERT(k < in.size());
			if (k >= in.size())
				break;
			TS_ASSERT_EQUALS(atoi(lines[i].c_str() + 4), (int)in[k].size());
			Common::Array<Common::String> want, got;
			TS_ASSERT(VideoSubEn1997::parseGoldenRows(lines[i], want));
			CryOmni3D::Atlantis::splitSubtitleBitmap(in[k], got);
			TSM_ASSERT_EQUALS(in[k].c_str(), got.size(), want.size());
			for (uint r = 0; r < got.size() && r < want.size(); r++)
				TSM_ASSERT_EQUALS(in[k].c_str(), got[r], want[r]);
			k++;
		}
		TS_ASSERT_EQUALS(k, in.size());
	}

	// More emulated texts (kCases), the buffer overrun included.
	void test_rows_vs_original() {
		for (uint i = 0; i < ARRAYSIZE(VideoSubEn1997::kCases); i++) {
			const VideoSubEn1997::Case &c = VideoSubEn1997::kCases[i];
			Common::Array<Common::String> got;
			CryOmni3D::Atlantis::splitSubtitleBitmap(c.input, got);
			TSM_ASSERT_EQUALS(i, (int)got.size(), c.n);
			for (int r = 0; r < c.n && r < (int)got.size(); r++)
				TSM_ASSERT_EQUALS(i, got[r], Common::String(c.rows[r]));
		}
	}

	// The scan of the shipped dialog lines in the golden ({'lines': 2540,
	// 'max_rows': 5, 'rows_hist': {...}, 'max_row_len': 81, 'rows_over_79':
	// 3, ...}): the port's rows give the same statistics.
	void test_con_scan_vs_golden() {
		if (!AtlantisTest::haveData()) { TS_WARN("data not configured; skipped"); return; }
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_en688128.txt", lines));
		Common::String scan;
		for (uint i = 0; i < lines.size(); i++)
			if (lines[i].hasPrefix("# scan of every dialog line"))
				scan = lines[i];
		TS_ASSERT(!scan.empty());
		auto field = [&](const char *key) {
			const size_t p = scan.find(Common::String::format("'%s': ", key));
			return p == Common::String::npos ? -1 : atoi(scan.c_str() + p + strlen(key) + 4);
		};
		VideoTest::Big big;
		TS_ASSERT(!big.empty());
		int count = 0, maxRows = 0, maxLen = 0, over79 = 0, hist[9] = { 0 };
		const Common::Array<Common::String> cons = big.names(".CON");
		for (uint f = 0; f < cons.size(); f++) {
			uint32 size = 0;
			byte *b = big.get(cons[f], size);
			if (!b)
				continue;
			Common::Array<Common::String> texts;
			VideoSubEn1997::conDialogTexts(b, size, texts);
			delete[] b;
			for (uint t = 0; t < texts.size(); t++) {
				Common::Array<Common::String> rows;
				CryOmni3D::Atlantis::splitSubtitleBitmap(texts[t], rows);
				count++;
				maxRows = MAX(maxRows, (int)rows.size());
				if (rows.size() < ARRAYSIZE(hist))
					hist[rows.size()]++;
				for (uint r = 0; r < rows.size(); r++) {
					maxLen = MAX(maxLen, (int)rows[r].size());
					over79 += rows[r].size() > 79;
				}
			}
		}
		TS_ASSERT_EQUALS(count, field("lines"));
		TS_ASSERT_EQUALS(maxRows, field("max_rows"));
		TS_ASSERT_EQUALS(maxLen, field("max_row_len"));
		TS_ASSERT_EQUALS(over79, field("rows_over_79"));
		// rows_hist: {2: 513, 1: 1947, ...}
		const size_t h = scan.find("'rows_hist': {");
		TS_ASSERT(h != Common::String::npos);
		const char *p = scan.c_str() + h + 14;
		int entries = 0;
		while (*p && *p != '}') {
			const int n = atoi(p);
			const char *c = strchr(p, ':');
			if (!c)
				break;
			const int v = atoi(c + 1);
			TSM_ASSERT_EQUALS(n, n >= 0 && n < (int)ARRAYSIZE(hist) ? hist[n] : -1, v);
			entries++;
			p = strchr(c, ',');
			if (!p || p > strchr(c, '}'))
				break;
			p++;
		}
		TS_ASSERT(entries >= 5);
	}

	// FUN_0042eb3c's layout: rows 10 px apart, the last one at 0x1ce - 20.
	void test_row_y() {
		using CryOmni3D::Atlantis::subtitleLineYBitmap;
		TS_ASSERT_EQUALS(subtitleLineYBitmap(0, 1), 0x1ce - 2 * 10);
		TS_ASSERT_EQUALS(subtitleLineYBitmap(1, 2) - subtitleLineYBitmap(0, 2), 10);
		TS_ASSERT_EQUALS(subtitleLineYBitmap(4, 5), subtitleLineYBitmap(0, 1));
	}

	// No clipping: a glyph past x = 639 lands on the next pixel row at the
	// left (FUN_00410800 writes at y * 640 + x).
	void test_draw_wraps_past_right_edge() {
		byte font[16384];
		memset(font, 0, sizeof(font));
		font['A' * 64 + 7] = 1;   // glyph 'A': only its top-right pixel
		uint16 buf[640 * 4];
		memset(buf, 0, sizeof(buf));
		CryOmni3D::Atlantis::drawSubtitleBitmapRow((byte *)buf, 2, 640, 4, font, sizeof(font), 636, 1, "A", 0xffff);
		// x 636 + 7 = 643: row 2, x 3
		TS_ASSERT_EQUALS(buf[2 * 640 + 3], 0xffff);
		int lit = 0;
		for (uint i = 0; i < ARRAYSIZE(buf); i++)
			lit += buf[i] != 0;
		TS_ASSERT_EQUALS(lit, 1);
	}

	// The golden PNG: the harness's last sample drawn by the original on a
	// 640 x 480 RGB565 screen of 0x8410, rows 400..479.  The port draws the
	// same rows with FONTS.BIN: every pixel equal.
	void test_pixels_vs_golden_png() {
		if (!AtlantisTest::haveData()) { TS_WARN("data not configured; skipped"); return; }
		VideoTest::Big big;
		uint32 fontSize = 0;
		byte *raw = big.get("DIALOG\\FONTS.BIN", fontSize);
		TS_ASSERT(raw && fontSize == 16384);
		if (!raw)
			return;
		Common::Array<byte> font(raw, fontSize);
		delete[] raw;
		Common::Array<uint16> screen(640 * 480, 0x8410);
		Common::Array<Common::String> rows;
		CryOmni3D::Atlantis::splitSubtitleBitmap(VideoSubEn1997::samples().back(), rows);
		TS_ASSERT_EQUALS(rows.size(), 2u);
		for (uint i = 0; i < rows.size(); i++) {
			const int y = CryOmni3D::Atlantis::subtitleLineYBitmap(i, rows.size());
			CryOmni3D::Atlantis::drawSubtitleBitmapRow((byte *)screen.data(), 2, 640, 480, font.data(), fontSize,
			                                           CryOmni3D::Atlantis::kSubtitleX + 1, y + 1, rows[i], 0);
			CryOmni3D::Atlantis::drawSubtitleBitmapRow((byte *)screen.data(), 2, 640, 480, font.data(), fontSize,
			                                           CryOmni3D::Atlantis::kSubtitleX, y, rows[i], 0xffff);
		}
		Common::File f;
		TS_ASSERT(f.open(Common::FSNode(Common::Path(AtlantisTest::goldenDir(), '/')
		                                .appendComponent("video_subtitles_en688128.png"))));
		Image::PNGDecoder png;
		TS_ASSERT(png.loadStream(f));
		const Graphics::Surface *s = png.getSurface();
		TS_ASSERT(s && s->w == 640 && s->h == 80);
		if (!s || s->w != 640 || s->h != 80)
			return;
		int diffs = 0, white = 0;
		for (int y = 0; y < 80; y++) {
			for (int x = 0; x < 640; x++) {
				const uint16 v = screen[(400 + y) * 640 + x];
				// the harness's RGB565 -> RGB888 (no low-bit fill)
				const uint8 r = (v >> 11) << 3, g = ((v >> 5) & 0x3f) << 2, b = (v & 0x1f) << 3;
				uint8 pr, pg, pb;
				s->format.colorToRGB(s->getPixel(x, y), pr, pg, pb);
				diffs += (r != pr || g != pg || b != pb);
				white += v == 0xffff;
			}
		}
		TS_ASSERT_EQUALS(diffs, 0);
		TS_ASSERT(white > 100);
	}
};

#endif
