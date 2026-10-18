#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_IT1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_IT1997_H

// ui audit: the Italian CD of July 1997 (atlantis-cd-it-687104), as
// atlantis-re experiment 2026-10-18-10 read it and decision 0024 chose:
//   A1 the build entry, told apart from atlantis-cd-fr-687104 (same size)
//      by its PE timestamp 0x33e08f0f: fr-687104's flags, its player row
//      `Nuovo Giocatore` one byte longer, so FONTMAX, FONTMIN and the
//      credits table (0x004866d6) one byte higher than fr-687104's;
//   C2 the menu parser FUN_0041ecb4 sizes a positioned (`<x,y>`) item's
//      zone at 16 px per byte (`shl ebx,4` @ 0x0041f068) instead of 22:
//      16 by default, 22 under kEnhMinorBugFixes;
//   D  CON texts Windows-1252: the GDI breaker (FUN_0042e858 here) on the
//      Italian lines, against the emulated golden
//      video_subtitles_it687104.txt.
//
// Port code exercised directly: atlantisExeBuild / readPeTimestamp /
// menuPosItemZoneByteWidth (exe_build.h), walkMenuText, menuWalkHit,
// menuTextHitRect, MenuLayout, fontMinAdvance (menu_layout.cpp),
// splitSubtitle (subtitle_layout.cpp).  Real files: `it1997` (ATLANTIS.EXE
// and the four Italian BIGCD*.BIG), `fr1997` and `data` in
// atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "ui_de1997.h"
#include "ui_es1997.h"
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
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiIt1997 {

using namespace CryOmni3D::Atlantis;

static const uint32 kItTimestamp = 0x33e08f0f;
static const uint32 kFrTimestamp = 0x33ef1516;

// The Italian SUREMENU.TXT (BIGCD1.BIG; atlantis-re
// binaries/atlantis-cd-it-687104.md, the menu table).
static const char kSureMenu[] =
	"<>,250>SEI SICURO?\r\n/spr=5,130,220,10,10\r\n/spr=6,500,260,10,10\r\n"
	"<170,400>OK \r\n<400,400>Annulla\r\n/fin\r\n";

} // End of namespace UiIt1997

