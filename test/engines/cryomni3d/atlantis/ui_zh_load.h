#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_LOAD_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_LOAD_H

// ui audit: the Chinese CDs' LOAD list, CINEMA 20 captions and players.atl
// (atlantis-cd-zh-tw-696832, atlantis-cd-zh-cn-692736; atlantis-re decision
// 0025 F, binaries/atlantis-cd-zh-tw-696832.md Z6, Z7, Z10).
//
// Expected values come from the ORIGINAL code of both executables, run
// unmodified under Unicorn by atlantis-re/tools/emu/ui_load_list_zh.py
// (evidence/golden/ui_load_list_zh_tw.txt, ui_load_list_zh_cn.txt):
//   R  the row drawer FUN_004205a4: EPICHI1 / EPICHI2 frame = list entry;
//   P  the row pick of FUN_0041ce00 @ 0x0041d312..0x0041d452: the chapter
//      byte from the table 0x00486619 by entry, the title x by the width
//      table 0x00486663;
//   T  the title drawer FUN_004208bc mode 1: EPICHI2 frame `entry` at y 60;
//   C  the CINEMA 20 caption block of FUN_0041afa0 @ 0x0041b105..0x0041b2c2;
//   A  WinMain's players.atl reader @ 0x004191ca..0x004192e1 (114 bytes).
//
// Port code exercised directly: loadListRowBlits, loadListChapter,
// loadListTitleBlit, cinemaCaptionFrame (load_list.h, which menus.cpp
// displaySavePicker and place.cpp playTransitionVideo call), the build
// entries' loadList / cinemaCaptions (exe_build.h) with the tables read at
// their file offsets as exetables.cpp reads them, readPlayersFile and
// readOriginalPlayersLetters (saveload_format.cpp), readSprFile and
// BigFileArchive for the banks' frame counts.  Real files: `zhtw`, `zhcn`
// and `golden` in atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/ptr.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/load_list.h"
#include "cryomni3d/atlantis/saveload_format.h"
#include "cryomni3d/atlantis/sprite_decode.h"

namespace UiZhLoad {

using namespace CryOmni3D::Atlantis;

struct Release {
	const char *key;       // atlantis_test.cfg
	const char *golden;
	uint32 size, timestamp;
};
static const Release kReleases[] = {
	{ "zhtw", "ui_load_list_zh_tw.txt", 696832, 0x35af1f09 },
	{ "zhcn", "ui_load_list_zh_cn.txt", 692736, 0x36f11f20 },
};

inline bool readExe(const Common::String &dir, Common::Array<byte> &out) {
	Common::FSNode d(Common::Path(dir, '/'));
	Common::FSList list;
	if (dir.empty() || !d.getChildren(list, Common::FSNode::kListFilesOnly))
		return false;
	for (uint i = 0; i < list.size(); i++) {
		if (!list[i].getName().equalsIgnoreCase("atlantis.exe"))
			continue;
		Common::File f;
		if (!f.open(list[i]))
			return false;
		out.resize((uint)f.size());
		return f.read(out.data(), out.size()) == out.size();
	}
	return false;
}

// The LOAD tables as loadAtlantisExeTables reads them (exetables.cpp).
inline void loadTables(const AtlantisExeBuild &b, const Common::Array<byte> &exe,
                       Common::Array<byte> &chapters, Common::Array<byte> &widths) {
	chapters.resize(b.loadList.rows);
	widths.resize(b.loadList.rows);
	memcpy(chapters.data(), exe.data() + b.fileOffset(b.loadList.chaptersVA), chapters.size());
	memcpy(widths.data(), exe.data() + b.fileOffset(b.loadList.widthsVA), widths.size());
}

inline Common::String blitText(const LoadListBlit &b) {
	return Common::String::format("%s@%d,%d#%u", b.bank ? "EPICHI2" : "EPICHI1", b.x, b.y, b.frame);
}

// The text after "key: " of a golden line, and its fields before it.
inline void splitLine(const Common::String &line, Common::Array<Common::String> &head, Common::String &tail) {
	head.clear();
	const size_t colon = line.find(": ");
	const Common::String h = colon == Common::String::npos ? line : line.substr(0, colon);
	tail = colon == Common::String::npos ? Common::String() : line.substr(colon + 2);
	tail.trim();
	Common::String cur;
	for (uint i = 0; i <= h.size(); i++) {
		if (i == h.size() || h[i] == ' ') {
			if (!cur.empty())
				head.push_back(cur);
			cur.clear();
		} else {
			cur += h[i];
		}
	}
}

// The text of `line` between `a` and `b` (or its end).
inline Common::String between(const Common::String &line, const char *a, const char *b) {
	const size_t p = line.find(a);
	if (p == Common::String::npos)
		return Common::String();
	const size_t s = p + strlen(a);
	const size_t e = b ? line.find(b, s) : Common::String::npos;
	return line.substr(s, e == Common::String::npos ? Common::String::npos : e - s);
}

inline Common::String field(const Common::Array<Common::String> &f, const char *name) {
	for (uint i = 0; i + 1 < f.size(); i++)
		if (f[i] == name)
			return f[i + 1];
	return Common::String();
}

// The frame count of a SPRITE\2D bank of `dir`'s first archive.
inline int bankFrames(const Common::String &dir, const char *name) {
	SearchMan.addDirectory("ui_zh_load_data", Common::FSNode(Common::Path(dir, '/')));
	BigFileArchive big;
	const bool opened = big.open(Common::Path("BIGCD1.BIG"));
	SearchMan.remove("ui_zh_load_data");
	if (!opened)
		return -1;
	Common::ScopedPtr<Common::SeekableReadStream> s(
	    big.createReadStreamForMember(Common::String("SPRITE\\2D\\") + name));
	if (!s)
		return -1;
	Common::Array<SprSprite> spr;
	if (!readSprFile(*s, spr))
		return -1;
	return (int)spr.size();
}

} // End of namespace UiZhLoad

class UiZhLoadTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// R: the rows the port draws are the blits the original asks for, for
	// file counts 0..13, 74, 75, 80, offsets and hovered grid rows.
	void test_rows_as_emulated() {
		using namespace UiZhLoad;
		for (uint k = 0; k < ARRAYSIZE(kReleases); k++) {
			Common::Array<Common::String> lines;
			if (!AtlantisTest::loadGoldenLines(kReleases[k].golden, lines)) {
				TS_SKIP("ui_load_list_zh golden not found");
				return;
			}
			uint n = 0;
			for (uint i = 0; i < lines.size(); i++) {
				if (!lines[i].hasPrefix("R "))
					continue;
				Common::Array<Common::String> h;
				Common::String tail;
				splitLine(lines[i], h, tail);
				TS_ASSERT_EQUALS(h.size(), 4u);
				Common::Array<LoadListBlit> blits;
				loadListRowBlits((uint)atoi(h[1].c_str()), (uint)atoi(h[2].c_str()), atoi(h[3].c_str()), blits);
				Common::String got;
				for (uint b = 0; b < blits.size(); b++)
					got += (b ? " " : "") + blitText(blits[b]);
				TS_ASSERT_EQUALS(got, tail);
				n++;
			}
			TS_ASSERT_EQUALS(n, 330u);
		}
	}

	// P, T: the chapter byte and the title of every list entry 0..79 (74
	// files and past the tables) and of a list with a gap (pa2, pa5, pa9:
	// chapters 2, 3, 4), from the build's tables.
	void test_pick_and_title_as_emulated() {
		using namespace UiZhLoad;
		for (uint k = 0; k < ARRAYSIZE(kReleases); k++) {
			const Release &r = kReleases[k];
			Common::Array<byte> exe;
			Common::Array<Common::String> lines;
			if (!readExe(AtlantisTest::configValue(r.key), exe) || !AtlantisTest::loadGoldenLines(r.golden, lines)) {
				TS_SKIP("Chinese executables or golden not found");
				return;
			}
			const AtlantisExeBuild &b = atlantisExeBuild(r.size, r.timestamp);
			TS_ASSERT_EQUALS(b.loadList.mode, kLoadListSprites);
			Common::Array<byte> chapters, widths;
			loadTables(b, exe, chapters, widths);
			uint picks = 0, titles = 0;
			for (uint i = 0; i < lines.size(); i++) {
				Common::Array<Common::String> h;
				Common::String tail;
				if (lines[i].hasPrefix("P ")) {
					splitLine(lines[i], h, tail);
					const uint entry = (uint)atoi(h[1].c_str());
					TS_ASSERT_EQUALS(atoi(field(h, "entry").c_str()), (int)entry);
					TS_ASSERT_EQUALS((int)loadListChapter(entry, chapters, widths), atoi(field(h, "chapterbyte").c_str()));
					TS_ASSERT_EQUALS(field(h, "chapterbyte"), field(h, "load"));
					LoadListBlit t;
					if (loadListTitleBlit(entry, widths, t)) {
						TS_ASSERT_EQUALS(t.x, atoi(field(h, "x").c_str()));
					} else {
						TS_ASSERT(entry >= 74);
					}
					picks++;
				} else if (lines[i].hasPrefix("T ")) {
					splitLine(lines[i], h, tail);
					const uint entry = (uint)atoi(h[1].c_str());
					LoadListBlit t;
					if (loadListTitleBlit(entry, widths, t)) {
						TS_ASSERT_EQUALS(blitText(t), tail);
					} else {
						// past the 74 frames: the original's blit draws
						// nothing (0x00410462); the port makes none
						TS_ASSERT(entry >= 74);
						TS_ASSERT(tail.hasPrefix("EPICHI2@"));
					}
					titles++;
				}
			}
			TS_ASSERT_EQUALS(picks, 83u);
			TS_ASSERT_EQUALS(titles, 76u);
		}
	}

	// The banks: 74 frames each (the tables' rows), 11 captions.
	void test_bank_frames() {
		using namespace UiZhLoad;
		for (uint k = 0; k < ARRAYSIZE(kReleases); k++) {
			const Common::String dir = AtlantisTest::configValue(kReleases[k].key);
			if (dir.empty()) {
				TS_SKIP("Chinese discs not found");
				return;
			}
			const AtlantisExeBuild &b = atlantisExeBuild(kReleases[k].size, kReleases[k].timestamp);
			TS_ASSERT_EQUALS(bankFrames(dir, b.loadList.banks[0]), (int)b.loadList.rows);
			TS_ASSERT_EQUALS(bankFrames(dir, b.loadList.banks[1]), (int)b.loadList.rows);
			TS_ASSERT_EQUALS(bankFrames(dir, b.cinemaCaptions.bank), 11);
		}
	}

	// C: the caption drawn and the frame left by each presented frame, at t
	// every 100 ticks and at the window's and the first step's edges, then a
	// late first frame (t 9000) catching up five steps in one call; another
	// CINEMA value draws nothing.
	void test_captions_as_emulated() {
		using namespace UiZhLoad;
		for (uint k = 0; k < ARRAYSIZE(kReleases); k++) {
			Common::Array<Common::String> lines;
			if (!AtlantisTest::loadGoldenLines(kReleases[k].golden, lines)) {
				TS_SKIP("ui_load_list_zh golden not found");
				return;
			}
			const AtlantisCinemaCaptions &c = atlantisExeBuild(kReleases[k].size, kReleases[k].timestamp).cinemaCaptions;
			uint8 frame = 0;
			Common::String series;
			uint n = 0;
			for (uint i = 0; i < lines.size(); i++) {
				if (!lines[i].hasPrefix("C "))
					continue;
				Common::Array<Common::String> h;
				Common::String tail;
				splitLine(lines[i], h, tail);
				if (h[1] != series) {
					series = h[1];
					frame = 0;     // CINEMA= (0x00429fbe)
				}
				const bool on = series == "jump" || atoi(series.c_str()) == (int)c.cinema;
				uint drawn = 0;
				const bool draw = on && cinemaCaptionFrame(c, (uint32)atoi(h[2].c_str()), frame, drawn);
				const Common::String got = Common::String::format("%s -> frame %u",
				    draw ? Common::String::format("SUBCHINE@%d,%d#%u", c.x, c.y, drawn).c_str() : "-",
				    on ? frame : 0);
				TS_ASSERT_EQUALS(got, tail);
				n++;
			}
			TS_ASSERT_EQUALS(n, 413u);
		}
	}

	// A: a players.atl of 114 bytes reads as WinMain reads it: the rows up
	// to the `New Player` marker or an empty name (5 at most), the chapter
	// bytes at 102, the letters at 108.
	void test_players_atl_114_as_emulated() {
		using namespace UiZhLoad;
		Common::Array<Common::String> lines;
		if (!AtlantisTest::loadGoldenLines(kReleases[0].golden, lines)) {
			TS_SKIP("ui_load_list_zh golden not found");
			return;
		}
		struct Case { const char *name; const char *names[6]; byte chapters[6]; const char *letters; };
		static const Case kCases[] = {
			{ "two", { "Alice", "Bob", "New Player", "", "", "" }, { 3, 7, 0, 0, 0, 0 }, "abcdef" },
			{ "five", { "Ann", "Ben", "Cy", "Di", "Ed", "Fay" }, { 2, 9, 30, 41, 85, 12 }, "fedcba" },
			{ "empty", { "Alice", "", "Carl", "New Player", "", "" }, { 5, 6, 7, 0, 0, 0 }, "abcdef" },
		};
		const AtlantisExeBuild &tw = atlantisExeBuild(696832, 0x35af1f09);
		TS_ASSERT_EQUALS(tw.playersNamesBlock + 6u + 6u, 114u);
		uint found = 0;
		for (uint c = 0; c < ARRAYSIZE(kCases); c++) {
			byte blob[114];
			memset(blob, 0, sizeof(blob));
			for (uint i = 0; i < 6; i++)
				memcpy(blob + 17 * i, kCases[c].names[i], strlen(kCases[c].names[i]));
			memcpy(blob + tw.playersNamesBlock, kCases[c].chapters, 6);
			memcpy(blob + tw.playersNamesBlock + 6, kCases[c].letters, 6);
			for (uint i = 0; i < lines.size(); i++) {
				const Common::String &l = lines[i];
				if (!l.hasPrefix(Common::String::format("A %s ", kCases[c].name)))
					continue;
				found++;
				TS_ASSERT_EQUALS(between(l, " size ", " "), "114");
				TS_ASSERT_EQUALS(between(l, " reads ", nullptr), "102 6 6");
				TS_ASSERT_EQUALS(between(l, " letters ", " "), Common::String(kCases[c].letters));
				// The original's row count includes the `New Player` row
				// when there is room for it (0x004192db); the port's players
				// are the rows before it.
				const uint count = (uint)atoi(between(l, " count ", " ").c_str());
				uint players = count;
				if (players > 0 && players < 5)
					players--;
				Common::Array<Common::String> rows;
				{
					const Common::String all = between(l, " names ", " chapters ") + "|";
					Common::String cur;
					for (uint j = 0; j < all.size(); j++) {
						if (all[j] == '|') {
							rows.push_back(cur);
							cur.clear();
						} else {
							cur += all[j];
						}
					}
				}
				TS_ASSERT_EQUALS(rows.size(), 6u);
				Common::Array<int> goldChapters;
				{
					const Common::String all = between(l, " chapters ", " letters ") + " ";
					Common::String cur;
					for (uint j = 0; j < all.size(); j++) {
						if (all[j] == ' ') {
							goldChapters.push_back(atoi(cur.c_str()));
							cur.clear();
						} else {
							cur += all[j];
						}
					}
				}
				TS_ASSERT_EQUALS(goldChapters.size(), 6u);
				Common::String names[5];
				int chapters[5];
				Common::MemoryReadStream in(blob, sizeof(blob));
				TS_ASSERT(readPlayersFile(in, names, chapters, "New Player", tw.emptyNameEndsPlayerRows));
				for (uint p = 0; p < 5 && rows.size() == 6 && goldChapters.size() == 6; p++) {
					TS_ASSERT_EQUALS(names[p], p < players ? rows[p] : Common::String());
					TS_ASSERT_EQUALS(chapters[p], p < players ? goldChapters[p] : 0);
				}
				char letters[5];
				Common::MemoryReadStream in2(blob, sizeof(blob));
				TS_ASSERT(readOriginalPlayersLetters(in2, letters, "New Player", tw.emptyNameEndsPlayerRows));
				for (uint p = 0; p < 5; p++)
					TS_ASSERT_EQUALS(letters[p], kCases[c].letters[p]);
			}
		}
		TS_ASSERT_EQUALS(found, 3u);
	}
};

#endif
