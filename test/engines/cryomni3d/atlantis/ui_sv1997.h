#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_SV1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_SV1997_H

// ui audit: the Swedish CD of June 1997 (atlantis-cd-sv-688128), as
// atlantis-re experiment 2026-10-18-09 read it and decision 0023 chose:
//   A1 the build entry, told apart from atlantis-cd-en-688128 and
//      atlantis-cd-nl-688128 (same size) by its PE timestamp 0x339e656f:
//      en-688128's tables and flags but the name entry (0x20..0xa7, case
//      kept), the LOAD clamp, its player row `Ny spelare`;
//   C1 the six Swedish letters of the name entry (nameEntryChar);
//   D1 the backward-only subtitle breaker FUN_0042ea7c, D2 a space-free
//      run cut at 75 bytes (splitSubtitleBitmap backOnly), against the
//      emulated golden video_subtitles_sv688128.txt / .png;
//   E  the CON texts in code page 850 for any rendering but FONTS.BIN; the
//      per-disc copies (the members that differ between the Swedish discs
//      are all loaded once at start-up; FONTMIN, FONTMIN2, CREDITS.TXT,
//      read on the current disc, are the same on every disc);
//   no LOGORP start-up video.
//
// Port code exercised directly: atlantisExeBuild / readPeTimestamp
// (exe_build.h), readNewSoundNames (exe_newsound.h), fontMinAdvance,
// nameKeyCode, nameEntryChar (menu_layout.cpp), BigFileArchive
// (bigfile.cpp), splitSubtitleBitmap / drawSubtitleBitmapRow
// (subtitle_layout.cpp).
// Real files: `sv1997` (ATLANTIS.EXE and the four Swedish BIGCD*.BIG),
// `en1997` and `data` in atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "video_subtitles_en1997.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/hashmap.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/ustr.h"
#include "graphics/surface.h"
#include "image/png.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/exe_newsound.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiSv1997 {

using namespace CryOmni3D::Atlantis;

// The priority of the folder the BigFileArchive tests mount their archives
// from: above the data folders other suites leave in SearchMan (the
// reference's BIGCD*.BIG, same names).
static const int kOwnArchivesFirst = 100;

static const uint32 kSvTimestamp = 0x339e656f;
static const uint32 kEnTimestamp = 0x33886fd1;
static const uint32 kNlTimestamp = 0x3396af27;

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

// BEGTEXT: VA 0x00410000, raw 0x400 (the section table, as en-688128's).
inline uint32 textOffset(uint32 va) { return va - 0x00410000 + 0x400; }

inline bool codeIs(const Common::Array<byte> &e, uint32 va, const char *hex) {
	const uint32 off = textOffset(va);
	for (uint i = 0; hex[2 * i]; i++) {
		uint v = 0;
		for (uint k = 0; k < 2; k++) {
			const char c = hex[2 * i + k];
			v = v * 16 + (c <= '9' ? c - '0' : c - 'a' + 10);
		}
		if (off + i >= e.size() || e[off + i] != v)
			return false;
	}
	return true;
}

inline Common::String md5Of(Common::SeekableReadStream *s) {
	if (!s)
		return Common::String();
	Common::String m = Common::computeStreamMD5AsString(*s);
	delete s;
	return m;
}

inline Common::String joined(const Common::Array<Common::String> &rows) {
	Common::String s;
	for (uint i = 0; i < rows.size(); i++)
		s += rows[i];
	return s;
}

// The golden's row list "L:'text' | L:'text'" (synthetic texts, no quote
// inside them) as strings.
inline bool parseGoldenRows(const Common::String &line, Common::Array<Common::String> &rows) {
	rows.clear();
	const size_t colon = line.find("rows: ");
	if (colon == Common::String::npos)
		return false;
	Common::String rest = line.substr(colon + 6);
	while (!rest.empty()) {
		const size_t q1 = rest.find('\'');
		if (q1 == Common::String::npos)
			break;
		const size_t q2 = rest.find('\'', q1 + 1);
		if (q2 == Common::String::npos)
			return false;
		const int len = atoi(rest.substr(0, q1 - 1).c_str());
		const Common::String row = rest.substr(q1 + 1, q2 - q1 - 1);
		if ((int)row.size() != len)
			return false;
		rows.push_back(row);
		rest = rest.substr(q2 + 1);
		const size_t bar = rest.find("| ");
		if (bar == Common::String::npos)
			break;
		rest = rest.substr(bar + 2);
	}
	return !rows.empty();
}

} // End of namespace UiSv1997

