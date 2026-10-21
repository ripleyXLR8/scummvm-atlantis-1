#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_MENU_RECORD_TABLE_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_MENU_RECORD_TABLE_H

// The menu record table (one static table for the session, 15 blocks of a
// count byte and N records): the port's MenuRecordTable, walkMenuText,
// soundMenuTableWalk, loadPickerKnobRecord and menuWalkHit
// (menu_layout.cpp) print the lines of atlantis-re
// tools/emu/ui_menu_record_table.py, which runs the original walk, sprite
// record function and hit test under Unicorn (goldens
// ui_menu_record_table_{en714240,en688128,fr688640}.txt; sections counts,
// sequence, stale and loadknob; the writers section is the static scan).
// Used by ui_menus.h (the reference), ui_en1997.h and ui_frmay1997.h.

#include "atlantis_test_helper.h"
#include "video_test_big.h"

#include "common/array.h"
#include "common/memstream.h"
#include "common/str.h"

#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/menu_layout.h"

namespace UiMenuRecordTable {

using namespace CryOmni3D::Atlantis;

static const char *const kMenus[MenuRecordTable::kMenus] = {
	"MAINMENU", "LOADMENU", "OPTMENU", "NEWMENU", "SELEMENU", "SONMENU",
	"SUREMENU", "CD1MENU", "CD2MENU", "CD3MENU", "CD4MENU", "WAIT",
	"ERROR1", "ERROR2", "ERROR3" };

// SONMENU.TXT's knob records (/spr=13) x, the port's first knob positions
// (_sonMenuKnobX seeded from the text) in every release.
static const int kSonKnobX[4] = { 460, 465, 470, 490 };

struct Texts {
	Common::Array<byte> text[MenuRecordTable::kMenus];
};

inline bool loadTexts(const Common::String &dir, Texts &out) {
	VideoTest::Big big(dir);
	for (int m = 0; m < MenuRecordTable::kMenus; m++) {
		uint32 size = 0;
		byte *b = big.get(Common::String::format("SPRLIST\\%s.TXT", kMenus[m]), size);
		if (!b)
			return false;
		out.text[m] = Common::Array<byte>(b, size);
		delete[] b;
	}
	return true;
}

// The menu as loadMenuLayout builds it (menus.cpp): the file, its number,
// the session's table (null: per-menu records).
inline void layoutFor(const Texts &tx, int m, MenuRecordTable *table, MenuLayout &out) {
	Common::MemoryReadStream s(tx.text[m].data(), tx.text[m].size());
	out.loadFromStream(s);
	out.menu = m;
	out.table = table;
}

inline void walk(MenuLayout &l, int hover = -1) {
	walkMenuText(l.raw.data(), l.raw.size(), l.menu, hover, 0, 0, l.walk, 22, l.table);
}

inline Common::String recs(const Common::Array<MenuWalkRecord> &r, uint from, uint to) {
	Common::String s;
	for (uint k = from; k < to && k < r.size(); k++)
		s += Common::String::format("%s(%d,%d,%d,%d)", s.empty() ? "" : " ", r[k].x, r[k].y, r[k].w, r[k].h);
	return s;
}

inline Common::String blits(const MenuWalk &w) {
	Common::String s;
	for (uint i = 0; i < w.ops.size(); i++)
		if (w.ops[i].sprite && !w.ops[i].altBank)
			s += Common::String::format("%s(%d,%d,%d)", s.empty() ? "" : " ", w.ops[i].x, w.ops[i].y, w.ops[i].index);
	return s;
}

inline void block6(const MenuRecordTable &t, Common::Array<MenuWalkRecord> &out) {
	out.clear();
	for (uint k = 0; k < 5; k++)
		out.push_back(t.record(6, k));
}

inline void add(Common::Array<Common::String> &out, const Common::String &line) {
	Common::String l = line;
	l.trim();
	out.push_back(l);
}

// The harness's lines from the port, for a build with `n` records per menu
// and its LOAD picker knob rule.
inline void portLines(const Texts &tx, uint n, bool knobAtBar, Common::Array<Common::String> &out) {
	MenuRecordTable table(n);
	// counts
	for (int m = 0; m < MenuRecordTable::kMenus; m++) {
		table.reset(n);
		MenuLayout l;
		layoutFor(tx, m, &table, l);
		walk(l);
		const int c = table.count(m), over = c - (int)n;
		Common::String line = Common::String::format("count %-8s menu %2d records %2d", kMenus[m], m, c);
		if (over > 0)
			line += Common::String::format("  overflow %d into menu %d", over, m + 1);
		add(out, line);
	}
	// sequence: SUREMENU, SONMENU (the sound menu's walk), SUREMENU
	table.reset(n);
	static const int kSeq[3] = { 6, 5, 6 };
	for (int step = 0; step < 3; step++) {
		const int m = kSeq[step];
		MenuLayout l;
		layoutFor(tx, m, &table, l);
		if (m == 5)
			soundMenuTableWalk(l, kSonKnobX);
		else
			walk(l);
		add(out, Common::String::format("step %d %s count %d", step + 1, kMenus[m], table.count(m)));
		if (m == 5)
			add(out, Common::String::format("SONMENU records %u.. %s", n, recs(l.walk.records, n, l.walk.records.size()).c_str()));
		Common::Array<MenuWalkRecord> r6;
		block6(table, r6);
		add(out, Common::String::format("menu 6 block: count %d records 0-4 %s", table.count(6), recs(r6, 0, 5).c_str()));
		add(out, Common::String::format("SPRMENU blits (x, y, index): %s", blits(l.walk).c_str()));
	}
	// stale: menu 6's hit test before its walk and after it
	for (int setup = 0; setup < 2; setup++) {
		table.reset(n);
		Common::Array<int> prep;
		if (setup == 1)
			for (int m = 0; m < MenuRecordTable::kMenus; m++)
				prep.push_back(m);
		prep.push_back(6);
		prep.push_back(5);
		for (uint i = 0; i < prep.size(); i++) {
			MenuLayout l;
			layoutFor(tx, prep[i], &table, l);
			walk(l);
		}
		const int before = table.count(6);
		Common::Array<MenuWalkRecord> r;
		table.records(6, r);
		Common::Array<int> hb;
		for (int y = 0; y < 480; y += 4)
			for (int x = 0; x < 640; x += 4) {
				const int h = menuWalkHit(r, x, y);
				hb.push_back(h < 0 ? 255 : h);
			}
		MenuLayout l6;
		layoutFor(tx, 6, &table, l6);
		walk(l6);
		table.records(6, r);
		// (before, after) -> points, sorted as the harness's
		Common::Array<uint32> keys, counts;
		uint i = 0;
		for (int y = 0; y < 480; y += 4)
			for (int x = 0; x < 640; x += 4, i++) {
				const int h = menuWalkHit(r, x, y);
				const uint32 key = (uint32)hb[i] << 8 | (uint32)(h < 0 ? 255 : h);
				uint k = 0;
				while (k < keys.size() && keys[k] != key)
					k++;
				if (k == keys.size()) {
					keys.push_back(key);
					counts.push_back(0);
				}
				counts[k]++;
			}
		for (uint a = 0; a < keys.size(); a++)      // sort by key
			for (uint b = a + 1; b < keys.size(); b++)
				if (keys[b] < keys[a]) {
					SWAP(keys[a], keys[b]);
					SWAP(counts[a], counts[b]);
				}
		add(out, Common::String::format("stale %s: count before the walk %d, after %d",
		                                setup ? "after every menu" : "menus 7-14 empty", before, table.count(6)));
		for (uint k = 0; k < keys.size(); k++) {
			const uint b = keys[k] >> 8, a = keys[k] & 0xff;
			add(out, Common::String::format("before %3u after %3u points %u%s", b, a, counts[k], b == a ? "" : "  differs"));
		}
	}
	// loadknob: two visits of the LOAD picker, 5 then 20 files
	table.reset(n);
	static const int kFiles[2] = { 5, 20 };
	for (int v = 0; v < 2; v++) {
		LoadPickerScroll sb;
		loadPickerScrollInit(kFiles[v], sb, knobAtBar);
		MenuLayout l;
		layoutFor(tx, 1, &table, l);
		loadPickerKnobRecord(l, sb);
		walk(l);
		const Common::String r4 = recs(l.walk.records, 4, 5), bl = blits(l.walk);
		if (v == 0)
			add(out, Common::String::format("visit 1 (11 files or fewer: x 0xffce, y %d): record 4 %s blits %s",
			                                sb.knobY, r4.c_str(), bl.c_str()));
		else
			add(out, Common::String::format("visit 2 (more than 11 files: y %d only): record 4 %s blits %s",
			                                sb.knobY, r4.c_str(), bl.c_str()));
	}
}

// The golden's lines of the sections the port reproduces (counts to
// loadknob; the writers section is the static scan).
inline bool goldenLines(const char *name, Common::Array<Common::String> &out) {
	Common::Array<Common::String> all;
	if (!AtlantisTest::loadGoldenLines(name, all))
		return false;
	bool in = false;
	for (uint i = 0; i < all.size(); i++) {
		if (all[i].hasPrefix("# counts"))
			in = true;
		else if (all[i].hasPrefix("# writers"))
			in = false;
		if (in && all[i][0] != '#')
			out.push_back(all[i]);
	}
	return true;
}

// The port's lines against the golden, line by line.
inline void checkVsGolden(const Common::String &dir, const char *golden, const AtlantisExeBuild &b) {
	if (!AtlantisTest::haveData() || dir.empty()) {
		TS_WARN("golden / data not configured; skipped");
		return;
	}
	Texts tx;
	Common::Array<Common::String> want, got;
	if (!loadTexts(dir, tx) || !goldenLines(golden, want)) {
		TS_WARN("menu texts or golden missing; skipped");
		return;
	}
	portLines(tx, menuRecordTableRecords(b, false), b.loadPickerKnobAtBar, got);
	TS_ASSERT_EQUALS(got.size(), want.size());
	for (uint i = 0; i < got.size() && i < want.size(); i++)
		TSM_ASSERT_EQUALS(want[i].c_str(), got[i], want[i]);
}

} // End of namespace UiMenuRecordTable

#endif
