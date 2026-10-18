// Builds of atlantis.exe (exe_build.h) and the GOG release's file layout.
//
// The reference build (atlantis-cd-en-714240), the GOG build (one 1999
// executable for five languages, atlantis-re experiments 2026-10-17-05 ..
// -08) and the English retail CD's build of May 1997
// (atlantis-cd-en-688128, experiment 2026-10-18-01): the port's tables at
// each build's addresses must hold the same bytes; the 1999 build's
// alternative game-over narrations; the unpacked tree read behind the stub
// archives.
//
// Port code exercised directly: atlantisExeBuild (exe_build.h),
// readNewSoundNames (exe_newsound.h), conAltGameOverNarration
// (con_script.h), BigFileArchive::addLooseRoot (bigfile.cpp).  Real files:
// `data` (reference), `gog` (one sub-directory per language) and `en1997`
// in atlantis_test.cfg; skipped when absent.

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
		TS_ASSERT_EQUALS(Common::String(atlantisExeBuild(714752).name), "atlantis-gog-714752");
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
		TS_ASSERT(b97.bitmapSubtitles && !b97.subtitleClickCycle && b97.upperCaseNames);
		TS_ASSERT(!b97.hexaForcesObject402 && !b97.loadPickerKnobClamp && b97.musicFirstValueRule);
		TS_ASSERT_EQUALS(b97.ogreRevealFrames * b97.ogreRevealTicks, 5 * 75);
		TS_ASSERT(b97.ratFrameDeadline && !b97.ratT0023Restart && b97.gameOverVoiceAtEntry);
		TS_ASSERT_EQUALS(b97.muriaPlacedFrames, 1);
		TS_ASSERT_EQUALS(b97.fileOffset(0x004861d0), 0x725d0u);
		// The reference and GOG keep the 2001 behaviour of all of them.
		for (uint32 size = 714240; size <= 714752; size += 512) {
			const AtlantisExeBuild &b = atlantisExeBuild(size);
			TS_ASSERT(!b.bitmapSubtitles && b.subtitleClickCycle && !b.upperCaseNames);
			TS_ASSERT(b.hexaForcesObject402 && b.loadPickerKnobClamp && !b.musicFirstValueRule);
			TS_ASSERT(b.ogreRevealFrames == 3 && b.ogreRevealTicks == 200);
			TS_ASSERT(!b.ratFrameDeadline && b.ratT0023Restart && !b.gameOverVoiceAtEntry);
			TS_ASSERT_EQUALS(b.muriaPlacedFrames, 2);
			TS_ASSERT_EQUALS(b.fileOffset(0x004961e0), 0x773e0u);
		}
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

	// Each GOG language's executable: its build is the GOG one, and the
	// three tables at the GOG addresses equal the reference's tables; the
	// player-row strings at 0x00496570 (marker, then label).
	void test_gog_tables_equal_reference_tables() {
		using namespace CryOmni3D::Atlantis;
		const Common::String gog = AtlantisTest::configValue("gog");
		if (!AtlantisTest::haveData() || gog.empty()) { TS_WARN("data / gog not configured; skipped"); return; }
		Common::Array<byte> ref;
		TS_ASSERT(readExe(AtlantisTest::dataDir() + "/atlantis.exe", ref));
		const AtlantisExeBuild &rb = atlantisExeBuild(ref.size());
		TS_ASSERT_EQUALS(cstr(ref, fileOff(0x00496570)), "#New Player");
		static const char *const kLangs[] = { "en-US", "fr-FR", "de-DE", "es-ES", "it-IT" };
		for (uint l = 0; l < ARRAYSIZE(kLangs); l++) {
			Common::Array<byte> g;
			TSM_ASSERT(kLangs[l], readExe(gog + "/" + kLangs[l] + "/Atlantis.exe", g));
			if (g.empty())
				continue;
			const AtlantisExeBuild &gb = atlantisExeBuild(g.size());
			TSM_ASSERT_EQUALS(kLangs[l], Common::String(gb.name), "atlantis-gog-714752");
			TS_ASSERT(!memcmp(&ref[fileOff(rb.inventoryVA)], &g[fileOff(gb.inventoryVA)], 52 * 4));
			TS_ASSERT(!memcmp(&ref[fileOff(rb.guestVA)], &g[fileOff(gb.guestVA)], 64 * 4));
			TS_ASSERT(!memcmp(&ref[fileOff(rb.creditWidthsVA)], &g[fileOff(gb.creditWidthsVA)], 256));
			const Common::String marker = cstr(g, fileOff(0x00496570));
			TS_ASSERT(marker.hasPrefix("#"));
			// the label follows the marker's NUL
			TS_ASSERT(!cstr(g, fileOff(0x00496570) + marker.size() + 1).empty());
			if (l == 1)
				TS_ASSERT_EQUALS(marker, "#NOUVEAU JOUEUR");
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
