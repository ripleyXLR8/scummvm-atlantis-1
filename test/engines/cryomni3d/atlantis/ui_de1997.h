#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_DE1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_DE1997_H

// ui audit: the German CD of August 1997 (atlantis-cd-de-687104), as
// atlantis-re experiment 2026-10-18-07 read it and decision 0021 chose:
//   A1 the build entry, told apart from atlantis-cd-fr-687104 (same size)
//      by its PE timestamp 0x33f83e81: fr-687104's flags, its own player
//      row `NEUER SPIELER` and credits table 0x004866d4 (one byte below
//      fr-687104's), the other tables at fr-687104's addresses;
//   C3 `ß` (0xe1) in EPI.TXT: FONTMIN glyph 193, past the font, drawn as
//      nothing with the 7 px of credits byte 0x5c (the original); two 's'
//      under kEnhMinorBugFixes;
//   subtitles: the GDI breaker (fr-687104's FUN_0042e84c) on the German
//   lines, against the emulated golden video_subtitles_de687104.txt.
//
// Port code exercised directly: atlantisExeBuild / readPeTimestamp
// (exe_build.h), readNewSoundNames (exe_newsound.h), fontMinAdvance,
// fontMinBlits, fontMinLoadListWidth, sprTextSharpSAsSs (menu_layout.cpp),
// epiEpisodeName (saveload_format.cpp), splitSubtitle (subtitle_layout.cpp).
// Real files: `de1997` (ATLANTIS.EXE and the four German BIGCD*.BIG),
// `fr1997` and `data` in atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "video_subtitles_en1997.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/array.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/ustr.h"

#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/exe_newsound.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/saveload_format.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiDe1997 {

using namespace CryOmni3D::Atlantis;

static const uint32 kDeTimestamp = 0x33f83e81;
static const uint32 kFrTimestamp = 0x33ef1516;

inline bool readExeIn(const Common::String &dir, Common::Array<byte> &out) {
	if (dir.empty())
		return false;
	Common::File f;
	if (!f.open(Common::FSNode(Common::Path(dir + "/ATLANTIS.EXE", '/'))) &&
	        !f.open(Common::FSNode(Common::Path(dir + "/atlantis.exe", '/'))))
		return false;
	out.resize((uint)f.size());
	return f.read(out.data(), out.size()) == out.size();
}

// The build the port picks for an executable's bytes (size + PE timestamp,
// as loadAtlantisExeTables).
inline const AtlantisExeBuild &buildOf(const Common::Array<byte> &e) {
	Common::MemoryReadStream ms(e.data(), e.size());
	return atlantisExeBuild(e.size(), readPeTimestamp(ms));
}

inline Common::String cstr(const Common::Array<byte> &b, uint32 off) {
	Common::String s;
	while (off < b.size() && b[off])
		s += (char)b[off++];
	return s;
}

} // End of namespace UiDe1997

