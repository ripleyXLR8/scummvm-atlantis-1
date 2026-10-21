#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_MENUS_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_MENUS_H

// ui audit: the sprite menus of the Traditional and Simplified Chinese CDs
// (atlantis-cd-zh-tw-696832, atlantis-cd-zh-cn-692736; atlantis-re
// experiment 2026-10-19-01 Z4 / Z5, decision 0025 F).  Their 15 SPRLIST
// menus (the error boxes ERROR1..3 and WAIT included) are `/sprc=` records
// drawn from SPRCHI1.SPR, SPRCHI2.SPR when hovered, with the options
// toggles as sprite ids; the end picture is the shipped END.TGA.
//
// Expected values: atlantis-re tools/emu/ui_menus_zh.py, which runs the walk
// FUN_0041f5a0 and the record parser FUN_004201e8 of each executable under
// Unicorn on the release's files (goldens ui_menus_zh_tw.txt /
// ui_menus_zh_cn.txt: sections walk, hover, toggles, synth; `data` the
// END.TGA bytes).  Port code exercised: walkMenuText, MenuRecordTable,
// menuRecordTableRecords, MenuLayout::loadFromStream (menu_layout.cpp /
// exe_build.h) with the build entries' menuToggleSprites and
// menuSpriteBanks; BigFileArchive (bigfile.cpp), readSprFile and readTga15
// (sprite_decode.cpp).  Real files: `zhtw`, `zhcn`, `data` and `golden` in
// atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/str.h"
#include "graphics/pixelformat.h"
#include "graphics/surface.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/sprite_decode.h"

