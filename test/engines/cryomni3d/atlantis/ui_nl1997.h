#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_NL1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_NL1997_H

// ui audit: the Dutch CD of June 1997 (atlantis-cd-nl-688128), as
// atlantis-re experiment 2026-10-18-08 read it and decision 0022 chose:
//   A1 the build entry, told apart from atlantis-cd-en-688128 (same size)
//      by its PE timestamp 0x3396af27: en-688128's flags but the name entry
//      (0x20..0xa7, case kept), its player row `Nieuwe speler`, its
//      inventory 0x0048678c, guests 0x00486860 and credits 0x004866d8;
//   C1 the R&P logo UBB_VUE\LOGORP.HNM at start-up (rpLogoVideo);
//   D  the CON texts in code page 850 for any rendering but FONTS.BIN;
//   F  the per-disc copies: the members read after start-up (FONTMIN,
//      FONTMIN2, CREDITS.TXT in this code family) from the current disc
//      (BigFileArchive::createReadStreamForMemberOnDisc), disc 2 holding an
//      older revision;
//   subtitles: en-688128's FONTS.BIN breaker on the Dutch lines, against
//   the emulated golden video_subtitles_nl688128.txt / .png.
//
// Port code exercised directly: atlantisExeBuild / readPeTimestamp
// (exe_build.h), readNewSoundNames (exe_newsound.h), fontMinAdvance
// (menu_layout.cpp), nameEntryChar (menu_layout.cpp), BigFileArchive
// (bigfile.cpp), splitSubtitleBitmap / drawSubtitleBitmapRow
// (subtitle_layout.cpp), Video::HNMDecoder.
// Real files: `nl1997` (ATLANTIS.EXE and the four Dutch BIGCD*.BIG),
// `en1997` and `data` in atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "video_subtitles_en1997.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/ustr.h"
#include "graphics/surface.h"
#include "image/png.h"
#include "video/hnm_decoder.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/exe_newsound.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiNl1997 {

using namespace CryOmni3D::Atlantis;

static const uint32 kNlTimestamp = 0x3396af27;
static const uint32 kEnTimestamp = 0x33886fd1;

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

inline Common::String md5Of(Common::SeekableReadStream *s) {
	if (!s)
		return Common::String();
	Common::String m = Common::computeStreamMD5AsString(*s);
	delete s;
	return m;
}

} // End of namespace UiNl1997