class UiDe1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// A1: the entry by size and timestamp, fr-687104's flags, its own
	// player row and credits table.
	void test_build_entry() {
		using namespace UiDe1997;
		const AtlantisExeBuild &de = atlantisExeBuild(687104, kDeTimestamp);
		const AtlantisExeBuild &fr = atlantisExeBuild(687104, kFrTimestamp);
		TS_ASSERT_EQUALS(Common::String(de.name), "atlantis-cd-de-687104");
		TS_ASSERT_EQUALS(Common::String(fr.name), "atlantis-cd-fr-687104");
		// the size alone or an unknown timestamp: fr-687104 (the first)
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(687104).name), "atlantis-cd-fr-687104");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(687104, 0).name), "atlantis-cd-fr-687104");
		TS_ASSERT_EQUALS(de.size, 687104u);
		TS_ASSERT_EQUALS(de.dgroupVA, 0x00480000u);
		TS_ASSERT_EQUALS(de.dgroupRawPtr, 0x6ba00u);
		TS_ASSERT_EQUALS(de.inventoryVA, 0x00486788u);
		TS_ASSERT_EQUALS(de.guestVA, 0x0048685cu);
		TS_ASSERT_EQUALS(de.creditWidthsVA, 0x004866d4u);
		TS_ASSERT_EQUALS(fr.creditWidthsVA, 0x004866d5u);
		TS_ASSERT_EQUALS(de.newSoundTableVA, 0x004861ccu);
		TS_ASSERT(de.newSoundPtrMin == 0x004a0000u && de.newSoundPtrMax == 0x004b0000u);
		TS_ASSERT_EQUALS(de.playerRowVA, 0x00486546u);
		TS_ASSERT(!de.playerLabelAfterMarker);
		// every flag as fr-687104's
		TS_ASSERT_EQUALS(de.cursorIdRule, fr.cursorIdRule);
		TS_ASSERT_EQUALS(de.rheaCamera2, fr.rheaCamera2);
		TS_ASSERT_EQUALS(de.altGameOverNarration, fr.altGameOverNarration);
		TS_ASSERT_EQUALS(de.bitmapSubtitles, fr.bitmapSubtitles);
		TS_ASSERT_EQUALS(de.subtitleClickCycle, fr.subtitleClickCycle);
		TS_ASSERT_EQUALS(de.playerListVerbatim, fr.playerListVerbatim);
		TS_ASSERT_EQUALS(de.upperCaseNames, fr.upperCaseNames);
		TS_ASSERT_EQUALS(de.gameOverVoiceAtEntry, fr.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(de.loadPickerKnobAtBar, fr.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(de.loadPickerClamp, fr.loadPickerClamp);
		TS_ASSERT_EQUALS(de.hexaForcesObject402, fr.hexaForcesObject402);
		TS_ASSERT_EQUALS(de.ogreRevealFrames, fr.ogreRevealFrames);
		TS_ASSERT_EQUALS(de.ogreRevealTicks, fr.ogreRevealTicks);
		TS_ASSERT_EQUALS(de.ratFrameDeadline, fr.ratFrameDeadline);
		TS_ASSERT_EQUALS(de.ratT0023Restart, fr.ratT0023Restart);
		TS_ASSERT_EQUALS(de.musicFirstValueRule, fr.musicFirstValueRule);
		TS_ASSERT_EQUALS(de.muriaPlacedFrames, fr.muriaPlacedFrames);
		TS_ASSERT_EQUALS(de.musicLoadAllOrNothing, fr.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(de.musicLoadMinVolume, fr.musicLoadMinVolume);
		TS_ASSERT_EQUALS(de.musicStartSameTrackNoop, fr.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(de.conCodePage, Common::kWindows1252);
	}

	// A1 on the bytes: the German executable is picked by its timestamp;
	// its DGROUP section; the tables at its addresses hold fr-687104's
	// bytes (the credits table one byte lower), the player row is
	// `NEUER SPIELER` with the names array at 0x00486554, the newsound
	// names are the reference's 137.  Read at fr-687104's credits address,
	// every advance is the next glyph's (what the size-only lookup did).
	void test_tables_by_timestamp() {
		using namespace UiDe1997;
		Common::Array<byte> e, f, ref;
		if (!readExeIn(AtlantisTest::configValue("de1997"), e)) { TS_WARN("de1997 not configured; skipped"); return; }
		TS_ASSERT_EQUALS(e.size(), 687104u);
		{
			Common::MemoryReadStream ms(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(ms), kDeTimestamp);
		}
		const AtlantisExeBuild &b = buildOf(e);
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-de-687104");
		// the DGROUP section header: RVA 0x80000, raw 0x6ba00
		bool dgroup = false;
		const uint32 pe = READ_LE_UINT32(&e[0x3c]);
		const uint nsec = READ_LE_UINT16(&e[pe + 6]);
		const uint32 sec0 = pe + 0x18 + READ_LE_UINT16(&e[pe + 0x14]);
		for (uint i = 0; i < nsec; i++) {
			const byte *h = &e[sec0 + i * 40];
			if (!memcmp(h, "DGROUP", 6)) {
				dgroup = true;
				TS_ASSERT_EQUALS(READ_LE_UINT32(h + 12) + 0x00400000u, b.dgroupVA);
				TS_ASSERT_EQUALS(READ_LE_UINT32(h + 20), b.dgroupRawPtr);
			}
		}
		TS_ASSERT(dgroup);
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "NEUER SPIELER");
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486554)), "NEUER SPIELER");
		TS_ASSERT_EQUALS(b.playerRowVA + 14, 0x00486554u);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		// FONTMAX 0x004865e6, FONTMIN 0x0048666f, the credits table right
		// after FONTMIN
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e6)], kFontMaxMeasureWidths, 137));
		for (uint i = 0; i < 101; i++)
			TS_ASSERT_EQUALS((int)e[b.fileOffset(0x0048666f) + i], fontMinAdvance(i));
		TS_ASSERT_EQUALS(b.creditWidthsVA, 0x0048666fu + 101);
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		// 0xe1 (`ß`, glyph 193): credits byte 0x5c, 7 px; read one byte
		// late (fr-687104's address) it would be 5
		TS_ASSERT_EQUALS(fontMinAdvance(0xc1, credits), 7);
		const AtlantisExeBuild &frb = atlantisExeBuild(687104);
		TS_ASSERT_EQUALS(fontMinAdvance(0xc1, &e[frb.fileOffset(frb.creditWidthsVA)]), 5);

		// against the French executable: the same bytes at the same
		// addresses, but the credits table one byte lower
		if (readExeIn(AtlantisTest::configValue("fr1997"), f)) {
			const AtlantisExeBuild &fb = buildOf(f);
			TS_ASSERT_EQUALS(Common::String(fb.name), "atlantis-cd-fr-687104");
			static const uint32 kSame[][2] = {
				{ 0x00486788, 52 * 4 }, { 0x0048685c, 10 * 4 }, { 0x00485eec, 31 * 13 },
				{ 0x00486080, 16 }, { 0x004861cc, 0xb0 } };
			for (uint i = 0; i < ARRAYSIZE(kSame); i++)
				TSM_ASSERT(i, !memcmp(&e[b.fileOffset(kSame[i][0])], &f[fb.fileOffset(kSame[i][0])], kSame[i][1]));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e6)], &f[fb.fileOffset(0x004865e7)], 137));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x0048666f)], &f[fb.fileOffset(0x00486670)], 101));
			TS_ASSERT(!memcmp(credits, &f[fb.fileOffset(fb.creditWidthsVA)], 0xb3));
		}
		// the newsound names: the reference's
		if (readExeIn(AtlantisTest::dataDir(), ref)) {
			Common::Array<Common::String> refNames, names;
			Common::MemoryReadStream rs(ref.data(), ref.size()), es(e.data(), e.size());
			readNewSoundNames(rs, refNames);
			readNewSoundNames(es, names, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr, b.newSoundPtrMin,
			                  b.newSoundPtrMax);
			TS_ASSERT_EQUALS(refNames.size(), 137u);
			TS_ASSERT_EQUALS(names.size(), refNames.size());
			for (uint i = 0; i < names.size() && i < refNames.size(); i++)
				TS_ASSERT_EQUALS(names[i], refNames[i]);
		}
	}

	// C3 on synthetic text: 0xe1 alone, both ways, in the three FONTMIN
	// renderers and the load-list width.
	void test_sharp_s_rule() {
		using namespace UiDe1997;
		TS_ASSERT_EQUALS(sprTextSharpSAsSs("Au\xe1" "en"), "Aussen");
		TS_ASSERT_EQUALS(sprTextSharpSAsSs("abc"), "abc");
		const int s = fontMinAdvance('s' - 0x20);
		TS_ASSERT_EQUALS(s, 12);
		static const FontMinRenderer kRenderers[] = { kFontMinPlayerList, kFontMinNameField, kFontMinLoadList };
		for (uint r = 0; r < ARRAYSIZE(kRenderers); r++) {
			Common::Array<FontMinBlit> b;
			// faithful: one blit of glyph 193 (FONTMIN has 137: not
			// drawn), the next glyph 7 px on (credits byte 0x5c, the
			// reference's table here as the German one)
			fontMinBlits(kRenderers[r], "a\xe1" "b", 100, 50, b);
			TS_ASSERT_EQUALS(b.size(), 3u);
			if (b.size() == 3) {
				TS_ASSERT_EQUALS(b[1].idx, 0xc1);
				TS_ASSERT_EQUALS(b[2].x - b[1].x, 7);
			}
			// fixed: two 's' blits, each advancing as 's'
			fontMinBlits(kRenderers[r], "a\xe1" "b", 100, 50, b, true, nullptr, true);
			TS_ASSERT_EQUALS(b.size(), 4u);
			if (b.size() == 4) {
				TS_ASSERT_EQUALS(b[1].idx, 's' - 0x20);
				TS_ASSERT_EQUALS(b[2].idx, 's' - 0x20);
				TS_ASSERT_EQUALS(b[2].x - b[1].x, s);
				TS_ASSERT_EQUALS(b[3].x - b[2].x, s);
				TS_ASSERT_EQUALS(b[1].x, 100 + fontMinAdvance('a' - 0x20));
			}
		}
		TS_ASSERT_EQUALS(fontMinLoadListWidth("\xe1"), 7);
		TS_ASSERT_EQUALS(fontMinLoadListWidth("\xe1", nullptr, true), 2 * s);
		TS_ASSERT_EQUALS(fontMinLoadListWidth("Au\xe1" "en", nullptr, true), fontMinLoadListWidth("Aussen"));
	}

	// C3 on the German EPI.TXT: the three names with `ß` (experiment
	// 2026-10-18-07 item 4).  Faithful: glyph 193 where the 0xe1 is, 7 px
	// on, the width the sum of the bytes at FONTMIN + index in this
	// executable.  Fixed: "ss" drawn, the width (and so the centred title's
	// x, (640 - width) / 2) the "ss" text's, matching the blits' span.
	void test_epi_sharp_s_both_ways() {
		using namespace UiDe1997;
		const Common::String dir = AtlantisTest::configValue("de1997");
		Common::Array<byte> e;
		if (!readExeIn(dir, e)) { TS_WARN("de1997 not configured; skipped"); return; }
		const AtlantisExeBuild &b = buildOf(e);
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		const uint32 fm = b.fileOffset(0x0048666f);
		VideoTest::Big big(dir);
		uint32 size = 0;
		byte *epi = big.get("SPRLIST\\EPI.TXT", size);
		TS_ASSERT(epi);
		if (!epi)
			return;
		Common::Array<Common::String> withSharpS;
		int titles = 0;
		for (uint ch = 2; ch < 90; ch++) {
			const Common::String t = epiEpisodeName(epi, size, ch);
			if (t.empty())
				continue;
			titles++;
			// faithful width: the executable's bytes
			int want = 0;
			for (uint i = 0; i < t.size(); i++) {
				const byte c = (byte)t[i];
				const uint idx = c < 0x20 ? 0x20 : c - 0x20;
				want += idx == 0x20 ? 6 : e[fm + idx];
			}
			TSM_ASSERT_EQUALS(ch, fontMinLoadListWidth(t, credits), want);
			if (!t.contains((char)kSprTextSharpS)) {
				// no 0xe1: the fix changes nothing
				TS_ASSERT_EQUALS(fontMinLoadListWidth(t, credits, true), want);
				continue;
			}
			bool dup = false;
			for (uint i = 0; i < withSharpS.size(); i++)
				dup |= withSharpS[i] == t;
			if (!dup)
				withSharpS.push_back(t);
			const Common::String ss = sprTextSharpSAsSs(t);
			Common::Array<FontMinBlit> bl;
			// faithful, the LOAD list renderer
			fontMinBlits(kFontMinLoadList, t, 45, 108, bl, true, credits);
			uint k = 0;
			for (uint i = 0; i < t.size() && k < bl.size(); i++) {
				const byte c = (byte)t[i];
				if (c < 0x20 || c == 0x40)
					continue;   // glyph 0x20: not a blit (the space is one here)
				if (c == kSprTextSharpS) {
					TS_ASSERT_EQUALS(bl[k].idx, 0xc1);
					if (k + 1 < bl.size())
						TS_ASSERT_EQUALS(bl[k + 1].x - bl[k].x, 7);
				}
				k++;
			}
			// fixed: the blits are those of the "ss" text, the width too
			Common::Array<FontMinBlit> fixedBl, ssBl;
			fontMinBlits(kFontMinLoadList, t, 45, 108, fixedBl, true, credits, true);
			fontMinBlits(kFontMinLoadList, ss, 45, 108, ssBl, true, credits);
			TS_ASSERT_EQUALS(fixedBl.size(), ssBl.size());
			for (uint i = 0; i < fixedBl.size() && i < ssBl.size(); i++) {
				TS_ASSERT_EQUALS(fixedBl[i].x, ssBl[i].x);
				TS_ASSERT_EQUALS(fixedBl[i].idx, ssBl[i].idx);
				TS_ASSERT(fixedBl[i].idx != 0xc1);
			}
			const int fixedW = fontMinLoadListWidth(t, credits, true);
			TS_ASSERT_EQUALS(fixedW, fontMinLoadListWidth(ss, credits));
			uint n = 0;
			for (uint i = 0; i < t.size(); i++)
				n += (byte)t[i] == kSprTextSharpS;
			TS_ASSERT_EQUALS(fixedW, want + (int)n * (2 * fontMinAdvance('s' - 0x20, credits) - 7));
			// the title row (name field renderer) as well
			fontMinBlits(kFontMinNameField, t, (640 - fixedW) / 2, 0x50, fixedBl, true, credits, true);
			fontMinBlits(kFontMinNameField, ss, (640 - fixedW) / 2, 0x50, ssBl, true, credits);
			TS_ASSERT_EQUALS(fixedBl.size(), ssBl.size());
			for (uint i = 0; i < fixedBl.size() && i < ssBl.size(); i++)
				TS_ASSERT(fixedBl[i].x == ssBl[i].x && fixedBl[i].idx == ssBl[i].idx);
		}
		delete[] epi;
		TS_ASSERT(titles > 20);
		TS_ASSERT_EQUALS(withSharpS.size(), 3u);
		bool verlaesst = false, ausserhalb = false;
		for (uint i = 0; i < withSharpS.size(); i++) {
			const Common::String ss = sprTextSharpSAsSs(withSharpS[i]);
			verlaesst |= ss.contains("verl\x84sst den ");
			ausserhalb |= ss.hasPrefix("Ausserhalb des Palastes");
		}
		TS_ASSERT(verlaesst);
		TS_ASSERT(ausserhalb);
	}

	// The GDI breaker on German texts against the emulated golden
	// (atlantis-re tools/emu/video_subtitles_de687104.py): the port's
	// splitSubtitle gives its rows, the texts as Windows-1252 bytes.
	void test_subtitle_rows_vs_golden() {
		using namespace UiDe1997;
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_de687104.txt", lines));
		Common::Array<Common::String> in;
		in.push_back("Kurze Zeile.");
		Common::String t;
		for (int i = 0; i < 2; i++)
			t += Common::String((size_t)49, 'y') + " ";
		in.push_back(t);
		in.push_back(Common::String((size_t)99, 'x'));
		t.clear();
		for (int i = 0; i < 40; i++)
			t += "Wort ";
		t.trim();
		in.push_back(t);
		in.push_back(Common::U32String("Ich bin Seth, der Lehrling der K\xc3\xb6nigin. Sie hat mir eine \xc3\xa4u\xc3\x9f"
		                               "erst wichtige Aufgabe f\xc3\xbcr das K\xc3\xb6nigreich \xc3\xbc" "bertragen.")
		                 .encode(Common::kWindows1252));
		uint k = 0;
		for (uint i = 0; i < lines.size(); i++) {
			if (!lines[i].hasPrefix("len "))
				continue;
			TS_ASSERT(k < in.size());
			if (k >= in.size())
				break;
			TS_ASSERT_EQUALS(atoi(lines[i].c_str() + 4), (int)in[k].size());
			TS_ASSERT(lines[i].contains(" rows same: "));
			Common::Array<Common::String> want;
			Common::String rest = lines[i].substr(lines[i].find("same: ") + 6);
			while (!rest.empty()) {
				const size_t colon = rest.find(':');
				if (colon == Common::String::npos || colon + 1 >= rest.size())
					break;
				const char q = rest[colon + 1];
				const Common::String sep = Common::String(q) + " | ";
				size_t q2 = rest.find(sep, colon + 2);
				const bool last = q2 == Common::String::npos;
				if (last)
					q2 = rest.size() - 1;
				want.push_back(Common::U32String(rest.substr(colon + 2, q2 - colon - 2)).encode(Common::kWindows1252));
				if (last)
					break;
				rest = rest.substr(q2 + 4);
			}
			Common::Array<Common::String> got;
			splitSubtitle(in[k], got);
			TSM_ASSERT_EQUALS(k, got.size(), want.size());
			for (uint r = 0; r < got.size() && r < want.size(); r++)
				TSM_ASSERT_EQUALS(k, got[r], want[r]);
			k++;
		}
		TS_ASSERT_EQUALS(k, in.size());
	}

	// Every dialog line of the German CON files through splitSubtitle: the
	// golden's scan ({1: 2113, 2: 381, 3: 42, 4: 5, 5: 1} over 2542 lines,
	// none over 8 rows, 2519 bytes >= 0x80, the longest row 100 bytes in
	// CHAPI003).
	void test_subtitle_scan_german_lines() {
		const Common::String dir = AtlantisTest::configValue("de1997");
		if (dir.empty()) { TS_WARN("de1997 not configured; skipped"); return; }
		VideoTest::Big big(dir);
		TS_ASSERT(!big.empty());
		int count = 0, hist[9] = { 0 }, over8 = 0, longest = 0, high = 0;
		Common::String longestIn;
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
				CryOmni3D::Atlantis::splitSubtitle(texts[t], rows);
				count++;
				if (rows.size() < ARRAYSIZE(hist))
					hist[rows.size()]++;
				over8 += rows.size() > 8;
				for (uint i = 0; i < texts[t].size(); i++)
					high += (byte)texts[t][i] >= 0x80;
				for (uint r = 0; r < rows.size(); r++)
					if ((int)rows[r].size() > longest) {
						longest = rows[r].size();
						longestIn = cons[f];
					}
			}
		}
		TS_ASSERT_EQUALS(count, 2542);
		TS_ASSERT_EQUALS(hist[1], 2113);
		TS_ASSERT_EQUALS(hist[2], 381);
		TS_ASSERT_EQUALS(hist[3], 42);
		TS_ASSERT_EQUALS(hist[4], 5);
		TS_ASSERT_EQUALS(hist[5], 1);
		TS_ASSERT_EQUALS(over8, 0);
		TS_ASSERT_EQUALS(high, 2519);
		TS_ASSERT_EQUALS(longest, 100);
		TS_ASSERT_EQUALS(longestIn, "SCENAR\\CHAPI003.CON");
	}
};

#endif
