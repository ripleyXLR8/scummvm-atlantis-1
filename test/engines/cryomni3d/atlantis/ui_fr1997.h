#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_FR1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_FR1997_H

// ui audit: the French retail build of August 1997 (atlantis-cd-fr-687104),
// as atlantis-re experiment 2026-10-18-03 read it (STATIC, EMULATED, DATA)
// and decision 0019 chose, numbered as the experiment's list:
//   1  subtitles: the reference's breaker (FUN_0042e84c = FUN_0042ffe8),
//      against the emulated golden and every French dialog line
//   4  game over: GOG's alternative narrations, on the French scripts
//   5  SUBTITLES a plain toggle; 13 Muria one frame (build flags)
//   6  the player list drawn as stored (`NOUVEAU JOUEUR`, '#' drawn), names
//      typed 0x20..0xa7 with case kept, players.atl of 95 bytes
//   7  LOAD picker: knob from y 0, the reference's clamp
//   8  hexagon: no forcing of 402; 9 ogre 3 x 200; 10 rat pacing per frame,
//      T0023 search not restarted; 12 music copies as the reference
//   15 FONTMIN advances past the table: '£' 2, '¤' 10, 0xa1 17
//   and the DOS code-page bytes of EPI.TXT used as FONTMIN glyph indices.
//
// Port code exercised directly: atlantisExeBuild (exe_build.h),
// nameEntryChar, fontMinBlits, fontMinAdvance, fontMinLoadListWidth,
// loadPickerScrollInit / loadPickerScrollFrame (menu_layout.cpp),
// readPlayersFile / readOriginalPlayersLetters / epiEpisodeName
// (saveload_format.cpp), splitSubtitle (subtitle_layout.cpp), ConScript,
// conGameOverNarrator / conAltGameOverNarration (con_script.h), HexaGame,
// OgreDraw::reveal, RatGame, conMusicApply.  Real files: `fr1997` in
// atlantis_test.cfg (ATLANTIS.EXE and the four French BIGCD*.BIG); the
// golden video_subtitles_fr687104.txt; skipped when absent.

#include "atlantis_test_helper.h"
#include "ui_en1997.h"
#include "video_subtitles_en1997.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "common/array.h"
#include "common/hashmap.h"
#include "common/hash-str.h"
#include "common/memstream.h"
#include "common/ustr.h"

#include "cryomni3d/atlantis/con_eval.h"
#include "cryomni3d/atlantis/con_script.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/gameover.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/ogre_draw.h"
#include "cryomni3d/atlantis/saveload_format.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiFr1997 {

using namespace CryOmni3D::Atlantis;

inline const AtlantisExeBuild &fr() { return atlantisExeBuild(687104); }

// The French executable, or empty.
inline bool readFrExe(Common::Array<byte> &out) {
	const Common::String dir = AtlantisTest::configValue("fr1997");
	if (dir.empty())
		return false;
	Common::File f;
	if (!f.open(Common::FSNode(Common::Path(dir + "/ATLANTIS.EXE", '/'))) &&
	        !f.open(Common::FSNode(Common::Path(dir + "/atlantis.exe", '/'))))
		return false;
	out.resize((uint)f.size());
	return f.read(out.data(), out.size()) == out.size();
}

// The pen advance fontMinBlits(kFontMinNameField) gives byte c before an 'A'.
inline int nameFieldAdvance(byte c, const byte *credits) {
	const char s[3] = { (char)c, 'A', 0 };
	Common::Array<FontMinBlit> b;
	fontMinBlits(kFontMinNameField, s, 100, 100, b, true, credits);
	return b.size() == 2 ? b[1].x - b[0].x : -1;
}

} // End of namespace UiFr1997

