#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ES1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ES1997_H

// ui audit: the Spanish CD of August 1997 (atlantis-cd-es-687104), as
// atlantis-re experiment 2026-10-18-10 read it and decision 0024 chose:
//   A1 the build entry, told apart from atlantis-cd-fr-687104 and
//      atlantis-cd-de-687104 (same size) by its PE timestamp 0x340551c4:
//      fr-687104's flags but the rat T0023 search restarting at point 0
//      (FUN_004348fc @ 0x00434933, the reference's rule), de-687104's
//      table addresses (player row `Jugador nuevo`, credits 0x004866d4
//      holding the reference's bytes);
//   D  CON texts Windows-1252: the GDI breaker (fr-687104's FUN_0042e84c)
//      on the Spanish lines, against the emulated golden
//      video_subtitles_es687104.txt.
//
// Port code exercised directly: atlantisExeBuild / readPeTimestamp
// (exe_build.h), readNewSoundNames (exe_newsound.h), fontMinAdvance,
// fontMinLoadListWidth (menu_layout.cpp), epiEpisodeName
// (saveload_format.cpp), splitSubtitle (subtitle_layout.cpp), RatGame::t0023
// (rat_game.cpp).  Real files: `es1997` (ATLANTIS.EXE and the four Spanish
// BIGCD*.BIG), `fr1997`, `de1997` and `data` in atlantis_test.cfg; skipped
// when absent.

#include "atlantis_test_helper.h"
#include "ui_de1997.h"
#include "ui_en1997.h"
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

namespace UiEs1997 {

using namespace CryOmni3D::Atlantis;

static const uint32 kEsTimestamp = 0x340551c4;
static const uint32 kFrTimestamp = 0x33ef1516;
static const uint32 kDeTimestamp = 0x33f83e81;

// Rows of an emulated subtitle golden (`len N -> R rows same: ...`) as
// Windows-1252 bytes, the leading space of each row kept.
inline void goldenRows(const Common::String &line, Common::Array<Common::String> &want) {
	want.clear();
	Common::String rest = line.substr(line.find("same: ") + 6);
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
}

// Every `len` line of the golden against splitSubtitle on the inputs.
inline void checkGolden(const char *golden, const Common::Array<Common::String> &in) {
	Common::Array<Common::String> lines;
	TS_ASSERT(AtlantisTest::loadGoldenLines(golden, lines));
	uint k = 0;
	for (uint i = 0; i < lines.size(); i++) {
		if (!lines[i].hasPrefix("len "))
			continue;
		TS_ASSERT(k < in.size());
		if (k >= in.size())
			break;
		TS_ASSERT_EQUALS(atoi(lines[i].c_str() + 4), (int)in[k].size());
		TS_ASSERT(lines[i].contains(" rows same: "));
		Common::Array<Common::String> want, got;
		goldenRows(lines[i], want);
		splitSubtitle(in[k], got);
		TSM_ASSERT_EQUALS(k, got.size(), want.size());
		for (uint r = 0; r < got.size() && r < want.size(); r++)
			TSM_ASSERT_EQUALS(k, got[r], want[r]);
		k++;
	}
	TS_ASSERT_EQUALS(k, in.size());
}

// Every CON dialog line of an install through splitSubtitle: the count,
// the rows histogram (1..8), lines over 8 rows, bytes >= 0x80, the longest
// row and its file.
struct ConScan {
	int count, hist[9], over8, longest, high;
	Common::String longestIn;
};
inline bool scanConLines(const Common::String &dir, ConScan &s) {
	s.count = s.over8 = s.longest = s.high = 0;
	for (uint i = 0; i < ARRAYSIZE(s.hist); i++)
		s.hist[i] = 0;
	VideoTest::Big big(dir);
	if (big.empty())
		return false;
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
			splitSubtitle(texts[t], rows);
			s.count++;
			if (rows.size() < ARRAYSIZE(s.hist))
				s.hist[rows.size()]++;
			s.over8 += rows.size() > 8;
			for (uint i = 0; i < texts[t].size(); i++)
				s.high += (byte)texts[t][i] >= 0x80;
			for (uint r = 0; r < rows.size(); r++)
				if ((int)rows[r].size() > s.longest) {
					s.longest = rows[r].size();
					s.longestIn = cons[f];
				}
		}
	}
	return true;
}

} // End of namespace UiEs1997