namespace UiZhMenus {

using namespace CryOmni3D::Atlantis;

static const char *const kMenus[MenuRecordTable::kMenus] = {
	"MAINMENU", "LOADMENU", "OPTMENU", "NEWMENU", "SELEMENU", "SONMENU",
	"SUREMENU", "CD1MENU", "CD2MENU", "CD3MENU", "CD4MENU", "WAIT",
	"ERROR1", "ERROR2", "ERROR3" };

static const char *const kKeys[2] = { "zhtw", "zhcn" };
static const char *const kGoldens[2] = { "ui_menus_zh_tw.txt", "ui_menus_zh_cn.txt" };

inline const AtlantisExeBuild &build(int k) {
	return k == 0 ? atlantisExeBuild(696832, 0x35af1f09) : atlantisExeBuild(692736, 0x36f11f20);
}

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

// The menu as loadMenuLayout builds it (menus.cpp).
inline void layoutFor(const Common::Array<byte> &text, int m, MenuRecordTable *table, MenuLayout &out) {
	Common::MemoryReadStream s(text.data(), text.size());
	out.loadFromStream(s);
	out.menu = m;
	out.table = table;
}

// renderMenuLayout's walk (menus.cpp): the build's toggle rule, the
// toggles as the globals give them.
inline void walk(const AtlantisExeBuild &b, MenuLayout &l, int hover, int sub, int omni,
                 bool fix = false) {
	walkMenuText(l.raw.data(), l.raw.size(), l.menu, hover, sub, omni, l.walk,
	             b.menuPosItemByteWidth, l.table, b.menuToggleSprites, fix);
}

// The bank renderMenuLayout draws an op from: FONTMAX / FONTMAX2 glyphs,
// SPRMENU sprites, the build's menuSpriteBanks (blitMenuWord) for the
// `/sprj=` `/sprr=` `/sprc=` records.
inline Common::String bankOf(const AtlantisExeBuild &b, const MenuWalkOp &op) {
	if (!op.sprite)
		return op.hoverFont ? "FONTMAX2" : "FONTMAX";
	if (!op.altBank)
		return "SPRMENU";
	Common::String n(b.menuSpriteBanks[op.hoverBank ? 1 : 0]);
	if (n.hasSuffix(".SPR"))
		n = Common::String(n.c_str(), n.size() - 4);
	return n;
}

inline Common::String blits(const AtlantisExeBuild &b, const MenuWalk &w, bool hoveredOnly = false) {
	Common::String s;
	for (uint i = 0; i < w.ops.size(); i++) {
		const Common::String bank = bankOf(b, w.ops[i]);
		if (hoveredOnly && bank != "SPRCHI2" && bank != "FONTMAX2")
			continue;
		s += Common::String::format("%s%s(%d,%d,%d)", s.empty() ? "" : " ", bank.c_str(),
		                            w.ops[i].x, w.ops[i].y, w.ops[i].index);
	}
	return s;
}

inline Common::String recs(const Common::Array<MenuWalkRecord> &r, uint from = 0, uint to = 0xffff) {
	Common::String s;
	for (uint k = from; k < to && k < r.size(); k++)
		s += Common::String::format("%s(%d,%d,%d,%d)", s.empty() ? "" : " ", r[k].x, r[k].y, r[k].w, r[k].h);
	return s;
}

inline void add(Common::Array<Common::String> &out, const Common::String &line) {
	Common::String l = line;
	l.trim();
	out.push_back(l);
}

// The harness's walk, hover, toggles and synth lines from the port.
inline void portLines(const AtlantisExeBuild &b, const Texts &tx, Common::Array<Common::String> &out) {
	const uint n = menuRecordTableRecords(b, false);
	MenuRecordTable table(n);
	for (int m = 0; m < MenuRecordTable::kMenus; m++) {
		table.reset(n);
		MenuLayout l;
		layoutFor(tx.text[m], m, &table, l);
		walk(b, l, -1, 1, 0);
		add(out, Common::String::format("walk %-8s menu %2d count %2d records %s", kMenus[m], m,
		                                table.count(m), recs(l.walk.records).c_str()));
		add(out, "blits " + blits(b, l.walk));
	}
	for (int m = 0; m < MenuRecordTable::kMenus; m++) {
		table.reset(n);
		MenuLayout l;
		layoutFor(tx.text[m], m, &table, l);
		walk(b, l, -1, 1, 0);
		const int count = table.count(m);
		for (int h = 0; h < count; h++) {
			table.reset(n);
			walk(b, l, h, 1, 0);
			const Common::String hov = blits(b, l.walk, true);
			add(out, Common::String::format("hover %-8s menu %2d line %2d %s", kMenus[m], m, h,
			                                hov.empty() ? "-" : hov.c_str()));
		}
	}
	for (int first = 0; first < 2; first++)
		for (int sub = 0; sub < 2; sub++)
			for (int omni = 0; omni < 2; omni++) {
				table.reset(n);
				MenuLayout l;
				layoutFor(tx.text[2], 2, &table, l);
				if (first)
					walk(b, l, -1, 1, 0);
				walk(b, l, -1, sub, omni);
				add(out, Common::String::format("toggles %s SUBTITLES %d OMNI %d count %d record3 %s record5 %s",
				                                first ? "after-first-walk" : "zeroed", sub, omni, table.count(2),
				                                recs(l.walk.records, 3, 4).c_str(), recs(l.walk.records, 5, 6).c_str()));
				add(out, "blits " + blits(b, l.walk));
			}
	static const struct {
		const char *label;
		int menu;
		const char *text;
	} kSynth[] = {
		{ "id9-menu0", 0, "/sprc=9,100,200,50,32\r\n/fin\r\n" },
		{ "id9-menu2", 2, "/sprc=9,100,200,50,32\r\n/fin\r\n" },
		{ "id12-menu0", 0, "/sprc=12,100,200,50,32\r\n/fin\r\n" },
		{ "id12-menu2", 2, "/sprc=12,100,200,50,32\r\n/fin\r\n" },
		{ "banks", 0, "/sprj=3,10,20,30,40\r\n/spr=4,50,60,70,80\r\n/sprr=5,90,100,110,120\r\n/fin\r\n" },
	};
	for (uint i = 0; i < ARRAYSIZE(kSynth); i++) {
		const Common::Array<byte> text((const byte *)kSynth[i].text, strlen(kSynth[i].text));
		static const int kFlags[2][2] = { { 0, 0 }, { 1, 1 } };
		for (int f = 0; f < 2; f++)
			for (int hover = -1; hover <= 2; hover++) {
				table.reset(n);
				MenuLayout l;
				layoutFor(text, kSynth[i].menu, &table, l);
				walk(b, l, hover, kFlags[f][0], kFlags[f][1]);
				add(out, Common::String::format("synth %-10s menu %d SUBTITLES %d OMNI %d hover %2d records %s blits %s",
				                                kSynth[i].label, kSynth[i].menu, kFlags[f][0], kFlags[f][1], hover,
				                                recs(l.walk.records).c_str(), blits(b, l.walk).c_str()));
			}
	}
}

// The golden's line `data disc 4 END.TGA md5 ...`: md5 and size.
inline bool goldenEnd(const char *golden, Common::String &md5, uint32 &size) {
	Common::Array<Common::String> lines;
	if (!AtlantisTest::loadGoldenLines(golden, lines))
		return false;
	for (uint i = 0; i < lines.size(); i++) {
		if (!lines[i].hasPrefix("data disc 4 END.TGA md5 "))
			continue;
		md5 = Common::String(lines[i].c_str() + 24, 32);
		size = (uint32)atoi(lines[i].c_str() + 24 + 32 + 6);
		return true;
	}
	return false;
}

// Member `name` of `dir`'s disc `disc` archive, through the port's reader.
inline bool bigMember(const Common::String &dir, int disc, const char *name, Common::Array<byte> &out) {
	SearchMan.addDirectory("ui_zh_menus_data", Common::FSNode(Common::Path(dir, '/')));
	BigFileArchive big;
	const bool opened = big.open(Common::Path(Common::String::format("BIGCD%d.BIG", disc)));
	SearchMan.remove("ui_zh_menus_data");
	if (!opened)
		return false;
	Common::ScopedPtr<Common::SeekableReadStream> s(big.createReadStreamForMember(Common::String(name)));
	if (!s)
		return false;
	out.resize((uint)s->size());
	return s->read(out.data(), out.size()) == out.size();
}

} // End of namespace UiZhMenus

class UiZhMenusTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// F: every menu's records and blits, each line hovered, the options
	// toggles in their four states (on a zeroed table and after a first
	// walk) and the id rules on synthetic texts: the port's walk with the
	// build's rules gives the emulated original's lines.
	void test_walk_vs_golden() {
		using namespace UiZhMenus;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		for (int k = 0; k < 2; k++) {
			const Common::String dir = AtlantisTest::configValue(kKeys[k]);
			if (dir.empty()) { TS_WARN(Common::String::format("%s not configured; skipped", kKeys[k]).c_str()); continue; }
			Texts tx;
			TS_ASSERT(loadTexts(dir, tx));
			Common::Array<Common::String> golden, port;
			TS_ASSERT(AtlantisTest::loadGoldenLines(kGoldens[k], golden));
			Common::Array<Common::String> want;
			for (uint i = 0; i < golden.size(); i++)
				if (!golden[i].hasPrefix("#") && !golden[i].hasPrefix("data "))
					want.push_back(golden[i]);
			portLines(build(k), tx, port);
			TSM_ASSERT_EQUALS(kKeys[k], port.size(), want.size());
			for (uint i = 0; i < port.size() && i < want.size(); i++)
				TSM_ASSERT_EQUALS(kKeys[k], port[i], want[i]);
		}
	}

	// kEnhMinorBugFixes: an id 9 left at 9 (SUBTITLES off) reads the file's
	// numbers, on the per-menu records the fixes use: OPTMENU's record 3 is
	// the line's (336, 255, 50, 32) and `關閉` is drawn there, where the
	// original reads (0, 36, 255, 50) on a fresh table (golden `toggles
	// zeroed SUBTITLES 0`).  SUBTITLES on is the original's.
	void test_toggle_step_fix() {
		using namespace UiZhMenus;
		const Common::String dir = AtlantisTest::configValue("zhtw");
		if (dir.empty()) { TS_WARN("zhtw not configured; skipped"); return; }
		Texts tx;
		TS_ASSERT(loadTexts(dir, tx));
		const AtlantisExeBuild &b = build(0);
		for (int sub = 0; sub < 2; sub++) {
			MenuLayout l;
			layoutFor(tx.text[2], 2, nullptr, l);
			walk(b, l, -1, sub, 0, true);
			TS_ASSERT_EQUALS(l.walk.records.size(), 7u);
			TS_ASSERT_EQUALS(recs(l.walk.records, 3, 4), "(336,255,50,32)");
			TS_ASSERT_EQUALS(blits(b, l.walk), Common::String::format(
			    "SPRCHI1(260,50,2) SPRCHI1(280,200,7) SPRCHI1(160,250,8) SPRCHI1(336,255,%d) "
			    "SPRCHI1(175,300,11) SPRCHI1(344,300,12) SPRCHI1(300,420,5)", 9 + sub));
		}
		// The other builds take no id rule, with or without the fix.
		const AtlantisExeBuild &ref = atlantisExeBuild(714240, 0x3b8275d6);
		const char *text = "/sprc=9,100,200,50,32\r\n/sprc=12,1,2,3,4\r\n/fin\r\n";
		for (int fix = 0; fix < 2; fix++) {
			MenuWalk w;
			walkMenuText((const byte *)text, strlen(text), 2, -1, 0, 1, w, 22, nullptr,
			             ref.menuToggleSprites, fix != 0);
			TS_ASSERT_EQUALS(recs(w.records), "(100,200,50,32) (1,2,3,4)");
			TS_ASSERT(w.ops.size() == 2 && w.ops[0].index == 9 && w.ops[1].index == 12);
		}
	}

	// The `/sprc=` lines as the menus find them: a kText item with no text
	// (bankWord) per line, so each menu's text ordinals point at the same
	// records as the reference's labels (main menu actions, LOAD's RESTART /
	// OK / Cancel, SELEMENU's OK 5 / DELETE 6, the sound menu's labels, ARE
	// YOU SURE's OK / Cancel); only OPTMENU differs, whose toggles are no
	// toggle lines (displayOptionsMenu takes records 1, 3, 5, 6 there, as
	// FUN_0041d5c0 does).
	void test_items_keep_the_reference_ordinals() {
		using namespace UiZhMenus;
		const Common::String ref = AtlantisTest::dataDir();
		if (ref.empty()) { TS_WARN("data not configured; skipped"); return; }
		Texts rtx;
		TS_ASSERT(loadTexts(ref, rtx));
		for (int k = 0; k < 2; k++) {
			const Common::String dir = AtlantisTest::configValue(kKeys[k]);
			if (dir.empty()) { TS_WARN(Common::String::format("%s not configured; skipped", kKeys[k]).c_str()); continue; }
			Texts tx;
			TS_ASSERT(loadTexts(dir, tx));
			for (int m = 0; m < MenuRecordTable::kMenus; m++) {
				MenuLayout zl, rl;
				layoutFor(tx.text[m], m, nullptr, zl);
				layoutFor(rtx.text[m], m, nullptr, rl);
				Common::String zs, rs;
				uint words = 0;
				for (uint i = 0; i < zl.items.size(); i++) {
					const MenuItem &it = zl.items[i];
					TSM_ASSERT(kMenus[m], it.kind != MenuItem::kToggle);
					if (it.kind == MenuItem::kText) {
						TSM_ASSERT(kMenus[m], it.bankWord && it.text.empty());
						zs += Common::String::format(" %u", i);
						words++;
					} else {
						TSM_ASSERT(kMenus[m], !it.bankWord);
					}
				}
				for (uint i = 0; i < rl.items.size(); i++) {
					TSM_ASSERT(kMenus[m], !rl.items[i].bankWord);
					if (rl.items[i].kind == MenuItem::kText)
						rs += Common::String::format(" %u", i);
				}
				// one item per record, in the walk's order
				MenuWalk w;
				walkMenuText(zl.raw.data(), zl.raw.size(), m, -1, 1, 0, w, 22, nullptr, true);
				TSM_ASSERT_EQUALS(kMenus[m], zl.items.size(), w.records.size());
				uint altOps = 0;
				for (uint i = 0; i < w.ops.size(); i++) {
					TSM_ASSERT(kMenus[m], w.ops[i].sprite);
					altOps += w.ops[i].altBank ? 1 : 0;
				}
				TSM_ASSERT_EQUALS(kMenus[m], altOps, words);
				if (m == 2) {
					TS_ASSERT_EQUALS(rs, " 0 1 2 4 6");
					TS_ASSERT_EQUALS(zs, " 0 1 2 3 4 5 6");
					TS_ASSERT(zl.items.size() == 7 && zl.items[3].sprIdx == 9 && zl.items[5].sprIdx == 12);
				} else {
					TSM_ASSERT_EQUALS(kMenus[m], zs, rs);
				}
			}
		}
	}

	// The banks: SPRCHI1 / SPRCHI2 of disc 1 through the port's reader, 31
	// words each (atlantis-re evidence/compare/zh_archives.txt), every frame
	// a shipped record names present and drawn (no frame past the bank).
	void test_sprchi_banks() {
		using namespace UiZhMenus;
		for (int k = 0; k < 2; k++) {
			const Common::String dir = AtlantisTest::configValue(kKeys[k]);
			if (dir.empty()) { TS_WARN(Common::String::format("%s not configured; skipped", kKeys[k]).c_str()); continue; }
			const AtlantisExeBuild &b = build(k);
			Common::Array<SprSprite> bank[2];
			for (int h = 0; h < 2; h++) {
				Common::Array<byte> bytes;
				TS_ASSERT(bigMember(dir, 1, (Common::String("SPRITE\\2D\\") + b.menuSpriteBanks[h]).c_str(), bytes));
				Common::MemoryReadStream s(bytes.data(), bytes.size());
				TS_ASSERT(readSprFile(s, bank[h]));
				TSM_ASSERT_EQUALS(b.menuSpriteBanks[h], bank[h].size(), 31u);
			}
			Texts tx;
			TS_ASSERT(loadTexts(dir, tx));
			for (int m = 0; m < MenuRecordTable::kMenus; m++)
				for (int sub = 0; sub < 2; sub++)
					for (int omni = 0; omni < 2; omni++) {
						MenuLayout l;
						layoutFor(tx.text[m], m, nullptr, l);
						walk(b, l, -1, sub, omni, true);
						for (uint i = 0; i < l.walk.ops.size(); i++) {
							const MenuWalkOp &op = l.walk.ops[i];
							if (!op.altBank)
								continue;
							for (int h = 0; h < 2; h++) {
								TSM_ASSERT(kMenus[m], op.index >= 0 && op.index < (int)bank[h].size());
								if (op.index >= 0 && op.index < (int)bank[h].size())
									TSM_ASSERT(kMenus[m], !bank[h][op.index].pixels.empty());
							}
						}
					}
		}
	}

	// The end picture: FUN_0041a658 shows `images\end.tga` as the
	// reference's FUN_0041a66c does; the port's reader and decoder take the
	// release's disc 4 copy (`劇終`), not the reference's.
	void test_end_picture() {
		using namespace UiZhMenus;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<byte> refBytes;
		const Common::String ref = AtlantisTest::dataDir();
		const bool haveRef = !ref.empty() && bigMember(ref, 4, "IMAGES\\END.TGA", refBytes);
		const Graphics::PixelFormat fmt(2, 5, 6, 5, 0, 11, 5, 0, 0);
		for (int k = 0; k < 2; k++) {
			const Common::String dir = AtlantisTest::configValue(kKeys[k]);
			if (dir.empty()) { TS_WARN(Common::String::format("%s not configured; skipped", kKeys[k]).c_str()); continue; }
			Common::String md5;
			uint32 size = 0;
			TS_ASSERT(goldenEnd(kGoldens[k], md5, size));
			Common::Array<byte> bytes;
			TS_ASSERT(bigMember(dir, 4, "IMAGES\\END.TGA", bytes));
			TS_ASSERT_EQUALS(bytes.size(), size);
			TS_ASSERT_EQUALS(AtlantisTest::md5Hex(bytes.data(), bytes.size()), md5);
			Common::MemoryReadStream s(bytes.data(), bytes.size());
			Graphics::Surface *pic = readTga15(s, fmt);
			TS_ASSERT(pic);
			if (!pic)
				continue;
			TS_ASSERT(pic->w == 640 && pic->h == 480);
			if (haveRef) {
				Common::MemoryReadStream rs(refBytes.data(), refBytes.size());
				Graphics::Surface *rpic = readTga15(rs, fmt);
				TS_ASSERT(rpic);
				if (rpic) {
					TS_ASSERT(memcmp(pic->getPixels(), rpic->getPixels(), 640 * 480 * 2) != 0);
					rpic->free();
					delete rpic;
				}
			}
			pic->free();
			delete pic;
		}
	}
};

#endif
