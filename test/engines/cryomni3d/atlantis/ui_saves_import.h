// Verifier "ui": importing the original's checkpoints (owner decision,
// 2026-10-14).
//
// Original layouts: atlantis-re formats/ui-gam.md (checked on 74 real French
// checkpoints, experiments 2026-10-14-12) and formats/ui-players-atl.md
// (English 97-byte layout emulated, two real English files decoded; 95-byte
// layout of the 1997 builds [CONFIRMED-STATIC]: readers atlantis-cd-en-688128
// @ 0x00419116..0x00419166 and atlantis-cd-fr-687104 @ 0x00418d29..0x00418d86
// ReadFile 0x55 names, 5 chapters, 5 letters (0x5a); experiments
// 2026-10-18-01 / -03, also de-687104 and sv-688128). Port:
// parseOriginalCheckpoint, readOriginalPlayersLetters, readPlayersFile
// (saveload_format.h), called directly on synthetic files laid out as the
// original writes them; the 1997 row count against the emulated readers.

#ifndef TEST_ATLANTIS_UI_SAVES_IMPORT_H
#define TEST_ATLANTIS_UI_SAVES_IMPORT_H

#include "common/memstream.h"
#include "common/endian.h"

#include "cryomni3d/atlantis/saveload_format.h"

class UiSavesImportTestSuite : public CxxTest::TestSuite {
public:
	void test_parse_checkpoint() {
		using namespace CryOmni3D::Atlantis;
		Common::Array<byte> b(kOrigCheckpointSize, 0);
		const char *path = "WAM\\COMPLEXE.WAM";
		memcpy(b.data(), path, strlen(path));
		b[0x104] = 30;                                   // chapter
		b[0x105] = 29;                                   // previous chapter
		WRITE_LE_UINT32(b.data() + 0x112, 9);            // place
		WRITE_LE_UINT32(b.data() + 0x116, 351);          // LastVue
		b[0x11e] = 2;                                    // IncPerso
		WRITE_LE_UINT32(b.data() + 0x11f, 2);            // inventory count
		b[0x123] = 5; WRITE_LE_UINT32(b.data() + 0x124, 262);
		b[0x12c] = 7; WRITE_LE_UINT32(b.data() + 0x12d, 300);
		b[0x231] = 1; b[0x231 + 0x5db] = 3;               // /con counters
		WRITE_LE_UINT32(b.data() + 0x80d, 11);           // CON variable 0
		WRITE_LE_UINT32(b.data() + 0x80d + 24, 17);      // CON variable 6
		b[0x829] = 1;                                    // one guest
		WRITE_LE_UINT32(b.data() + 0x82a, 587); b[0x82e] = 2;
		WRITE_LE_UINT32(b.data() + 0x83e, 2);            // two hidden persos
		WRITE_LE_UINT16(b.data() + 0x842, 523); WRITE_LE_UINT16(b.data() + 0x844, 600);
		OrigCheckpoint cp;
		TS_ASSERT(parseOriginalCheckpoint(b.data(), b.size(), cp));
		TS_ASSERT_EQUALS(cp.wamName, Common::String("COMPLEXE.WAM"));
		TS_ASSERT_EQUALS(cp.chapter, 30u);
		TS_ASSERT_EQUALS(cp.prevChapter, 29u);
		TS_ASSERT_EQUALS(cp.placeId, 9);
		TS_ASSERT_EQUALS(cp.lastVue, 351);
		TS_ASSERT_EQUALS(cp.incPerso, 2);
		TS_ASSERT_EQUALS(cp.inventory.size(), 2u);
		TS_ASSERT_EQUALS(cp.inventory[1].sprite, 7);
		TS_ASSERT_EQUALS(cp.inventory[1].id, 300);
		TS_ASSERT_EQUALS(cp.conCounters.size(), 0x5dcu);
		TS_ASSERT_EQUALS(cp.conCounters[0x5db], 3);
		TS_ASSERT_EQUALS(cp.vars[0], 11);
		TS_ASSERT_EQUALS(cp.vars[6], 17);
		TS_ASSERT_EQUALS(cp.guestCount, 1u);
		TS_ASSERT_EQUALS(cp.guestId[0], 587);
		TS_ASSERT_EQUALS(cp.guestSprite[0], 2);
		TS_ASSERT_EQUALS(cp.hiddenPersos.size(), 2u);
		TS_ASSERT_EQUALS(cp.hiddenPersos[1], 600);
		// not a checkpoint
		TS_ASSERT(!parseOriginalCheckpoint(b.data(), 2638, cp));
	}