class UiEs1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// A1: the entry by size and timestamp; fr-687104's flags but the T0023
	// restart; de-687104's addresses.
	void test_build_entry() {
		using namespace UiEs1997;
		const AtlantisExeBuild &es = atlantisExeBuild(687104, kEsTimestamp);
		const AtlantisExeBuild &fr = atlantisExeBuild(687104, kFrTimestamp);
		const AtlantisExeBuild &de = atlantisExeBuild(687104, kDeTimestamp);
		const AtlantisExeBuild &ref = atlantisExeBuild(714240);
		TS_ASSERT_EQUALS(Common::String(es.name), "atlantis-cd-es-687104");
		// the size alone or an unknown timestamp: fr-687104 (the first)
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(687104).name), "atlantis-cd-fr-687104");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(687104, 0x340551c5).name), "atlantis-cd-fr-687104");
		TS_ASSERT_EQUALS(es.size, 687104u);
		TS_ASSERT_EQUALS(es.dgroupVA, de.dgroupVA);
		TS_ASSERT_EQUALS(es.dgroupRawPtr, de.dgroupRawPtr);
		TS_ASSERT_EQUALS(es.inventoryVA, 0x00486788u);
		TS_ASSERT_EQUALS(es.guestVA, 0x0048685cu);
		TS_ASSERT_EQUALS(es.creditWidthsVA, 0x004866d4u);
		TS_ASSERT_EQUALS(es.creditWidthsVA, de.creditWidthsVA);
		TS_ASSERT_EQUALS(es.newSoundTableVA, 0x004861ccu);
		TS_ASSERT(es.newSoundPtrMin == 0x004a0000u && es.newSoundPtrMax == 0x004b0000u);
		TS_ASSERT_EQUALS(es.playerRowVA, 0x00486546u);
		TS_ASSERT(!es.playerLabelAfterMarker);
		// the restart: the reference's rule, not fr-687104's
		TS_ASSERT(es.ratT0023Restart);
		TS_ASSERT(!fr.ratT0023Restart);
		TS_ASSERT_EQUALS(es.ratT0023Restart, ref.ratT0023Restart);
		// every other flag as fr-687104's
		TS_ASSERT_EQUALS(es.cursorIdRule, fr.cursorIdRule);
		TS_ASSERT_EQUALS(es.rheaCamera2, fr.rheaCamera2);
		TS_ASSERT_EQUALS(es.altGameOverNarration, fr.altGameOverNarration);
		TS_ASSERT_EQUALS(es.bitmapSubtitles, fr.bitmapSubtitles);
		TS_ASSERT_EQUALS(es.subtitleClickCycle, fr.subtitleClickCycle);
		TS_ASSERT_EQUALS(es.playerListVerbatim, fr.playerListVerbatim);
		TS_ASSERT_EQUALS(es.upperCaseNames, fr.upperCaseNames);
		TS_ASSERT_EQUALS(es.gameOverVoiceAtEntry, fr.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(es.loadPickerKnobAtBar, fr.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(es.loadPickerClamp, fr.loadPickerClamp);
		TS_ASSERT_EQUALS(es.hexaForcesObject402, fr.hexaForcesObject402);
		TS_ASSERT_EQUALS(es.ogreRevealFrames, fr.ogreRevealFrames);
		TS_ASSERT_EQUALS(es.ogreRevealTicks, fr.ogreRevealTicks);
		TS_ASSERT_EQUALS(es.ratFrameDeadline, fr.ratFrameDeadline);
		TS_ASSERT_EQUALS(es.musicFirstValueRule, fr.musicFirstValueRule);
		TS_ASSERT_EQUALS(es.muriaPlacedFrames, fr.muriaPlacedFrames);
		TS_ASSERT_EQUALS(es.musicLoadAllOrNothing, fr.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(es.musicLoadMinVolume, fr.musicLoadMinVolume);
		TS_ASSERT_EQUALS(es.musicStartSameTrackNoop, fr.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(es.rpLogoVideo, fr.rpLogoVideo);
		TS_ASSERT_EQUALS(es.creditsPerRun, fr.creditsPerRun);
		TS_ASSERT_EQUALS(es.subtitleBreakBackOnly, fr.subtitleBreakBackOnly);
		TS_ASSERT_EQUALS(es.nameSwedishLetters, fr.nameSwedishLetters);
		TS_ASSERT_EQUALS(es.menuPosItemByteWidth, 22);
		TS_ASSERT_EQUALS(es.conCodePage, Common::kWindows1252);
	}

	// A1 on the bytes: picked by its timestamp; the tables at its
	// addresses; the credits table the reference's 256 bytes; the restart
	// instruction in the code.  Read at fr-687104's credits address (what
	// the size-only lookup did), `í ú ñ` advance 20 / 14 / 13 instead of
	// 10 / 14 / 14.
	void test_tables_by_timestamp() {
		using namespace UiEs1997;
		Common::Array<byte> e, f, d, ref;
		if (!UiDe1997::readExeIn(AtlantisTest::configValue("es1997"), e)) { TS_WARN("es1997 not configured; skipped"); return; }
		TS_ASSERT_EQUALS(e.size(), 687104u);
		{
			Common::MemoryReadStream ms(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(ms), kEsTimestamp);
		}
		const AtlantisExeBuild &b = UiDe1997::buildOf(e);
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-es-687104");
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
		TS_ASSERT_EQUALS(UiDe1997::cstr(e, b.fileOffset(b.playerRowVA)), "Jugador nuevo");
		TS_ASSERT_EQUALS(UiDe1997::cstr(e, b.fileOffset(0x00486554)), "Jugador nuevo");
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		// FONTMAX 0x004865e6, FONTMIN 0x0048666f, the credits table right
		// after FONTMIN
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e6)], kFontMaxMeasureWidths, 137));
		for (uint i = 0; i < 101; i++)
			TS_ASSERT_EQUALS((int)e[b.fileOffset(0x0048666f) + i], fontMinAdvance(i));
		TS_ASSERT_EQUALS(b.creditWidthsVA, 0x0048666fu + 101);
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		// 0xa1 0xa3 0xa4 (code page 850 `í ú ñ`): 10, 14, 14, the
		// reference's (fontMinAdvance without a table)
		TS_ASSERT_EQUALS(fontMinAdvance(0x81, credits), 10);
		TS_ASSERT_EQUALS(fontMinAdvance(0x83, credits), 14);
		TS_ASSERT_EQUALS(fontMinAdvance(0x84, credits), 14);
		for (uint i = 0; i <= 0xdf; i++)
			TS_ASSERT_EQUALS(fontMinAdvance(i, credits), fontMinAdvance(i));
		const AtlantisExeBuild &frb = atlantisExeBuild(687104);
		const byte *late = &e[frb.fileOffset(frb.creditWidthsVA)];
		TS_ASSERT_EQUALS(fontMinAdvance(0x81, late), 20);
		TS_ASSERT_EQUALS(fontMinAdvance(0x83, late), 14);
		TS_ASSERT_EQUALS(fontMinAdvance(0x84, late), 13);
		// FUN_004348fc @ 0x00434933: mov word ptr [0x00762b54], 0
		// (BEGTEXT VA 0x00410000 at raw 0x400)
		static const byte kRestart[] = { 0x66, 0xc7, 0x05, 0x54, 0x2b, 0x76, 0x00, 0x00, 0x00 };
		TS_ASSERT(!memcmp(&e[0x00434933 - 0x00410000 + 0x400], kRestart, sizeof(kRestart)));

		// against the reference: the credits table's 256 bytes; the
		// newsound names
		if (UiDe1997::readExeIn(AtlantisTest::dataDir(), ref)) {
			const AtlantisExeBuild &rb = atlantisExeBuild(714240);
			TS_ASSERT(!memcmp(credits, &ref[rb.fileOffset(rb.creditWidthsVA)], 256));
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
		// against the French executable: the same bytes at the same
		// addresses but the credits table one byte lower with three bytes
		// changed (0x1c 0x1e 0x1f: 17 2 10 there)
		if (UiDe1997::readExeIn(AtlantisTest::configValue("fr1997"), f)) {
			const AtlantisExeBuild &fb = UiDe1997::buildOf(f);
			TS_ASSERT_EQUALS(Common::String(fb.name), "atlantis-cd-fr-687104");
			static const uint32 kSame[][2] = {
				{ 0x00486788, 52 * 4 }, { 0x0048685c, 10 * 4 }, { 0x00485eec, 31 * 13 },
				{ 0x00486080, 16 }, { 0x0048609c, 4 }, { 0x004861cc, 0xb0 } };
			for (uint i = 0; i < ARRAYSIZE(kSame); i++)
				TSM_ASSERT(i, !memcmp(&e[b.fileOffset(kSame[i][0])], &f[fb.fileOffset(kSame[i][0])], kSame[i][1]));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e6)], &f[fb.fileOffset(0x004865e7)], 137));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x0048666f)], &f[fb.fileOffset(0x00486670)], 101));
			const byte *fc = &f[fb.fileOffset(fb.creditWidthsVA)];
			for (uint i = 0; i < 0xb3; i++)
				TSM_ASSERT_EQUALS(i, credits[i] != fc[i], i == 0x1c || i == 0x1e || i == 0x1f);
			TS_ASSERT(fc[0x1c] == 17 && fc[0x1e] == 2 && fc[0x1f] == 10);
		}
		// against the German executable: the same addresses for every table
		// the port reads
		if (UiDe1997::readExeIn(AtlantisTest::configValue("de1997"), d)) {
			const AtlantisExeBuild &db = UiDe1997::buildOf(d);
			TS_ASSERT_EQUALS(Common::String(db.name), "atlantis-cd-de-687104");
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e6)], &d[db.fileOffset(0x004865e6)], 137));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x0048666f)], &d[db.fileOffset(0x0048666f)], 101));
		}
	}

	// A1, the rat puzzle: T0023's search restarts at point 0 in this build
	// (the reference's rule); fr-687104's goes on from the slot's position.
	// The rat at x 281 over points 300, 298 ...: from position 30, this
	// build finds point 10 (280), fr-687104 steps on to 31.
	void test_rat_t0023_restart() {
		using namespace UiEs1997;
		const AtlantisExeBuild &es = atlantisExeBuild(687104, kEsTimestamp);
		const AtlantisExeBuild &fr = atlantisExeBuild(687104, kFrTimestamp);
		for (int k = 0; k < 2; k++) {
			UiEn1997::RatStub g;
			UiEn1997::fillT0023(g, 60);
			g.t0023Restart = (k == 0 ? es : fr).ratT0023Restart;
			g.slots[0x16].pos = 30;
			g.maxFrames = 0;
			g.rules.setF(0x00496ca6, 281.0f);
			g.runT0023();
			const int want = k == 0 ? 10 + 1 : 31 + 1;
			TSM_ASSERT_EQUALS(k, g.slots[0x16].pos, want);
			TSM_ASSERT_EQUALS(k, g.rules.f(0x00496ca6), 300.0f - 2 * want);
		}
	}

	// The Spanish EPI.TXT through the LOAD list's width: each title's width
	// is the sum of the bytes at FONTMIN + index in this executable (the
	// credits table read past FONTMIN for 0x85 and up); titles with `í ú
	// ñ` exist, and with fr-687104's credits address they would measure
	// differently.
	void test_epi_widths() {
		using namespace UiEs1997;
		const Common::String dir = AtlantisTest::configValue("es1997");
		Common::Array<byte> e;
		if (!UiDe1997::readExeIn(dir, e)) { TS_WARN("es1997 not configured; skipped"); return; }
		const AtlantisExeBuild &b = UiDe1997::buildOf(e);
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		const AtlantisExeBuild &frb = atlantisExeBuild(687104);
		const byte *late = &e[frb.fileOffset(frb.creditWidthsVA)];
		const uint32 fm = b.fileOffset(0x0048666f);
		VideoTest::Big big(dir);
		uint32 size = 0;
		byte *epi = big.get("SPRLIST\\EPI.TXT", size);
		TS_ASSERT(epi);
		if (!epi)
			return;
		int titles = 0, accented = 0, differ = 0;
		for (uint ch = 2; ch < 90; ch++) {
			const Common::String t = epiEpisodeName(epi, size, ch);
			if (t.empty())
				continue;
			titles++;
			int want = 0;
			bool acc = false;
			for (uint i = 0; i < t.size(); i++) {
				const byte c = (byte)t[i];
				const uint idx = c < 0x20 ? 0x20 : c - 0x20;
				want += idx == 0x20 ? 6 : e[fm + idx];
				acc |= c == 0xa1 || c == 0xa3 || c == 0xa4;
			}
			TSM_ASSERT_EQUALS(ch, fontMinLoadListWidth(t, credits), want);
			accented += acc;
			differ += fontMinLoadListWidth(t, late) != want;
		}
		delete[] epi;
		TS_ASSERT(titles > 20);
		TS_ASSERT(accented > 0);
		TS_ASSERT(differ > 0);
	}

	// D: the GDI breaker on Spanish texts against the emulated golden
	// (atlantis-re tools/emu/video_subtitles_es687104.py): the port's
	// splitSubtitle gives its rows, the texts as Windows-1252 bytes.
	void test_subtitle_rows_vs_golden() {
		using namespace UiEs1997;
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> in;
		in.push_back(Common::U32String("L\xc3\xadnea corta.").encode(Common::kWindows1252));
		Common::String t;
		for (int i = 0; i < 2; i++)
			t += Common::String((size_t)49, 'y') + " ";
		in.push_back(t);
		in.push_back(Common::String((size_t)99, 'x'));
		t.clear();
		for (int i = 0; i < 30; i++)
			t += "Palabra ";
		t.trim();
		in.push_back(t);
		in.push_back(Common::U32String("\xc2\xbfQui\xc3\xa9n eres? \xc2\xa1Soy Seth, el aprendiz de la reina! Ella me ha "
		                               "confiado una misi\xc3\xb3n muy importante para el reino de Atl\xc3\xa1ntida, "
		                               "se\xc3\xb1or.")
		                 .encode(Common::kWindows1252));
		checkGolden("video_subtitles_es687104.txt", in);
	}

	// Every dialog line of the Spanish CON files through splitSubtitle: the
	// golden's scan ({1: 2183, 2: 331, 3: 23, 4: 2, 5: 2} over 2541 lines,
	// none over 8 rows, 5484 bytes >= 0x80, the longest row 100 bytes in
	// CHAPI001).
	void test_subtitle_scan_spanish_lines() {
		using namespace UiEs1997;
		const Common::String dir = AtlantisTest::configValue("es1997");
		if (dir.empty()) { TS_WARN("es1997 not configured; skipped"); return; }
		ConScan s;
		TS_ASSERT(scanConLines(dir, s));
		TS_ASSERT_EQUALS(s.count, 2541);
		TS_ASSERT_EQUALS(s.hist[1], 2183);
		TS_ASSERT_EQUALS(s.hist[2], 331);
		TS_ASSERT_EQUALS(s.hist[3], 23);
		TS_ASSERT_EQUALS(s.hist[4], 2);
		TS_ASSERT_EQUALS(s.hist[5], 2);
		TS_ASSERT_EQUALS(s.over8, 0);
		TS_ASSERT_EQUALS(s.high, 5484);
		TS_ASSERT_EQUALS(s.longest, 100);
		TS_ASSERT_EQUALS(s.longestIn, "SCENAR\\CHAPI001.CON");
	}
};

#endif