class UiFr1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// 6: FUN_0041d45c @ 0x0041d847 takes 0x20..0xa7 as typed (the
	// reference's rule), where en-688128 upper-cases 0x20..0x7a; the list
	// drawer FUN_0041fd6c draws '#' (en-688128's, no '#' rule).
	void test_names_and_player_list() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFr1997::fr();
		TS_ASSERT_EQUALS(nameEntryChar('a', b.upperCaseNames), 'a');
		TS_ASSERT_EQUALS(nameEntryChar('{', b.upperCaseNames), '{');
		TS_ASSERT_EQUALS(nameEntryChar(0xa3, b.upperCaseNames), 0xa3);   // '£'
		TS_ASSERT_EQUALS(nameEntryChar(0xa4, b.upperCaseNames), 0xa4);   // '¤'
		TS_ASSERT_EQUALS(nameEntryChar(0xa7, b.upperCaseNames), 0xa7);
		TS_ASSERT_EQUALS(nameEntryChar(0xa8, b.upperCaseNames), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0xe9, b.upperCaseNames), -1);
		TS_ASSERT_EQUALS(nameEntryChar(0x1f, b.upperCaseNames), -1);
		// the English build of 1997 for contrast
		TS_ASSERT_EQUALS(nameEntryChar('a', atlantisExeBuild(688128).upperCaseNames), 'A');
		Common::Array<FontMinBlit> blits;
		fontMinBlits(kFontMinPlayerList, "A#B", 100, 50, blits, !b.playerListVerbatim);
		TS_ASSERT_EQUALS(blits.size(), 3u);
		// the new-player row: "NOUVEAU JOUEUR", 13 glyphs and a space of 20
		fontMinBlits(kFontMinPlayerList, "NOUVEAU JOUEUR", 185, 270, blits, !b.playerListVerbatim);
		TS_ASSERT_EQUALS(blits.size(), 13u);
	}

	// 6: players.atl of 95 bytes (WinMain @ 0x00418d29..0x00418d86: 0x55
	// name bytes, 5 chapters, 5 letters at 0x5a); the rows stop at the name
	// equal to the marker `NOUVEAU JOUEUR` (0x00418d8d..0x00418dc4; no test
	// of an empty name, exe_build.h emptyNameEndsPlayerRows, tested in
	// ui_saves_import.h); names keep their case and bytes up to 0xa7.
	void test_players_atl_95_bytes() {
		using namespace CryOmni3D::Atlantis;
		Common::Array<byte> f(95, 0);
		memcpy(f.data(), "Am\xa3lie", 6);
		memcpy(f.data() + 17, "bob#2", 5);
		memcpy(f.data() + 34, "NOUVEAU JOUEUR", 14);
		f[85] = 7;
		f[86] = 12;
		memcpy(f.data() + 0x5a, "ab", 2);
		Common::MemoryReadStream in(f.data(), f.size());
		Common::String players[kSaveMaxPlayers];
		int chapters[kSaveMaxPlayers];
		const bool emptyEnds = atlantisExeBuild(687104).emptyNameEndsPlayerRows;
		TS_ASSERT(!emptyEnds);
		TS_ASSERT(readPlayersFile(in, players, chapters, "NOUVEAU JOUEUR", emptyEnds));
		TS_ASSERT_EQUALS(players[0], Common::String("Am\xa3lie"));
		TS_ASSERT_EQUALS(players[1], Common::String("bob#2"));
		TS_ASSERT(players[2].empty());
		TS_ASSERT_EQUALS(chapters[0], 7);
		TS_ASSERT_EQUALS(chapters[1], 12);
		Common::MemoryReadStream in2(f.data(), f.size());
		char letters[kSaveMaxPlayers];
		TS_ASSERT(readOriginalPlayersLetters(in2, letters, "NOUVEAU JOUEUR", emptyEnds));
		TS_ASSERT_EQUALS(letters[0], 'a');
		TS_ASSERT_EQUALS(letters[1], 'b');
	}

	// 7: FUN_0041c708: the knob starts at y 0 (0x0041c98a, as en-688128);
	// a drag above the bar puts it back at 0x7b, offset 0
	// (0x0041ca60..0x0041ca76, as the reference).
	void test_load_picker() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFr1997::fr();
		LoadPickerScroll ref, f;
		loadPickerScrollInit(20, ref);
		loadPickerScrollInit(20, f, b.loadPickerKnobAtBar);
		TS_ASSERT_EQUALS(ref.knobY, 0x7b);
		TS_ASSERT_EQUALS(f.knobY, 0);
		TS_ASSERT_EQUALS(f.knobX, ref.knobX);
		TS_ASSERT_EQUALS(f.step, ref.step);
		for (int y = 0x7c; y < 0x152; y++) {
			loadPickerScrollFrame(ref, true, true, y);
			loadPickerScrollFrame(f, true, true, y, b.loadPickerClamp);
			TS_ASSERT_EQUALS(f.knobY, ref.knobY);
			TS_ASSERT_EQUALS(f.offset, ref.offset);
		}
		loadPickerScrollFrame(f, true, true, 0x7c, b.loadPickerClamp);
		TS_ASSERT_EQUALS(f.knobY, 0x7b);
		TS_ASSERT_EQUALS(f.offset, 0);
		// up to the first drag the knob stays at 0
		LoadPickerScroll g;
		loadPickerScrollInit(20, g, b.loadPickerKnobAtBar);
		loadPickerScrollFrame(g, false, false, 200, b.loadPickerClamp);
		TS_ASSERT_EQUALS(g.knobY, 0);
	}

	// 15 (decision 0019 E): FONTMIN index i >= 101 reads the credits table
	// at i - 101.  This build's bytes 0x1c, 0x1e, 0x1f are 17, 2, 10: a
	// typed 0xa1 / '£' / '¤' advances 17 / 2 / 10 (the reference 10 / 14 /
	// 14).  With the executable: its table; without, the bytes of the note.
	void test_fontmin_advance_pound_sign() {
		using namespace CryOmni3D::Atlantis;
		byte credits[256];
		Common::Array<byte> e;
		if (UiFr1997::readFrExe(e)) {
			memcpy(credits, &e[UiFr1997::fr().fileOffset(UiFr1997::fr().creditWidthsVA)], 256);
		} else {
			TS_WARN("fr1997 not configured; the credits bytes of the binary note");
			for (uint i = 0; i < 256; i++)
				credits[i] = (byte)fontMinAdvance(i + 101);
			credits[0x1c] = 17;
			credits[0x1e] = 2;
			credits[0x1f] = 10;
		}
		TS_ASSERT_EQUALS(UiFr1997::nameFieldAdvance(0xa1, credits), 17);
		TS_ASSERT_EQUALS(UiFr1997::nameFieldAdvance(0xa3, credits), 2);
		TS_ASSERT_EQUALS(UiFr1997::nameFieldAdvance(0xa4, credits), 10);
		TS_ASSERT_EQUALS(UiFr1997::nameFieldAdvance(0xa1, nullptr), 10);
		TS_ASSERT_EQUALS(UiFr1997::nameFieldAdvance(0xa3, nullptr), 14);
		TS_ASSERT_EQUALS(UiFr1997::nameFieldAdvance(0xa4, nullptr), 14);
		// the others are the reference's
		for (uint c = 0x20; c <= 0xa7; c++)
			if (c != 0xa1 && c != 0xa3 && c != 0xa4)
				TSM_ASSERT_EQUALS(c, UiFr1997::nameFieldAdvance((byte)c, credits),
				                  UiFr1997::nameFieldAdvance((byte)c, nullptr));
	}

	// EPI.TXT keeps the DOS code page (0x82 é, 0x85 à, 0x8a è...), drawn as
	// FONTMIN glyphs byte - 0x20: the LOAD title's glyph indices are the
	// raw bytes, and its width (FUN_00420b48's sum) the bytes at FONTMIN +
	// index in this executable (0x00486670).
	void test_epi_titles_dos_bytes() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("fr1997");
		Common::Array<byte> e;
		if (dir.empty() || !UiFr1997::readFrExe(e)) { TS_WARN("fr1997 not configured; skipped"); return; }
		VideoTest::Big big(dir);
		uint32 size = 0;
		byte *epi = big.get("SPRLIST\\EPI.TXT", size);
		TS_ASSERT(epi);
		if (!epi)
			return;
		const AtlantisExeBuild &b = UiFr1997::fr();
		const byte *credits = &e[b.fileOffset(b.creditWidthsVA)];
		const uint32 fm = b.fileOffset(0x00486670);
		int titles = 0, high = 0;
		for (uint ch = 2; ch < 90; ch++) {
			const Common::String t = epiEpisodeName(epi, size, ch);
			if (t.empty())
				continue;
			titles++;
			int want = 0;
			for (uint i = 0; i < t.size(); i++) {
				const byte c = (byte)t[i];
				high += c >= 0x80;
				const uint idx = c < 0x20 ? 0x20 : c - 0x20;
				want += idx == 0x20 ? 6 : e[fm + idx];
			}
			TSM_ASSERT_EQUALS(ch, fontMinLoadListWidth(t, credits), want);
			Common::Array<FontMinBlit> blits;
			fontMinBlits(kFontMinNameField, t, 0, 0x50, blits, true, credits);
			uint k = 0;
			for (uint i = 0; i < t.size(); i++) {
				if (t[i] == ' ')
					continue;
				const byte c = (byte)t[i];
				TS_ASSERT(k < blits.size());
				if (k < blits.size())
					TS_ASSERT_EQUALS(blits[k].idx, (uint8)(c < 0x20 ? 0x20 : c - 0x20));
				k++;
			}
		}
		delete[] epi;
		TS_ASSERT(titles > 20);
		TS_ASSERT(high > 0);   // accented names in the DOS code page
	}

	// 1: the emulated breaker FUN_0042e84c (golden
	// video_subtitles_fr687104.txt, atlantis-re
	// tools/emu/video_subtitles_fr687104.py) on the harness's samples: the
	// port's splitSubtitle gives its rows, the texts as Windows-1252 bytes.
	void test_subtitle_rows_vs_golden() {
		using namespace CryOmni3D::Atlantis;
		if (!AtlantisTest::haveData()) { TS_WARN("golden not configured; skipped"); return; }
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_subtitles_fr687104.txt", lines));
		Common::Array<Common::String> in;
		in.push_back("Ligne courte.");
		Common::String t;
		for (int i = 0; i < 2; i++)
			t += Common::String((size_t)49, 'y') + " ";
		in.push_back(t);
		in.push_back(Common::String((size_t)99, 'x'));
		in.push_back(Common::String((size_t)60, 'x') + " " + Common::String((size_t)50, 'x'));
		t.clear();
		for (int i = 0; i < 3; i++)
			t += Common::String((size_t)60, 'x') + " ";
		in.push_back(t);
		t.clear();
		for (int i = 0; i < 40; i++)
			t += "mot ";
		t.trim();
		in.push_back(t);
		in.push_back(Common::U32String("Je suis Seth, l'apprenti de la reine. Elle m'a confi\xc3\xa9 une mission "
		                               "tr\xc3\xa8s importante pour le royaume d'Atlantide.")
		                 .encode(Common::kWindows1252));
		uint k = 0;
		for (uint i = 0; i < lines.size(); i++) {
			if (!lines[i].hasPrefix("len "))
				continue;
			TS_ASSERT(k < in.size());
			if (k >= in.size())
				break;
			TS_ASSERT_EQUALS(atoi(lines[i].c_str() + 4), (int)in[k].size());
			TS_ASSERT(lines[i].contains(" rows same: "));
			// "N:'row' | N:'row'": Python reprs (in double quotes when the
			// row holds a quote), the rows in UTF-8
			Common::Array<Common::String> want;
			Common::String rest = lines[i].substr(lines[i].find("same: ") + 6);
			while (!rest.empty()) {
				const size_t colon = rest.find(':');
				if (colon == Common::String::npos || colon + 1 >= rest.size())
					break;
				const char q = rest[colon + 1];
				const Common::String sep = Common::String(q) + " | ";
				size_t q2 = rest.find(sep, colon + 2);
				const bool last = q2 == Common::String::npos;
				if (last)
					q2 = rest.size() - 1;
				want.push_back(Common::U32String(rest.substr(colon + 2, q2 - colon - 2)).encode(Common::kWindows1252));
				if (last)
					break;
				rest = rest.substr(q2 + 4);
			}
			Common::Array<Common::String> got;
			splitSubtitle(in[k], got);
			TSM_ASSERT_EQUALS(k, got.size(), want.size());
			for (uint r = 0; r < got.size() && r < want.size(); r++)
				TSM_ASSERT_EQUALS(k, got[r], want[r]);
			k++;
		}
		TS_ASSERT_EQUALS(k, in.size());
	}

	// 1: every dialog line of the French CON files through splitSubtitle:
	// the golden's scan ({1: 2191, 2: 314, 3: 30, 4: 6, 5: 1} over 2542
	// lines, none over 8 rows, the longest row 100 bytes in CHAPI004).
	void test_subtitle_scan_french_lines() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("fr1997");
		if (dir.empty()) { TS_WARN("fr1997 not configured; skipped"); return; }
		VideoTest::Big big(dir);
		TS_ASSERT(!big.empty());
		int count = 0, hist[9] = { 0 }, over8 = 0, longest = 0, high = 0;
		Common::String longestIn;
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
				Common::Array<Common::String> rows;
				splitSubtitle(texts[t], rows);
				count++;
				if (rows.size() < ARRAYSIZE(hist))
					hist[rows.size()]++;
				over8 += rows.size() > 8;
				for (uint i = 0; i < texts[t].size(); i++)
					high += (byte)texts[t][i] >= 0x80;
				for (uint r = 0; r < rows.size(); r++)
					if ((int)rows[r].size() > longest) {
						longest = rows[r].size();
						longestIn = cons[f];
					}
			}
		}
		TS_ASSERT_EQUALS(count, 2542);
		TS_ASSERT_EQUALS(hist[1], 2191);
		TS_ASSERT_EQUALS(hist[2], 314);
		TS_ASSERT_EQUALS(hist[3], 30);
		TS_ASSERT_EQUALS(hist[4], 6);
		TS_ASSERT_EQUALS(hist[5], 1);
		TS_ASSERT_EQUALS(over8, 0);
		TS_ASSERT_EQUALS(high, 3303);
		TS_ASSERT_EQUALS(longest, 100);
		TS_ASSERT_EQUALS(longestIn, "SCENAR\\CHAPI004.CON");
	}

	// 4: FUN_00425430's rule on the French scripts (the data scan of
	// evidence/compare/fr_687104_gameover_narrations.txt): 144 game overs,
	// all first narrations voiced; 118 followed by a second narrator line,
	// which a second death in the section plays, 117 of them voiced
	// (WAV\ATA0191B.APC is missing).
	void test_game_over_narrations_french_scripts() {
		using namespace CryOmni3D::Atlantis;
		const Common::String dir = AtlantisTest::configValue("fr1997");
		if (dir.empty()) { TS_WARN("fr1997 not configured; skipped"); return; }
		TS_ASSERT(UiFr1997::fr().altGameOverNarration && !UiFr1997::fr().gameOverVoiceAtEntry);
		VideoTest::Big big(dir);
		Common::HashMap<Common::String, bool> apc;
		const Common::Array<Common::String> apcs = big.names(".APC");
		for (uint i = 0; i < apcs.size(); i++)
			apc[apcs[i]] = true;
		int overs = 0, firstVoiced = 0, second = 0, secondVoiced = 0;
		Common::String missing;
		const Common::Array<Common::String> cons = big.names(".CON");
		for (uint f = 0; f < cons.size(); f++) {
			uint32 size = 0;
			byte *b = big.get(cons[f], size);
			if (!b)
				continue;
			ConScript script;
			script.parse((const char *)b, size);
			delete[] b;
			for (const ConSection &sec : script.sections()) {
				uint lineIdx = 0;
				uint8 count = 0;
				for (uint k = 0; k < sec.items.size(); k++) {
					const ConSectionItem &it = sec.items[k];
					if (!it.isSet) {
						lineIdx++;
						continue;
					}
					if (scumm_strnicmp(it.setCmd.c_str(), "gameover=", 9) != 0 ||
					        atoi(it.setCmd.c_str() + 9) == 0)
						continue;
					Common::String text;
					if (!conGameOverNarrator(sec.items, k, text))
						continue;
					overs++;
					const char letter = conTrackLetter(lineIdx);
					firstVoiced += apc.contains("WAV\\" + gameOverVoiceName(sec.id, letter));
					// the first death, then the second in the same section
					count = 0;
					TS_ASSERT_EQUALS(conAltGameOverNarration(sec.items, k, count), 0u);
					const uint pick = conAltGameOverNarration(sec.items, k, count);
					if (pick) {
						second++;
						const Common::String name = "WAV\\" + gameOverVoiceName(sec.id, (char)(letter + pick));
						if (apc.contains(name))
							secondVoiced++;
						else
							missing += cons[f] + " " + name + ";";
					}
				}
			}
		}
		TS_ASSERT_EQUALS(overs, 144);
		TS_ASSERT_EQUALS(firstVoiced, 144);
		TS_ASSERT_EQUALS(second, 118);
		TS_ASSERT_EQUALS(secondVoiced, 117);
		TS_ASSERT_EQUALS(missing, "SCENAR\\CHAPI015.CON WAV\\ATA0191B.APC;");
	}

	// 8, 9, 10, 12: the build's flags on the puzzle and music rules (the
	// rules themselves are checked in ui_en1997.h): hexagon pieces only with
	// 402 in hand; the queen's reveal 3 x 200; the rat's frame 5 ticks
	// after each frame and T0023's search going on from the slot's
	// position; LOADMUSIK / DELMUSIK acting on the playing track's list.
	void test_puzzle_and_music_flags() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFr1997::fr();
		UiEn1997::HexaStub h;
		h.forceObject402 = b.hexaForcesObject402;
		h.objectInHand = 0;
		h.flagTemp6 = 0;
		h.setUp();
		TS_ASSERT_EQUALS(h.pieces[0].cls, -1);
		OgreRules r;
		r.p[1].x = 3;
		r.p[1].y = 3;
		OgreDraw::Events ev;
		OgreDraw::reveal(ev, r, b.ogreRevealFrames, b.ogreRevealTicks);
		int presents = 0, total = 0;
		for (uint i = 0; i < ev.size(); i++) {
			presents += ev[i].kind == OgreEvent::kPresent;
			if (ev[i].kind == OgreEvent::kWait)
				total += ev[i].frame;
		}
		TS_ASSERT_EQUALS(presents, 3);
		TS_ASSERT_EQUALS(total, 600);
		UiEn1997::RatStub g;
		UiEn1997::fillT0023(g, 60);
		g.frameDeadline = b.ratFrameDeadline;
		g.t0023Restart = b.ratT0023Restart;
		g.slots[0x16].pos = 30;
		g.maxFrames = 6;
		g.rules.setF(0x00496ca6, 281.0f);
		g.runT0023();
		TS_ASSERT(g.presents.size() >= 3u);
		for (uint i = 2; i < g.presents.size(); i++)
			TS_ASSERT_EQUALS((int)(g.presents[i] - g.presents[i - 1]), 2 + 5);
		TS_ASSERT(g.slots[0x16].pos > 30);
		Common::Array<uint8> list;
		list.push_back(4);
		list.push_back(6);
		Common::Array<uint8> copies;
		uint32 disk = (1u << 4) | (1u << 6);
		conMusicApply(false, list, 5, disk, copies, b.musicFirstValueRule);
		TS_ASSERT_EQUALS(disk, 1u << 4);   // the playing track spared
		TS_ASSERT(!b.subtitleClickCycle);
		TS_ASSERT_EQUALS(b.muriaPlacedFrames, 1);
	}
};

#endif
