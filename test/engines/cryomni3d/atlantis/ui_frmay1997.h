#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_FRMAY1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_FRMAY1997_H

// ui audit: the French CD's first pressing of May 1997
// (atlantis-cd-fr-688640), as atlantis-re experiment 2026-10-18-04 read it
// (STATIC, EMULATED, DATA) and decision 0020 chose.  The build is
// en-688128's code (its rules are checked in ui_en1997.h and
// video_subtitles_en1997.h; the flags in exe_builds.h), so here:
//   M1 LoadMusik all or nothing on its first track (FUN_0041e830)
//   M2 LOADMUSIK only with a current music volume of 5 or more
//      (FUN_00429184 @ 0x00429c5e), and how soon a StartMusik reaches it
//   B  the per-track copy kept for kEnhMinorBugFixes (the other builds' rule)
//   A1 the subtitles: FONTS.BIN glyph = byte (FUN_0042eab4 / FUN_0042ec80)
//      against the emulated golden on the 2540 French lines and its
//      rendered PNG; the DOS code page bytes decoded as code page 850,
//      never Windows-1252, by any other path
//   E  the game-over voices of the May scripts (voice at entry, as
//      en-688128), the player row `NOUVEAU JOUEUR` drawn as stored, the
//      archives (FONTS.BIN the English one, WAV\ATA0897D.APC present).
//
// Port code exercised directly: conMusicApply / conLoadMusikVolumeOk
// (con_eval.cpp), AtlantisMusicFade::tick (music_fade.h),
// splitSubtitleBitmap / subtitleLineYBitmap / drawSubtitleBitmapRow
// (subtitle_layout.cpp), atlantisExeBuild / loadMusikMinVolume /
// loadMusikAllOrNothing (exe_build.h), fontMinBlits
// (menu_layout.cpp), ConScript, conGameOverNarrator, gameOverVoiceName.
// Real files: `frmay1997` in atlantis_test.cfg (ATLANTIS.EXE and the four
// May BIGCD*.BIG); the golden video_subtitles_fr688640.txt / .png; skipped
// when absent.

#include "atlantis_test_helper.h"
#include "video_subtitles_en1997.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/algorithm.h"
#include "common/array.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/md5.h"
#include "common/memstream.h"
#include "common/ustr.h"
#include "graphics/surface.h"
#include "image/png.h"

#include "cryomni3d/atlantis/con_eval.h"
#include "cryomni3d/atlantis/con_script.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/gameover.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/music_fade.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiFrMay1997 {

using namespace CryOmni3D::Atlantis;

inline const AtlantisExeBuild &may() { return atlantisExeBuild(688640); }

inline Common::String dir() { return AtlantisTest::configValue("frmay1997"); }

// The dialog texts of the May archives' CON files in name order, as the
// harness takes them (tools/emu/video_subtitles_fr688640.py
// con_dialog_texts).
inline void dialogTexts(const VideoTest::Big &big, Common::Array<Common::String> &texts) {
	Common::Array<Common::String> cons = big.names(".CON");
	Common::sort(cons.begin(), cons.end());
	for (uint f = 0; f < cons.size(); f++) {
		if (!cons[f].hasPrefix("SCENAR\\"))
			continue;
		uint32 size = 0;
		byte *b = big.get(cons[f], size);
		if (!b)
			continue;
		VideoSubEn1997::conDialogTexts(b, size, texts);
		delete[] b;
	}
}

// The golden's "# scan of every dialog line" entry `key`, or -1.
inline int scanField(const Common::String &scan, const char *key) {
	const size_t p = scan.find(Common::String::format("'%s': ", key));
	return p == Common::String::npos ? -1 : atoi(scan.c_str() + p + strlen(key) + 4);
}

} // End of namespace UiFrMay1997