class UiIt1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// A1: the entry by size and timestamp, fr-687104's flags, its own
	// player row and table addresses; C2: 16 px per byte.
	void test_build_entry() {
		using namespace UiIt1997;
		const AtlantisExeBuild &it = atlantisExeBuild(687104, kItTimestamp);
		const AtlantisExeBuild &fr = atlantisExeBuild(687104, kFrTimestamp);
		TS_ASSERT_EQUALS(Common::String(it.name), "atlantis-cd-it-687104");
		// the size alone or an unknown timestamp: fr-687104 (the first)
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(687104).name), "atlantis-cd-fr-687104");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(687104, 0x33e08f0e).name), "atlantis-cd-fr-687104");
		TS_ASSERT_EQUALS(it.size, 687104u);
		TS_ASSERT_EQUALS(it.dgroupVA, 0x00480000u);
		TS_ASSERT_EQUALS(it.dgroupRawPtr, 0x6ba00u);
		TS_ASSERT_EQUALS(it.inventoryVA, fr.inventoryVA);
		TS_ASSERT_EQUALS(it.guestVA, fr.guestVA);
		TS_ASSERT_EQUALS(it.creditWidthsVA, 0x004866d6u);
		TS_ASSERT_EQUALS(it.newSoundTableVA, fr.newSoundTableVA);
		TS_ASSERT(it.newSoundPtrMin == fr.newSoundPtrMin && it.newSoundPtrMax == fr.newSoundPtrMax);
		TS_ASSERT_EQUALS(it.playerRowVA, 0x00486546u);
		TS_ASSERT(!it.playerLabelAfterMarker);
		// every flag as fr-687104's (the T0023 search not restarted)
		TS_ASSERT_EQUALS(it.cursorIdRule, fr.cursorIdRule);
		TS_ASSERT_EQUALS(it.rheaCamera2, fr.rheaCamera2);
		TS_ASSERT_EQUALS(it.altGameOverNarration, fr.altGameOverNarration);
		TS_ASSERT_EQUALS(it.bitmapSubtitles, fr.bitmapSubtitles);
		TS_ASSERT_EQUALS(it.subtitleClickCycle, fr.subtitleClickCycle);
		TS_ASSERT_EQUALS(it.playerListVerbatim, fr.playerListVerbatim);
		TS_ASSERT_EQUALS(it.upperCaseNames, fr.upperCaseNames);
		TS_ASSERT_EQUALS(it.gameOverVoiceAtEntry, fr.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(it.loadPickerKnobAtBar, fr.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(it.loadPickerClamp, fr.loadPickerClamp);
		TS_ASSERT_EQUALS(it.hexaForcesObject402, fr.hexaForcesObject402);
		TS_ASSERT_EQUALS(it.ogreRevealFrames, fr.ogreRevealFrames);
		TS_ASSERT_EQUALS(it.ogreRevealTicks, fr.ogreRevealTicks);
		TS_ASSERT_EQUALS(it.ratFrameDeadline, fr.ratFrameDeadline);
		TS_ASSERT_EQUALS(it.ratT0023Restart, fr.ratT0023Restart);
		TS_ASSERT(!it.ratT0023Restart);
		TS_ASSERT_EQUALS(it.musicFirstValueRule, fr.musicFirstValueRule);
		TS_ASSERT_EQUALS(it.muriaPlacedFrames, fr.muriaPlacedFrames);
		TS_ASSERT_EQUALS(it.musicLoadAllOrNothing, fr.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(it.musicLoadMinVolume, fr.musicLoadMinVolume);
		TS_ASSERT_EQUALS(it.musicStartSameTrackNoop, fr.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(it.rpLogoVideo, fr.rpLogoVideo);
		TS_ASSERT_EQUALS(it.creditsPerRun, fr.creditsPerRun);
		TS_ASSERT_EQUALS(it.subtitleBreakBackOnly, fr.subtitleBreakBackOnly);
		TS_ASSERT_EQUALS(it.nameSwedishLetters, fr.nameSwedishLetters);
		TS_ASSERT_EQUALS(it.conCodePage, Common::kWindows1252);
		// C2: 16 per byte here, 22 in every other build; 22 under the minor
		// bug fixes
		TS_ASSERT_EQUALS(it.menuPosItemByteWidth, 16);
		TS_ASSERT_EQUALS(menuPosItemZoneByteWidth(it, false), 16);
		TS_ASSERT_EQUALS(menuPosItemZoneByteWidth(it, true), 22);
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			if (&b == &it)
				continue;
			TSM_ASSERT_EQUALS(b.name, b.menuPosItemByteWidth, 22);
			TSM_ASSERT_EQUALS(b.name, menuPosItemZoneByteWidth(b, false), 22);
			TSM_ASSERT_EQUALS(b.name, menuPosItemZoneByteWidth(b, true), 22);
		}
	}

	// A1 and C2 on the bytes: picked by its timestamp; the tables at its
	// addresses (fr-687104's bytes one byte higher from FONTMAX to the
	// credits); the 16-px zone instruction where fr-687104 multiplies by
	// 0x16.  Read at fr-687104's credits address (the size-only lookup),
	// `ú ñ` advance 20 / 2 instead of 2 / 10.
	void test_tables_by_timestamp() {
		using namespace UiIt1997;
		Common::Array<byte> e, f, ref;
		if (!UiDe1997::readExeIn(AtlantisTest::configValue("it1997"), e)) { TS_WARN("it1997 not configured; skipped"); return; }
		TS_ASSERT_EQUALS(e.size(), 687104u);
		{
			Common::MemoryReadStream ms(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(ms), kItTimestamp);
		}
		const AtlantisExeBuild &b = UiDe1997::buildOf(e);
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-it-687104");
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
		TS_ASSERT_EQUALS(UiDe1997::cstr(e, b.fileOffset(b.playerRowVA)), "Nuovo Giocatore");
		TS_ASSERT_EQUALS(UiDe1997::cstr(e, b.fileOffset(0x00486556)), "Nuovo Giocatore");
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e8)], kFontMaxMeasureWidths, 137));
		for (uint i = 0; i < 101; i++)
			TS_ASSERT_EQUALS((int)e[b.fileOffset(0x00486671) + i], fontMinAdvance(i));
		TS_ASSERT_EQUALS(b.creditWidthsVA, 0x00486671u + 101);
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		TS_ASSERT_EQUALS(fontMinAdvance(0x81, credits), 17);
		TS_ASSERT_EQUALS(fontMinAdvance(0x83, credits), 2);
		TS_ASSERT_EQUALS(fontMinAdvance(0x84, credits), 10);
		const AtlantisExeBuild &frb = atlantisExeBuild(687104);
		const byte *early = &e[frb.fileOffset(frb.creditWidthsVA)];
		TS_ASSERT_EQUALS(fontMinAdvance(0x83, early), 20);
		TS_ASSERT_EQUALS(fontMinAdvance(0x84, early), 2);
		// FUN_0041ecb4 @ 0x0041f068: mov ebx,[ebp-0x28]; shl ebx,4 (BEGTEXT
		// VA 0x00410000 at raw 0x400); the FONTMAX 0xa8 test @ 0x0041f235:
		// cmp dword [ebp-0x28],0xa8
		static const byte kShl[] = { 0x8b, 0x5d, 0xd8, 0xc1, 0xe3, 0x04 };
		TS_ASSERT(!memcmp(&e[0x0041f068 - 0x00410000 + 0x400], kShl, sizeof(kShl)));
		static const byte kCmp[] = { 0x81, 0x7d, 0xd8, 0xa8, 0x00, 0x00, 0x00 };
		TS_ASSERT(!memcmp(&e[0x0041f235 - 0x00410000 + 0x400], kCmp, sizeof(kCmp)));

		if (UiDe1997::readExeIn(AtlantisTest::configValue("fr1997"), f)) {
			const AtlantisExeBuild &fb = UiDe1997::buildOf(f);
			TS_ASSERT_EQUALS(Common::String(fb.name), "atlantis-cd-fr-687104");
			static const uint32 kSame[][2] = {
				{ 0x00486788, 52 * 4 }, { 0x0048685c, 10 * 4 }, { 0x00485eec, 31 * 13 },
				{ 0x00486080, 16 }, { 0x0048609c, 4 }, { 0x004861cc, 0xb0 } };
			for (uint i = 0; i < ARRAYSIZE(kSame); i++)
				TSM_ASSERT(i, !memcmp(&e[b.fileOffset(kSame[i][0])], &f[fb.fileOffset(kSame[i][0])], kSame[i][1]));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e8)], &f[fb.fileOffset(0x004865e7)], 137));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x00486671)], &f[fb.fileOffset(0x00486670)], 101));
			TS_ASSERT(!memcmp(credits, &f[fb.fileOffset(fb.creditWidthsVA)], 0xb2));
			// fr-687104 at the same place: imul ebx,[ebp-0x28],0x16
			static const byte kImul[] = { 0x6b, 0x5d, 0xd8, 0x16 };
			TS_ASSERT(!memcmp(&f[0x0041f068 - 0x00410000 + 0x400], kImul, sizeof(kImul)));
		}
		if (UiDe1997::readExeIn(AtlantisTest::dataDir(), ref)) {
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

	// C2 both ways on the Italian SUREMENU.TXT through the menu walk: the
	// `OK ` record (x 159) is 48 px wide at 16 per byte, 66 at 22; the
	// `Annulla` record (x 389) 112 / 154.  The drawn `OK` ends at 218
	// (FONTMAX O 28 + K 20), `Annulla` at 519: their right ends answer only
	// at 22 px per byte (the minor bug fixes); their left parts both ways.
	void test_menu_zones_both_ways() {
		using namespace UiIt1997;
		const byte *t = (const byte *)kSureMenu;
		const uint32 n = sizeof(kSureMenu) - 1;
		const int menu = menuNumber("SUREMENU.TXT");
		TS_ASSERT_EQUALS(menu, 6);
		const AtlantisExeBuild &it = atlantisExeBuild(687104, kItTimestamp);
		for (int fix = 0; fix < 2; fix++) {
			const int bw = menuPosItemZoneByteWidth(it, fix != 0);
			MenuWalk w;
			walkMenuText(t, n, menu, -1, 0, 0, w, bw);
			TS_ASSERT_EQUALS(w.records.size(), 5u);
			if (w.records.size() != 5)
				continue;
			TS_ASSERT_EQUALS(w.records[3].x, 159);
			TS_ASSERT_EQUALS(w.records[3].y, 372);
			TS_ASSERT_EQUALS(w.records[3].w, fix ? 66 : 48);
			TS_ASSERT_EQUALS(w.records[4].x, 389);
			TS_ASSERT_EQUALS(w.records[4].w, fix ? 154 : 112);
			// the drawn text: the last glyph op of each record
			int okEnd = 0, annullaEnd = 0;
			for (uint i = 0; i < w.ops.size(); i++) {
				if (w.ops[i].sprite)
					continue;
				const int end = w.ops[i].x + kFontMaxMeasureWidths[w.ops[i].index];
				if (w.ops[i].record == 3)
					okEnd = end;
				else if (w.ops[i].record == 4)
					annullaEnd = end;
			}
			TS_ASSERT_EQUALS(okEnd, 218);
			TS_ASSERT_EQUALS(annullaEnd, 519);
			TS_ASSERT_EQUALS(menuWalkHit(w.records, 175, 390), 3);
			TS_ASSERT_EQUALS(menuWalkHit(w.records, 420, 390), 4);
			TS_ASSERT_EQUALS(menuWalkHit(w.records, 210, 390), fix ? 3 : -1);
			TS_ASSERT_EQUALS(menuWalkHit(w.records, 510, 390), fix ? 4 : -1);
			// the zone's last answering x: left + w - 1
			TS_ASSERT_EQUALS(menuWalkHit(w.records, 159 + (fix ? 66 : 48) - 1, 390), 3);
			TS_ASSERT_EQUALS(menuWalkHit(w.records, 159 + (fix ? 66 : 48), 390), -1);
		}
		// the parsed layout's rect (the sound menu's hit test) agrees
		Common::MemoryReadStream ms(t, n);
		MenuLayout l;
		TS_ASSERT(l.loadFromStream(ms));
		int ok = -1;
		for (uint i = 0; i < l.items.size(); i++)
			if (l.items[i].kind == MenuItem::kText && !l.items[i].centerX && l.items[i].anchorX == 170)
				ok = (int)i;
		TS_ASSERT(ok >= 0);
		if (ok >= 0) {
			const MenuItem &mi = l.items[ok];
			TS_ASSERT_EQUALS(mi.rawLen, 3);
			const Common::Rect r16 = menuTextHitRect(mi, 170, 16), r22 = menuTextHitRect(mi, 170, 22);
			TS_ASSERT_EQUALS(r16.right, 159 + 48);
			TS_ASSERT_EQUALS(r22.right, 159 + 66);
			TS_ASSERT(!r16.contains(210, 390));
			TS_ASSERT(r22.contains(210, 390));
			TS_ASSERT_EQUALS(menuTextHitRect(mi, 170).right, r22.right);
		}
	}

	// C2 on the Italian menus as shipped: every positioned item's record is
	// its bytes to the CR x 16 (x 22 under the minor bug fixes); the
	// centred items keep the measured width either way.  The SONMENU
	// labels `Volume Musica Effetti Voci Sinistra Destra`: 96 96 112 64 128
	// 96 (132 132 154 88 176 132).
	void test_italian_menus_zone_widths() {
		using namespace UiIt1997;
		const Common::String dir = AtlantisTest::configValue("it1997");
		if (dir.empty()) { TS_WARN("it1997 not configured; skipped"); return; }
		VideoTest::Big big(dir);
		static const char *const kMenus[] = { "SUREMENU", "LOADMENU", "OPTMENU", "SONMENU", "MAINMENU", "SELEMENU" };
		int positioned = 0;
		for (uint m = 0; m < ARRAYSIZE(kMenus); m++) {
			uint32 size = 0;
			byte *t = big.get(Common::String::format("SPRLIST\\%s.TXT", kMenus[m]), size);
			TSM_ASSERT(kMenus[m], t);
			if (!t)
				continue;
			// no byte above 0xa8 (the 0xa8 space rule changes nothing)
			for (uint32 i = 0; i < size; i++)
				TSM_ASSERT(kMenus[m], t[i] <= 0x97);
			MenuWalk w16, w22;
			walkMenuText(t, size, menuNumber(kMenus[m]), -1, 0, 0, w16, 16);
			walkMenuText(t, size, menuNumber(kMenus[m]), -1, 0, 0, w22, 22);
			TS_ASSERT_EQUALS(w16.records.size(), w22.records.size());
			// the records' order: the file's `<` lines and /spr lines
			uint rec = 0;
			for (uint32 p = 0; p < size && rec < w16.records.size(); p++) {
				if (p && t[p - 1] != '\n')
					continue;
				if (t[p] == '/' && p + 4 < size && !memcmp(t + p, "/spr", 4)) {
					TS_ASSERT_EQUALS(w16.records[rec].w, w22.records[rec].w);
					rec++;
				} else if (t[p] == '<') {
					if (p + 1 < size && t[p + 1] == '>') {
						// `<>,Y>`: centred, the measured width either way
						TS_ASSERT_EQUALS(w16.records[rec].w, w22.records[rec].w);
					} else {
						uint32 q = p;
						while (q < size && t[q] != '>')
							q++;
						uint32 cr = ++q;
						while (cr < size && t[cr] != 0x0d)
							cr++;
						TS_ASSERT_EQUALS((int)w16.records[rec].w, 16 * (int)(cr - q));
						TS_ASSERT_EQUALS((int)w22.records[rec].w, 22 * (int)(cr - q));
						positioned++;
					}
					rec++;
				}
			}
			TSM_ASSERT_EQUALS(kMenus[m], rec, w16.records.size());
			if (!strcmp(kMenus[m], "SONMENU")) {
				// 9 sprites after the title: labels are records 10..15
				static const int kW16[] = { 96, 96, 112, 64, 128, 96 };
				for (uint i = 0; i < ARRAYSIZE(kW16) && 10 + i < w16.records.size(); i++) {
					TS_ASSERT_EQUALS(w16.records[10 + i].w, kW16[i]);
					TS_ASSERT_EQUALS(w22.records[10 + i].w, kW16[i] / 16 * 22);
				}
			}
			delete[] t;
		}
		// SURE 2, LOAD 2, OPT 4, SON 8
		TS_ASSERT_EQUALS(positioned, 16);
	}

	// D: the GDI breaker on Italian texts against the emulated golden
	// (atlantis-re tools/emu/video_subtitles_it687104.py).
	void test_subtitle_rows_vs_golden() {
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> in;
		in.push_back(Common::U32String("Riga corta, per\xc3\xb2.").encode(Common::kWindows1252));
		Common::String t;
		for (int i = 0; i < 2; i++)
			t += Common::String((size_t)49, 'y') + " ";
		in.push_back(t);
		in.push_back(Common::String((size_t)99, 'x'));
		t.clear();
		for (int i = 0; i < 30; i++)
			t += "Parola ";
		t.trim();
		in.push_back(t);
		in.push_back(Common::U32String("Sono Seth, l'apprendista della regina. Mi ha affidato una missione pi\xc3\xb9 "
		                               "importante di qualsiasi altra: \xc3\xa8 cos\xc3\xac, perch\xc3\xa9 \xc3\xa8 lei.")
		                 .encode(Common::kWindows1252));
		UiEs1997::checkGolden("video_subtitles_it687104.txt", in);
	}

	// Every dialog line of the Italian CON files through splitSubtitle: the
	// golden's scan ({1: 2171, 2: 337, 3: 27, 4: 6} over 2541 lines, none
	// over 8 rows, 1472 bytes >= 0x80, the longest row 100 bytes in
	// CHAPI003).
	void test_subtitle_scan_italian_lines() {
		const Common::String dir = AtlantisTest::configValue("it1997");
		if (dir.empty()) { TS_WARN("it1997 not configured; skipped"); return; }
		UiEs1997::ConScan s;
		TS_ASSERT(UiEs1997::scanConLines(dir, s));
		TS_ASSERT_EQUALS(s.count, 2541);
		TS_ASSERT_EQUALS(s.hist[1], 2171);
		TS_ASSERT_EQUALS(s.hist[2], 337);
		TS_ASSERT_EQUALS(s.hist[3], 27);
		TS_ASSERT_EQUALS(s.hist[4], 6);
		TS_ASSERT_EQUALS(s.hist[5], 0);
		TS_ASSERT_EQUALS(s.over8, 0);
		TS_ASSERT_EQUALS(s.high, 1472);
		TS_ASSERT_EQUALS(s.longest, 100);
		TS_ASSERT_EQUALS(s.longestIn, "SCENAR\\CHAPI003.CON");
	}
};

#endif
