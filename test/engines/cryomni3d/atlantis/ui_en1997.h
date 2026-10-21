#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_EN1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_EN1997_H

// ui audit: the behaviours of the English retail build of 1997
// (atlantis-cd-en-688128) that differ from the reference, as atlantis-re
// experiment 2026-10-18-01 read them in the code (STATIC), each against
// the reference's on the same input:
//   5  SUBTITLES a plain toggle (the build flag; the click cycle is
//      subtitleToggleClick, ui_menus.h)
//   6  name entry 0x20..0x7a upper-cased; '#' drawn in the player list
//   7  LOAD picker knob: starts at y 0, no clamp at 0x7b
//   8  hexagon: no forcing of object 402 (and its kEnhMinorBugFixes fix)
//   9  ogre queen reveal 5 frames x 75 ticks
//   10 rat frame deadline; T0023's search not restarted at point 0
//   12 LOADMUSIK / DELMUSIK first-value rule
//   13 Muria: one frame before the hook (the build flag)
// and the 16-record menu table (Fable pass 2 opus-gap2-per-build-flags#0):
// against the emulated golden ui_menu_record_table_en688128.txt.
//
// Port code exercised directly: nameEntryChar, fontMinBlits,
// loadPickerScrollInit / loadPickerScrollFrame (menu_layout.cpp),
// HexaGame::setUp, OgreDraw::reveal, RatGame (frame pacing, t0023),
// conMusicApply (con_eval.cpp), atlantisExeBuild (exe_build.h),
// MenuRecordTable / walkMenuText / soundMenuTableWalk /
// loadPickerKnobRecord (menu_layout.cpp, through ui_menu_record_table.h).

#include "atlantis_test_helper.h"
#include "ui_menu_record_table.h"
#include "../../../system/null_osystem.h"

#include "common/array.h"

#include "cryomni3d/atlantis/con_eval.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/hexa_game.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/ogre_draw.h"
#include "cryomni3d/atlantis/rat_game.h"

namespace UiEn1997 {

using namespace CryOmni3D::Atlantis;

// A hexagon game with no screen, no input and an empty map.
class HexaStub : public HexaGame {
public:
	using HexaGame::setUp;
protected:
	void startInput(int &mx, int &my, bool &click, bool &right) override { mx = my = 0; click = right = false; }
	void boundary(int &mx, int &my, bool &click, bool &right) override { startInput(mx, my, click, right); }
	void reloadMap() override {}
	void drawBackground() override {}
	void drawSprite(char, int, int, int) override {}
	void drawStandardCursor(int, int) override {}
	void present() override {}
	void playSound(int) override {}
};

// A rat game on a clock: each present costs `cost` ticks; waitUntil moves
// the clock to its target; the times of the presents are kept; the run
// stops after `frames` frames.
class RatStub : public RatGame {
public:
	uint32 clock = 1000, cost = 2;
	int frames = 0, maxFrames = 1;
	Common::Array<uint32> presents;

	void runT0023() { t0023(); }

protected:
	void readInput(int &mx, int &my, bool &click, bool &held) override {
		mx = 300;
		my = 470;
		click = held = false;
		if (frames++ >= maxFrames)
			rules.result = 99;
	}
	void drawBackground(int) override {}
	void drawSprite(char, int, int, int) override {}
	void present() override {
		clock += cost;
		presents.push_back(clock);
	}
	void playSound(int) override {}
	int randomValue() override { return 1; }
	uint32 ticks() override { return clock; }
	void waitUntil(uint32 t) override { if (clock < t) clock = t; }
};

// Slot 22 (T0023): n points, x from 300 down by 2.
inline void fillT0023(RatGame &g, int n) {
	RatGame::Slot &s = g.slots[0x16];
	s.b.resize(n);
	s.x.resize(n);
	s.y.resize(n);
	for (int i = 0; i < n; i++) {
		s.b[i] = 1;
		s.x[i] = 300.0f - 2 * i;
		s.y[i] = 200.0f + i;
	}
}

} // End of namespace UiEn1997

