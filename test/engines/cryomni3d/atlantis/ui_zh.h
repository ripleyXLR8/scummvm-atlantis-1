#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_H

// ui audit: the Traditional and Simplified Chinese CDs
// (atlantis-cd-zh-tw-696832, atlantis-cd-zh-cn-692736), as atlantis-re
// experiment 2026-10-19-01 read them and decision 0025 chose:
//   A1 the two build entries, told apart from every other by size and PE
//      timestamp (0x35af1f09, 0x36f11f20): fr-687104's flags but
//      atlantis-pe-2018-695296's (three-click SUBTITLES, LOAD knob at 0x7b
//      and clamp, rat T0023 restart, Muria two frames, the reference's
//      credits), the player row `New Player` as its own label, the CON
//      texts in code page 950 / 936, and the Chinese fields (CJK breaker,
//      18-px rows, SPRCHI menu banks and toggles, EPICHI LOAD rows with the
//      chapter and width tables, the CINEMA 20 captions, SUBTITLES on at
//      start, the 102-byte players.atl names block), each checked against
//      the instruction bytes at the addresses exe_build.h cites; every other
//      build keeps its behaviour (no CJK breaker, 14-px rows, FONTMAX menus,
//      EPI.TXT rows, no captions, the configuration's SUBTITLES, 85 bytes);
//   B1 (ui_detection.h) the detection entries;
//   the CON texts decoded with the build's code page give the goldens'
//   rows (evidence/golden/video_subtitles_zh_tw.txt / zh_cn.txt, emulated).
//
// Port code exercised directly: atlantisExeBuild, readPeTimestamp
// (exe_build.h), readNewSoundNames (exe_newsound.h), BigFileArchive
// (bigfile.cpp), Common::U32String with conCodePage (the decoding
// drawSubtitleRows does).  Real files: `zhtw`, `zhcn` (atlantis.exe and the
// four archives of each release), `data` and `golden` in atlantis_test.cfg;
// skipped when absent.

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/ustr.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/exe_newsound.h"