class UiSv1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// A1: the entry by size and timestamp; en-688128's tables and flags but
	// the name entry and the LOAD clamp; C1, D1, E flags.
	void test_build_entry() {
		using namespace UiSv1997;
		const AtlantisExeBuild &sv = atlantisExeBuild(688128, kSvTimestamp);
		const AtlantisExeBuild &en = atlantisExeBuild(688128, kEnTimestamp);
		const AtlantisExeBuild &nl = atlantisExeBuild(688128, kNlTimestamp);
		TS_ASSERT_EQUALS(Common::String(sv.name), "atlantis-cd-sv-688128");
		TS_ASSERT_EQUALS(Common::String(en.name), "atlantis-cd-en-688128");
		TS_ASSERT_EQUALS(Common::String(nl.name), "atlantis-cd-nl-688128");
		// the size alone (what was picked before this entry): en-688128
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(688128).name), "atlantis-cd-en-688128");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(688128, 0).name), "atlantis-cd-en-688128");
		TS_ASSERT_EQUALS(sv.size, 688128u);
		TS_ASSERT_EQUALS(sv.peTimestamp, kSvTimestamp);
		// every table at en-688128's address
		TS_ASSERT_EQUALS(sv.dgroupVA, en.dgroupVA);
		TS_ASSERT_EQUALS(sv.dgroupRawPtr, en.dgroupRawPtr);
		TS_ASSERT_EQUALS(sv.inventoryVA, 0x00486788u);
		TS_ASSERT_EQUALS(sv.guestVA, 0x0048685cu);
		TS_ASSERT_EQUALS(sv.creditWidthsVA, 0x004866d5u);
		TS_ASSERT_EQUALS(sv.inventoryVA, en.inventoryVA);
		TS_ASSERT_EQUALS(sv.guestVA, en.guestVA);
		TS_ASSERT_EQUALS(sv.creditWidthsVA, en.creditWidthsVA);
		TS_ASSERT_EQUALS(sv.newSoundTableVA, en.newSoundTableVA);
		TS_ASSERT(sv.newSoundPtrMin == 0x004a0000u && sv.newSoundPtrMax == 0x004b0000u);
		TS_ASSERT_EQUALS(sv.playerRowVA, 0x0048654au);
		TS_ASSERT(!sv.playerLabelAfterMarker);
		// S2: 0x20..0xa7 as typed (the Dutch rule) plus the six letters
		TS_ASSERT(!sv.upperCaseNames);
		TS_ASSERT(sv.nameSwedishLetters);
		// S3: the LOAD clamp and row + offset test, the knob still at y 0
		TS_ASSERT(sv.loadPickerClamp);
		TS_ASSERT(!sv.loadPickerKnobAtBar);
		TS_ASSERT(!en.loadPickerClamp);
		// S1: the backward-only breaker on the FONTS.BIN subtitles
		TS_ASSERT(sv.bitmapSubtitles);
		TS_ASSERT(sv.subtitleBreakBackOnly);
		// every other flag as en-688128's
		TS_ASSERT_EQUALS(sv.cursorIdRule, en.cursorIdRule);
		TS_ASSERT_EQUALS(sv.rheaCamera2, en.rheaCamera2);
		TS_ASSERT_EQUALS(sv.altGameOverNarration, en.altGameOverNarration);
		TS_ASSERT_EQUALS(sv.subtitleClickCycle, en.subtitleClickCycle);
		TS_ASSERT_EQUALS(sv.playerListVerbatim, en.playerListVerbatim);
		TS_ASSERT_EQUALS(sv.gameOverVoiceAtEntry, en.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(sv.loadPickerKnobAtBar, en.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(sv.hexaForcesObject402, en.hexaForcesObject402);
		TS_ASSERT_EQUALS(sv.ogreRevealFrames, en.ogreRevealFrames);
		TS_ASSERT_EQUALS(sv.ogreRevealTicks, en.ogreRevealTicks);
		TS_ASSERT_EQUALS(sv.ratFrameDeadline, en.ratFrameDeadline);
		TS_ASSERT_EQUALS(sv.ratT0023Restart, en.ratT0023Restart);
		TS_ASSERT_EQUALS(sv.musicFirstValueRule, en.musicFirstValueRule);
		TS_ASSERT_EQUALS(sv.muriaPlacedFrames, en.muriaPlacedFrames);
		TS_ASSERT_EQUALS(sv.musicLoadAllOrNothing, en.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(sv.musicLoadMinVolume, en.musicLoadMinVolume);
		TS_ASSERT_EQUALS(sv.musicStartSameTrackNoop, en.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(sv.creditsPerRun, en.creditsPerRun);
		TS_ASSERT(sv.creditsPerRun);
		// E: code page 850; no LOGORP
		TS_ASSERT_EQUALS(sv.conCodePage, Common::kDos850);
		TS_ASSERT(!sv.rpLogoVideo);
		// the two new rules in this build only
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			TSM_ASSERT(b.name, b.subtitleBreakBackOnly == (&b == &sv));
			TSM_ASSERT(b.name, b.nameSwedishLetters == (&b == &sv));
		}
		// size + timestamp name one build each
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
			for (uint j = i + 1; j < ARRAYSIZE(kAtlantisExeBuilds); j++)
				TS_ASSERT(kAtlantisExeBuilds[i].size != kAtlantisExeBuilds[j].size ||
				          kAtlantisExeBuilds[i].peTimestamp != kAtlantisExeBuilds[j].peTimestamp);
	}

	// C1: the name entry (FUN_0041d704 @ 0x0041dae5..0x0041dbe9).  ScummVM
	// gives Unicode; nameKeyCode turns it into the Windows-1252 code
	// WM_CHAR would carry, nameEntryChar into the stored byte.
	void test_name_entry_letters() {
		using namespace UiSv1997;
		const AtlantisExeBuild &sv = atlantisExeBuild(688128, kSvTimestamp);
		const AtlantisExeBuild &nl = atlantisExeBuild(688128, kNlTimestamp);
		const AtlantisExeBuild &en = atlantisExeBuild(688128, kEnTimestamp);
		static const struct { uint16 unicode; int stored; } kLetters[] = {
			{ 0xc4, 0x8e }, { 0xc5, 0x8f }, { 0xd6, 0x99 }, { 0xe4, 0x84 }, { 0xe5, 0x86 }, { 0xf6, 0x94 } };
		for (uint i = 0; i < ARRAYSIZE(kLetters); i++) {
			const uint code = nameKeyCode(kLetters[i].unicode);
			TS_ASSERT_EQUALS(code, (uint)kLetters[i].unicode);
			TS_ASSERT_EQUALS(nameEntryChar(code, sv.upperCaseNames, sv.nameSwedishLetters), kLetters[i].stored);
			// the Dutch and English builds refuse them (over 0xa7 / 0x7a)
			TS_ASSERT_EQUALS(nameEntryChar(code, nl.upperCaseNames, nl.nameSwedishLetters), -1);
			TS_ASSERT_EQUALS(nameEntryChar(code, en.upperCaseNames, en.nameSwedishLetters), -1);
		}
		// every other code by the 0x20..0xa7 rule, case kept
		TS_ASSERT_EQUALS(nameEntryChar('a', sv.upperCaseNames, sv.nameSwedishLetters), 'a');
		TS_ASSERT_EQUALS(nameEntryChar('Z', sv.upperCaseNames, sv.nameSwedishLetters), 'Z');
		TS_ASSERT_EQUALS(nameEntryChar(0x1f, sv.upperCaseNames, sv.nameSwedishLetters), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0x20, sv.upperCaseNames, sv.nameSwedishLetters), 0x20);
		TS_ASSERT_EQUALS(nameEntryChar(0xa7, sv.upperCaseNames, sv.nameSwedishLetters), 0xa7);
		TS_ASSERT_EQUALS(nameEntryChar(0xa8, sv.upperCaseNames, sv.nameSwedishLetters), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0xc6, sv.upperCaseNames, sv.nameSwedishLetters), -1);   // AE
		TS_ASSERT_EQUALS(nameEntryChar(0xe9, sv.upperCaseNames, sv.nameSwedishLetters), -1);   // e acute
		TS_ASSERT_EQUALS(nameEntryChar(0xfc, sv.upperCaseNames, sv.nameSwedishLetters), -1);   // u diaeresis
		// the code page bytes typed directly (0x84 is U+201E in
		// Windows-1252) stay as typed, as 0x20..0xa7 in the original
		TS_ASSERT_EQUALS(nameEntryChar(nameKeyCode(0x201e), sv.upperCaseNames, sv.nameSwedishLetters), 0x84);
		// the stored bytes have FONTMIN glyphs (advance table, 101 entries
		// then the credits table: the Swedish executable's bytes below)
		for (uint i = 0; i < ARRAYSIZE(kLetters); i++)
			TS_ASSERT(kLetters[i].stored - 0x20 >= 0 && kLetters[i].stored <= 0xa7);
	}

	// A1 on the bytes: the Swedish executable is picked by its timestamp;
	// its DGROUP section; the tables at en-688128's addresses hold
	// en-688128's bytes, the player row is `Ny spelare`, the newsound names
	// the reference's 137.  The code bytes of S1, S2, S3.
	void test_tables_by_timestamp() {
		using namespace UiSv1997;
		Common::Array<byte> e, en, ref;
		if (!readExeIn(AtlantisTest::configValue("sv1997"), e)) { TS_WARN("sv1997 not configured; skipped"); return; }
		TS_ASSERT_EQUALS(e.size(), 688128u);
		{
			Common::MemoryReadStream ms(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(ms), kSvTimestamp);
		}
		const AtlantisExeBuild &b = buildOf(e);
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-sv-688128");
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
			if (!memcmp(h, "BEGTEXT", 7)) {
				TS_ASSERT_EQUALS(READ_LE_UINT32(h + 12), 0x10000u);
				TS_ASSERT_EQUALS(READ_LE_UINT32(h + 20), 0x400u);
			}
		}
		TS_ASSERT(dgroup);
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "Ny spelare");
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486555)), "Ny spelare");
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		// FONTMAX 0x004865e7, FONTMIN 0x00486670, the credits right after
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e7)], kFontMaxMeasureWidths, 137));
		for (uint i = 0; i < 101; i++)
			TS_ASSERT_EQUALS((int)e[b.fileOffset(0x00486670) + i], fontMinAdvance(i));
		TS_ASSERT_EQUALS(b.creditWidthsVA, 0x00486670u + 101);
		// the FONTMIN advances of the six stored letters (0x84 15, 0x86 16,
		// 0x8e 18, 0x8f 18, 0x94 17, 0x99 19; experiment 2026-10-18-09)
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		TS_ASSERT_EQUALS(fontMinAdvance(0x84 - 0x20, credits), 15);
		TS_ASSERT_EQUALS(fontMinAdvance(0x86 - 0x20, credits), 16);
		TS_ASSERT_EQUALS(fontMinAdvance(0x8e - 0x20, credits), 18);
		TS_ASSERT_EQUALS(fontMinAdvance(0x8f - 0x20, credits), 18);
		TS_ASSERT_EQUALS(fontMinAdvance(0x94 - 0x20, credits), 17);
		TS_ASSERT_EQUALS(fontMinAdvance(0x99 - 0x20, credits), 19);

		// S2, FUN_0041d704: `cmp word ptr [0x0073da42], 0xa7` @ 0x0041daef,
		// then 0xc4 @ 0x0041dafa ... 0xf6 @ 0x0041db3b, the stores of 0x8e
		// @ 0x0041dbb2 ... 0x94 @ 0x0041dbe9
		TS_ASSERT(codeIs(e, 0x0041daef, "66813d42da7300a700"));
		TS_ASSERT(codeIs(e, 0x0041dafa, "66813d42da7300c400"));
		TS_ASSERT(codeIs(e, 0x0041db07, "66813d42da7300c500"));
		TS_ASSERT(codeIs(e, 0x0041db14, "66813d42da7300d600"));
		TS_ASSERT(codeIs(e, 0x0041db21, "66813d42da7300e400"));
		TS_ASSERT(codeIs(e, 0x0041db2e, "66813d42da7300e500"));
		TS_ASSERT(codeIs(e, 0x0041db3b, "66813d42da7300f600"));
		TS_ASSERT(codeIs(e, 0x0041dbb2, "66c70542da73008e00"));
		TS_ASSERT(codeIs(e, 0x0041dbbd, "66c70542da73008f00"));
		TS_ASSERT(codeIs(e, 0x0041dbc8, "66c70542da73009900"));
		TS_ASSERT(codeIs(e, 0x0041dbd3, "66c70542da73008400"));
		TS_ASSERT(codeIs(e, 0x0041dbde, "66c70542da73008600"));
		TS_ASSERT(codeIs(e, 0x0041dbe9, "66c70542da73009400"));
		// S3, FUN_0041c9b0: the knob at y 0 (`mov word ptr [0x00602774], 0`
		// @ 0x0041cc32), the clamp at 0x7b (@ 0x0041cd10 / 0x0041cd15), row
		// + offset < count (@ 0x0041cdb9..0x0041cdc3)
		TS_ASSERT(codeIs(e, 0x0041cc32, "66c705742760000000"));
		TS_ASSERT(codeIs(e, 0x0041cd10, "83f87b7d10"));
		TS_ASSERT(codeIs(e, 0x0041cd15, "66c705742760007b00"));
		TS_ASSERT(codeIs(e, 0x0041cdb9, "a0de65480001d03b45d0"));
		// S1, FUN_0042ea7c: the backward scan ends in `jmp 0x0042eb50`, which
		// is `mov ebx, 0x4b; sub ebx, [ebp - 0x14]` (75 - b), no forward branch
		TS_ASSERT(codeIs(e, 0x0042eb4c, "eb02"));
		TS_ASSERT(codeIs(e, 0x0042eb50, "bb4b0000002b5dec"));
		// no LOGORP: the `logorp.hnm` string at 0x00486921 has no reference
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486921)), "logorp.hnm");
		bool referenced = false;
		for (uint32 i = textOffset(0x00410000); i + 4 <= b.dgroupRawPtr; i++)
			referenced |= READ_LE_UINT32(&e[i]) == 0x00486921u;
		TS_ASSERT(!referenced);
		// the credits FUN_0041a600: `mov edx, 0x004ac1a4` (`credits.txt`) @
		// 0x0041a71e (creditsPerRun)
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x004ac1a4)), "credits.txt");
		TS_ASSERT(codeIs(e, 0x0041a71e, "baa4c14a00"));

		// against the English executable of May 1997: the same bytes at the
		// same addresses
		if (readExeIn(AtlantisTest::configValue("en1997"), en)) {
			const AtlantisExeBuild &enb = buildOf(en);
			TS_ASSERT_EQUALS(Common::String(enb.name), "atlantis-cd-en-688128");
			TS_ASSERT(!memcmp(&e[b.fileOffset(b.inventoryVA)], &en[enb.fileOffset(enb.inventoryVA)], 52 * 4));
			TS_ASSERT(!memcmp(&e[b.fileOffset(b.guestVA)], &en[enb.fileOffset(enb.guestVA)], 10 * 4));
			TS_ASSERT(!memcmp(&e[b.fileOffset(b.creditWidthsVA)], &en[enb.fileOffset(enb.creditWidthsVA)], 0xb3));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e7)], &en[enb.fileOffset(0x004865e7)], 137));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x00485eec)], &en[enb.fileOffset(0x00485eec)], 31 * 13));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x00486080)], &en[enb.fileOffset(0x00486080)], 16));
			TS_ASSERT_EQUALS(cstr(en, enb.fileOffset(enb.playerRowVA)), "New Player");
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

	// D1 / D2 on the golden's synthetic texts: each build's emulated rows
	// (sv-688128 FUN_0042ea7c, en-688128 FUN_0042e970) against the port's
	// breaker with and without backOnly; the space-free runs, where the
	// Swedish original faults, cut at 75 bytes by the port.
	void test_breaker_synthetic_vs_golden() {
		using namespace UiSv1997;
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> golden;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_sv688128.txt", golden));
		int checked = 0;
		for (uint i = 0; i < golden.size(); i++) {
			const bool sv = golden[i].hasPrefix("sv-688128 len");
			const bool en = golden[i].hasPrefix("en-688128 len");
			if ((!sv && !en) || !golden[i].contains("'"))
				continue;
			Common::Array<Common::String> want, got;
			TSM_ASSERT(golden[i].c_str(), parseGoldenRows(golden[i], want));
			const Common::String b = joined(want);
			TSM_ASSERT(golden[i].c_str(), b.hasPrefix(" "));
			splitSubtitleBitmap(b.substr(1), got, sv);
			TSM_ASSERT_EQUALS(golden[i].c_str(), got.size(), want.size());
			for (uint r = 0; r < got.size() && r < want.size(); r++)
				TSM_ASSERT_EQUALS(golden[i].c_str(), got[r], want[r]);
			checked++;
		}
		TS_ASSERT_EQUALS(checked, 16);
		// the space-free runs of the golden: `x` * 75, `x` * 76 + " tail",
		// "short " + `x` * 80; en-688128's rows as emulated with the zero
		// tail of a first line after start-up (10 empty rows; 77 | 5; a
		// fault), sv-688128 faults: the port's cuts (76 and 87 at the end
		// of the text, decision 0016 row 26; 75, decision 0023 D2)
		const Common::String run75 = Common::String(75, 'x');
		const Common::String run81 = Common::String(76, 'x') + " tail";
		const Common::String run86 = "short " + Common::String(80, 'x');
		bool sawFaults = false;
		for (uint i = 0; i < golden.size(); i++)
			sawFaults |= golden[i].hasPrefix("sv-688128 len  86 -> faults");
		TS_ASSERT(sawFaults);
		Common::Array<Common::String> rows;
		splitSubtitleBitmap(run75, rows, false);
		TS_ASSERT(rows.size() == 1 && rows[0].size() == 76);
		splitSubtitleBitmap(run81, rows, false);
		TS_ASSERT(rows.size() == 2 && rows[0].size() == 77 && rows[1].size() == 5);
		// " short" (the forward scan runs into the zero tail: the backward
		// cut), then a window without a space: the rest of the text
		splitSubtitleBitmap(run86, rows, false);
		TS_ASSERT(rows.size() == 2 && rows[0] == " short" && rows[1].size() == 81);
		splitSubtitleBitmap(run75, rows, true);
		TS_ASSERT(rows.size() == 2 && rows[0].size() == 75 && rows[1] == "x");
		splitSubtitleBitmap(run81, rows, true);
		TS_ASSERT(rows.size() == 2 && rows[0] == " " + Common::String(74, 'x') && rows[1] == "xx tail");
		// " short" (cut at its space), then the run from that space: no
		// space after its first byte within 75, cut at 75, and the rest
		splitSubtitleBitmap(run86, rows, true);
		TS_ASSERT_EQUALS(rows.size(), 3u);
		if (rows.size() == 3) {
			TS_ASSERT_EQUALS(rows[0], " short");
			TS_ASSERT_EQUALS(rows[1], " " + Common::String(74, 'x'));
			TS_ASSERT_EQUALS(rows[2], Common::String(6, 'x'));
		}
		for (uint i = 0; i < rows.size(); i++)
			TS_ASSERT(rows[i].size() <= 75);
	}

	// D1 on the 2539 Swedish dialog lines, against the golden's scan
	// (atlantis-re tools/emu/video_subtitles_sv688128.py): this build's
	// rows 1..5 = 1920 / 520 / 85 / 9 / 5, longest 75, none over 75;
	// en-688128's breaker 1920 / 525 / 80 / 10 / 4, longest 81, 255 rows
	// over 75 and 16 over 79; 242 lines broken differently, 6 with one row
	// more in this build, none with fewer (each line as the first after
	// start-up, a zero tail after its NUL).
	void test_subtitle_scan_vs_golden() {
		const Common::String dir = AtlantisTest::configValue("sv1997");
		if (dir.empty()) { TS_WARN("sv1997 not configured; skipped"); return; }
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> golden;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_sv688128.txt", golden));
		bool sawSv = false, sawEn = false, sawDiff = false;
		for (uint i = 0; i < golden.size(); i++) {
			sawSv |= golden[i].hasPrefix("sv-688128: {'lines': 2539, 'rows_hist': {1: 1920, 2: 520, 3: 85, 4: 9, 5: 5}, "
			                             "'max_row_len': 75, 'rows_over_75': 0, 'rows_over_79': 0}");
			sawEn |= golden[i].hasPrefix("en-688128: {'lines': 2539, 'rows_hist': {1: 1920, 2: 525, 3: 80, 4: 10, 5: 4}, "
			                             "'max_row_len': 81, 'rows_over_75': 255, 'rows_over_79': 16}");
			sawDiff |= golden[i].hasPrefix("## lines whose rows differ between the two breakers: 242 "
			                               "(more rows with sv-688128: 6, fewer: 0)");
		}
		TS_ASSERT(sawSv && sawEn && sawDiff);
		VideoTest::Big big(dir);
		TS_ASSERT(!big.empty());
		int lines = 0, histSv[9] = { 0 }, histEn[9] = { 0 }, longestSv = 0, longestEn = 0;
		int over75Sv = 0, over75En = 0, over79En = 0, differ = 0, more = 0, fewer = 0;
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
				Common::Array<Common::String> sv, en;
				CryOmni3D::Atlantis::splitSubtitleBitmap(texts[t], sv, true);
				CryOmni3D::Atlantis::splitSubtitleBitmap(texts[t], en, false);
				lines++;
				if (sv.size() < ARRAYSIZE(histSv))
					histSv[sv.size()]++;
				if (en.size() < ARRAYSIZE(histEn))
					histEn[en.size()]++;
				for (uint r = 0; r < sv.size(); r++) {
					longestSv = MAX(longestSv, (int)sv[r].size());
					over75Sv += sv[r].size() > 75;
				}
				for (uint r = 0; r < en.size(); r++) {
					longestEn = MAX(longestEn, (int)en[r].size());
					over75En += en[r].size() > 75;
					over79En += en[r].size() > 79;
				}
				// the rows partition the buffer " " + text
				TS_ASSERT_EQUALS(UiSv1997::joined(sv), " " + texts[t]);
				if (sv != en) {
					differ++;
					more += sv.size() > en.size();
					fewer += sv.size() < en.size();
				}
			}
		}
		TS_ASSERT_EQUALS(lines, 2539);
		TS_ASSERT_EQUALS(histSv[1], 1920);
		TS_ASSERT_EQUALS(histSv[2], 520);
		TS_ASSERT_EQUALS(histSv[3], 85);
		TS_ASSERT_EQUALS(histSv[4], 9);
		TS_ASSERT_EQUALS(histSv[5], 5);
		TS_ASSERT_EQUALS(longestSv, 75);
		TS_ASSERT_EQUALS(over75Sv, 0);
		TS_ASSERT_EQUALS(histEn[1], 1920);
		TS_ASSERT_EQUALS(histEn[2], 525);
		TS_ASSERT_EQUALS(histEn[3], 80);
		TS_ASSERT_EQUALS(histEn[4], 10);
		TS_ASSERT_EQUALS(histEn[5], 4);
		TS_ASSERT_EQUALS(longestEn, 81);
		TS_ASSERT_EQUALS(over75En, 255);
		TS_ASSERT_EQUALS(over79En, 16);
		TS_ASSERT_EQUALS(differ, 242);
		TS_ASSERT_EQUALS(more, 6);
		TS_ASSERT_EQUALS(fewer, 0);
	}

	// The golden PNG: a Swedish subtitle broken by FUN_0042ea7c and drawn by
	// the original drawer (FUN_0042ebfc + FUN_00410800) with the Swedish
	// FONTS.BIN on a 640 x 480 RGB565 screen of 0x8410, rows 400..479; the
	// port draws the same rows: every pixel equal, a ring / a diaeresis /
	// o diaeresis the IBM PC glyphs 0x86 0x84 0x94.
	void test_pixels_vs_golden_png() {
		const Common::String dir = AtlantisTest::configValue("sv1997");
		if (dir.empty()) { TS_WARN("sv1997 not configured; skipped"); return; }
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		VideoTest::Big big(dir);
		uint32 fontSize = 0;
		byte *raw = big.get("DIALOG\\FONTS.BIN", fontSize);
		TS_ASSERT(raw && fontSize == 16384);
		if (!raw)
			return;
		Common::Array<byte> font(raw, fontSize);
		delete[] raw;
		const Common::String text("Nej. Hon \x84r p\x86 en rundtur p\x86 \x94n. Creon \x84r antagligen i palatset med "
		                          "Gimbas, Sa'ats nya \x94verstepr\x84st. De tillbringar mycket tid tillsammans.");
		Common::Array<uint16> screen(640 * 480, 0x8410);
		Common::Array<Common::String> rows;
		CryOmni3D::Atlantis::splitSubtitleBitmap(text, rows, true);
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
		                                .appendComponent("video_subtitles_sv688128.png"))));
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

	// E: the CON bytes in code page 850 (the Swedish letters 0x84 0x86 0x94
	// 0x8e 0x8f 0x99 and e acute 0x82), not Windows-1252, the patch
	// build's (decision 0023 B2: punctuation, 0x8f undefined).
	void test_con_code_page() {
		using namespace UiSv1997;
		const AtlantisExeBuild &sv = atlantisExeBuild(688128, kSvTimestamp);
		const Common::String bytes("\x84 \x86 \x94 \x8e \x8f \x99 \x82");
		const Common::U32String want = Common::U32String("\xc3\xa4 \xc3\xa5 \xc3\xb6 \xc3\x84 \xc3\x85 \xc3\x96 \xc3\xa9");
		TS_ASSERT(Common::U32String(bytes, sv.conCodePage) == want);
		const AtlantisExeBuild &patch = atlantisExeBuild(714240, 0x385a4dc6);
		TS_ASSERT_EQUALS(Common::String(patch.name), "atlantis-patch-fr-714240");
		TS_ASSERT(Common::U32String(bytes, patch.conCodePage) != want);
	}

	// E (decision 0023, its 2026-10-19 correction): the per-disc copies.
	// Of the members on all four Swedish discs only six differ
	// (evidence/compare/sv_688128_archives.txt): EPI.TXT and SELEMENU.TXT
	// (disc 1 newer than 2..4) and CD1..4MENU.TXT (disc 1, discs 2..3,
	// disc 4); the original loads them once at start-up from disc 1 (the
	// merged lookup).  What it reads on the current disc (FONTMIN,
	// FONTMIN2, CREDITS.TXT) is the same on every disc.  No LOGORP member.
	void test_disc_copies() {
		using namespace UiSv1997;
		const Common::String dir = AtlantisTest::configValue("sv1997");
		if (dir.empty()) { TS_WARN("sv1997 not configured; skipped"); return; }
		SearchMan.addDirectory("ui_sv_data", Common::FSNode(Common::Path(dir, '/')), kOwnArchivesFirst);
		BigFileArchive big;
		bool opened = true;
		for (uint disc = 1; disc <= 4; disc++)
			opened &= big.open(Common::Path(Common::String::format("BIGCD%u.BIG", disc)), disc);
		SearchMan.remove("ui_sv_data");
		TS_ASSERT(opened);
		if (!opened)
			return;
		static const char *const kStartUp[] = {
			"SPRLIST\\EPI.TXT", "SPRLIST\\SELEMENU.TXT",
			"SPRLIST\\CD1MENU.TXT", "SPRLIST\\CD2MENU.TXT", "SPRLIST\\CD3MENU.TXT", "SPRLIST\\CD4MENU.TXT" };
		for (uint i = 0; i < ARRAYSIZE(kStartUp); i++) {
			const Common::String n = kStartUp[i];
			const Common::String merged = md5Of(big.createReadStreamForMember(n));
			const Common::String d1 = md5Of(big.createReadStreamForMemberOnDisc(n, 1));
			const Common::String d2 = md5Of(big.createReadStreamForMemberOnDisc(n, 2));
			const Common::String d3 = md5Of(big.createReadStreamForMemberOnDisc(n, 3));
			const Common::String d4 = md5Of(big.createReadStreamForMemberOnDisc(n, 4));
			TSM_ASSERT(n.c_str(), !merged.empty());
			TSM_ASSERT_EQUALS(n.c_str(), merged, d1);
			TSM_ASSERT(n.c_str(), d2 != d1);
			TSM_ASSERT_EQUALS(n.c_str(), d3, d2);
			if (n.hasPrefix("SPRLIST\\CD")) {
				TSM_ASSERT(n.c_str(), d4 != d2 && d4 != d1);
			} else {
				TSM_ASSERT_EQUALS(n.c_str(), d4, d2);
			}
		}
		TS_ASSERT(md5Of(big.createReadStreamForMember(Common::String("SPRLIST\\EPI.TXT"))).hasPrefix("43439dd3"));
		TS_ASSERT(md5Of(big.createReadStreamForMemberOnDisc("SPRLIST\\SELEMENU.TXT", 2)).hasPrefix("04c8e342"));
		// disc 1's SELEMENU: `SPELARNAMN`, discs 2..4 `SPELARNAMM`
		{
			Common::ScopedPtr<Common::SeekableReadStream> s(big.createReadStreamForMemberOnDisc("SPRLIST\\SELEMENU.TXT", 1));
			TS_ASSERT(s);
			Common::String t;
			while (s && !s->eos())
				t += s->readLine() + "\n";
			TS_ASSERT(t.contains("SPELARNAMN"));
			s.reset(big.createReadStreamForMemberOnDisc("SPRLIST\\SELEMENU.TXT", 2));
			t.clear();
			while (s && !s->eos())
				t += s->readLine() + "\n";
			TS_ASSERT(t.contains("SPELARNAMM"));
		}
		static const char *const kCurrentDisc[] = {
			"SPRITE\\2D\\FONTMIN.SPR", "SPRITE\\2D\\FONTMIN2.SPR", "SPRLIST\\CREDITS.TXT" };
		for (uint i = 0; i < ARRAYSIZE(kCurrentDisc); i++) {
			const Common::String n = kCurrentDisc[i];
			const Common::String d1 = md5Of(big.createReadStreamForMemberOnDisc(n, 1));
			TSM_ASSERT(n.c_str(), !d1.empty());
			for (uint disc = 2; disc <= 4; disc++) {
				TSM_ASSERT(n.c_str(), big.hasFile(n));
				TSM_ASSERT_EQUALS(n.c_str(), md5Of(big.createReadStreamForMemberOnDisc(n, disc)), d1);
			}
		}
		// FONTMIN is fr-687104's and the Dutch build's (adf25e4b...)
		TS_ASSERT(md5Of(big.createReadStreamForMemberOnDisc("SPRITE\\2D\\FONTMIN.SPR", 3)).hasPrefix("adf25e4b"));
		TS_ASSERT(!big.hasFile(Common::String("UBB_VUE\\LOGORP.HNM")));
	}
};

#endif