class UiEn1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// 5, 13: the plain SUBTITLES toggle and the Muria frame count are build
	// flags read by the menu and the puzzle.
	void test_build_flags() {
		using namespace CryOmni3D::Atlantis;
		TS_ASSERT(!atlantisExeBuild(688128).subtitleClickCycle);
		TS_ASSERT(atlantisExeBuild(714240).subtitleClickCycle);
		TS_ASSERT_EQUALS(atlantisExeBuild(688128).muriaPlacedFrames, 1);
		TS_ASSERT_EQUALS(atlantisExeBuild(714240).muriaPlacedFrames, 2);
	}

	// 6: FUN_0041d6d0 @ 0x0041dab1..0x0041daf8 takes 0x20..0x7a and stores
	// a..z as A..Z; the reference (FUN_0041dea4 @ 0x0041e283) 0x20..0xa7 as
	// typed.
	void test_name_entry() {
		using CryOmni3D::Atlantis::nameEntryChar;
		TS_ASSERT_EQUALS(nameEntryChar('a', true), 'A');
		TS_ASSERT_EQUALS(nameEntryChar('z', true), 'Z');
		TS_ASSERT_EQUALS(nameEntryChar('Q', true), 'Q');
		TS_ASSERT_EQUALS(nameEntryChar('#', true), '#');
		TS_ASSERT_EQUALS(nameEntryChar(' ', true), ' ');
		TS_ASSERT_EQUALS(nameEntryChar(0x7b, true), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0xe9, true), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0x1f, true), -1);
		TS_ASSERT_EQUALS(nameEntryChar('a', false), 'a');
		TS_ASSERT_EQUALS(nameEntryChar(0xa7, false), 0xa7);
		TS_ASSERT_EQUALS(nameEntryChar(0xa8, false), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0x1f, false), -1);
	}

	// 6: FUN_0041fff4 has no '#' rule (the reference's FUN_00420d8c skips
	// '#' @ 0x00420e7e, 0x00420eff).
	void test_player_list_hash() {
		using namespace CryOmni3D::Atlantis;
		Common::Array<FontMinBlit> ref, b97;
		fontMinBlits(kFontMinPlayerList, "A#B", 100, 50, ref);
		fontMinBlits(kFontMinPlayerList, "A#B", 100, 50, b97, false);
		TS_ASSERT_EQUALS(ref.size(), 2u);
		TS_ASSERT_EQUALS(b97.size(), 3u);
		if (b97.size() == 3 && ref.size() == 2) {
			TS_ASSERT_EQUALS(b97[1].idx, '#' - 0x20);
			TS_ASSERT_EQUALS(b97[2].x, ref[1].x + fontMinAdvance('#' - 0x20));
		}
	}

	// 7: FUN_0041c9a8: the knob starts at y 0 (0x0041cc23), and above the
	// bar (mouse y 0x7c..0x84) it stays at mouse y - 10 with the offset
	// (knob y - 0x7b) / step truncated toward zero, as a byte; the
	// reference puts it back at 0x7b, offset 0 (0x0041d2d0..0x0041d2e6).
	void test_load_picker_knob() {
		using namespace CryOmni3D::Atlantis;
		LoadPickerScroll ref, b97;
		loadPickerScrollInit(20, ref);
		loadPickerScrollInit(20, b97, false);
		TS_ASSERT_EQUALS(ref.knobY, 0x7b);
		TS_ASSERT_EQUALS(b97.knobY, 0);
		TS_ASSERT_EQUALS(b97.step, 0xd7 / 10);
		loadPickerScrollFrame(ref, true, true, 0x7c);
		loadPickerScrollFrame(b97, true, true, 0x7c, false);
		TS_ASSERT_EQUALS(ref.knobY, 0x7b);
		TS_ASSERT_EQUALS(ref.offset, 0);
		TS_ASSERT_EQUALS(b97.knobY, 0x7c - 10);
		TS_ASSERT_EQUALS(b97.offset, (uint8)((0x7c - 10 - 0x7b) / (0xd7 / 10)));   // 0
		// A long list (step 2): -9 / 2 = -4, the byte 252.
		LoadPickerScroll many;
		loadPickerScrollInit(100, many, false);
		TS_ASSERT_EQUALS(many.step, 2);
		loadPickerScrollFrame(many, true, true, 0x7c, false);
		TS_ASSERT_EQUALS(many.offset, (uint8)-4);
		// Below the top both are the same.
		loadPickerScrollFrame(ref, true, true, 0x100);
		loadPickerScrollFrame(b97, true, true, 0x100, false);
		TS_ASSERT_EQUALS(ref.knobY, b97.knobY);
		TS_ASSERT_EQUALS(ref.offset, b97.offset);
	}

	// 8: FUN_0043c570 tests OBJETENMAIN without setting it to 402: pieces
	// only with 402 in hand (FLAGTEMP6 = 1) or FLAGTEMP6 = 2; else class -1
	// and FLAGTEMP6 unchanged.  forceObject402 (the reference, and the 1997
	// build under kEnhMinorBugFixes) always places them.
	void test_hexa_pieces() {
		UiEn1997::HexaStub g;
		g.forceObject402 = false;
		g.objectInHand = 0;
		g.flagTemp6 = 0;
		g.setUp();
		for (int i = 0; i < CryOmni3D::Atlantis::HexaGame::kNumPieces; i++)
			TS_ASSERT_EQUALS(g.pieces[i].cls, -1);
		TS_ASSERT_EQUALS(g.flagTemp6, 0);
		g.objectInHand = 402;
		g.setUp();
		TS_ASSERT(g.pieces[0].cls >= 0);
		TS_ASSERT_EQUALS(g.flagTemp6, 1);
		TS_ASSERT_EQUALS(g.objectInHand, 0);
		UiEn1997::HexaStub h;
		h.forceObject402 = false;
		h.flagTemp6 = 2;
		h.setUp();
		TS_ASSERT(h.pieces[0].cls >= 0);
		TS_ASSERT_EQUALS(h.flagTemp6, 2);
		UiEn1997::HexaStub f;
		f.forceObject402 = true;
		f.setUp();
		for (int i = 0; i < CryOmni3D::Atlantis::HexaGame::kNumPieces; i++)
			TS_ASSERT(f.pieces[i].cls >= 0);
		TS_ASSERT_EQUALS(f.flagTemp6, 1);
	}

	// 9: FUN_0043eebc: 5 presents of 75 ticks, the cell plain / lit / plain
	// / lit / plain; the reference 3 of 200.
	void test_ogre_reveal() {
		using namespace CryOmni3D::Atlantis;
		OgreRules r;
		r.p[1].x = 3;
		r.p[1].y = 3;
		for (int b = 0; b < 2; b++) {
			const int frames = b ? 5 : 3, ticks = b ? 75 : 200;
			OgreDraw::Events ev;
			if (b)
				OgreDraw::reveal(ev, r, 5, 75);
			else
				OgreDraw::reveal(ev, r);
			int presents = 0, total = 0;
			Common::Array<int> lit;
			for (uint i = 0; i < ev.size(); i++) {
				if (ev[i].kind == OgreEvent::kPresent)
					presents++;
				if (ev[i].kind == OgreEvent::kWait)
					total += ev[i].frame;
				// the queen's cell: frame y * 7 + x + 0x34, lit 0x31 less
				if (ev[i].kind == OgreEvent::kSprite && ev[i].set == 'P') {
					if (ev[i].frame == 3 * 7 + 3 + 0x34)
						lit.push_back(0);
					else if (ev[i].frame == 3 * 7 + 3 + 3)
						lit.push_back(1);
				}
			}
			TS_ASSERT_EQUALS(presents, frames);
			TS_ASSERT_EQUALS(total, frames * ticks);
			TS_ASSERT_EQUALS((int)lit.size(), frames);
			for (uint i = 0; i < lit.size(); i++)
				TS_ASSERT_EQUALS(lit[i], (int)(i & 1));
		}
	}

	// 10: the frame period.  The reference waits 5 ticks after each frame
	// (FUN_00438fd4 @ 0x004393f1..0x00439409): a frame drawn in 2 ticks
	// lasts 7.  The 1997 build (FUN_00435a0c @ 0x00435df9..0x00435e0e)
	// waits for the deadline set 5 ticks after the previous wait: 5; a frame
	// drawn in 7 ticks lasts 7 (no wait), the reference 12.
	void test_rat_frame_deadline() {
		for (int cost = 2; cost <= 7; cost += 5) {
			for (int b = 0; b < 2; b++) {
				UiEn1997::RatStub g;
				UiEn1997::fillT0023(g, 60);
				g.cost = cost;
				g.maxFrames = 6;
				g.frameDeadline = b != 0;
				g.rules.setF(0x00496ca6, 400.0f);   // the rat's x, right of every point
				g.runT0023();
				TS_ASSERT(g.presents.size() >= 5u);
				const int want = b ? MAX(5, cost) : cost + 5;
				for (uint i = 2; i < g.presents.size(); i++)
					TSM_ASSERT_EQUALS(Common::String::format("cost %d build %d", cost, b).c_str(),
					                  (int)(g.presents[i] - g.presents[i - 1]), want);
			}
		}
	}

	// 10: T0023's search.  The reference sets the position to 0 and walks
	// to the first point whose x is not above the rat's.  The 1997 build
	// (FUN_00434c6c @ 0x00434c99..0x00434cd0) tests point 0, then goes on
	// from the position left in the slot.  The stub stops the run at the
	// first frame, after which the loop has stepped once (position + 1).
	void test_rat_t0023_search() {
		struct Run { bool restart; int16 pos; float ratX; int16 wantPos; };
		static const Run kRuns[] = {
			// the rat at x 281: the first point at or left of it is 10 (280)
			{ true, 30, 281.0f, 10 + 1 },
			// 1997, position 30 left: point 0 (300) is right of the rat, the
			// walk goes on from 30 to 31 (238)
			{ false, 30, 281.0f, 31 + 1 },
			// 1997, the rat right of point 0: no step, the position is kept
			{ false, 30, 350.0f, 30 + 1 },
			// 1997 from a fresh slot: as the reference
			{ false, 0, 281.0f, 10 + 1 },
			// 1997, the position left at the end (60 points): the original
			// reads past the points, the port takes the last one
			{ false, 60, 350.0f, 60 },
		};
		for (uint i = 0; i < ARRAYSIZE(kRuns); i++) {
			UiEn1997::RatStub g;
			UiEn1997::fillT0023(g, 60);
			g.t0023Restart = kRuns[i].restart;
			g.slots[0x16].pos = kRuns[i].pos;
			g.maxFrames = 0;
			g.rules.setF(0x00496ca6, kRuns[i].ratX);
			g.runT0023();
			TSM_ASSERT_EQUALS(i, g.slots[0x16].pos, kRuns[i].wantPos);
			if (kRuns[i].wantPos < 60)
				TSM_ASSERT_EQUALS(i, g.rules.f(0x00496ca6), 300.0f - 2 * kRuns[i].wantPos);
		}
	}

	// 12: FUN_00429044 @ 0x00429b12 / 0x00429b85: nothing when the first
	// value is the playing track; DelMusik (FUN_0041ee78) deletes the
	// playing track when it is not first; LoadMusik's copy (the same code
	// as the reference's) still skips it.
	void test_music_first_value_rule() {
		using CryOmni3D::Atlantis::conMusicApply;
		Common::Array<uint8> list;
		list.push_back(4);   // StartMusik 5
		list.push_back(6);   // 7
		Common::Array<uint8> copies;
		// the playing track (5) first
		uint32 disk = (1u << 4) | (1u << 6);
		conMusicApply(false, list, 5, disk, copies, true);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6));
		disk = (1u << 4) | (1u << 6);
		conMusicApply(false, list, 5, disk, copies);
		TS_ASSERT_EQUALS(disk, 1u << 4);
		disk = 0;
		conMusicApply(true, list, 5, disk, copies, true);
		TS_ASSERT_EQUALS(disk, 0u);
		TS_ASSERT(copies.empty());
		disk = 0;
		conMusicApply(true, list, 5, disk, copies);
		TS_ASSERT_EQUALS(disk, 1u << 6);
		TS_ASSERT_EQUALS(copies.size(), 1u);
		// the playing track (7) second
		copies.clear();
		disk = (1u << 4) | (1u << 6);
		conMusicApply(false, list, 7, disk, copies, true);
		TS_ASSERT_EQUALS(disk, 1u << 6);
		disk = (1u << 4) | (1u << 6);
		conMusicApply(false, list, 7, disk, copies);
		TS_ASSERT_EQUALS(disk, 1u << 6);
		disk = 0;
		conMusicApply(true, list, 7, disk, copies, true);
		TS_ASSERT_EQUALS(disk, 1u << 4);
	}

	// The menu record table, 16 records per menu (FUN_0041ef34 stride 0x81
	// @ 0x0041ef84): the record counts of the 15 menus, SONMENU's records
	// 16-17 over SUREMENU's count and records 0-1 (the ARE YOU SURE ?
	// /spr=5 then blitted at (-23039,-31743) for the session), menu 6's
	// stale hit test, the LOAD knob kept at x 0xffce, all against the
	// original under emulation (ui_menu_record_table.py).
	void test_menu_record_table_vs_golden() {
		using namespace CryOmni3D::Atlantis;
		TS_ASSERT_EQUALS(menuRecordTableRecords(atlantisExeBuild(688128), false), 16u);
		UiMenuRecordTable::checkVsGolden(AtlantisTest::configValue("en1997"),
		                                 "ui_menu_record_table_en688128.txt", atlantisExeBuild(688128));
	}

	// kEnhMinorBugFixes (decision 0016 row 35): per-menu records from zero
	// at each visit; the ornament stays at (130,220) after the sound menu
	// and the LOAD knob comes back on a visit with more than 11 files.
	void test_menu_record_table_minor_bug_fixes() {
		using namespace CryOmni3D::Atlantis;
		TS_ASSERT_EQUALS(menuRecordTableRecords(atlantisExeBuild(688128), true), 0u);
		const Common::String dir = AtlantisTest::configValue("en1997");
		UiMenuRecordTable::Texts tx;
		if (dir.empty() || !UiMenuRecordTable::loadTexts(dir, tx)) {
			TS_WARN("en1997 not configured; skipped");
			return;
		}
		MenuLayout a, son, b;
		UiMenuRecordTable::layoutFor(tx, 6, nullptr, a);
		UiMenuRecordTable::walk(a);
		UiMenuRecordTable::layoutFor(tx, 5, nullptr, son);
		soundMenuTableWalk(son, UiMenuRecordTable::kSonKnobX);   // no table: nothing
		TS_ASSERT(son.walk.records.empty());
		UiMenuRecordTable::layoutFor(tx, 6, nullptr, b);
		UiMenuRecordTable::walk(b);
		TS_ASSERT_EQUALS(UiMenuRecordTable::blits(b.walk), Common::String("(130,220,5) (500,260,6)"));
		// the same with the session table: off screen
		MenuRecordTable table(16);
		UiMenuRecordTable::layoutFor(tx, 5, &table, son);
		soundMenuTableWalk(son, UiMenuRecordTable::kSonKnobX);
		UiMenuRecordTable::layoutFor(tx, 6, &table, b);
		UiMenuRecordTable::walk(b);
		TS_ASSERT_EQUALS(UiMenuRecordTable::blits(b.walk), Common::String("(-23039,-31743,5) (500,260,6)"));
		// LOAD: 5 files then 20, each visit a fresh per-menu layout
		LoadPickerScroll sb;
		MenuLayout l1, l2, plain;
		UiMenuRecordTable::layoutFor(tx, 1, nullptr, plain);
		UiMenuRecordTable::walk(plain);
		loadPickerScrollInit(5, sb, false);
		UiMenuRecordTable::layoutFor(tx, 1, nullptr, l1);
		loadPickerKnobRecord(l1, sb);
		UiMenuRecordTable::walk(l1);
		TS_ASSERT_EQUALS(l1.walk.records[4].x, -50);
		loadPickerScrollInit(20, sb, false);
		UiMenuRecordTable::layoutFor(tx, 1, nullptr, l2);
		loadPickerKnobRecord(l2, sb);
		UiMenuRecordTable::walk(l2);
		TS_ASSERT_EQUALS(l2.walk.records[4].x, plain.walk.records[4].x);
		TS_ASSERT_DIFFERS(l2.walk.records[4].x, -50);
	}
};

#endif