namespace UiZh {

using namespace CryOmni3D::Atlantis;

static const uint32 kTwTimestamp = 0x35af1f09;
static const uint32 kCnTimestamp = 0x36f11f20;

inline bool readExe(const Common::String &dir, Common::Array<byte> &out) {
	Common::FSNode d(Common::Path(dir, '/'));
	Common::FSList list;
	if (!d.getChildren(list, Common::FSNode::kListFilesOnly))
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

inline Common::String cstr(const Common::Array<byte> &b, uint32 off) {
	Common::String s;
	while (off < b.size() && b[off])
		s += (char)b[off++];
	return s;
}

// BEGTEXT: VA 0x00410000 at file offset 0x400 in both executables.
inline bool codeAt(const Common::Array<byte> &e, uint32 va, const char *hex) {
	uint32 off = va - 0x00410000 + 0x400;
	for (const char *p = hex; *p; ) {
		if (*p == ' ') {
			p++;
			continue;
		}
		uint v = 0;
		for (int k = 0; k < 2; k++, p++)
			v = v * 16 + (*p <= '9' ? *p - '0' : (*p | 0x20) - 'a' + 10);
		if (off >= e.size() || e[off++] != v)
			return false;
	}
	return true;
}

// The member `name` of `dir`'s first archive, through the port's reader.
inline bool bigMember(const Common::String &dir, const char *name, Common::Array<byte> &out) {
	SearchMan.addDirectory("ui_zh_data", Common::FSNode(Common::Path(dir, '/')));
	BigFileArchive big;
	const bool opened = big.open(Common::Path("BIGCD1.BIG"));
	SearchMan.remove("ui_zh_data");
	if (!opened)
		return false;
	Common::ScopedPtr<Common::SeekableReadStream> s(big.createReadStreamForMember(Common::String(name)));
	if (!s)
		return false;
	out.resize((uint)s->size());
	return s->read(out.data(), out.size()) == out.size();
}

// The rows of the golden line starting with `prefix` ("len 114 -> 2 rows:
// 72:<row> | 42:<row>"), as UTF-8.
inline Common::Array<Common::String> goldenRows(const char *file, const char *prefix) {
	Common::Array<Common::String> rows;
	Common::File f;
	if (!f.open(Common::FSNode(Common::Path(AtlantisTest::goldenDir(), '/').appendComponent(file))))
		return rows;
	while (!f.eos()) {
		const Common::String line = f.readLine();
		if (!line.hasPrefix(prefix))
			continue;
		size_t p = line.find("rows: ");
		if (p == Common::String::npos)
			break;
		Common::String rest = line.substr(p + 6);
		for (;;) {
			const size_t colon = rest.find(':');
			if (colon == Common::String::npos)
				break;
			const size_t bar = rest.find(" | ");
			rows.push_back(rest.substr(colon + 1, bar == Common::String::npos ? Common::String::npos : bar - colon - 1));
			if (bar == Common::String::npos)
				break;
			rest = rest.substr(bar + 3);
		}
		break;
	}
	return rows;
}

} // End of namespace UiZh

class UiZhTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// A1: the entries by size and timestamp (each size its own), their
	// addresses and flags; zh-cn is zh-tw but for its file offsets, code
	// page and breaker.
	void test_build_entries() {
		using namespace UiZh;
		const AtlantisExeBuild &tw = atlantisExeBuild(696832, kTwTimestamp);
		const AtlantisExeBuild &cn = atlantisExeBuild(692736, kCnTimestamp);
		const AtlantisExeBuild &fr = atlantisExeBuild(687104, 0x33ef1516);
		const AtlantisExeBuild &ref = atlantisExeBuild(714240, 0x3b8275d6);
		TS_ASSERT_EQUALS(Common::String(tw.name), "atlantis-cd-zh-tw-696832");
		TS_ASSERT_EQUALS(Common::String(cn.name), "atlantis-cd-zh-cn-692736");
		TS_ASSERT_EQUALS(&atlantisExeBuild(696832), &tw);
		TS_ASSERT_EQUALS(&atlantisExeBuild(692736), &cn);
		TS_ASSERT_EQUALS(tw.dgroupVA, 0x00480000u);
		TS_ASSERT_EQUALS(tw.dgroupRawPtr, 0x6dc00u);
		TS_ASSERT_EQUALS(cn.dgroupRawPtr, 0x6ce00u);
		TS_ASSERT_EQUALS(tw.inventoryVA, 0x0048688cu);
		TS_ASSERT_EQUALS(tw.guestVA, 0x00486960u);
		TS_ASSERT_EQUALS(tw.creditWidthsVA, 0x004867d8u);
		TS_ASSERT_EQUALS(tw.newSoundTableVA, 0x004861dcu);
		TS_ASSERT(tw.newSoundPtrMin == 0x004a0000u && tw.newSoundPtrMax == 0x004b0000u);
		TS_ASSERT_EQUALS(tw.playerRowVA, 0x004865a8u);
		TS_ASSERT(!tw.playerLabelAfterMarker);
		// fr-687104's flags ...
		TS_ASSERT_EQUALS(tw.cursorIdRule, fr.cursorIdRule);
		TS_ASSERT_EQUALS(tw.rheaCamera2, fr.rheaCamera2);
		TS_ASSERT_EQUALS(tw.altGameOverNarration, fr.altGameOverNarration);
		TS_ASSERT_EQUALS(tw.bitmapSubtitles, fr.bitmapSubtitles);
		TS_ASSERT_EQUALS(tw.upperCaseNames, fr.upperCaseNames);
		TS_ASSERT_EQUALS(tw.gameOverVoiceAtEntry, fr.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(tw.loadPickerClamp, fr.loadPickerClamp);
		TS_ASSERT_EQUALS(tw.hexaForcesObject402, fr.hexaForcesObject402);
		TS_ASSERT_EQUALS(tw.ogreRevealFrames, fr.ogreRevealFrames);
		TS_ASSERT_EQUALS(tw.ogreRevealTicks, fr.ogreRevealTicks);
		TS_ASSERT_EQUALS(tw.ratFrameDeadline, fr.ratFrameDeadline);
		TS_ASSERT_EQUALS(tw.musicFirstValueRule, fr.musicFirstValueRule);
		TS_ASSERT_EQUALS(tw.musicLoadAllOrNothing, fr.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(tw.musicLoadMinVolume, fr.musicLoadMinVolume);
		TS_ASSERT_EQUALS(tw.musicStartSameTrackNoop, fr.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(tw.rpLogoVideo, fr.rpLogoVideo);
		TS_ASSERT_EQUALS(tw.creditsPerRun, fr.creditsPerRun);
		TS_ASSERT_EQUALS(tw.subtitleBreakBackOnly, fr.subtitleBreakBackOnly);
		TS_ASSERT_EQUALS(tw.nameSwedishLetters, fr.nameSwedishLetters);
		TS_ASSERT_EQUALS(tw.menuPosItemByteWidth, fr.menuPosItemByteWidth);
		TS_ASSERT_EQUALS(tw.menuRecordsPerMenu, fr.menuRecordsPerMenu);
		// ... but atlantis-pe-2018-695296's, as the reference
		TS_ASSERT(tw.subtitleClickCycle && ref.subtitleClickCycle && !fr.subtitleClickCycle);
		TS_ASSERT(tw.loadPickerKnobAtBar && ref.loadPickerKnobAtBar && !fr.loadPickerKnobAtBar);
		TS_ASSERT(tw.ratT0023Restart && ref.ratT0023Restart && !fr.ratT0023Restart);
		TS_ASSERT(tw.muriaPlacedFrames == 2 && ref.muriaPlacedFrames == 2 && fr.muriaPlacedFrames == 1);
		TS_ASSERT(!tw.playerListVerbatim && !ref.playerListVerbatim && fr.playerListVerbatim);
		TS_ASSERT(tw.emptyNameEndsPlayerRows && ref.emptyNameEndsPlayerRows && !fr.emptyNameEndsPlayerRows);
		// the code pages
		TS_ASSERT_EQUALS(tw.conCodePage, Common::kWindows950);
		TS_ASSERT_EQUALS(cn.conCodePage, Common::kWindows936);
		// the Chinese fields
		TS_ASSERT_EQUALS(tw.cjkBreaker, kCjkBreakerBig5);
		TS_ASSERT_EQUALS(cn.cjkBreaker, kCjkBreakerGb2312);
		TS_ASSERT_EQUALS(tw.subtitleRowStep, 18);
		TS_ASSERT_EQUALS(Common::String(tw.menuSpriteBanks[0]), "SPRCHI1.SPR");
		TS_ASSERT_EQUALS(Common::String(tw.menuSpriteBanks[1]), "SPRCHI2.SPR");
		TS_ASSERT(tw.menuToggleSprites);
		TS_ASSERT_EQUALS(tw.loadList.mode, kLoadListSprites);
		TS_ASSERT_EQUALS(Common::String(tw.loadList.banks[0]), "EPICHI1.SPR");
		TS_ASSERT_EQUALS(Common::String(tw.loadList.banks[1]), "EPICHI2.SPR");
		TS_ASSERT_EQUALS(tw.loadList.chaptersVA, 0x00486619u);
		TS_ASSERT_EQUALS(tw.loadList.widthsVA, 0x00486663u);
		TS_ASSERT_EQUALS(tw.loadList.chaptersVA + tw.loadList.rows, tw.loadList.widthsVA);
		TS_ASSERT_EQUALS(tw.cinemaCaptions.cinema, 20);
		TS_ASSERT_EQUALS(Common::String(tw.cinemaCaptions.bank), "SUBCHINE.SPR");
		TS_ASSERT(tw.cinemaCaptions.x == 10 && tw.cinemaCaptions.y == 430);
		TS_ASSERT(tw.cinemaCaptions.after == 1200 && tw.cinemaCaptions.before == 18800);
		static const uint16 kSteps[10] = { 2200, 4400, 5000, 7200, 7800, 9400, 10600, 12600, 14800, 16400 };
		for (uint i = 0; i < 10; i++)
			TSM_ASSERT_EQUALS(i, tw.cinemaCaptions.steps[i], kSteps[i]);
		TS_ASSERT(tw.subtitlesOnAtStart);
		TS_ASSERT_EQUALS(tw.playersNamesBlock, 102);
		// zh-cn: zh-tw's entry but for these
		TS_ASSERT_EQUALS(cn.dgroupVA, tw.dgroupVA);
		TS_ASSERT_EQUALS(cn.inventoryVA, tw.inventoryVA);
		TS_ASSERT_EQUALS(cn.guestVA, tw.guestVA);
		TS_ASSERT_EQUALS(cn.creditWidthsVA, tw.creditWidthsVA);
		TS_ASSERT_EQUALS(cn.newSoundTableVA, tw.newSoundTableVA);
		TS_ASSERT_EQUALS(cn.playerRowVA, tw.playerRowVA);
		TS_ASSERT_EQUALS(cn.subtitleRowStep, tw.subtitleRowStep);
		TS_ASSERT(!strcmp(cn.menuSpriteBanks[0], tw.menuSpriteBanks[0]) &&
		          !strcmp(cn.menuSpriteBanks[1], tw.menuSpriteBanks[1]));
		TS_ASSERT_EQUALS(cn.menuToggleSprites, tw.menuToggleSprites);
		TS_ASSERT(cn.loadList.mode == tw.loadList.mode && cn.loadList.chaptersVA == tw.loadList.chaptersVA &&
		          cn.loadList.widthsVA == tw.loadList.widthsVA && cn.loadList.rows == tw.loadList.rows);
		TS_ASSERT(!memcmp(cn.cinemaCaptions.steps, tw.cinemaCaptions.steps, sizeof(kSteps)));
		TS_ASSERT_EQUALS(cn.subtitlesOnAtStart, tw.subtitlesOnAtStart);
		TS_ASSERT_EQUALS(cn.playersNamesBlock, tw.playersNamesBlock);
	}

	// Every other build keeps the behaviour the port has for it.
	void test_other_builds_keep_their_behaviour() {
		using namespace UiZh;
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			const bool zh = !strncmp(b.name, "atlantis-cd-zh-", 15);
			TSM_ASSERT(b.name, zh == (b.size == 696832 || b.size == 692736));
			if (zh)
				continue;
			TSM_ASSERT_EQUALS(b.name, b.cjkBreaker, kCjkBreakerNone);
			TSM_ASSERT_EQUALS(b.name, b.subtitleRowStep, 14);
			TSM_ASSERT(b.name, !b.menuSpriteBanks[0] && !b.menuSpriteBanks[1] && !b.menuToggleSprites);
			TSM_ASSERT(b.name, b.loadList.mode == kLoadListEpiText && !b.loadList.banks[0] && !b.loadList.rows);
			TSM_ASSERT(b.name, b.cinemaCaptions.cinema == 0 && !b.cinemaCaptions.bank);
			TSM_ASSERT(b.name, !b.subtitlesOnAtStart);
			TSM_ASSERT_EQUALS(b.name, b.playersNamesBlock, 85);
			TSM_ASSERT(b.name, b.conCodePage != Common::kWindows950 && b.conCodePage != Common::kWindows936);
		}
	}

	// A1 on the bytes of both executables: picked by their timestamps; the
	// tables at their addresses hold the reference's bytes; the player row;
	// the LOAD tables; the instructions behind each Chinese field.
	void test_tables_and_code_on_the_bytes() {
		using namespace UiZh;
		const char *const keys[2] = { "zhtw", "zhcn" };
		Common::Array<byte> ref;
		if (!AtlantisTest::haveData() || !readExe(AtlantisTest::dataDir(), ref)) {
			TS_WARN("data not configured; skipped");
			return;
		}
		const AtlantisExeBuild &rb = atlantisExeBuild(ref.size());
		Common::Array<Common::String> refNames;
		Common::MemoryReadStream rs(ref.data(), ref.size());
		readNewSoundNames(rs, refNames);
		int checked = 0;
		for (uint k = 0; k < 2; k++) {
			const Common::String dir = AtlantisTest::configValue(keys[k]);
			Common::Array<byte> e;
			if (dir.empty() || !readExe(dir, e)) {
				TS_WARN(Common::String::format("%s not configured; skipped", keys[k]).c_str());
				continue;
			}
			checked++;
			Common::MemoryReadStream es(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(es), k ? kCnTimestamp : kTwTimestamp);
			const AtlantisExeBuild &b = atlantisExeBuild(e.size(), readPeTimestamp(es));
			TS_ASSERT_EQUALS(Common::String(b.name), k ? "atlantis-cd-zh-cn-692736" : "atlantis-cd-zh-tw-696832");
			// the tables the port reads: the reference's bytes
			TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.inventoryVA)], &e[b.fileOffset(b.inventoryVA)], 52 * 4));
			TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.guestVA)], &e[b.fileOffset(b.guestVA)], 10 * 4));
			TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.creditWidthsVA)], &e[b.fileOffset(b.creditWidthsVA)], 256));
			Common::Array<Common::String> names;
			readNewSoundNames(es, names, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr, b.newSoundPtrMin,
			                  b.newSoundPtrMax);
			TS_ASSERT_EQUALS(refNames.size(), 137u);
			TS_ASSERT_EQUALS(names.size(), refNames.size());
			for (uint i = 0; i < names.size() && i < refNames.size(); i++)
				TS_ASSERT_EQUALS(names[i], refNames[i]);
			// music names, FONTMAX / FONTMIN advances, dialog camera FOV
			static const uint32 kPairs[][3] = {
				{ 0x00495eec, 0x00485eec, 31 * 13 }, { 0x0049660e, 0x004866ea, 137 },
				{ 0x00496697, 0x00486773, 101 }, { 0x00496080, 0x00486080, 16 } };
			for (uint i = 0; i < ARRAYSIZE(kPairs); i++)
				TSM_ASSERT(i, !memcmp(&ref[rb.fileOffset(kPairs[i][0])], &e[b.fileOffset(kPairs[i][1])], kPairs[i][2]));
			// the player row: one string, marker = label, then names[0]
			TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "New Player");
			TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x004865b3)), "New Player");
			// SUBTITLES 0 in the file, set at start-up (@ 0x0041971f)
			TS_ASSERT_EQUALS(e[b.fileOffset(0x004860a8)], 0);
			TS_ASSERT(codeAt(e, 0x0041971f, "c7 05 a8 60 48 00 01 00 00 00"));
			// players.atl: `push 0x66` @ 0x0041921a
			TS_ASSERT(codeAt(e, 0x0041921a, "6a 66"));
			// rows 18 px apart: `imul eax,eax,0x12` in the drawer
			const uint32 drawer = k ? 0x0042f7d4 : 0x0042f7f8;
			TS_ASSERT(codeAt(e, drawer, "6b c0 12") && codeAt(e, drawer + 0x30, "6b c0 12"));
			// the menu banks (FUN_0042676c) and the options toggles (FUN_004201e8)
			TS_ASSERT(codeAt(e, 0x004267a2, "ba 56 65 48 00") && codeAt(e, 0x0042681e, "ba 62 65 48 00"));
			TS_ASSERT(cstr(e, b.fileOffset(0x00486556)).equalsIgnoreCase(b.menuSpriteBanks[0]));
			TS_ASSERT(cstr(e, b.fileOffset(0x00486562)).equalsIgnoreCase(b.menuSpriteBanks[1]));
			TS_ASSERT(codeAt(e, 0x00420212, "83 7d e8 09") && codeAt(e, 0x00420220, "a1 a8 60 48 00") &&
			          codeAt(e, 0x00420234, "83 3d ac 60 48 00 01"));
			// the LOAD banks, chapter and width tables (FUN_0041ce00)
			TS_ASSERT(codeAt(e, 0x0041cf19, "ba 6e 65 48 00") && codeAt(e, 0x0041cf95, "ba 7a 65 48 00"));
			TS_ASSERT(cstr(e, b.fileOffset(0x0048656e)).equalsIgnoreCase(b.loadList.banks[0]));
			TS_ASSERT(cstr(e, b.fileOffset(0x0048657a)).equalsIgnoreCase(b.loadList.banks[1]));
			TS_ASSERT(codeAt(e, 0x0041d432, "8a 80 19 66 48 00") && codeAt(e, 0x0041d442, "8a 80 19 66 48 00"));
			TS_ASSERT(codeAt(e, 0x0041d3d6, "8a 90 63 66 48 00"));
			const byte *chapters = &e[b.fileOffset(b.loadList.chaptersVA)];
			const byte *widths = &e[b.fileOffset(b.loadList.widthsVA)];
			TS_ASSERT(chapters[0] == 2 && chapters[1] == 3 && chapters[5] == 9 && chapters[b.loadList.rows - 1] == 0x55);
			for (uint i = 1; i < b.loadList.rows; i++)
				TSM_ASSERT(i, chapters[i] > chapters[i - 1]);
			TS_ASSERT(widths[0] == 80 && widths[1] == 98 && widths[2] == 139 && widths[3] == 140);
			// the CINEMA 20 captions (FUN_00429958, FUN_0041afa0)
			TS_ASSERT(codeAt(e, 0x00429fcd, "80 3d a1 65 48 00 14") && codeAt(e, 0x00429ff8, "ba 86 65 48 00"));
			TS_ASSERT(cstr(e, b.fileOffset(0x00486586)).equalsIgnoreCase(b.cinemaCaptions.bank));
			TS_ASSERT(codeAt(e, 0x0041b105, "80 3d a1 65 48 00 14"));
			TS_ASSERT(codeAt(e, 0x0041b11d, Common::String::format("3d %02x %02x 00 00", b.cinemaCaptions.after & 0xff,
			                                                      b.cinemaCaptions.after >> 8).c_str()));
			TS_ASSERT(codeAt(e, 0x0041b12f, Common::String::format("3d %02x %02x 00 00", b.cinemaCaptions.before & 0xff,
			                                                      b.cinemaCaptions.before >> 8).c_str()));
			TS_ASSERT(codeAt(e, 0x0041b144, Common::String::format("b8 %02x 01 00 00", b.cinemaCaptions.y - 0x100).c_str()));
			TS_ASSERT(codeAt(e, 0x0041b14a, Common::String::format("b8 %02x 00 00 00", b.cinemaCaptions.x).c_str()));
			for (uint i = 0; i < 10; i++) {
				const uint16 t = b.cinemaCaptions.steps[i];
				TSM_ASSERT(i, codeAt(e, 0x0041b16f + 0x23 * i, Common::String::format("3d %02x %02x 00 00", t & 0xff, t >> 8).c_str()));
			}
		}
		if (!checked)
			TS_WARN("zhtw / zhcn not configured; skipped");
	}

	// The CON texts decoded with the build's code page (exe_build.h
	// conCodePage, as drawSubtitleRows decodes a row) hold the rows of the
	// goldens' second sample (two CHAPI001 lines joined; zh-cn's second row
	// is the breaker's out-of-step row, so its first only); the GBK pair
	// `85 85` of the Simplified scripts decodes as GBK draws it (U+5396).
	void test_con_texts_decode_with_code_page() {
		using namespace UiZh;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		const char *const keys[2] = { "zhtw", "zhcn" };
		const char *const goldens[2] = { "video_subtitles_zh_tw.txt", "video_subtitles_zh_cn.txt" };
		for (uint k = 0; k < 2; k++) {
			const Common::String dir = AtlantisTest::configValue(keys[k]);
			if (dir.empty()) { TS_WARN(Common::String::format("%s not configured; skipped", keys[k]).c_str()); continue; }
			Common::Array<byte> e, con;
			TS_ASSERT(readExe(dir, e));
			if (e.empty())
				continue;
			Common::MemoryReadStream es(e.data(), e.size());
			const AtlantisExeBuild &b = atlantisExeBuild(e.size(), readPeTimestamp(es));
			TS_ASSERT(bigMember(dir, "SCENAR\\CHAPI001.CON", con));
			const Common::String text = Common::U32String(Common::String((const char *)con.data(), con.size()),
			                                              b.conCodePage).encode(Common::kUtf8);
			const Common::Array<Common::String> rows = goldenRows(goldens[k], k ? "len 118 " : "len 114 ");
			TS_ASSERT_EQUALS(rows.size(), 2u);
			for (uint r = 0; r < rows.size() && r < (k ? 1u : 2u); r++) {
				Common::String row = rows[r];
				row.trim();
				TSM_ASSERT(row.c_str(), !row.empty() && text.contains(row));
			}
			// no reading in the other code page gives them
			const Common::String other = Common::U32String(Common::String((const char *)con.data(), con.size()),
			                                               k ? Common::kWindows950 : Common::kWindows936).encode(Common::kUtf8);
			if (!rows.empty()) {
				Common::String row = rows[0];
				row.trim();
				TS_ASSERT(!other.contains(row));
			}
			if (k) {
				const Common::U32String gbk(Common::String("\x85\x85"), b.conCodePage);
				TS_ASSERT_EQUALS(gbk.size(), 1u);
				if (gbk.size() == 1)
					TS_ASSERT_EQUALS((uint32)gbk[0], 0x5396u);
			}
		}
	}
};

#endif
