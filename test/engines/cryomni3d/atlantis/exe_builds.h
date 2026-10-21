// Builds of atlantis.exe (exe_build.h) and the GOG release's file layout.
//
// The reference build (atlantis-cd-en-714240), the GOG build (one 1999
// executable for five languages, atlantis-re experiments 2026-10-17-05 ..
// -08) and the English retail CD's build of May 1997
// (atlantis-cd-en-688128, experiment 2026-10-18-01), the French retail
// CD's builds of August 1997 (atlantis-cd-fr-687104, experiment
// 2026-10-18-03) and May 1997 (atlantis-cd-fr-688640, experiment
// 2026-10-18-04), the executable of Cryo's patch AT1CDINT
// (atlantis-patch-fr-714240, experiment 2026-10-18-06), the German CD's
// build (atlantis-cd-de-687104, experiment 2026-10-18-07; its own tests in
// ui_de1997.h), the Dutch CD's build (atlantis-cd-nl-688128, experiment
// 2026-10-18-08; its own tests in ui_nl1997.h), the Swedish CD's build
// (atlantis-cd-sv-688128, experiment 2026-10-18-09; its own tests in
// ui_sv1997.h), the Spanish and Italian CDs' builds (atlantis-cd-es-687104,
// atlantis-cd-it-687104, experiment 2026-10-18-10; their own tests in
// ui_es1997.h, ui_it1997.h): the port's tables at
// each build's addresses must hold the same bytes; the 1999 build's
// alternative game-over narrations; the unpacked tree read behind the stub
// archives.
//
// Port code exercised directly: atlantisExeBuild (exe_build.h),
// readNewSoundNames (exe_newsound.h), conAltGameOverNarration
// (con_script.h), BigFileArchive::addLooseRoot (bigfile.cpp).  Real files:
// `data` (reference), `gog` (one sub-directory per language), `en1997`,
// `fr1997`, `frmay1997`, `patch101`, `de1997`, `nl1997`, `sv1997`, `es1997`
// and `it1997` in atlantis_test.cfg;
// skipped when absent.

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/file.h"
#include "common/fs.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/con_script.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/exe_newsound.h"
#include "cryomni3d/atlantis/menu_layout.h"

class AtlantisExeBuildsTestSuite : public CxxTest::TestSuite {
	static uint32 fileOff(uint32 va) { return va - 0x400000 - 0x90000 + 0x71200; }

	static bool readExe(const Common::String &path, Common::Array<byte> &out) {
		Common::File f;
		if (!f.open(Common::FSNode(Common::Path(path, '/'))))
			return false;
		out.resize((uint)f.size());
		return f.read(out.data(), out.size()) == out.size();
	}

	// dir/atlantis.exe or dir/ATLANTIS.EXE (the retail CDs' spelling).
	static bool readExeIn(const Common::String &dir, Common::Array<byte> &out) {
		return readExe(dir + "/atlantis.exe", out) || readExe(dir + "/ATLANTIS.EXE", out);
	}

	// The build of an executable's bytes, by size and PE timestamp, as
	// loadAtlantisExeTables picks it.
	static const CryOmni3D::Atlantis::AtlantisExeBuild &buildOf(const Common::Array<byte> &e) {
		Common::MemoryReadStream ms(e.data(), e.size());
		return CryOmni3D::Atlantis::atlantisExeBuild(e.size(), CryOmni3D::Atlantis::readPeTimestamp(ms));
	}

	static Common::String cstr(const Common::Array<byte> &b, uint32 off) {
		Common::String s;
		while (off < b.size() && b[off])
			s += (char)b[off++];
		return s;
	}