class UiNl1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// A1: the entry by size and timestamp, en-688128's flags but the name
	// entry, its own tables; C1 and D flags.
	void test_build_entry() {
		using namespace UiNl1997;
		const AtlantisExeBuild &nl = atlantisExeBuild(688128, kNlTimestamp);
		const AtlantisExeBuild &en = atlantisExeBuild(688128, kEnTimestamp);
		TS_ASSERT_EQUALS(Common::String(nl.name), "atlantis-cd-nl-688128");
		TS_ASSERT_EQUALS(Common::String(en.name), "atlantis-cd-en-688128");
		// the size alone or an unknown timestamp: en-688128 (the first)
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(688128).name), "atlantis-cd-en-688128");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(688128, 0).name), "atlantis-cd-en-688128");
		TS_ASSERT_EQUALS(nl.size, 688128u);
		TS_ASSERT_EQUALS(nl.dgroupVA, 0x00480000u);
		TS_ASSERT_EQUALS(nl.dgroupRawPtr, 0x6c400u);
		TS_ASSERT_EQUALS(nl.inventoryVA, 0x0048678cu);
		TS_ASSERT_EQUALS(nl.guestVA, 0x00486860u);
		TS_ASSERT_EQUALS(nl.creditWidthsVA, 0x004866d8u);
		TS_ASSERT_EQUALS(nl.inventoryVA - en.inventoryVA, 4u);
		TS_ASSERT_EQUALS(nl.guestVA - en.guestVA, 4u);
		TS_ASSERT_EQUALS(nl.creditWidthsVA - en.creditWidthsVA, 3u);
		TS_ASSERT_EQUALS(nl.newSoundTableVA, 0x004861d0u);
		TS_ASSERT(nl.newSoundPtrMin == 0x004a0000u && nl.newSoundPtrMax == 0x004b0000u);
		TS_ASSERT_EQUALS(nl.playerRowVA, 0x0048654au);
		TS_ASSERT(!nl.playerLabelAfterMarker);
		// the name entry: 0x20..0xa7, case kept (FUN_0041d70c)
		TS_ASSERT(en.upperCaseNames);
		TS_ASSERT(!nl.upperCaseNames);
		TS_ASSERT_EQUALS(nameEntryChar('a', nl.upperCaseNames), 'a');
		TS_ASSERT_EQUALS(nameEntryChar('a', en.upperCaseNames), 'A');
		TS_ASSERT_EQUALS(nameEntryChar(0xa7, nl.upperCaseNames), 0xa7);
		TS_ASSERT_EQUALS(nameEntryChar(0xa8, nl.upperCaseNames), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0x7b, en.upperCaseNames), -1);
		// every other flag as en-688128's
		TS_ASSERT_EQUALS(nl.cursorIdRule, en.cursorIdRule);
		TS_ASSERT_EQUALS(nl.rheaCamera2, en.rheaCamera2);
		TS_ASSERT_EQUALS(nl.altGameOverNarration, en.altGameOverNarration);
		TS_ASSERT_EQUALS(nl.bitmapSubtitles, en.bitmapSubtitles);
		TS_ASSERT(nl.bitmapSubtitles);
		TS_ASSERT_EQUALS(nl.subtitleClickCycle, en.subtitleClickCycle);
		TS_ASSERT_EQUALS(nl.playerListVerbatim, en.playerListVerbatim);
		TS_ASSERT(nl.playerListVerbatim);
		TS_ASSERT_EQUALS(nl.gameOverVoiceAtEntry, en.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(nl.loadPickerKnobAtBar, en.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(nl.loadPickerClamp, en.loadPickerClamp);
		TS_ASSERT_EQUALS(nl.hexaForcesObject402, en.hexaForcesObject402);
		TS_ASSERT_EQUALS(nl.ogreRevealFrames, en.ogreRevealFrames);
		TS_ASSERT_EQUALS(nl.ogreRevealTicks, en.ogreRevealTicks);
		TS_ASSERT_EQUALS(nl.ratFrameDeadline, en.ratFrameDeadline);
		TS_ASSERT_EQUALS(nl.ratT0023Restart, en.ratT0023Restart);
		TS_ASSERT_EQUALS(nl.musicFirstValueRule, en.musicFirstValueRule);
		TS_ASSERT_EQUALS(nl.muriaPlacedFrames, en.muriaPlacedFrames);
		TS_ASSERT_EQUALS(nl.musicLoadAllOrNothing, en.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(nl.musicLoadMinVolume, en.musicLoadMinVolume);
		TS_ASSERT_EQUALS(nl.musicStartSameTrackNoop, en.musicStartSameTrackNoop);
		// D: code page 850; C1: the R&P logo, in this build only
		TS_ASSERT_EQUALS(nl.conCodePage, Common::kDos850);
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
			TSM_ASSERT(kAtlantisExeBuilds[i].name,
			           kAtlantisExeBuilds[i].rpLogoVideo == (&kAtlantisExeBuilds[i] == &nl));
		// F: CREDITS.TXT loaded by each credits run in the en-688128 code
		// family only (else once at start-up with the menus)
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const Common::String n = kAtlantisExeBuilds[i].name;
			TSM_ASSERT(n.c_str(), kAtlantisExeBuilds[i].creditsPerRun ==
			           (n == "atlantis-cd-en-688128" || n == "atlantis-cd-fr-688640" || n == "atlantis-cd-nl-688128"));
		}
		// size + timestamp name one build each
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
			for (uint j = i + 1; j < ARRAYSIZE(kAtlantisExeBuilds); j++)
				TS_ASSERT(kAtlantisExeBuilds[i].size != kAtlantisExeBuilds[j].size ||
				          kAtlantisExeBuilds[i].peTimestamp != kAtlantisExeBuilds[j].peTimestamp);
	}

	// A1 on the bytes: the Dutch executable is picked by its timestamp; its
	// DGROUP section; the tables at its addresses hold en-688128's bytes,
	// the player row is `Nieuwe speler`, the newsound names the
	// reference's 137.  At en-688128's addresses (what the size-only lookup
	// read) the inventory and guest tables and the credits are other bytes.
	void test_tables_by_timestamp() {
		using namespace UiNl1997;
		Common::Array<byte> e, en, ref;
		if (!readExeIn(AtlantisTest::configValue("nl1997"), e)) { TS_WARN("nl1997 not configured; skipped"); return; }
		TS_ASSERT_EQUALS(e.size(), 688128u);
		{
			Common::MemoryReadStream ms(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(ms), kNlTimestamp);
		}
		const AtlantisExeBuild &b = buildOf(e);
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-nl-688128");
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
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "Nieuwe speler");
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486558)), "Nieuwe speler");
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		// FONTMAX 0x004865ea, FONTMIN 0x00486673, the credits table right
		// after FONTMIN
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865ea)], kFontMaxMeasureWidths, 137));
		for (uint i = 0; i < 101; i++)
			TS_ASSERT_EQUALS((int)e[b.fileOffset(0x00486673) + i], fontMinAdvance(i));
		TS_ASSERT_EQUALS(b.creditWidthsVA, 0x00486673u + 101);
		// the bytes the size-only lookup (en-688128's entry) read: the
		// inventory table's terminator 4 bytes later than it expects
		const AtlantisExeBuild &eb = atlantisExeBuild(688128);
		TS_ASSERT_EQUALS(Common::String(eb.name), "atlantis-cd-en-688128");
		TS_ASSERT(READ_LE_UINT32(&e[eb.fileOffset(eb.inventoryVA) + 51 * 4]) != 0xffffffffu);
		TS_ASSERT(READ_LE_UINT32(&e[eb.fileOffset(eb.guestVA) + 9 * 4]) != 0xffffffffu);
		TS_ASSERT(memcmp(&e[eb.fileOffset(eb.creditWidthsVA)], &e[b.fileOffset(b.creditWidthsVA)], 0xb3));
		// the R&P logo in WinMain: `mov edx, 0x00486925` (`logorp.hnm`) @
		// 0x0041957e, `call FUN_00423444` @ 0x0041958d, the cinema flag
		// 0x0048617c cleared @ 0x004195e0, after PREINTRO's call @ 0x004195db
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486925)), "logorp.hnm");
		static const byte kMovLogoRp[] = { 0xba, 0x25, 0x69, 0x48, 0x00 };
		TS_ASSERT(!memcmp(&e[textOffset(0x0041957e)], kMovLogoRp, sizeof(kMovLogoRp)));
		TS_ASSERT_EQUALS(e[textOffset(0x0041958d)], 0xe8);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[textOffset(0x0041958e)]), 0x00423444u - 0x00419592u);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[textOffset(0x004195dc)]), 0x00423444u - 0x004195e0u);
		static const byte kCinemaOff[] = { 0xc7, 0x05, 0x7c, 0x61, 0x48, 0x00, 0, 0, 0, 0 };
		TS_ASSERT(!memcmp(&e[textOffset(0x004195e0)], kCinemaOff, sizeof(kCinemaOff)));
		// F: the start-up loader FUN_00425f70 called by WinMain @ 0x0041931b,
		// `preintro.tga` (0x004abc3c) @ 0x00419608 in WinMain; the credits
		// FUN_0041a630 push `credits.txt` (0x004ac1b8) @ 0x0041a74e
		TS_ASSERT_EQUALS(e[textOffset(0x0041931b)], 0xe8);
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[textOffset(0x0041931c)]), 0x00425f70u - 0x00419320u);
		static const byte kMovPreintroTga[] = { 0xba, 0x3c, 0xbc, 0x4a, 0x00 };
		TS_ASSERT(!memcmp(&e[textOffset(0x00419608)], kMovPreintroTga, sizeof(kMovPreintroTga)));
		static const byte kMovCredits[] = { 0xba, 0xb8, 0xc1, 0x4a, 0x00 };
		TS_ASSERT(!memcmp(&e[textOffset(0x0041a74e)], kMovCredits, sizeof(kMovCredits)));
		// the name entry's bound: `cmp word ptr [0x0073da42], 0xa7` @ 0x0041daf7
		static const byte kCmpA7[] = { 0x66, 0x81, 0x3d, 0x42, 0xda, 0x73, 0x00, 0xa7, 0x00 };
		TS_ASSERT(!memcmp(&e[textOffset(0x0041daf7)], kCmpA7, sizeof(kCmpA7)));

		// against the English executable of May 1997: the same bytes, the
		// tables after the player row 3 / 4 bytes later
		if (readExeIn(AtlantisTest::configValue("en1997"), en)) {
			const AtlantisExeBuild &enb = buildOf(en);
			TS_ASSERT_EQUALS(Common::String(enb.name), "atlantis-cd-en-688128");
			TS_ASSERT(!memcmp(&e[b.fileOffset(b.inventoryVA)], &en[enb.fileOffset(enb.inventoryVA)], 52 * 4));
			TS_ASSERT(!memcmp(&e[b.fileOffset(b.guestVA)], &en[enb.fileOffset(enb.guestVA)], 10 * 4));
			TS_ASSERT(!memcmp(&e[b.fileOffset(b.creditWidthsVA)], &en[enb.fileOffset(enb.creditWidthsVA)], 0xb3));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865ea)], &en[enb.fileOffset(0x004865e7)], 137));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x00485eec)], &en[enb.fileOffset(0x00485eec)], 31 * 13));
			TS_ASSERT(!memcmp(&e[b.fileOffset(0x00486080)], &en[enb.fileOffset(0x00486080)], 16));
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

	// C1: the logo video the start-up plays, on disc 1 only, 60 HNM6 frames
	// of 640 x 480 that the decoder reads; no LOGORP.APC (no sound).
	void test_logorp_video() {
		const Common::String dir = AtlantisTest::configValue("nl1997");
		if (dir.empty()) { TS_WARN("nl1997 not configured; skipped"); return; }
		VideoTest::Big big(dir);
		uint32 size = 0;
		byte *d = big.get("UBB_VUE\\LOGORP.HNM", size);
		TS_ASSERT(d);
		if (!d)
			return;
		TS_ASSERT_EQUALS(size, 1082252u);
		TS_ASSERT(!memcmp(d, "HNM6", 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(d + 0x10), 60u);
		uint32 apcSize = 0;
		byte *apc = big.get("WAV\\LOGORP.APC", apcSize);
		TS_ASSERT(!apc);
		delete[] apc;
		const Graphics::PixelFormat fmt(2, 5, 6, 5, 0, 11, 5, 0, 0);
		Video::HNMDecoder dec(fmt);
		dec.setRegularFrameDelay(13 * 5);
		TS_ASSERT(dec.loadStream(new Common::MemoryReadStream(d, size, DisposeAfterUse::YES)));
		TS_ASSERT_EQUALS(dec.getFrameCount(), 60);
		TS_ASSERT_EQUALS(dec.getWidth(), 640u);
		TS_ASSERT_EQUALS(dec.getHeight(), 480u);
		int frames = 0;
		while (!dec.endOfVideo() && frames < 100) {
			if (!dec.decodeNextFrame())
				break;
			frames++;
		}
		TS_ASSERT_EQUALS(frames, 60);
	}

	// F: the per-disc copies.  The four Dutch archives mounted as the
	// engine mounts them; the members whose disc 2 copy is the older
	// revision (atlantis-re evidence/compare/nl_688128_archives.txt) read
	// on each disc: disc 2's own copy, discs 1 and 3 the newest, the
	// merged lookup disc 1's; EPI.TXT has a third revision on disc 4.
	void test_disc_copies() {
		using namespace UiNl1997;
		const Common::String dir = AtlantisTest::configValue("nl1997");
		if (dir.empty()) { TS_WARN("nl1997 not configured; skipped"); return; }
		SearchMan.addDirectory("ui_nl_data", Common::FSNode(Common::Path(dir, '/')));
		BigFileArchive big;
		bool opened = true;
		for (uint disc = 1; disc <= 4; disc++)
			opened &= big.open(Common::Path(Common::String::format("BIGCD%u.BIG", disc)), disc);
		SearchMan.remove("ui_nl_data");
		TS_ASSERT(opened);
		if (!opened)
			return;
		static const char *const kDisc2Older[] = {
			"IMAGES\\PREINTRO.TGA", "SPRITE\\2D\\FONTMIN.SPR", "SPRITE\\2D\\FONTMIN2.SPR",
			"SPRLIST\\CD1MENU.TXT", "SPRLIST\\CD2MENU.TXT", "SPRLIST\\CD3MENU.TXT", "SPRLIST\\CD4MENU.TXT",
			"SPRLIST\\CREDITS.TXT", "SPRLIST\\EPI.TXT", "SPRLIST\\ERROR1.TXT", "SPRLIST\\ERROR2.TXT",
			"SPRLIST\\ERROR3.TXT", "SPRLIST\\LOADMENU.TXT", "SPRLIST\\OPTMENU.TXT", "SPRLIST\\SONMENU.TXT",
			"SPRLIST\\SUREMENU.TXT", "SPRLIST\\WAIT.TXT" };
		for (uint i = 0; i < ARRAYSIZE(kDisc2Older); i++) {
			const Common::String n = kDisc2Older[i];
			const Common::String merged = md5Of(big.createReadStreamForMember(n));
			const Common::String d1 = md5Of(big.createReadStreamForMemberOnDisc(n, 1));
			const Common::String d2 = md5Of(big.createReadStreamForMemberOnDisc(n, 2));
			const Common::String d3 = md5Of(big.createReadStreamForMemberOnDisc(n, 3));
			const Common::String d4 = md5Of(big.createReadStreamForMemberOnDisc(n, 4));
			TSM_ASSERT(n.c_str(), !merged.empty());
			TSM_ASSERT_EQUALS(n.c_str(), merged, d1);
			TSM_ASSERT(n.c_str(), d2 != d1);
			TSM_ASSERT_EQUALS(n.c_str(), d3, d1);
			if (n != "SPRLIST\\EPI.TXT") {
				TSM_ASSERT_EQUALS(n.c_str(), d4, d1);
			} else {
				TSM_ASSERT(n.c_str(), d4 != d1 && d4 != d2);
			}
		}
		// disc 2's FONTMIN is the reference's file (md5 72e8d5b7...), discs
		// 1, 3, 4 fr-687104's (adf25e4b...)
		TS_ASSERT(md5Of(big.createReadStreamForMemberOnDisc("SPRITE\\2D\\FONTMIN.SPR", 2)).hasPrefix("72e8d5b7"));
		TS_ASSERT(md5Of(big.createReadStreamForMemberOnDisc("SPRITE\\2D\\FONTMIN.SPR", 1)).hasPrefix("adf25e4b"));
		// disc 2's SONMENU: `Musiek`; disc 1's `Muziek`
		{
			Common::ScopedPtr<Common::SeekableReadStream> s(big.createReadStreamForMemberOnDisc("SPRLIST\\SONMENU.TXT", 2));
			TS_ASSERT(s);
			Common::String t;
			while (s && !s->eos())
				t += s->readLine() + "\n";
			TS_ASSERT(t.contains("<80,190>Musiek"));
			s.reset(big.createReadStreamForMemberOnDisc("SPRLIST\\SONMENU.TXT", 1));
			t.clear();
			while (s && !s->eos())
				t += s->readLine() + "\n";
			TS_ASSERT(t.contains("<80,190>Muziek"));
		}
		// a member the disc lacks (LOGORP.HNM is on disc 1 only) or a disc
		// not mounted: the merged lookup
		const Common::String logo = md5Of(big.createReadStreamForMember(Common::String("UBB_VUE\\LOGORP.HNM")));
		TS_ASSERT(logo.hasPrefix("284494a7"));
		TS_ASSERT_EQUALS(md5Of(big.createReadStreamForMemberOnDisc("UBB_VUE\\LOGORP.HNM", 2)), logo);
		TS_ASSERT_EQUALS(md5Of(big.createReadStreamForMemberOnDisc("SPRLIST\\EPI.TXT", 7)),
		                 md5Of(big.createReadStreamForMemberOnDisc("SPRLIST\\EPI.TXT", 1)));
		TS_ASSERT(!big.createReadStreamForMemberOnDisc("NOPE\\NOPE.TXT", 2));
	}

	// F with archives mounted without disc numbers (the default: the order
	// of the open calls) and with a disc missing: discs 1, 3 mounted, disc
	// 3's archive answers for disc 3, disc 2 falls back to the merged copy.
	void test_disc_numbers_default_and_gap() {
		using namespace UiNl1997;
		const Common::String dir = AtlantisTest::configValue("nl1997");
		if (dir.empty()) { TS_WARN("nl1997 not configured; skipped"); return; }
		SearchMan.addDirectory("ui_nl_data", Common::FSNode(Common::Path(dir, '/')));
		BigFileArchive seq, gap;
		bool opened = seq.open(Common::Path("BIGCD1.BIG")) && seq.open(Common::Path("BIGCD2.BIG"));
		opened &= gap.open(Common::Path("BIGCD1.BIG"), 1) && gap.open(Common::Path("BIGCD4.BIG"), 4);
		SearchMan.remove("ui_nl_data");
		TS_ASSERT(opened);
		if (!opened)
			return;
		const Common::String fontMin = "SPRITE\\2D\\FONTMIN.SPR";
		TS_ASSERT(md5Of(seq.createReadStreamForMemberOnDisc(fontMin, 2)).hasPrefix("72e8d5b7"));
		TS_ASSERT(md5Of(seq.createReadStreamForMemberOnDisc(fontMin, 1)).hasPrefix("adf25e4b"));
		const Common::String epi4 = md5Of(gap.createReadStreamForMemberOnDisc("SPRLIST\\EPI.TXT", 4));
		const Common::String epi1 = md5Of(gap.createReadStreamForMemberOnDisc("SPRLIST\\EPI.TXT", 1));
		TS_ASSERT(epi4 != epi1);
		TS_ASSERT(epi1.hasPrefix("95fc6fa3"));
		TS_ASSERT(epi4.hasPrefix("8a5e77c6"));
		TS_ASSERT_EQUALS(md5Of(gap.createReadStreamForMemberOnDisc("SPRLIST\\EPI.TXT", 2)), epi1);
	}

	// D: the CON bytes in code page 850 (the Dutch accents 0x82 0x89 0x8a
	// 0x8b 0xa3), not Windows-1252, the patch build's (which shows them as
	// the punctuation of decision 0022 B2).
	void test_con_code_page() {
		using namespace UiNl1997;
		const AtlantisExeBuild &nl = atlantisExeBuild(688128, kNlTimestamp);
		const Common::String bytes("All\x82\x82n \x89 \x8a \x8b \xa3");
		const Common::U32String want = Common::U32String("All\xc3\xa9\xc3\xa9n \xc3\xab \xc3\xa8 \xc3\xaf \xc3\xba");
		TS_ASSERT(Common::U32String(bytes, nl.conCodePage) == want);
		const AtlantisExeBuild &patch = atlantisExeBuild(714240, 0x385a4dc6);
		TS_ASSERT_EQUALS(Common::String(patch.name), "atlantis-patch-fr-714240");
		TS_ASSERT(Common::U32String(bytes, patch.conCodePage) != want);
	}

	// The FONTS.BIN breaker (en-688128's FUN_0042e970, this build's
	// FUN_0042e990) on every distinct Dutch dialog text, against the
	// golden's scan (atlantis-re tools/emu/video_subtitles_nl688128.py):
	// 2539 lines, 1945 distinct texts, rows {1: 1510, 2: 355, 3: 67, 4: 9,
	// 5: 4}, the longest row 81 bytes, 8 rows over 79 bytes, the high bytes
	// of the distinct texts.
	void test_subtitle_scan_vs_golden() {
		const Common::String dir = AtlantisTest::configValue("nl1997");
		if (dir.empty()) { TS_WARN("nl1997 not configured; skipped"); return; }
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> golden;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_nl688128.txt", golden));
		bool sawHist = false;
		for (uint i = 0; i < golden.size(); i++)
			sawHist |= golden[i].hasPrefix("# rows per line: {1: 1510, 2: 355, 3: 67, 4: 9, 5: 4}");
		TS_ASSERT(sawHist);
		VideoTest::Big big(dir);
		TS_ASSERT(!big.empty());
		int lines = 0, hist[9] = { 0 }, over79 = 0, longest = 0, high[256] = { 0 };
		Common::Array<Common::String> distinct;
		Common::HashMap<Common::String, bool> seen;
		const Common::Array<Common::String> cons = big.names(".CON");
		for (uint f = 0; f < cons.size(); f++) {
			uint32 size = 0;
			byte *b = big.get(cons[f], size);
			if (!b)
				continue;
			Common::Array<Common::String> texts;
			VideoSubEn1997::conDialogTexts(b, size, texts);
			delete[] b;
			lines += texts.size();
			for (uint t = 0; t < texts.size(); t++) {
				if (seen.contains(texts[t]))
					continue;
				seen[texts[t]] = true;
				distinct.push_back(texts[t]);
			}
		}
		for (uint t = 0; t < distinct.size(); t++) {
			Common::Array<Common::String> rows;
			CryOmni3D::Atlantis::splitSubtitleBitmap(distinct[t], rows);
			if (rows.size() < ARRAYSIZE(hist))
				hist[rows.size()]++;
			for (uint r = 0; r < rows.size(); r++) {
				longest = MAX(longest, (int)rows[r].size());
				over79 += rows[r].size() > 79;
			}
			for (uint i = 0; i < distinct[t].size(); i++)
				high[(byte)distinct[t][i]]++;
		}
		TS_ASSERT_EQUALS(lines, 2539);
		TS_ASSERT_EQUALS(distinct.size(), 1945u);
		TS_ASSERT_EQUALS(hist[1], 1510);
		TS_ASSERT_EQUALS(hist[2], 355);
		TS_ASSERT_EQUALS(hist[3], 67);
		TS_ASSERT_EQUALS(hist[4], 9);
		TS_ASSERT_EQUALS(hist[5], 4);
		TS_ASSERT_EQUALS(longest, 81);
		TS_ASSERT_EQUALS(over79, 8);
		TS_ASSERT_EQUALS(high[0x82], 64);
		TS_ASSERT_EQUALS(high[0x89], 1);
		TS_ASSERT_EQUALS(high[0x8a], 19);
		TS_ASSERT_EQUALS(high[0x8b], 1);
		TS_ASSERT_EQUALS(high[0xa3], 1);
	}

	// The golden PNG: one Dutch subtitle drawn by the original drawer
	// (FUN_0042eb5c + FUN_00410800) with the Dutch FONTS.BIN on a 640 x 480
	// RGB565 screen of 0x8410, rows 400..479; the port draws the same rows:
	// every pixel equal, the accents the code page 437 glyphs.
	void test_pixels_vs_golden_png() {
		const Common::String dir = AtlantisTest::configValue("nl1997");
		if (dir.empty()) { TS_WARN("nl1997 not configured; skipped"); return; }
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		VideoTest::Big big(dir);
		uint32 fontSize = 0;
		byte *raw = big.get("DIALOG\\FONTS.BIN", fontSize);
		TS_ASSERT(raw && fontSize == 16384);
		if (!raw)
			return;
		Common::Array<byte> font(raw, fontSize);
		delete[] raw;
		const Common::String text("Hier zijn geen lijfwachten. All\x82\x82n soldaten. En jij bent daar \x82\x82n van!");
		Common::Array<uint16> screen(640 * 480, 0x8410);
		Common::Array<Common::String> rows;
		CryOmni3D::Atlantis::splitSubtitleBitmap(text, rows);
		TS_ASSERT_EQUALS(rows.size(), 1u);
		for (uint i = 0; i < rows.size(); i++) {
			const int y = CryOmni3D::Atlantis::subtitleLineYBitmap(i, rows.size());
			CryOmni3D::Atlantis::drawSubtitleBitmapRow((byte *)screen.data(), 2, 640, 480, font.data(), fontSize,
			                                           CryOmni3D::Atlantis::kSubtitleX + 1, y + 1, rows[i], 0);
			CryOmni3D::Atlantis::drawSubtitleBitmapRow((byte *)screen.data(), 2, 640, 480, font.data(), fontSize,
			                                           CryOmni3D::Atlantis::kSubtitleX, y, rows[i], 0xffff);
		}
		Common::File f;
		TS_ASSERT(f.open(Common::FSNode(Common::Path(AtlantisTest::goldenDir(), '/')
		                                .appendComponent("video_subtitles_nl688128.png"))));
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
};

#endif