class UiFrMay1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// M1: FUN_0041e830 tests only the first listed track: its copy there
	// (FUN_0041ee40 @ 0x0041e86e) or playing (0x0041e878..0x0041e891), then
	// nothing; else every listed track is copied without a test (loop
	// 0x0041e926..0x0041e96e), present or playing ones included.
	void test_music_load_all_or_nothing() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		Common::Array<uint8> list;
		list.push_back(4);   // tracks 5, 7, 9
		list.push_back(6);
		list.push_back(8);
		Common::Array<uint8> copies;
		// the first one there: nothing, the missing ones included
		uint32 disk = 1u << 4;
		conMusicApply(true, list, 0, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 1u << 4);
		TS_ASSERT(copies.empty());
		// the other builds copy the missing ones
		conMusicApply(true, list, 0, disk, copies, true);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6) | (1u << 8));
		TS_ASSERT_EQUALS(copies.size(), 2u);
		// the first one missing: all three copied, the present one again
		copies.clear();
		disk = 1u << 6;
		conMusicApply(true, list, 0, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6) | (1u << 8));
		TS_ASSERT_EQUALS(copies.size(), 3u);
		if (copies.size() == 3)
			TS_ASSERT(copies[0] == 4 && copies[1] == 6 && copies[2] == 8);
		// the playing track's copy is attempted when it is not first but
		// fails before its read loop (its install copy is held open with
		// share 0, FUN_004316cc @ 0x00431747; CREATE_ALWAYS @ 0x0041eb3a
		// fails) [INFERRED]: not queued, still on disk
		copies.clear();
		disk = 0;
		conMusicApply(true, list, 7, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(copies.size(), 2u);
		if (copies.size() == 2)
			TS_ASSERT(copies[0] == 4 && copies[1] == 8);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6) | (1u << 8));
		// ... and nothing when it is first
		copies.clear();
		disk = 0;
		conMusicApply(true, list, 5, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 0u);
		TS_ASSERT(copies.empty());
		// DelMusik: FUN_0041eee8 has no playing test, but the playing file
		// is held open with share 0 (FUN_004316cc), so its DeleteFileA's
		// outcome is [UNKNOWN]; the port keeps the reference (spared), as
		// for en-688128
		disk = (1u << 4) | (1u << 6) | (1u << 8);
		conMusicApply(false, list, 7, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 1u << 6);
		// a first index of 0x20 or more: nothing, as in every build
		Common::Array<uint8> high;
		high.push_back(0x20);
		high.push_back(4);
		disk = 0;
		conMusicApply(true, high, 0, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 0u);
	}

	// M2: FUN_00429184 @ 0x00429c5e runs LoadMusik only when 0x00485e71 is
	// 5 or more; the other builds always.  0x00485e71 starts at 0 with each
	// StartMusik and gains 1 every 5 ticks of the 200 Hz timer
	// (AtlantisMusicFade::tick): 5 is reached after 25 ticks (125 ms).
	void test_music_load_volume_rule() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		for (uint v = 0; v < 5; v++)
			TSM_ASSERT(v, !conLoadMusikVolumeOk((uint8)v, b.musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(5, b.musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(126, b.musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(0, atlantisExeBuild(688128).musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(0, atlantisExeBuild(714240).musicLoadMinVolume));
		// the initial value (0x6e in the data) lets the first LOADMUSIK run
		AtlantisMusicFade initial;
		TS_ASSERT(conLoadMusikVolumeOk(initial.cur, b.musicLoadMinVolume));
		// after a StartMusik (musicSwitchTrack: cur 0, fade-in to vol)
		AtlantisMusicFade f;
		f.vol = 126;
		f.cur = 0;
		f.mode = 1;
		f.upTarget = f.vol;
		f.ticks = 0;
		f.lastStep = 0;
		int ticks = 0;
		while (!conLoadMusikVolumeOk(f.cur, b.musicLoadMinVolume) && ticks < 1000) {
			f.tick();
			ticks++;
		}
		TS_ASSERT_EQUALS(ticks, 25);
		// with a music volume option below 5 it is never reached
		AtlantisMusicFade low;
		low.vol = 4;
		low.cur = 0;
		low.mode = 1;
		low.upTarget = low.vol;
		for (int i = 0; i < 1000; i++)
			low.tick();
		TS_ASSERT(!conLoadMusikVolumeOk(low.cur, b.musicLoadMinVolume));
	}

	// Decision 0020 B: the rules music.cpp execMusicList applies, through
	// the gating helpers it calls (exe_build.h loadMusikMinVolume /
	// loadMusikAllOrNothing).  Without kEnhMinorBugFixes this build's M1
	// and M2 (the values of test_music_load_all_or_nothing and
	// test_music_load_volume_rule); with it the port copies per track: the
	// rule of FUN_0041f1d0, each listed track not on disk and not playing;
	// M2's volume test stays (decision 0020 B, Fable pass 2
	// opus-gap3-builds-1997-en-fr#4).
	void test_music_minor_bug_fix_rule() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		Common::Array<uint8> list;
		list.push_back(4);
		list.push_back(6);
		list.push_back(8);
		// the original's rules (fixed = false)
		{
			const uint8 minVolume = loadMusikMinVolume(b, false);
			TS_ASSERT_EQUALS(minVolume, 5);
			TS_ASSERT(!conLoadMusikVolumeOk(0, minVolume));
			const bool allOrNothing = loadMusikAllOrNothing(b, false);
			TS_ASSERT(allOrNothing);
			Common::Array<uint8> copies;
			uint32 disk = 1u << 4;
			conMusicApply(true, list, 9, disk, copies, b.musicFirstValueRule, allOrNothing);
			TS_ASSERT_EQUALS(disk, 1u << 4);
			TS_ASSERT(copies.empty());
		}
		// kEnhMinorBugFixes (fixed = true)
		{
			TS_ASSERT_EQUALS(loadMusikMinVolume(b, true), 5);
			TS_ASSERT(!conLoadMusikVolumeOk(0, loadMusikMinVolume(b, true)));
			const bool allOrNothing = loadMusikAllOrNothing(b, true);
			TS_ASSERT(!allOrNothing);
			Common::Array<uint8> copies;
			uint32 disk = 1u << 4;
			conMusicApply(true, list, 9, disk, copies, b.musicFirstValueRule, allOrNothing);
			TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6));
			TS_ASSERT_EQUALS(copies.size(), 1u);
			if (copies.size() == 1)
				TS_ASSERT_EQUALS(copies[0], 6);
		}
		// the other builds: no gating either way
		const AtlantisExeBuild &en = atlantisExeBuild(688128);
		TS_ASSERT_EQUALS(loadMusikMinVolume(en, false), 0);
		TS_ASSERT(!loadMusikAllOrNothing(en, false));
	}

	// A1: the emulated breaker FUN_0042eab4 on every dialog line of the May
	// CON files (the golden's scan): the port's splitSubtitleBitmap gives
	// the same statistics, and the dialog bytes >= 0x80 are the golden's 11
	// DOS code page bytes with its counts.
	void test_subtitle_scan_vs_golden() {
		using namespace CryOmni3D::Atlantis;
		if (!AtlantisTest::haveData() || UiFrMay1997::dir().empty()) { TS_WARN("golden / frmay1997 not configured; skipped"); return; }
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_fr688640.txt", lines));
		Common::String scan;
		Common::HashMap<int, int> wantHigh;
		for (uint i = 0; i < lines.size(); i++) {
			if (lines[i].hasPrefix("# scan of every dialog line"))
				scan = lines[i];
			// "#   0x82:  1697  é / é  (as Windows-1252: '‚')"
			if (lines[i].hasPrefix("#   0x") && lines[i].contains("Windows-1252"))
				wantHigh[(int)strtol(lines[i].c_str() + 6, nullptr, 16)] = atoi(strchr(lines[i].c_str(), ':') + 1);
		}
		TS_ASSERT(!scan.empty());
		TS_ASSERT_EQUALS(wantHigh.size(), 11u);
		VideoTest::Big big(UiFrMay1997::dir());
		TS_ASSERT(!big.empty());
		Common::Array<Common::String> texts;
		UiFrMay1997::dialogTexts(big, texts);
		int maxLen = 0, over79 = 0, over84 = 0, withHigh = 0, hist[9] = { 0 };
		Common::HashMap<int, int> high;
		for (uint t = 0; t < texts.size(); t++) {
			Common::Array<Common::String> rows;
			splitSubtitleBitmap(texts[t], rows);
			if (rows.size() < ARRAYSIZE(hist))
				hist[rows.size()]++;
			for (uint r = 0; r < rows.size(); r++) {
				maxLen = MAX(maxLen, (int)rows[r].size());
				over79 += rows[r].size() > 79;
				over84 += rows[r].size() > 84;
			}
			bool any = false;
			for (uint i = 0; i < texts[t].size(); i++) {
				const byte c = (byte)texts[t][i];
				if (c >= 0x80) {
					high[c]++;
					any = true;
				}
			}
			withHigh += any;
		}
		TS_ASSERT_EQUALS((int)texts.size(), UiFrMay1997::scanField(scan, "lines"));
		TS_ASSERT_EQUALS((int)texts.size(), 2540);
		TS_ASSERT_EQUALS(maxLen, UiFrMay1997::scanField(scan, "max_row_len"));
		TS_ASSERT_EQUALS(over79, UiFrMay1997::scanField(scan, "rows_over_79"));
		TS_ASSERT_EQUALS(over84, UiFrMay1997::scanField(scan, "rows_over_84"));
		TS_ASSERT_EQUALS(withHigh, UiFrMay1997::scanField(scan, "lines_with_high_bytes"));
		// rows_hist: {1: 1891, 2: 555, 3: 73, 4: 16, 5: 4, 6: 1}
		static const int kHist[][2] = { { 1, 1891 }, { 2, 555 }, { 3, 73 }, { 4, 16 }, { 5, 4 }, { 6, 1 } };
		TS_ASSERT(scan.contains("'rows_hist': {1: 1891, 2: 555, 3: 73, 4: 16, 5: 4, 6: 1}"));
		for (uint i = 0; i < ARRAYSIZE(kHist); i++)
			TSM_ASSERT_EQUALS(kHist[i][0], hist[kHist[i][0]], kHist[i][1]);
		TS_ASSERT_EQUALS(high.size(), wantHigh.size());
		for (Common::HashMap<int, int>::const_iterator it = wantHigh.begin(); it != wantHigh.end(); ++it)
			TSM_ASSERT_EQUALS(it->_key, high.getValOrDefault(it->_key, -1), it->_value);
	}

	// A1: the 11 DOS code page bytes of the texts are letters of code page
	// 850 (the golden names each: "0x82: 1697  é / é"); the build's
	// conCodePage decodes them so, where Windows-1252 gives '‚' ... (the
	// golden's "as Windows-1252").  drawSubtitleRows's GDI path decodes
	// with conCodePage, so no path decodes them as Windows-1252.
	void test_code_page_850() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		TS_ASSERT_EQUALS(b.conCodePage, Common::kDos850);
		TS_ASSERT(b.bitmapSubtitles);
		// "Créon", "île", "à", "ça" as the May CON files hold them
		const Common::U32String s(Common::String("Cr\x82on \x8cle \x85 \x87" "a"), b.conCodePage);
		TS_ASSERT_EQUALS(s.encode(Common::kUtf8), Common::String("Cr\xc3\xa9on \xc3\xaele \xc3\xa0 \xc3\xa7" "a"));
		const Common::U32String w(Common::String("Cr\x82on"), Common::kWindows1252);
		TS_ASSERT_DIFFERS(w.encode(Common::kUtf8), Common::String("Cr\xc3\xa9on"));
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; the golden's letters skipped"); return; }
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_fr688640.txt", lines));
		int checked = 0;
		for (uint i = 0; i < lines.size(); i++) {
			if (!lines[i].hasPrefix("#   0x") || !lines[i].contains("Windows-1252"))
				continue;
			const byte c = (byte)strtol(lines[i].c_str() + 6, nullptr, 16);
			// the letter: after the count, up to " / "
			const char *p = strchr(lines[i].c_str(), ':') + 1;
			while (*p == ' ')
				p++;
			while (*p >= '0' && *p <= '9')
				p++;
			while (*p == ' ')
				p++;
			const char *slash = strstr(p, " / ");
			TS_ASSERT(slash);
			if (!slash)
				continue;
			const Common::String letter(p, slash - p);
			const char raw[2] = { (char)c, 0 };
			TSM_ASSERT_EQUALS(c, Common::U32String(Common::String(raw), b.conCodePage).encode(Common::kUtf8), letter);
			TSM_ASSERT_DIFFERS(c, Common::U32String(Common::String(raw), Common::kWindows1252).encode(Common::kUtf8), letter);
			checked++;
		}
		TS_ASSERT_EQUALS(checked, 11);
	}

	// A1: the golden PNG: the first two-row line with four accented letters
	// (SCENAR\CHAPI002.CON) drawn by the original FUN_0042ec80 with the May
	// FONTS.BIN on a 640 x 480 RGB565 screen of 0x8410, rows 400..479; the
	// port draws the same rows by byte: every pixel equal.
	void test_pixels_vs_golden_png() {
		using namespace CryOmni3D::Atlantis;
		if (!AtlantisTest::haveData() || UiFrMay1997::dir().empty()) { TS_WARN("golden / frmay1997 not configured; skipped"); return; }
		VideoTest::Big big(UiFrMay1997::dir());
		uint32 fontSize = 0;
		byte *raw = big.get("DIALOG\\FONTS.BIN", fontSize);
		TS_ASSERT(raw && fontSize == 16384);
		if (!raw)
			return;
		Common::Array<byte> font(raw, fontSize);
		delete[] raw;
		Common::Array<Common::String> texts;
		UiFrMay1997::dialogTexts(big, texts);
		Common::String sample;
		for (uint t = 0; t < texts.size() && sample.empty(); t++) {
			int high = 0;
			for (uint i = 0; i < texts[t].size(); i++)
				high += (byte)texts[t][i] >= 0x80;
			if (high >= 4 && texts[t].size() > 80)
				sample = texts[t];
		}
		TS_ASSERT(sample.hasPrefix("Non. Elle fait la tourn\x82" "e de l'\x8cle."));
		Common::Array<uint16> screen(640 * 480, 0x8410);
		Common::Array<Common::String> rows;
		splitSubtitleBitmap(sample, rows);
		TS_ASSERT_EQUALS(rows.size(), 2u);
		for (uint i = 0; i < rows.size(); i++) {
			const int y = subtitleLineYBitmap(i, rows.size());
			drawSubtitleBitmapRow((byte *)screen.data(), 2, 640, 480, font.data(), fontSize,
			                      kSubtitleX + 1, y + 1, rows[i], 0);
			drawSubtitleBitmapRow((byte *)screen.data(), 2, 640, 480, font.data(), fontSize,
			                      kSubtitleX, y, rows[i], 0xffff);
		}
		Common::File f;
		TS_ASSERT(f.open(Common::FSNode(Common::Path(AtlantisTest::goldenDir(), '/')
		                                .appendComponent("video_subtitles_fr688640.png"))));
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
				const uint8 r = (v >> 11) << 3, g = ((v >> 5) & 0x3f) << 2, bl = (v & 0x1f) << 3;
				uint8 pr, pg, pb;
				s->format.colorToRGB(s->getPixel(x, y), pr, pg, pb);
				diffs += (r != pr || g != pg || bl != pb);
				white += v == 0xffff;
			}
		}
		TS_ASSERT_EQUALS(diffs, 0);
		TS_ASSERT(white > 100);
	}

	// E: the game overs of the May scripts with en-688128's rule (voice at
	// entry, the narrator line's letter, no alternative narration): 144,
	// all voiced (experiment 2026-10-18-04, tools/compare/gameover_narrations.py).
	void test_game_over_voices_may_scripts() {
		using namespace CryOmni3D::Atlantis;
		if (UiFrMay1997::dir().empty()) { TS_WARN("frmay1997 not configured; skipped"); return; }
		TS_ASSERT(UiFrMay1997::may().gameOverVoiceAtEntry && !UiFrMay1997::may().altGameOverNarration);
		VideoTest::Big big(UiFrMay1997::dir());
		Common::HashMap<Common::String, bool> apc;
		const Common::Array<Common::String> apcs = big.names(".APC");
		for (uint i = 0; i < apcs.size(); i++)
			apc[apcs[i]] = true;
		int overs = 0, voiced = 0;
		const Common::Array<Common::String> cons = big.names(".CON");
		for (uint f = 0; f < cons.size(); f++) {
			uint32 size = 0;
			byte *b = big.get(cons[f], size);
			if (!b)
				continue;
			ConScript script;
			script.parse((const char *)b, size);
			delete[] b;
			for (const ConSection &sec : script.sections()) {
				uint lineIdx = 0;
				for (uint k = 0; k < sec.items.size(); k++) {
					const ConSectionItem &it = sec.items[k];
					if (!it.isSet) {
						lineIdx++;
						continue;
					}
					if (scumm_strnicmp(it.setCmd.c_str(), "gameover=", 9) != 0 ||
					        atoi(it.setCmd.c_str() + 9) == 0)
						continue;
					Common::String text;
					if (!conGameOverNarrator(sec.items, k, text))
						continue;
					overs++;
					voiced += apc.contains("WAV\\" + gameOverVoiceName(sec.id, conTrackLetter(lineIdx)));
				}
			}
		}
		TS_ASSERT_EQUALS(overs, 144);
		TS_ASSERT_EQUALS(voiced, 144);
	}

	// E: the player row `NOUVEAU JOUEUR` (0x0048654a, marker = label) drawn
	// as stored by FUN_00420064 (no '#' rule): 13 glyphs; names typed
	// 0x20..0x7a upper-cased (FUN_0041d728).
	void test_player_row_and_names() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		Common::Array<FontMinBlit> blits;
		fontMinBlits(kFontMinPlayerList, "NOUVEAU JOUEUR", 185, 270, blits, !b.playerListVerbatim);
		TS_ASSERT_EQUALS(blits.size(), 13u);
		fontMinBlits(kFontMinPlayerList, "A#B", 100, 50, blits, !b.playerListVerbatim);
		TS_ASSERT_EQUALS(blits.size(), 3u);
		TS_ASSERT_EQUALS(nameEntryChar('e', b.upperCaseNames), 'E');
		TS_ASSERT_EQUALS(nameEntryChar(0x82, b.upperCaseNames), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0x7b, b.upperCaseNames), -1);
	}

	// The May archives: DIALOG\FONTS.BIN is the English (reference) file,
	// the code page 437 glyph set (md5 4aec21cc...); WAV\ATA0897D.APC is
	// there (the August release lacks it); the CON texts hold the DOS code
	// page bytes (0x82 'é' in CHAPI002).
	void test_archives() {
		if (UiFrMay1997::dir().empty()) { TS_WARN("frmay1997 not configured; skipped"); return; }
		VideoTest::Big big(UiFrMay1997::dir());
		TS_ASSERT(!big.empty());
		uint32 size = 0;
		byte *font = big.get("DIALOG\\FONTS.BIN", size);
		TS_ASSERT(font);
		if (font) {
			Common::MemoryReadStream ms(font, size);
			TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(ms), Common::String("4aec21ccbf3bd593dc01751adb834964"));
			delete[] font;
		}
		byte *apc = big.get("WAV\\ATA0897D.APC", size);
		TS_ASSERT(apc);
		TS_ASSERT_EQUALS(size, 37875u);
		delete[] apc;
		byte *con = big.get("SCENAR\\CHAPI002.CON", size);
		TS_ASSERT(con);
		if (con) {
			const Common::String text((const char *)con, size);
			TS_ASSERT(text.contains("Cr\x82on"));
			delete[] con;
		}
	}
};

#endif