	// The 2001 reference build's players.atl has 6 chapter and 6 letter
	// bytes (97, letters at 0x5b); the 1997 builds (English retail, French,
	// German, Swedish) have 5 and 5 (95, letters at 0x5a), read from their
	// executables (formats/ui-players-atl.md).  The 1997 markers ("New
	// Player", "NOUVEAU JOUEUR") are tested in ui_fr1997.h and below; here
	// the reference marker is used for both sizes since the layout is one
	// code path.
	void test_players_letters_both_builds() {
		using namespace CryOmni3D::Atlantis;
		for (int fr = 0; fr < 2; fr++) {
			const uint size = fr ? 95 : 97;
			Common::Array<byte> b(size, 0);
			memcpy(b.data(), "Alice", 5);
			memcpy(b.data() + 17, "#New Player", 11);
			memcpy(b.data() + (fr ? 0x5a : 0x5b), "edcba", 5);
			Common::MemoryReadStream in(b.data(), size);
			char letters[kSaveMaxPlayers];
			TS_ASSERT(readOriginalPlayersLetters(in, letters));
			TS_ASSERT_EQUALS(letters[0], 'e');
			TS_ASSERT_EQUALS(letters[4], 'a');
			Common::MemoryReadStream in2(b.data(), size);
			Common::String players[kSaveMaxPlayers];
			int chapters[kSaveMaxPlayers];
			TS_ASSERT(readPlayersFile(in2, players, chapters));
			TS_ASSERT_EQUALS(players[0], Common::String("Alice"));
			TS_ASSERT(players[1].empty());
		}
	}

	// The row count with an empty name before the marker: names ALICE, "",
	// BOB, then the marker, chapters 3 0 7.  Emulated 2026-10-21 (Fable
	// pass 2 r1:saves-exe#0): the readers of atlantis-cd-en-688128
	// (0x004190ce..0x004191c2, marker `New Player`) and atlantis-cd-fr-687104
	// (0x00418ce1..0x00418dd5, `NOUVEAU JOUEUR`) count 4 rows, three players
	// and the new-player row (one strcmp, against the marker); the
	// reference's (0x0041916b..0x004192e0, `#New Player`, 97 bytes) counts 2,
	// ALICE and the new-player row (a second strcmp against "").  The port
	// keeps the empty-named row empty, its absent player.
	void test_players_rows_empty_name() {
		using namespace CryOmni3D::Atlantis;
		static const char *const kMarkers[] = { "New Player", "NOUVEAU JOUEUR", "#New Player" };
		for (uint m = 0; m < ARRAYSIZE(kMarkers); m++) {
			const bool reference = (m == 2);
			const uint size = reference ? 97 : 95;
			Common::Array<byte> b(size, 0);
			memcpy(b.data(), "ALICE", 5);
			memcpy(b.data() + 34, "BOB", 3);
			memcpy(b.data() + 51, kMarkers[m], strlen(kMarkers[m]));
			b[85] = 3;
			b[87] = 7;
			memcpy(b.data() + (reference ? 0x5b : 0x5a), "abcde", 5);
			Common::MemoryReadStream in(b.data(), size);
			Common::String players[kSaveMaxPlayers];
			int chapters[kSaveMaxPlayers];
			TS_ASSERT(readPlayersFile(in, players, chapters, kMarkers[m], reference));
			TSM_ASSERT_EQUALS(kMarkers[m], players[0], Common::String("ALICE"));
			TSM_ASSERT_EQUALS(kMarkers[m], chapters[0], 3);
			TSM_ASSERT(kMarkers[m], players[1].empty());
			TSM_ASSERT_EQUALS(kMarkers[m], players[2], Common::String(reference ? "" : "BOB"));
			TSM_ASSERT_EQUALS(kMarkers[m], chapters[2], reference ? 0 : 7);
			TSM_ASSERT(kMarkers[m], players[3].empty() && players[4].empty());
			// the 1997 file read with the reference's rule stops at row 1
			if (!reference) {
				Common::MemoryReadStream in2(b.data(), size);
				TS_ASSERT(readPlayersFile(in2, players, chapters, kMarkers[m], true));
				TSM_ASSERT(kMarkers[m], players[2].empty());
			}
			// the letters do not depend on the row count
			Common::MemoryReadStream in3(b.data(), size);
			char letters[kSaveMaxPlayers];
			TS_ASSERT(readOriginalPlayersLetters(in3, letters, kMarkers[m], reference));
			TSM_ASSERT_EQUALS(kMarkers[m], letters[2], 'c');
		}
	}
};

#endif