	static CryOmni3D::Atlantis::ConSectionItem line(const char *speaker, const char *text) {
		CryOmni3D::Atlantis::ConSectionItem it;
		it.line.speaker = speaker;
		it.line.text = text;
		return it;
	}

public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	void test_build_by_size() {
		using namespace CryOmni3D::Atlantis;
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714240).name), "atlantis-cd-en-714240");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714752).name), "atlantis-gog-fr-714752");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(1).name), "atlantis-cd-en-714240");
		TS_ASSERT(atlantisExeBuild(714240).cursorIdRule && atlantisExeBuild(714240).rheaCamera2);
		TS_ASSERT(!atlantisExeBuild(714240).altGameOverNarration);
		TS_ASSERT(!atlantisExeBuild(714752).cursorIdRule && !atlantisExeBuild(714752).rheaCamera2);
		TS_ASSERT(atlantisExeBuild(714752).altGameOverNarration);
		// The 1997 build: none of the three 2001 / 1999 rules, and its own
		// differences (experiment 2026-10-18-01).
		const AtlantisExeBuild &b97 = atlantisExeBuild(688128);
		TS_ASSERT_EQUALS(Common::String(b97.name), "atlantis-cd-en-688128");
		TS_ASSERT(!b97.cursorIdRule && !b97.rheaCamera2 && !b97.altGameOverNarration);
		TS_ASSERT(b97.bitmapSubtitles && !b97.subtitleClickCycle && b97.upperCaseNames && b97.playerListVerbatim);
		TS_ASSERT(!b97.hexaForcesObject402 && !b97.loadPickerKnobAtBar && !b97.loadPickerClamp);
		TS_ASSERT(b97.musicFirstValueRule);
		TS_ASSERT_EQUALS(b97.ogreRevealFrames * b97.ogreRevealTicks, 5 * 75);
		TS_ASSERT(b97.ratFrameDeadline && !b97.ratT0023Restart && b97.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(b97.muriaPlacedFrames, 1);
		TS_ASSERT_EQUALS(b97.fileOffset(0x004861d0), 0x725d0u);
		// players.atl rows end at the marker only (0x00419194..0x004191b1);
		// 16 menu records per menu (stride 0x81, FUN_0041ef34 @ 0x0041ef84)
		TS_ASSERT(!b97.emptyNameEndsPlayerRows);
		TS_ASSERT_EQUALS(b97.menuRecordsPerMenu, 16);
		// The reference and GOG keep the 2001 behaviour of all of them.
		for (uint32 size = 714240; size <= 714752; size += 512) {
			const AtlantisExeBuild &b = atlantisExeBuild(size);
			TS_ASSERT(!b.bitmapSubtitles && b.subtitleClickCycle && !b.upperCaseNames && !b.playerListVerbatim);
			TS_ASSERT(b.hexaForcesObject402 && b.loadPickerKnobAtBar && b.loadPickerClamp);
			TS_ASSERT(!b.musicFirstValueRule);
			TS_ASSERT(b.ogreRevealFrames == 3 && b.ogreRevealTicks == 200);
			TS_ASSERT(!b.ratFrameDeadline && b.ratT0023Restart && !b.gameOverVoiceAtEntry);
			TS_ASSERT_EQUALS(b.muriaPlacedFrames, 2);
			TS_ASSERT_EQUALS(b.fileOffset(0x004961e0), 0x773e0u);
			TS_ASSERT(b.emptyNameEndsPlayerRows);
			TS_ASSERT_EQUALS(b.menuRecordsPerMenu, 20);
		}
		// Every build: the empty-name row end only in the 2001 / 1999 code
		// (the reference, GOG, the patch: a second strcmp against "", as
		// atlantis-cd-en-714240 @ 0x004192ae); 16 menu records only in
		// en-688128 and fr-688640 (the only builds with `imul ...,0x81`).
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			TSM_ASSERT_EQUALS(b.name, b.emptyNameEndsPlayerRows, b.size >= 714240);
			const Common::String n = b.name;
			TSM_ASSERT_EQUALS(b.name, b.menuRecordsPerMenu,
			                  (n == "atlantis-cd-en-688128" || n == "atlantis-cd-fr-688640") ? 16 : 20);
		}
		// The French build of August 1997 (experiment 2026-10-18-03,
		// decision 0019): en-688128's 1997 code but the reference's GDI
		// subtitles, name entry, LOAD clamp, ogre timing, rat pacing and
		// music copies, and GOG's alternative narrations with the voice
		// after the image.
		const AtlantisExeBuild &fr = atlantisExeBuild(687104);
		TS_ASSERT_EQUALS(Common::String(fr.name), "atlantis-cd-fr-687104");
		TS_ASSERT(!fr.cursorIdRule && !fr.rheaCamera2 && fr.altGameOverNarration);
		TS_ASSERT(!fr.bitmapSubtitles && !fr.subtitleClickCycle && fr.playerListVerbatim && !fr.upperCaseNames);
		TS_ASSERT(!fr.gameOverVoiceAtEntry && !fr.loadPickerKnobAtBar && fr.loadPickerClamp);
		TS_ASSERT(!fr.hexaForcesObject402 && fr.ogreRevealFrames == 3 && fr.ogreRevealTicks == 200);
		TS_ASSERT(!fr.ratFrameDeadline && !fr.ratT0023Restart && !fr.musicFirstValueRule);
		TS_ASSERT_EQUALS(fr.muriaPlacedFrames, 1);
		TS_ASSERT(!fr.playerLabelAfterMarker);
		TS_ASSERT_EQUALS(fr.fileOffset(0x004861cc), 0x71bccu);
		TS_ASSERT_EQUALS(fr.fileOffset(0x00486546), 0x71f46u);
		TS_ASSERT_EQUALS(fr.conCodePage, Common::kWindows1252);
		// No build but the French one of May 1997 has its music rules; it,
		// the Dutch build (decision 0022 D) and the Swedish one (decision
		// 0023 E) have DOS code page texts.
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			if (b.size == 688640)
				continue;
			TSM_ASSERT(b.name, !b.musicLoadAllOrNothing && b.musicLoadMinVolume == 0 && !b.musicStartSameTrackNoop);
			TSM_ASSERT_EQUALS(b.name, b.conCodePage,
			                  (!strcmp(b.name, "atlantis-cd-nl-688128") || !strcmp(b.name, "atlantis-cd-sv-688128")) ?
			                  Common::kDos850 : Common::kWindows1252);
		}
	}

	// The French build of May 1997 (experiment 2026-10-18-04, decision
	// 0020): en-688128's code, so every flag of en-688128 (E), with its own
	// tables, the music rules M1..M3 (B) and the DOS code page of its texts
	// (A1).
	void test_frmay1997_build() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &m = atlantisExeBuild(688640);
		const AtlantisExeBuild &en = atlantisExeBuild(688128);
		TS_ASSERT_EQUALS(Common::String(m.name), "atlantis-cd-fr-688640");
		TS_ASSERT_EQUALS(m.dgroupVA, 0x00480000u);
		TS_ASSERT_EQUALS(m.dgroupRawPtr, 0x6c600u);
		TS_ASSERT_EQUALS(m.inventoryVA, 0x0048678cu);
		TS_ASSERT_EQUALS(m.guestVA, 0x00486860u);
		TS_ASSERT_EQUALS(m.creditWidthsVA, 0x004866d9u);
		TS_ASSERT_EQUALS(m.newSoundTableVA, 0x004861d0u);
		TS_ASSERT(m.newSoundPtrMin == 0x004a0000u && m.newSoundPtrMax == 0x004b0000u);
		TS_ASSERT_EQUALS(m.playerRowVA, 0x0048654au);
		TS_ASSERT(!m.playerLabelAfterMarker);
		TS_ASSERT_EQUALS(m.fileOffset(0x00486559), 0x72b59u);   // the names array
		TS_ASSERT_EQUALS(m.cursorIdRule, en.cursorIdRule);
		TS_ASSERT_EQUALS(m.rheaCamera2, en.rheaCamera2);
		TS_ASSERT_EQUALS(m.altGameOverNarration, en.altGameOverNarration);
		TS_ASSERT_EQUALS(m.bitmapSubtitles, en.bitmapSubtitles);
		TS_ASSERT_EQUALS(m.subtitleClickCycle, en.subtitleClickCycle);
		TS_ASSERT_EQUALS(m.playerListVerbatim, en.playerListVerbatim);
		TS_ASSERT_EQUALS(m.upperCaseNames, en.upperCaseNames);
		TS_ASSERT_EQUALS(m.gameOverVoiceAtEntry, en.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(m.loadPickerKnobAtBar, en.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(m.loadPickerClamp, en.loadPickerClamp);
		TS_ASSERT_EQUALS(m.hexaForcesObject402, en.hexaForcesObject402);
		TS_ASSERT_EQUALS(m.ogreRevealFrames, en.ogreRevealFrames);
		TS_ASSERT_EQUALS(m.ogreRevealTicks, en.ogreRevealTicks);
		TS_ASSERT_EQUALS(m.ratFrameDeadline, en.ratFrameDeadline);
		TS_ASSERT_EQUALS(m.ratT0023Restart, en.ratT0023Restart);
		TS_ASSERT_EQUALS(m.musicFirstValueRule, en.musicFirstValueRule);
		TS_ASSERT_EQUALS(m.muriaPlacedFrames, en.muriaPlacedFrames);
		TS_ASSERT(m.bitmapSubtitles && m.upperCaseNames && m.gameOverVoiceAtEntry && m.musicFirstValueRule);
		TS_ASSERT(m.musicLoadAllOrNothing && m.musicLoadMinVolume == 5 && m.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(m.conCodePage, Common::kDos850);
	}

	// The 1997 executable against the reference: the tables the port reads
	// at the build's addresses hold the reference's bytes (up to each
	// table's end), the newsound names read with the build's pointer range
	// are the reference's, the player row is `New Player`; and the tables
	// the port embeds (music names, FONTMAX / FONTMIN advances, dialog FOV)
	// are at the addresses binaries/atlantis-cd-en-688128.md gives.
	void test_en1997_tables_equal_reference_tables() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("en1997");
		if (!AtlantisTest::haveData() || dir.empty()) { TS_WARN("data / en1997 not configured; skipped"); return; }
		Common::Array<byte> ref, e;
		TS_ASSERT(readExe(AtlantisTest::dataDir() + "/atlantis.exe", ref));
		TS_ASSERT(readExe(dir + "/atlantis.exe", e));
		if (ref.empty() || e.empty())
			return;
		const AtlantisExeBuild &rb = atlantisExeBuild(ref.size());
		const AtlantisExeBuild &b = atlantisExeBuild(e.size());
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-en-688128");
		// inventory objects: 51 entries and the -1
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.inventoryVA)], &e[b.fileOffset(b.inventoryVA)], 52 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		// guest sprites: 9 entries and the -1
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.guestVA)], &e[b.fileOffset(b.guestVA)], 10 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		// credits advances: equal for the drawn codes 0x20..0xb2
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.creditWidthsVA) + 0x20], &e[b.fileOffset(b.creditWidthsVA) + 0x20],
		                  0xb3 - 0x20));
		// the player row: one string, then the names array (`NEW PLAYER`)
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "New Player");
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486555)), "NEW PLAYER");
		TS_ASSERT(!b.playerLabelAfterMarker && rb.playerLabelAfterMarker);
		// newsound names: 137 APC names, the reference's
		Common::Array<Common::String> refNames, names;
		Common::MemoryReadStream rs(ref.data(), ref.size()), es(e.data(), e.size());
		readNewSoundNames(rs, refNames);
		readNewSoundNames(es, names, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr, b.newSoundPtrMin,
		                  b.newSoundPtrMax);
		TS_ASSERT_EQUALS(refNames.size(), 137u);
		TS_ASSERT_EQUALS(names.size(), refNames.size());
		for (uint i = 0; i < names.size() && i < refNames.size(); i++)
			TS_ASSERT_EQUALS(names[i], refNames[i]);
		// The reference's pointer range rejects this build's pointers.
		Common::Array<Common::String> wrongRange;
		readNewSoundNames(es, wrongRange, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr);
		TS_ASSERT(wrongRange.empty());
		// Embedded tables: music names 31 x 13, FONTMAX (137) and FONTMIN
		// (101) advances, dialog camera FOV (4 floats).
		static const uint32 kPairs[][3] = {
			{ 0x00495eec, 0x00485eec, 31 * 13 }, { 0x0049660e, 0x004865e7, 137 },
			{ 0x00496697, 0x00486670, 101 }, { 0x00496080, 0x00486080, 16 } };
		for (uint i = 0; i < ARRAYSIZE(kPairs); i++)
			TSM_ASSERT(i, !memcmp(&ref[rb.fileOffset(kPairs[i][0])], &e[b.fileOffset(kPairs[i][1])], kPairs[i][2]));
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865e7)], kFontMaxMeasureWidths, 137));
		for (uint i = 0; i < 101; i++)
			TS_ASSERT_EQUALS((int)e[b.fileOffset(0x00486670) + i], fontMinAdvance(i));
	}

	// The French executable of August 1997 against the reference, as for
	// en-688128 (binaries/atlantis-cd-fr-687104.md, "Tables the ports
	// read"): the tables at the build's addresses hold the reference's
	// bytes, but the player row (`NOUVEAU JOUEUR`, no '#') and the credits
	// bytes 0x1c, 0x1e, 0x1f (17, 2, 10; the reference 10, 14, 14); the
	// newsound names read with the build's pointer range are the
	// reference's.
	void test_fr1997_tables_equal_reference_tables() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("fr1997");
		if (!AtlantisTest::haveData() || dir.empty()) { TS_WARN("data / fr1997 not configured; skipped"); return; }
		Common::Array<byte> ref, e;
		TS_ASSERT(readExeIn(AtlantisTest::dataDir(), ref));
		TS_ASSERT(readExeIn(dir, e));
		if (ref.empty() || e.empty())
			return;
		const AtlantisExeBuild &rb = atlantisExeBuild(ref.size());
		const AtlantisExeBuild &b = atlantisExeBuild(e.size());
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-fr-687104");
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.inventoryVA)], &e[b.fileOffset(b.inventoryVA)], 52 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.guestVA)], &e[b.fileOffset(b.guestVA)], 10 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		const uint32 rc = rb.fileOffset(rb.creditWidthsVA), fc = b.fileOffset(b.creditWidthsVA);
		TS_ASSERT(!memcmp(&ref[rc + 0x20], &e[fc + 0x20], 0xb3 - 0x20));
		TS_ASSERT(!memcmp(&ref[rc], &e[fc], 0x1c));
		TS_ASSERT_EQUALS(e[fc + 0x1c], 17);
		TS_ASSERT_EQUALS(e[fc + 0x1d], ref[rc + 0x1d]);
		TS_ASSERT_EQUALS(e[fc + 0x1e], 2);
		TS_ASSERT_EQUALS(e[fc + 0x1f], 10);
		TS_ASSERT_EQUALS(ref[rc + 0x1c], 10);
		TS_ASSERT_EQUALS(ref[rc + 0x1e], 14);
		TS_ASSERT_EQUALS(ref[rc + 0x1f], 14);
		// the player row: one string, then the names array
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "NOUVEAU JOUEUR");
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486555)), "NOUVEAU JOUEUR");
		TS_ASSERT_EQUALS(b.playerRowVA + 15, 0x00486555u);
		Common::Array<Common::String> refNames, names;
		Common::MemoryReadStream rs(ref.data(), ref.size()), es(e.data(), e.size());
		readNewSoundNames(rs, refNames);
		readNewSoundNames(es, names, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr, b.newSoundPtrMin,
		                  b.newSoundPtrMax);
		TS_ASSERT_EQUALS(refNames.size(), 137u);
		TS_ASSERT_EQUALS(names.size(), refNames.size());
		for (uint i = 0; i < names.size() && i < refNames.size(); i++)
			TS_ASSERT_EQUALS(names[i], refNames[i]);
		// Embedded tables at the addresses of the binary note: music names,
		// FONTMAX (137) and FONTMIN (101) advances, dialog camera FOV.
		static const uint32 kPairs[][3] = {
			{ 0x00495eec, 0x00485eec, 31 * 13 }, { 0x0049660e, 0x004865e7, 137 },
			{ 0x00496697, 0x00486670, 101 }, { 0x00496080, 0x00486080, 16 } };
		for (uint i = 0; i < ARRAYSIZE(kPairs); i++)
			TSM_ASSERT(i, !memcmp(&ref[rb.fileOffset(kPairs[i][0])], &e[b.fileOffset(kPairs[i][1])], kPairs[i][2]));
		// SUBTITLES 0x0048609c starts at 0, as the reference's 0x004960a8.
		TS_ASSERT_EQUALS(e[b.fileOffset(0x0048609c)], 0);
		TS_ASSERT_EQUALS(ref[rb.fileOffset(0x004960a8)], 0);
	}

	// The French executable of May 1997 against the reference and
	// en-688128 (binaries/atlantis-cd-fr-688640.md, "Tables the ports
	// read"): the reference's bytes at the build's addresses, but the player
	// row (`NOUVEAU JOUEUR`, marker = label) and the credits advances: E, b,
	// d, g, n, t one less than in the three other builds, 0x1c, 0x1e, 0x1f
	// = 17, 2, 10 as the 1997 builds; the newsound names the reference's.
	void test_frmay1997_tables_equal_reference_tables() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("frmay1997");
		if (!AtlantisTest::haveData() || dir.empty()) { TS_WARN("data / frmay1997 not configured; skipped"); return; }
		Common::Array<byte> ref, e;
		TS_ASSERT(readExeIn(AtlantisTest::dataDir(), ref));
		TS_ASSERT(readExeIn(dir, e));
		if (ref.empty() || e.empty())
			return;
		TS_ASSERT_EQUALS(e.size(), 688640u);
		const AtlantisExeBuild &rb = atlantisExeBuild(ref.size());
		const AtlantisExeBuild &b = atlantisExeBuild(e.size());
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-cd-fr-688640");
		// the DGROUP section header (pefile): VA 0x80000 (RVA), raw 0x6c600
		TS_ASSERT(e.size() > 0x400);
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
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.inventoryVA)], &e[b.fileOffset(b.inventoryVA)], 52 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.guestVA)], &e[b.fileOffset(b.guestVA)], 10 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		// credits advances
		const uint32 rc = rb.fileOffset(rb.creditWidthsVA), fc = b.fileOffset(b.creditWidthsVA);
		static const char kNarrower[] = "Ebdgnt";
		for (uint c = 0; c < 0xb3; c++) {
			const bool narrower = c >= 0x20 && strchr(kNarrower, (int)c) != nullptr;
			int want = ref[rc + c] - (narrower ? 1 : 0);
			if (c == 0x1c)
				want = 17;
			else if (c == 0x1e)
				want = 2;
			else if (c == 0x1f)
				want = 10;
			TSM_ASSERT_EQUALS(c, (int)e[fc + c], want);
		}
		TS_ASSERT_EQUALS(e[fc + 'E'], 6);
		TS_ASSERT_EQUALS(e[fc + 'b'], 8);
		TS_ASSERT_EQUALS(e[fc + 'd'], 8);
		TS_ASSERT_EQUALS(e[fc + 'g'], 8);
		TS_ASSERT_EQUALS(e[fc + 'n'], 7);
		TS_ASSERT_EQUALS(e[fc + 't'], 5);
		// from 0xb3 the inventory table, as en-688128 (+ 4 bytes here)
		TS_ASSERT_EQUALS(b.creditWidthsVA + 0xb3, b.inventoryVA);
		// the player row: one string, the marker and the label, then the
		// names array with its initial text
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(b.playerRowVA)), "NOUVEAU JOUEUR");
		TS_ASSERT_EQUALS(cstr(e, b.fileOffset(0x00486559)), "NOUVEAU JOUEUR");
		Common::Array<Common::String> refNames, names;
		Common::MemoryReadStream rs(ref.data(), ref.size()), es(e.data(), e.size());
		readNewSoundNames(rs, refNames);
		readNewSoundNames(es, names, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr, b.newSoundPtrMin,
		                  b.newSoundPtrMax);
		TS_ASSERT_EQUALS(refNames.size(), 137u);
		TS_ASSERT_EQUALS(names.size(), refNames.size());
		for (uint i = 0; i < names.size() && i < refNames.size(); i++)
			TS_ASSERT_EQUALS(names[i], refNames[i]);
		// Embedded tables: music names, FONTMAX (137) and FONTMIN (101)
		// advances, dialog camera FOV.
		static const uint32 kPairs[][3] = {
			{ 0x00495eec, 0x00485eec, 31 * 13 }, { 0x0049660e, 0x004865eb, 137 },
			{ 0x00496697, 0x00486674, 101 }, { 0x00496080, 0x00486080, 16 } };
		for (uint i = 0; i < ARRAYSIZE(kPairs); i++)
			TSM_ASSERT(i, !memcmp(&ref[rb.fileOffset(kPairs[i][0])], &e[b.fileOffset(kPairs[i][1])], kPairs[i][2]));
		TS_ASSERT(!memcmp(&e[b.fileOffset(0x004865eb)], kFontMaxMeasureWidths, 137));
		// SUBTITLES 0x0048609c starts at 0; the current music volume
		// 0x00485e71 at 0x6e, the music volume 0x00485e72 at 0x6e, as the
		// reference's 0x00495e71 / 0x00495e72.
		TS_ASSERT_EQUALS(e[b.fileOffset(0x0048609c)], 0);
		TS_ASSERT_EQUALS(e[b.fileOffset(0x00485e71)], 0x6e);
		TS_ASSERT_EQUALS(e[b.fileOffset(0x00485e72)], 0x6e);
		TS_ASSERT_EQUALS(ref[rb.fileOffset(0x00495e71)], 0x6e);
		// M2's test: `cmp byte ptr [0x485e71], 5` (80 3d 71 5e 48 00 05) at
		// 0x00429c5e; the text section starts at VA 0x00410000, raw 0x400.
		static const byte kCmp[] = { 0x80, 0x3d, 0x71, 0x5e, 0x48, 0x00, 0x05 };
		TS_ASSERT(!memcmp(&e[0x00429c5e - 0x00410000 + 0x400], kCmp, sizeof(kCmp)));
	}

	// The FONTMIN drawers index the 101-byte advance table with byte - 0x20
	// and no bound, reading on into the credits table (FONTMIN + 101) for
	// bytes 0x85..0xff, in every build (experiment 2026-10-18-03, decision
	// 0019 E).  For every executable configured: the port's advance of
	// every index 0..0xdf, given the build's credits table, is the byte at
	// FONTMIN + index in that executable.
	void test_fontmin_advance_past_table_all_builds() {
		using namespace CryOmni3D::Atlantis;
		struct Exe { Common::String dir; uint32 fontMinVA; };
		Common::Array<Exe> exes;
		if (AtlantisTest::haveData())
			exes.push_back({ AtlantisTest::dataDir(), 0x00496697 });
		if (!AtlantisTest::configValue("en1997").empty())
			exes.push_back({ AtlantisTest::configValue("en1997"), 0x00486670 });
		if (!AtlantisTest::configValue("fr1997").empty())
			exes.push_back({ AtlantisTest::configValue("fr1997"), 0x00486670 });
		if (!AtlantisTest::configValue("frmay1997").empty())
			exes.push_back({ AtlantisTest::configValue("frmay1997"), 0x00486674 });
		if (!AtlantisTest::configValue("de1997").empty())
			exes.push_back({ AtlantisTest::configValue("de1997"), 0x0048666f });
		if (!AtlantisTest::configValue("nl1997").empty())
			exes.push_back({ AtlantisTest::configValue("nl1997"), 0x00486673 });
		if (!AtlantisTest::configValue("sv1997").empty())
			exes.push_back({ AtlantisTest::configValue("sv1997"), 0x00486670 });
		if (!AtlantisTest::configValue("es1997").empty())
			exes.push_back({ AtlantisTest::configValue("es1997"), 0x0048666f });
		if (!AtlantisTest::configValue("it1997").empty())
			exes.push_back({ AtlantisTest::configValue("it1997"), 0x00486671 });
		if (!AtlantisTest::configValue("patch101").empty())
			exes.push_back({ AtlantisTest::configValue("patch101"), 0x0049669b });
		const Common::String gog = AtlantisTest::configValue("gog");
		if (!gog.empty())
			exes.push_back({ gog + "/fr-FR", 0x0049669b });
		if (exes.empty()) { TS_WARN("no executable configured; skipped"); return; }
		int checked = 0;
		for (uint x = 0; x < exes.size(); x++) {
			Common::Array<byte> e;
			if (!readExeIn(exes[x].dir, e) && !readExe(exes[x].dir + "/Atlantis.exe", e)) {
				TS_FAIL(exes[x].dir.c_str());
				continue;
			}
			const AtlantisExeBuild &b = buildOf(e);
			// the credits table follows FONTMIN directly
			TS_ASSERT_EQUALS(b.creditWidthsVA, exes[x].fontMinVA + 101);
			const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
			const uint32 fm = b.fileOffset(exes[x].fontMinVA);
			for (uint i = 0; i <= 0xdf; i++)
				TSM_ASSERT_EQUALS(Common::String::format("%s index %u", b.name, i).c_str(),
				                  fontMinAdvance(i, credits), (int)e[fm + i]);
			// the reference's bytes when no table is given
			if (b.size == 714240)
				for (uint i = 0; i <= 0xdf; i++)
					TS_ASSERT_EQUALS(fontMinAdvance(i), (int)e[fm + i]);
			checked++;
		}
		TS_ASSERT_EQUALS(checked, (int)exes.size());
	}

	// Each GOG language's executable: its build is the GOG one, and the
	// three tables at the GOG addresses equal the reference's tables; the
	// player-row marker at 0x00496570, then names[0] of the players.atl
	// image, which the port takes as the label (exe_build.h
	// playerLabelAfterMarker) because it is the marker without its '#'.
	void test_gog_tables_equal_reference_tables() {
		using namespace CryOmni3D::Atlantis;
		const Common::String gog = AtlantisTest::configValue("gog");
		if (!AtlantisTest::haveData() || gog.empty()) { TS_WARN("data / gog not configured; skipped"); return; }
		Common::Array<byte> ref;
		TS_ASSERT(readExe(AtlantisTest::dataDir() + "/atlantis.exe", ref));
		const AtlantisExeBuild &rb = atlantisExeBuild(ref.size());
		TS_ASSERT_EQUALS(cstr(ref, fileOff(0x00496570)), "#New Player");
		TS_ASSERT_EQUALS(cstr(ref, fileOff(0x0049657c)), "New Player");
		static const char *const kLangs[] = { "en-US", "fr-FR", "de-DE", "es-ES", "it-IT" };
		for (uint l = 0; l < ARRAYSIZE(kLangs); l++) {
			Common::Array<byte> g;
			TSM_ASSERT(kLangs[l], readExe(gog + "/" + kLangs[l] + "/Atlantis.exe", g));
			if (g.empty())
				continue;
			const AtlantisExeBuild &gb = atlantisExeBuild(g.size());
			TSM_ASSERT_EQUALS(kLangs[l], Common::String(gb.name), "atlantis-gog-fr-714752");
			TS_ASSERT(!memcmp(&ref[fileOff(rb.inventoryVA)], &g[fileOff(gb.inventoryVA)], 52 * 4));
			TS_ASSERT(!memcmp(&ref[fileOff(rb.guestVA)], &g[fileOff(gb.guestVA)], 64 * 4));
			TS_ASSERT(!memcmp(&ref[fileOff(rb.creditWidthsVA)], &g[fileOff(gb.creditWidthsVA)], 256));
			const Common::String marker = cstr(g, fileOff(0x00496570));
			TS_ASSERT(marker.hasPrefix("#"));
			// names[0] follows the marker's NUL: the marker without '#'
			// (the row FUN_00420d8c draws skips the '#', @ 0x00420eff)
			TS_ASSERT_EQUALS(cstr(g, fileOff(0x00496570) + marker.size() + 1), Common::String(marker.c_str() + 1));
			if (l == 1)
				TS_ASSERT_EQUALS(marker, "#NOUVEAU JOUEUR");
		}
	}

	// atlantis-patch-fr-714240 (experiment 2026-10-18-06): the reference's
	// size, told apart by its PE timestamp; GOG's flags and table addresses.
	void test_patch101_build() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &p = atlantisExeBuild(714240, 0x385a4dc6);
		const AtlantisExeBuild &g = atlantisExeBuild(714752);
		TS_ASSERT_EQUALS(Common::String(p.name), "atlantis-patch-fr-714240");
		// the size alone, or the reference's timestamp, or an unknown one:
		// the reference
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714240).name), "atlantis-cd-en-714240");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714240, 0x3b8275d6).name), "atlantis-cd-en-714240");
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714240, 0).name), "atlantis-cd-en-714240");
		// GOG's timestamp with GOG's size stays GOG
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714752, 0x385a4dc6).name), "atlantis-gog-fr-714752");
		TS_ASSERT(!p.cursorIdRule && !p.rheaCamera2 && p.altGameOverNarration);
		TS_ASSERT_EQUALS(p.dgroupVA, g.dgroupVA);
		TS_ASSERT_EQUALS(p.dgroupRawPtr, g.dgroupRawPtr);
		TS_ASSERT_EQUALS(p.inventoryVA, g.inventoryVA);
		TS_ASSERT_EQUALS(p.guestVA, g.guestVA);
		TS_ASSERT_EQUALS(p.creditWidthsVA, g.creditWidthsVA);
		TS_ASSERT_EQUALS(p.newSoundTableVA, g.newSoundTableVA);
		TS_ASSERT_EQUALS(p.newSoundPtrMin, g.newSoundPtrMin);
		TS_ASSERT_EQUALS(p.newSoundPtrMax, g.newSoundPtrMax);
		TS_ASSERT_EQUALS(p.playerRowVA, g.playerRowVA);
		TS_ASSERT_EQUALS(p.playerLabelAfterMarker, g.playerLabelAfterMarker);
		// every other flag as GOG's (from bitmapSubtitles to conCodePage)
		TS_ASSERT_EQUALS(p.bitmapSubtitles, g.bitmapSubtitles);
		TS_ASSERT_EQUALS(p.subtitleClickCycle, g.subtitleClickCycle);
		TS_ASSERT_EQUALS(p.playerListVerbatim, g.playerListVerbatim);
		TS_ASSERT_EQUALS(p.upperCaseNames, g.upperCaseNames);
		TS_ASSERT_EQUALS(p.gameOverVoiceAtEntry, g.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(p.loadPickerKnobAtBar, g.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(p.loadPickerClamp, g.loadPickerClamp);
		TS_ASSERT_EQUALS(p.hexaForcesObject402, g.hexaForcesObject402);
		TS_ASSERT_EQUALS(p.ogreRevealFrames, g.ogreRevealFrames);
		TS_ASSERT_EQUALS(p.ogreRevealTicks, g.ogreRevealTicks);
		TS_ASSERT_EQUALS(p.ratFrameDeadline, g.ratFrameDeadline);
		TS_ASSERT_EQUALS(p.ratT0023Restart, g.ratT0023Restart);
		TS_ASSERT_EQUALS(p.musicFirstValueRule, g.musicFirstValueRule);
		TS_ASSERT_EQUALS(p.muriaPlacedFrames, g.muriaPlacedFrames);
		TS_ASSERT_EQUALS(p.musicLoadAllOrNothing, g.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(p.musicLoadMinVolume, g.musicLoadMinVolume);
		TS_ASSERT_EQUALS(p.musicStartSameTrackNoop, g.musicStartSameTrackNoop);
		TS_ASSERT_EQUALS(p.conCodePage, g.conCodePage);
		TS_ASSERT_EQUALS(p.emptyNameEndsPlayerRows, g.emptyNameEndsPlayerRows);
		TS_ASSERT_EQUALS(p.menuRecordsPerMenu, g.menuRecordsPerMenu);
	}

	// The patch executable and the reference read through the port's
	// selection (size + PE timestamp): each gets its own build; the
	// patch's tables equal the reference's and GOG fr-FR's bytes, its
	// player row is `#NOUVEAU JOUEUR` / `NOUVEAU JOUEUR`, and its newsound
	// names are the reference's 137.
	void test_patch101_tables_equal_reference_tables() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("patch101");
		if (!AtlantisTest::haveData() || dir.empty()) { TS_WARN("data / patch101 not configured; skipped"); return; }
		Common::Array<byte> ref, e;
		TS_ASSERT(readExe(AtlantisTest::dataDir() + "/atlantis.exe", ref));
		TS_ASSERT(readExeIn(dir, e));
		if (ref.empty() || e.empty())
			return;
		TS_ASSERT_EQUALS(e.size(), 714240u);
		const AtlantisExeBuild &rb = buildOf(ref);
		const AtlantisExeBuild &b = buildOf(e);
		TS_ASSERT_EQUALS(Common::String(rb.name), "atlantis-cd-en-714240");
		TS_ASSERT_EQUALS(Common::String(b.name), "atlantis-patch-fr-714240");
		{
			Common::MemoryReadStream ms(e.data(), e.size());
			TS_ASSERT_EQUALS(readPeTimestamp(ms), 0x385a4dc6u);
		}
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.inventoryVA)], &e[b.fileOffset(b.inventoryVA)], 52 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.inventoryVA) + 51 * 4]), 0xffffffffu);
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.guestVA)], &e[b.fileOffset(b.guestVA)], 10 * 4));
		TS_ASSERT_EQUALS(READ_LE_UINT32(&e[b.fileOffset(b.guestVA) + 9 * 4]), 0xffffffffu);
		TS_ASSERT(!memcmp(&ref[rb.fileOffset(rb.creditWidthsVA)], &e[b.fileOffset(b.creditWidthsVA)], 256));
		const uint32 row = b.fileOffset(b.playerRowVA);
		TS_ASSERT_EQUALS(cstr(e, row), "#NOUVEAU JOUEUR");
		TS_ASSERT_EQUALS(cstr(e, row + 16), "NOUVEAU JOUEUR");
		Common::Array<Common::String> refNames, names;
		Common::MemoryReadStream rs(ref.data(), ref.size()), es(e.data(), e.size());
		readNewSoundNames(rs, refNames);
		readNewSoundNames(es, names, b.newSoundTableVA, b.dgroupVA, b.dgroupRawPtr, b.newSoundPtrMin,
		                  b.newSoundPtrMax);
		TS_ASSERT_EQUALS(refNames.size(), 137u);
		TS_ASSERT_EQUALS(names.size(), refNames.size());
		for (uint i = 0; i < names.size() && i < refNames.size(); i++)
			TS_ASSERT_EQUALS(names[i], refNames[i]);
		// DGROUP equals GOG fr-FR's but the drive strings (file 0x770b4..
		// 0x770b6, 0x773a0..0x773aa)
		const Common::String gog = AtlantisTest::configValue("gog");
		Common::Array<byte> g;
		if (!gog.empty() && readExe(gog + "/fr-FR/Atlantis.exe", g)) {
			uint diffs = 0;
			for (uint32 off = 0x71200; off < 0x71200 + 0x2fc00; off++)
				if (e[off] != g[off]) {
					diffs++;
					TS_ASSERT((off >= 0x770b4 && off <= 0x770b6) || (off >= 0x773a0 && off <= 0x773aa));
				}
			TS_ASSERT_EQUALS(diffs, 11u);
		}
	}

	// FUN_00426570 of the 1999 build: deaths in one section alternate
	// between the first narrator line and the next one; with no second
	// narrator line the first is always used.
	void test_alt_game_over_narration() {
		using namespace CryOmni3D::Atlantis;
		Common::Array<ConSectionItem> items;
		ConSectionItem set;
		set.isSet = true;
		set.setCmd = "gameover=3";
		items.push_back(set);
		items.push_back(line("Narratrice", "first"));
		items.push_back(line("Narratrice", "second"));
		uint8 count = 0;
		TS_ASSERT_EQUALS(conAltGameOverNarration(items, 0, count), 0u);
		TS_ASSERT_EQUALS(count, 1);
		TS_ASSERT_EQUALS(conAltGameOverNarration(items, 0, count), 1u);
		TS_ASSERT_EQUALS(count, 2);
		TS_ASSERT_EQUALS(conAltGameOverNarration(items, 0, count), 0u);   // 2 -> 0
		TS_ASSERT_EQUALS(count, 1);
		items.pop_back();
		items.push_back(line("Seth", "not a narrator"));
		TS_ASSERT_EQUALS(conAltGameOverNarration(items, 0, count), 0u);   // reset
		TS_ASSERT_EQUALS(count, 1);
	}

	// FUN_0041dea4 tests the player screen's OK and DELETE by record number
	// (5 and 6), which menus.cpp takes as layout items 5 and 6.  In every
	// language's SELEMENU.TXT they are the two labels after the title and
	// four sprites; the delete label's text differs ("SUPPRIMER UN JOUEUR").
	void test_selemenu_records_all_languages() {
		using namespace CryOmni3D::Atlantis;
		const Common::String gog = AtlantisTest::configValue("gog");
		if (gog.empty()) { TS_WARN("gog not configured; skipped"); return; }
		static const char *const kLangs[] = { "en-US", "fr-FR", "de-DE", "es-ES", "it-IT" };
		for (uint l = 0; l < ARRAYSIZE(kLangs); l++) {
			Common::File f;
			TSM_ASSERT(kLangs[l], f.open(Common::FSNode(Common::Path(gog + "/" + kLangs[l] + "/SPRLIST/SELEMENU.TXT", '/'))));
			MenuLayout layout;
			TS_ASSERT(layout.loadFromStream(f));
			TSM_ASSERT_EQUALS(kLangs[l], layout.items.size(), 7u);
			if (layout.items.size() < 7)
				continue;
			for (uint i = 1; i <= 4; i++)
				TS_ASSERT_EQUALS(layout.items[i].kind, MenuItem::kSprite);
			TS_ASSERT_EQUALS(layout.items[5].kind, MenuItem::kText);
			TS_ASSERT_EQUALS(layout.items[5].text, "OK");
			TS_ASSERT_EQUALS(layout.items[6].kind, MenuItem::kText);
			TS_ASSERT_EQUALS(layout.items[6].anchorY, 420);
			if (l == 1)
				TS_ASSERT_EQUALS(layout.items[6].text, "SUPPRIMER UN JOUEUR");
		}
	}

	// GOG French: the stub BIGCD1.BIG holds 14 sounds; everything else comes
	// from the unpacked tree through addLooseRoot.
	void test_gog_loose_tree() {
		using namespace CryOmni3D::Atlantis;
		const Common::String gog = AtlantisTest::configValue("gog");
		if (gog.empty()) { TS_WARN("gog not configured; skipped"); return; }
		const Common::FSNode dir(Common::Path(gog + "/fr-FR", '/'));
		SearchMan.addDirectory("atl_gog_fr", dir);
		BigFileArchive big;
		const bool opened = big.open(Common::Path("BIGCD1.BIG"));
		SearchMan.remove("atl_gog_fr");
		TS_ASSERT(opened);
		TS_ASSERT_EQUALS(big.numFiles(), 14u);
		TS_ASSERT(big.hasFile(Common::String("WAV\\CRAK_1.APC")));
		TS_ASSERT(!big.hasFile(Common::String("SCENAR\\CHAPI001.CON")));
		big.addLooseRoot(dir);
		TS_ASSERT(big.hasFile(Common::String("SCENAR\\CHAPI001.CON")));
		Common::ScopedPtr<Common::SeekableReadStream> s(
		    big.createReadStreamForMember(Common::String("SCENAR\\CHAPI001.CON")));
		TS_ASSERT(s);
		Common::File f;
		TS_ASSERT(f.open(dir.getChild("SCENAR").getChild("CHAPI001.CON")));
		if (s)
			TS_ASSERT_EQUALS(s->size(), f.size());
		// a name in a deeper sub-directory, read through the tree too
		TS_ASSERT(big.hasFile(Common::String("SPRITE\\2D\\FONTMIN.SPR")));
	}
};
