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
// files, and one subtitle rendered on a grey screen), each text as the
// first line after start-up: " " + text, NUL, the zeros of the start-up
// clear (before 2026-10-21 the harness planted a CR after the NUL, which the
// original never stores).  The history of the text buffer, the stale tail an
// earlier, longer text leaves after the NUL: atlantis-re
// tools/emu/video_subtitle_buffer_1997.py, goldens
// evidence/golden/video_subtitle_buffer_<build>.txt (the original's
// start-up clear, the executor's and the game-over screen's copy loops and
// the breaker run unmodified; experiment 2026-10-21-01, decision 0016 row
// 34).
//
// Port code exercised directly: SubtitleTextBuffer, splitSubtitleBitmap,
// subtitleLineYBitmap, drawSubtitleBitmapRow (subtitle_layout.cpp),
// conLineBufferText, conNarratorBufferText and ConLine::raw (con_script.h).

#include "atlantis_test_helper.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/algorithm.h"
#include "common/array.h"
#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/fs.h"
#include "common/ptr.h"
#include "graphics/surface.h"
#include "image/png.h"

#include "cryomni3d/atlantis/con_script.h"
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
// harness's split(), 2026-10-18, again with a zero tail 2026-10-21): rows
// of 85 bytes or more running into the next row's buffer, a leading space.
// The fourth, 76 bytes without a space: with a zero tail the original's
// window has no space (b = 75, the forward scan leaves the buffer): it
// loops and stores 10 empty rows (decision 0016 row 26, [UNKNOWN] on a real
// machine); the port keeps the text whole (the rows the CR model gave).
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

// The text buffer of the 1997 builds and its history (atlantis-re
// tools/emu/video_subtitle_buffer_1997.py, goldens
// evidence/golden/video_subtitle_buffer_<build>.txt).
namespace VideoSubBuf {

struct Build { const char *golden; const char *dirKey; bool backOnly; };
// dirKey: the release's archives for the CON walk (the English CON files
// are byte-identical in the reference's archives, `data`).
static const Build kBuilds[] = {
	{ "video_subtitle_buffer_en688128.txt", "data", false },
	{ "video_subtitle_buffer_fr688640.txt", "frmay1997", false },
	{ "video_subtitle_buffer_nl688128.txt", "nl1997", false },
	{ "video_subtitle_buffer_sv688128.txt", "sv1997", true },
};

// The harness's SAMPLES: VideoSubEn1997::samples() and four texts whose
// last word crosses a window end.
inline Common::Array<Common::String> samples() {
	Common::Array<Common::String> s = VideoSubEn1997::samples();
	s.push_back(Common::String((size_t)60, 'x') + " " + Common::String((size_t)16, 'y'));
	s.push_back(Common::String((size_t)70, 'x') + " " + Common::String((size_t)5, 'y'));
	s.push_back("one two three four five six seven eight nine ten eleven twelve thirteen fourteen");
	Common::String t;
	for (int i = 0; i < 15; i++)
		t += "abcdefghi ";
	s.push_back(t + "abcdefghijklmnopqrst");
	return s;
}

// The harness's HISTORY: (name, steps), a step 'e' (the executor's copy of
// a `[` line) or 'g' (the game-over screen's copy of a narrator line).
struct Step { char kind; Common::String line; };
inline Common::Array<Step> history(const Common::String &name) {
	const Common::String x60y16 = Common::String((size_t)60, 'x') + " " + Common::String((size_t)16, 'y');
	Common::Array<Step> h;
	auto ex = [&](const Common::String &l) { Step st = { 'e', l }; h.push_back(st); };
	if (name == "stale_space") {
		ex("[Seth,cam2] " + Common::String((size_t)40, 'q') + " " + Common::String((size_t)38, 'q') + " qq");
		ex("[Seth,cam2] " + x60y16);
	} else if (name == "stale_nospace") {
		ex("[Seth,cam2] " + Common::String((size_t)60, 'q') + " " + Common::String((size_t)30, 'q'));
		ex("[Seth,cam2] " + x60y16);
	} else if (name == "shorter_then_longer") {
		ex("[Seth] " + Common::String((size_t)70, 'r') + " s");
		ex("[Seth] " + x60y16);
		ex("[Seth] " + Common::String((size_t)70, 'x') + " " + Common::String((size_t)5, 'y'));
	} else if (name == "game_over") {
		Step g = { 'g', "[Narratrice] " + Common::String((size_t)40, 'n') + " " + Common::String((size_t)39, 'n') +
		               " end of the story" };
		h.push_back(g);
		ex("[Seth] " + x60y16);
	} else if (name == "trailing_blanks") {
		ex("[Seth] " + x60y16 + "   ");
		ex("[Seth] " + x60y16);
	} else if (name == "no_blank_after_bracket") {
		ex("[Seth]" + x60y16);
	}
	return h;
}

// "[61,17]" for the rows' lengths, as the harness prints them.
inline Common::String fmt(const Common::Array<Common::String> &rows) {
	Common::String s = "[";
	for (uint i = 0; i < rows.size(); i++)
		s += Common::String::format(i ? ",%u" : "%u", rows[i].size());
	return s + "]";
}

// The first "[...]" of `s` from `from`.
inline Common::String bracket(const Common::String &s, uint32 from = 0) {
	const uint32 a = s.find('[', from);
	const uint32 b = a == Common::String::npos ? a : s.find(']', a);
	return b == Common::String::npos ? Common::String() : s.substr(a, b - a + 1);
}

// A window without a space (decision 0016 row 26): the golden says so, the
// port's cut differs by design.
inline bool undefinedRows(const Common::String &s) {
	return s.contains("nospace") || s.contains("no-return");
}

// The `[` lines of the CON files of `dir` in the harness's order: names
// sorted, lines split at CR LF, a line kept from its '[' when it holds a
// ']'; its name and line number.
struct ConLineRef { Common::String where; Common::String raw; };
inline void conLines(const Common::String &dir, Common::Array<ConLineRef> &out) {
	VideoTest::Big big(dir);
	Common::Array<Common::String> cons = big.names(".CON");
	Common::sort(cons.begin(), cons.end());
	for (uint f = 0; f < cons.size(); f++) {
		uint32 size = 0;
		byte *b = big.get(cons[f], size);
		if (!b)
			continue;
		uint32 i = 0, no = 0;
		while (i <= size) {
			uint32 j = i;
			while (j < size && !(b[j] == '\r' && j + 1 < size && b[j + 1] == '\n'))
				j++;
			no++;
			if (j > i && b[i] == '[' && memchr(b + i, ']', j - i)) {
				// the executor's copy stops at the first CR
				uint32 e = i;
				while (e < j && b[e] != '\r')
					e++;
				ConLineRef r = { Common::String::format("%s:%u", cons[f].c_str(), no),
				                 Common::String((const char *)b + i, e - i) };
				out.push_back(r);
			}
			if (j >= size)
				break;
			i = j + 2;
		}
		delete[] b;
	}
}

} // End of namespace VideoSubBuf

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
			if (want.size() > 8) {
				// 75 bytes without a space: the original loops (10 empty
				// rows); the port keeps the text whole (decision 0016 row 26)
				TS_ASSERT_EQUALS(in[k], Common::String((size_t)75, 'x'));
				TS_ASSERT(got.size() == 1 && got[0] == " " + in[k]);
				k++;
				continue;
			}
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
	// The text buffer as the first line after start-up leaves it (zeros
	// after the NUL), in the four bitmap builds: the original's rows for the
	// harness's samples (its start-up clear and executor copy run, then the
	// breaker); the port's splitSubtitleBitmap gives the same, except the
	// windows without a space (decision 0016 row 26, 0023 D2).
	void test_buffer_fresh_vs_golden() {
		using namespace CryOmni3D::Atlantis;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		const Common::Array<Common::String> in = VideoSubBuf::samples();
		for (uint g = 0; g < ARRAYSIZE(VideoSubBuf::kBuilds); g++) {
			const VideoSubBuf::Build &bd = VideoSubBuf::kBuilds[g];
			Common::Array<Common::String> lines;
			TS_ASSERT(AtlantisTest::loadGoldenLines(bd.golden, lines));
			uint k = 0, undef = 0, past = 0;
			for (uint i = 0; i < lines.size(); i++) {
				if (!lines[i].hasPrefix("fresh len "))
					continue;
				TS_ASSERT(k < in.size());
				if (k >= in.size())
					break;
				TS_ASSERT_EQUALS(atoi(lines[i].c_str() + 10), (int)in[k].size());
				Common::Array<Common::String> got;
				splitSubtitleBitmap(in[k], got, bd.backOnly);
				if (VideoSubBuf::undefinedRows(lines[i])) {
					undef++;
				} else {
					TSM_ASSERT_EQUALS(lines[i].c_str(), VideoSubBuf::fmt(got),
					                  VideoSubBuf::bracket(lines[i], lines[i].find("rows ")));
				}
				past += lines[i].contains(" past");
				k++;
			}
			TSM_ASSERT_EQUALS(bd.golden, k, in.size());
			// x * 75 (and x * 76 + " tail" in the Swedish build); three
			// samples run their forward scan out of the buffer
			TSM_ASSERT_EQUALS(bd.golden, undef, bd.backOnly ? 2u : 1u);
			TSM_ASSERT_EQUALS(bd.golden, past, 3u);
		}
	}

	// The buffer's history: the original's copies (executor, game-over
	// narrator) leave the tail of an earlier, longer text after the NUL,
	// and its spaces can stop the forward scan (stale_space, game_over: one
	// row where a first line after start-up has two).  The port's
	// SubtitleTextBuffer, filled by conLineBufferText / conNarratorBufferText,
	// gives the same rows after every step and the same tail.
	void test_buffer_history_vs_golden() {
		using namespace CryOmni3D::Atlantis;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		for (uint g = 0; g < ARRAYSIZE(VideoSubBuf::kBuilds); g++) {
			const VideoSubBuf::Build &bd = VideoSubBuf::kBuilds[g];
			Common::Array<Common::String> lines;
			TS_ASSERT(AtlantisTest::loadGoldenLines(bd.golden, lines));
			uint seen = 0, checked = 0;
			for (uint i = 0; i < lines.size(); i++) {
				if (!lines[i].hasPrefix("history "))
					continue;
				const uint32 colon = lines[i].find(':');
				const Common::String name = lines[i].substr(8, colon - 8);
				const Common::Array<VideoSubBuf::Step> steps = VideoSubBuf::history(name);
				TSM_ASSERT(lines[i].c_str(), !steps.empty());
				seen++;
				SubtitleTextBuffer buf;
				uint32 pos = colon + 1;
				for (uint st = 0; st < steps.size(); st++) {
					buf.store(steps[st].kind == 'e' ? conLineBufferText(steps[st].line)
					                                : conNarratorBufferText(steps[st].line));
					Common::Array<Common::String> rows;
					splitSubtitleBitmap(buf, rows, bd.backOnly);
					uint32 end = lines[i].find(" ; ", pos);
					Common::String token = lines[i].substr(pos, end - pos);
					token.trim();
					pos = end + 3;
					TSM_ASSERT(lines[i].c_str(), token.hasPrefix(steps[st].kind == 'e' ? "ex " : "go "));
					if (VideoSubBuf::undefinedRows(token))
						continue;
					TSM_ASSERT_EQUALS(lines[i].c_str(), VideoSubBuf::fmt(rows), VideoSubBuf::bracket(token));
					checked++;
				}
				// the 24 bytes from the NUL on
				const uint32 t = lines[i].find("tail ");
				TS_ASSERT(t != Common::String::npos);
				const uint nul = buf.text().size();
				Common::String hex;
				for (uint b = 0; b < 24; b++)
					hex += Common::String::format("%02x", buf.at(nul + b));
				TSM_ASSERT_EQUALS(lines[i].c_str(), hex, lines[i].substr(t + 5));
			}
			TSM_ASSERT_EQUALS(bd.golden, seen, 6u);
			TSM_ASSERT(bd.golden, checked >= 9);
		}
		// stale_space in en-688128: the tail of the first line stops the
		// second line's forward scan, one row of 78; as the first line after
		// start-up it is 61 | 17
		SubtitleTextBuffer buf;
		Common::Array<Common::String> rows;
		const Common::Array<VideoSubBuf::Step> h = VideoSubBuf::history("stale_space");
		buf.store(conLineBufferText(h[1].line));
		splitSubtitleBitmap(buf, rows);
		TS_ASSERT_EQUALS(VideoSubBuf::fmt(rows), "[61,17]");
		buf.store(conLineBufferText(h[0].line));
		buf.store(conLineBufferText(h[1].line));
		splitSubtitleBitmap(buf, rows);
		TS_ASSERT_EQUALS(VideoSubBuf::fmt(rows), "[78]");
	}

	// Every `[` line of each release's CON files: as the first line after
	// start-up (the golden's scan statistics, and the lines whose rows the
	// planted CR changed), then all of them in file order through one
	// buffer (the lines whose rows the stale tail changes).
	void test_buffer_con_walk_vs_golden() {
		using namespace CryOmni3D::Atlantis;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		for (uint g = 0; g < ARRAYSIZE(VideoSubBuf::kBuilds); g++) {
			const VideoSubBuf::Build &bd = VideoSubBuf::kBuilds[g];
			const Common::String dir = AtlantisTest::configValue(bd.dirKey);
			if (dir.empty()) {
				TS_WARN(Common::String::format("%s not configured; %s skipped", bd.dirKey, bd.golden).c_str());
				continue;
			}
			Common::Array<Common::String> golden;
			TS_ASSERT(AtlantisTest::loadGoldenLines(bd.golden, golden));
			Common::String scan;
			Common::HashMap<Common::String, Common::String> crdiff;
			Common::Array<Common::String> walkWant;
			int crdiffCount = -1, walkCount = -1;
			for (uint i = 0; i < golden.size(); i++) {
				const Common::String &l = golden[i];
				if (l.hasPrefix("# scan of every [ line"))
					scan = l;
				else if (l.hasPrefix("# lines whose fresh rows differ from the CR model: "))
					crdiffCount = atoi(l.c_str() + 51);
				else if (l.hasPrefix("# file-order walk after one start-up clear: "))
					walkCount = atoi(l.c_str() + 44);
				else if (l.hasPrefix("crdiff "))
					crdiff[l.substr(7, l.find(' ', 7) - 7)] = VideoSubBuf::bracket(l, l.find(" fresh "));
				else if (l.hasPrefix("walk ")) {
					const uint32 sp = l.find(' ', 5);
					walkWant.push_back(l.substr(5, sp - 5) + " " + VideoSubBuf::bracket(l, sp) + " " +
					                   VideoSubBuf::bracket(l, l.find(" fresh ")));
				}
			}
			TSM_ASSERT(bd.golden, !scan.empty());
			TSM_ASSERT_EQUALS(bd.golden, (int)crdiff.size(), crdiffCount);
			TSM_ASSERT_EQUALS(bd.golden, (int)walkWant.size(), walkCount);
			auto field = [&](const char *key) {
				const uint32 p = scan.find(Common::String::format("'%s': ", key));
				return p == Common::String::npos ? -1 : atoi(scan.c_str() + p + strlen(key) + 4);
			};
			Common::Array<VideoSubBuf::ConLineRef> refs;
			VideoSubBuf::conLines(dir, refs);
			int maxRows = 0, maxLen = 0, over79 = 0, over84 = 0, maxText = 0, hist[9] = { 0 };
			Common::Array<Common::String> fresh;
			uint crChecked = 0;
			for (uint i = 0; i < refs.size(); i++) {
				SubtitleTextBuffer buf;
				buf.store(conLineBufferText(refs[i].raw));
				Common::Array<Common::String> rows;
				splitSubtitleBitmap(buf, rows, bd.backOnly);
				fresh.push_back(VideoSubBuf::fmt(rows));
				maxRows = MAX(maxRows, (int)rows.size());
				maxText = MAX(maxText, (int)buf.text().size());
				if (rows.size() < ARRAYSIZE(hist))
					hist[rows.size()]++;
				for (uint r = 0; r < rows.size(); r++) {
					maxLen = MAX(maxLen, (int)rows[r].size());
					over79 += rows[r].size() > 79;
					over84 += rows[r].size() > 84;
				}
				if (crdiff.contains(refs[i].where)) {
					TSM_ASSERT_EQUALS(refs[i].where.c_str(), fresh.back(), crdiff[refs[i].where]);
					crChecked++;
				}
			}
			TSM_ASSERT_EQUALS(bd.golden, (int)refs.size(), field("lines"));
			TSM_ASSERT_EQUALS(bd.golden, maxRows, field("max_rows"));
			TSM_ASSERT_EQUALS(bd.golden, maxLen, field("max_row_len"));
			TSM_ASSERT_EQUALS(bd.golden, over79, field("rows_over_79"));
			TSM_ASSERT_EQUALS(bd.golden, over84, field("rows_over_84"));
			TSM_ASSERT_EQUALS(bd.golden, maxText, field("max_text"));
			TSM_ASSERT_EQUALS(bd.golden, field("dependent"), 0);
			const uint32 h0 = scan.find("'rows_hist': {");
			const uint32 h1 = scan.find('}', h0);
			TSM_ASSERT(bd.golden, h0 != Common::String::npos && h1 != Common::String::npos);
			Common::String histGot = "'rows_hist': {";
			for (int n = 1; n <= maxRows && n < (int)ARRAYSIZE(hist); n++)
				histGot += Common::String::format(n > 1 ? ", %d: %d" : "%d: %d", n, hist[n]);
			TSM_ASSERT_EQUALS(bd.golden, histGot + "}", scan.substr(h0, h1 - h0 + 1));
			TSM_ASSERT_EQUALS(bd.golden, crChecked, crdiff.size());
			// the walk: one buffer, every line in file order
			SubtitleTextBuffer buf;
			Common::Array<Common::String> walkGot;
			for (uint i = 0; i < refs.size(); i++) {
				buf.store(conLineBufferText(refs[i].raw));
				Common::Array<Common::String> rows;
				splitSubtitleBitmap(buf, rows, bd.backOnly);
				if (VideoSubBuf::fmt(rows) != fresh[i])
					walkGot.push_back(refs[i].where + " " + VideoSubBuf::fmt(rows) + " " + fresh[i]);
			}
			TSM_ASSERT_EQUALS(bd.golden, walkGot.size(), walkWant.size());
			for (uint i = 0; i < walkGot.size() && i < walkWant.size(); i++)
				TSM_ASSERT_EQUALS(bd.golden, walkGot[i], walkWant[i]);
		}
	}

	// ConLine::raw keeps the line up to its CR with its trailing blanks (the
	// parsed text drops them); conLineBufferText starts after the first ']'.
	void test_con_line_raw() {
		using namespace CryOmni3D::Atlantis;
		static const char kText[] = "/1\r\n/con(ClicPerso=600)\r\n[Seth,cam2] A b  \r\n[Seth]x]y\r\n/000\r\n";
		ConScript script;
		script.parse(kText, strlen(kText));
		const ConSection *sec = nullptr;
		for (uint i = 0; i < script.sections().size(); i++)
			if (script.sections()[i].id == 1)
				sec = &script.sections()[i];
		TS_ASSERT(sec && sec->items.size() == 2);
		if (!sec || sec->items.size() != 2)
			return;
		TS_ASSERT_EQUALS(sec->items[0].line.text, "A b");
		TS_ASSERT_EQUALS(sec->items[0].line.raw, "[Seth,cam2] A b  ");
		TS_ASSERT_EQUALS(conLineBufferText(sec->items[0].line.raw), " A b  ");
		TS_ASSERT_EQUALS(conLineBufferText(sec->items[1].line.raw), "x]y");
		TS_ASSERT_EQUALS(conNarratorBufferText("[Narratrice] Seth est mort."), " Seth est mort.");
		// a ']' right before the CR: the original copies on past the line
		// (none shipped); the port copies nothing
		TS_ASSERT_EQUALS(conLineBufferText("[Seth]"), "");
		// the copy keeps the bytes after the NUL
		SubtitleTextBuffer buf;
		buf.store("abcdef");
		buf.store("xy");
		TS_ASSERT_EQUALS(buf.text(), "xy");
		TS_ASSERT_EQUALS(buf.at(2), 0);
		TS_ASSERT_EQUALS(buf.at(3), 'd');
		TS_ASSERT_EQUALS(buf.at(SubtitleTextBuffer::kSize), 0);
	}
};

#endif
