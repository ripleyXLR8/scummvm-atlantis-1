/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CRYOMNI3D_ATLANTIS_EXE_BUILD_H
#define CRYOMNI3D_ATLANTIS_EXE_BUILD_H

#include "common/scummsys.h"
#include "common/str-enc.h"
#include "common/stream.h"

namespace CryOmni3D {
namespace Atlantis {

// What the port needs to know about one build of atlantis.exe: where the
// tables it reads sit, and the code differences between builds that change
// behaviour.  Behaviour is established on the reference build
// (atlantis-cd-en-714240); another build is described by its differences
// (atlantis-re binaries/, decisions 0017 to 0024).
struct AtlantisExeBuild {
	const char *name;        // atlantis-re binaries/ name
	uint32 size;             // file size of atlantis.exe
	// The PE header's TimeDateStamp (IMAGE_FILE_HEADER at e_lfanew + 8):
	// tells apart builds of the same size (the reference and
	// atlantis-patch-fr-714240 both have 714240 bytes).
	uint32 peTimestamp;
	// The DGROUP section: virtual address and file offset (pefile).
	uint32 dgroupVA, dgroupRawPtr;
	uint32 inventoryVA;      // FUN_0042b328's object table
	uint32 guestVA;          // FUN_0042b480's guest sprite table
	uint32 creditWidthsVA;   // credits font advance table
	// The WAM + NEWSOUND name pointer table (17 WAM names, then the APC
	// names up to "FIN"), and the range its string pointers fall in.
	uint32 newSoundTableVA, newSoundPtrMin, newSoundPtrMax;
	// The player-row string: the marker players.atl holds after the last
	// name; the label shown follows its NUL when playerLabelAfterMarker,
	// else the marker itself is the label.
	uint32 playerRowVA;
	bool playerLabelAfterMarker;
	// FUN_00422fcc @ 0x0042313d: ids 0x219 / 0x21a get the navigate cursor.
	bool cursorIdRule;
	// FUN_0042844c @ 0x0042892e: perso 0x230 (Rhea) always on camera 2.
	bool rheaCamera2;
	// FUN_00426590: a second death in the same section plays the next
	// narrator line (per-section counter 0x6cccac; the 2001 build resets it
	// before the test).
	bool altGameOverNarration;

	// The 1997 build's differences (atlantis-re experiment 2026-10-18-01,
	// numbered as its list).  The reference's behaviour when false / as in
	// the reference entry.
	// 1: subtitles in the 8x8 font DIALOG\FONTS.BIN, 75-byte rows, 10 px
	// apart (FUN_0042e970, FUN_0042eb3c), not GDI Arial (decision 0018).
	bool bitmapSubtitles;
	// 5: the SUBTITLES option's three-click cycle (FUN_0041d5f4 @
	// 0x0041d73b); the 1997 build has a plain toggle.
	bool subtitleClickCycle;
	// 6: the player list is drawn as stored: '#' like any glyph (no '#'
	// rule in atlantis-cd-en-688128 FUN_0041fff4, atlantis-cd-fr-687104
	// FUN_0041fd6c; the reference's FUN_00420d8c skips it).
	bool playerListVerbatim;
	// 6: name entry takes 0x20..0x7a and stores a..z as A..Z
	// (atlantis-cd-en-688128 FUN_0041d6d0); the reference and
	// atlantis-cd-fr-687104 (FUN_0041d45c @ 0x0041d847) take 0x20..0xa7 as
	// typed.
	bool upperCaseNames;
	// 4: the game-over voice is started at the screen's entry, before the
	// image (FUN_004256a4 @ 0x004256ee..0x0042587b).
	bool gameOverVoiceAtEntry;
	// 7: the LOAD picker's knob starts at y 0x7b (FUN_0041cf78; 0 in
	// atlantis-cd-en-688128 @ 0x0041cc23 and atlantis-cd-fr-687104 @
	// 0x0041c98a).
	bool loadPickerKnobAtBar;
	// 7: a drag above the bar puts the knob back at 0x7b, offset 0, and a
	// row click tests row + offset < count (FUN_0041cf78; also
	// atlantis-cd-fr-687104 FUN_0041c708 @ 0x0041ca60..0x0041ca76,
	// 0x0041cb0b..0x0041cb1b; atlantis-cd-en-688128 has neither).
	bool loadPickerClamp;
	// 8: the hexagon puzzle's entry forces OBJETENMAIN to 402 (FUN_004406fc
	// @ 0x0044076e), so its pieces are always there.
	bool hexaForcesObject402;
	// 9: the ogre puzzle's queen reveal: frames and ticks per frame.
	uint8 ogreRevealFrames;
	uint16 ogreRevealTicks;
	// 10: the rat puzzle's frame waits 5 ticks after each frame (reference)
	// or until a deadline set 5 ticks after the previous wait; T0023's
	// search restarts at point 0 (reference) or goes on from where the
	// trajectory was left.
	bool ratFrameDeadline;
	bool ratT0023Restart;
	// 12: LOADMUSIK / DELMUSIK do nothing when their first value is the
	// playing track, and DelMusik deletes the playing track too.
	bool musicFirstValueRule;
	// 13: frames drawn by the Muria puzzle before its piece-on-slot hook.
	uint8 muriaPlacedFrames;

	// The French build of May 1997's music differences (atlantis-re
	// experiment 2026-10-18-04 M1..M3, decision 0020 B; addresses of
	// atlantis-cd-fr-688640).  false: the rules of the other builds.
	// M1: LoadMusik (FUN_0041e830) copies nothing when the copy of its
	// first listed track is there (FUN_0041ee40) or that track is playing
	// (0x0041e862..0x0041e891), else every listed track, without testing
	// them (0x0041e926..0x0041e96e); the other builds test each track.
	bool musicLoadAllOrNothing;
	// M2: LOADMUSIK runs only when the current music volume 0x00485e71 is
	// 5 or more (FUN_00429184 @ 0x00429c52..0x00429c78); 0: no such test.
	uint8 musicLoadMinVolume;
	// M3: StartMusik of the playing track does nothing (FUN_00429184 @
	// 0x00429d47..0x00429d51); the other builds re-arm the fade-in
	// (reference 0x0042ae3f..0x0042ae4d, atlantis-cd-en-688128
	// 0x00429cce..0x00429cdc).
	bool musicStartSameTrackNoop;
	// The code page of the CON texts, for any rendering of them that is not
	// the FONTS.BIN glyph = byte path (the GDI path of drawSubtitleRows,
	// logs, tests): Windows-1252 for the builds that pass them to TextOutA
	// (decision 0019 A1); code page 850 for atlantis-cd-fr-688640, whose
	// archives hold DOS code page texts drawn by byte with the code page 437
	// FONTS.BIN (0x82 e acute ... 0x97 u grave, the same letters in 437 and
	// 850; decision 0020 A1); code page 850 too for atlantis-cd-nl-688128,
	// whose Dutch CON texts are DOS code page bytes (decision 0022 D), and
	// for atlantis-cd-sv-688128's Swedish ones (decision 0023 E).
	// atlantis-cd-en-688128 draws only with
	// FONTS.BIN (its English texts never take another path).
	Common::CodePage conCodePage;
	// atlantis-cd-nl-688128 (decision 0022 C1): WinMain plays
	// UBB_VUE\LOGORP.HNM (the R&P Electronic Media logo) after LOGO15N and
	// its 200-tick wait, before PREINTRO: the path built in 0x0073cdf4 @
	// 0x00419560..0x0041958c (`ubb_vue\` 0x004abc1c + `logorp.hnm`
	// 0x00486925), the start-up video player FUN_00423444 called @
	// 0x0041958d with no NEWSOUND store (LOGO15N's 18 at 0x00419535 and
	// PREINTRO's 19 at 0x004195d1 frame it), the cinema flag 0x0048617c
	// cleared only after PREINTRO (0x004195e0), so at 13 ticks per frame.
	bool rpLogoVideo;
	// The credits load SPRLIST\CREDITS.TXT themselves on each run, from the
	// archive open then (the current disc's): atlantis-cd-en-688128
	// FUN_0041a5f8 (`credits.txt` pushed @ 0x0041a716), atlantis-cd-fr-688640
	// FUN_0041a638 (@ 0x0041a756), atlantis-cd-nl-688128 FUN_0041a630 (@
	// 0x0041a74e), atlantis-cd-sv-688128 FUN_0041a600 (@ 0x0041a71e).  false: it is loaded once at start-up with the menus
	// (reference FUN_00426f20 @ 0x0042710b, atlantis-cd-fr-687104 and
	// atlantis-cd-de-687104 @ 0x00425eff, the 1999 builds likewise), from
	// the start-up disc.
	bool creditsPerRun;
	// atlantis-cd-sv-688128 (decision 0023 D1): the bitmap subtitle
	// breaker FUN_0042ea7c cuts every row at the last space at or before
	// the end of the 75-byte window: en-688128's `forward < back` branch
	// (FUN_0042e970 @ 0x0042ea44) is gone, the backward cut @ 0x0042eb50 is
	// always taken (the forward scan @ 0x0042eb12..0x0042eb2d still runs,
	// its count unused).  Only with bitmapSubtitles.
	bool subtitleBreakBackOnly;
	// atlantis-cd-sv-688128 (decision 0023 C1): besides 0x20..0xa7, the
	// name entry FUN_0041d704 takes the WM_CHAR codes 0xc4 0xc5 0xd6 0xe4
	// 0xe5 0xf6 (Windows-1252 A diaeresis, A ring, O diaeresis, a
	// diaeresis, a ring, o diaeresis) and stores them as the DOS code page
	// bytes 0x8e 0x8f 0x99 0x84 0x86 0x94 that FONTMIN draws
	// (0x0041dae5..0x0041dbe9; nameEntryChar).
	bool nameSwedishLetters;
	// The width per byte of a positioned (`<x,y>`) menu item's click / hover
	// zone, the menu parser's record width = bytes to the CR x this value:
	// 22 in every build (reference FUN_0041fb6c `imul ebx,[ebp-0x28],0x16`,
	// atlantis-cd-fr-687104 FUN_0041ecb4 likewise), 16 in
	// atlantis-cd-it-687104 (its FUN_0041ecb4 `mov ebx,[ebp-0x28]; shl
	// ebx,4` @ 0x0041f068, stored @ 0x0041f098; decision 0024 C2).
	uint8 menuPosItemByteWidth;

	uint32 fileOffset(uint32 va) const { return va - dgroupVA + dgroupRawPtr; }
};

// atlantis-re experiments 2026-10-17-06 / -07 / -08: the GOG executable
// (the same code for its five languages) is a 1999 build; its three tables
// are 4 bytes after the reference's; it lacks the 2001 cursor and camera
// rules and keeps the alternative game-over narrations.
// atlantis-re experiment 2026-10-18-01 (binaries/atlantis-cd-en-688128.md):
// the English retail CD's executable (May 1997), DGROUP 0x10000 lower, the
// tables at new addresses with the same bytes but the player row; it lacks
// the 2001 cursor and camera rules and the alternative narrations, and has
// the differences of its list.
// atlantis-re experiment 2026-10-18-03 (binaries/atlantis-cd-fr-687104.md,
// decision 0019): the French retail CD's executable (August 1997), DGROUP
// at 0x00480000 / raw 0x6ba00, the tables at en-688128's addresses but the
// newsound table and the player row (4 bytes lower; one string `NOUVEAU
// JOUEUR`, 0x00486546); the 1997 code of en-688128 with the reference's GDI
// subtitles (FUN_0042e84c, FUN_0042e9cc), name entry, LOAD clamp, ogre
// timing (FUN_0043eb5c), rat pacing (FUN_004356a0) and music copy rules
// (FUN_00428e64, FUN_0041ebe8), and the 1999 alternative game-over
// narrations with the voice after the image (FUN_00425430).
// atlantis-re experiment 2026-10-18-04 (binaries/atlantis-cd-fr-688640.md,
// decision 0020): the French CD's first pressing (May 1997), en-688128's
// code five days earlier (every function pairs) built against its older
// cdecl CRYO.DLL, DGROUP at 0x00480000 / raw 0x6c600, the tables at
// en-688128's addresses up to the player row and 4 bytes after from the
// names array on (inventory 0x0048678c, guests 0x00486860, credits
// 0x004866d9 with E b d g n t one pixel narrower), the player row one
// string `NOUVEAU JOUEUR` (0x0048654a); every en-688128 difference but the
// music copies, replaced by M1..M3.  This build's counterparts of the
// en-688128 functions the flags cite: cursor FUN_004222d4, Rhea camera
// FUN_00427560, game over FUN_004257c8, SUBTITLES FUN_0041cffc, player list
// FUN_00420064, name entry FUN_0041d728, players.atl writer FUN_0041e088,
// LOAD picker FUN_0041ca00, hexagon FUN_0043c730, ogre FUN_0043f07c, rat
// T0023 FUN_00434df8, Muria FUN_0043bdf8, warp cursor FUN_0041bc30,
// subtitles FUN_0042eab4 / FUN_0042ec80, DelMusik FUN_0041eee8, CON /set
// executor FUN_00429184 (atlantis-re binaries/maps/atlantis-cd-fr-688640.csv).
static const AtlantisExeBuild kAtlantisExeBuilds[] = {
	// name, size, PE timestamp, DGROUP VA / raw, inventory, guests, credits, newsound
	// table and pointer range, player row, label after marker;
	// cursorIdRule, rheaCamera2, altGameOverNarration;
	// bitmapSubtitles, subtitleClickCycle, playerListVerbatim,
	// upperCaseNames, gameOverVoiceAtEntry, loadPickerKnobAtBar,
	// loadPickerClamp, hexaForcesObject402, ogre frames x ticks,
	// ratFrameDeadline, ratT0023Restart, musicFirstValueRule,
	// muriaPlacedFrames; musicLoadAllOrNothing, musicLoadMinVolume,
	// musicStartSameTrackNoop, conCodePage; rpLogoVideo, creditsPerRun;
	// subtitleBreakBackOnly, nameSwedishLetters; menuPosItemByteWidth.
	{ "atlantis-cd-en-714240", 714240, 0x3b8275d6, 0x00490000, 0x71200,
	  0x004967b0, 0x00496884, 0x004966fc, 0x004961e0, 0x004b0000, 0x004d0000,
	  0x00496570, true,
	  true, true, false,
	  false, true, false, false, false, true, true, true, 3, 200, false, true, false, 2,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  22 },
	{ "atlantis-gog-714752", 714752, 0x385a4dc6, 0x00490000, 0x71200,
	  0x004967b4, 0x00496888, 0x00496700, 0x004961e0, 0x004b0000, 0x004d0000,
	  0x00496570, true,
	  false, false, true,
	  false, true, false, false, false, true, true, true, 3, 200, false, true, false, 2,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  22 },
	{ "atlantis-cd-en-688128", 688128, 0x33886fd1, 0x00480000, 0x6c400,
	  0x00486788, 0x0048685c, 0x004866d5, 0x004861d0, 0x004a0000, 0x004b0000,
	  0x0048654a, false,
	  false, false, false,
	  true, false, true, true, true, false, false, false, 5, 75, true, false, true, 1,
	  false, 0, false, Common::kWindows1252,
	  false, true,
	  false, false,
	  22 },
	{ "atlantis-cd-fr-687104", 687104, 0x33ef1516, 0x00480000, 0x6ba00,
	  0x00486788, 0x0048685c, 0x004866d5, 0x004861cc, 0x004a0000, 0x004b0000,
	  0x00486546, false,
	  false, false, true,
	  false, false, true, false, false, false, true, false, 3, 200, false, false, false, 1,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  22 },
	{ "atlantis-cd-fr-688640", 688640, 0x33818054, 0x00480000, 0x6c600,
	  0x0048678c, 0x00486860, 0x004866d9, 0x004861d0, 0x004a0000, 0x004b0000,
	  0x0048654a, false,
	  false, false, false,
	  true, false, true, true, true, false, false, false, 5, 75, true, false, true, 1,
	  true, 5, true, Common::kDos850,
	  false, true,
	  false, false,
	  22 },
	// atlantis-re experiment 2026-10-18-07 (binaries/atlantis-cd-de-687104.md,
	// decision 0021 A1): the German CD's executable (1997-08-18), the same
	// size as atlantis-cd-fr-687104, told apart by its PE timestamp.  Its
	// code is fr-687104's byte for byte (relocated dwords aside), hence
	// fr-687104's flags; DGROUP differs by the player row `NEUER SPIELER`
	// (0x00486546, one string, marker = label; the names array at
	// 0x00486554) and two error texts, the shorter row moving the credits
	// widths one byte down (0x004866d4, FONTMIN 0x0048666f, FONTMAX
	// 0x004865e6); the inventory (0x00486788), guests (0x0048685c), music
	// names (0x00485eec) and the newsound table (0x004861cc) are where
	// fr-687104 has them (evidence/compare/de_687104_vs_fr_687104_dgroup.txt).
	// Without this entry the size alone picks fr-687104's, whose credits
	// address reads every credits and FONTMIN-overflow advance one glyph
	// late.
	{ "atlantis-cd-de-687104", 687104, 0x33f83e81, 0x00480000, 0x6ba00,
	  0x00486788, 0x0048685c, 0x004866d4, 0x004861cc, 0x004a0000, 0x004b0000,
	  0x00486546, false,
	  false, false, true,
	  false, false, true, false, false, false, true, false, 3, 200, false, false, false, 1,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  22 },
	// atlantis-re experiment 2026-10-18-10 (binaries/atlantis-cd-es-687104.md,
	// decision 0024 A1): the Spanish CD's executable (1997-08-28), the same
	// size as atlantis-cd-fr-687104 and atlantis-cd-de-687104, told apart by
	// its PE timestamp.  Its code is fr-687104's but one function: the rat
	// puzzle's T0023 search FUN_004348fc restarts at point 0 (`mov word
	// [0x00762b54],0` @ 0x00434933, the reference's line at FUN_00437b38 @
	// 0x00437b6f), hence fr-687104's flags with ratT0023Restart set.  DGROUP:
	// de-687104's addresses (evidence/compare/es_687104_vs_fr_687104_dgroup.txt):
	// the player row `Jugador nuevo` (0x00486546, one string, marker =
	// label; the names array at 0x00486554), FONTMAX 0x004865e6, FONTMIN
	// 0x0048666f, the credits widths 0x004866d4 holding the reference's 256
	// bytes (0x1c / 0x1e / 0x1f = 10 / 14 / 14, fr-687104 17 / 2 / 10); the
	// inventory, guests, music names, newsound table, dialog FOV and
	// SUBTITLES where fr-687104 has them.  Without this entry the size alone
	// picks fr-687104's: the credits read one byte late, no search restart.
	// CON texts Windows-1252 (decision 0024 D).
	{ "atlantis-cd-es-687104", 687104, 0x340551c4, 0x00480000, 0x6ba00,
	  0x00486788, 0x0048685c, 0x004866d4, 0x004861cc, 0x004a0000, 0x004b0000,
	  0x00486546, false,
	  false, false, true,
	  false, false, true, false, false, false, true, false, 3, 200, false, true, false, 1,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  22 },
	// atlantis-re experiment 2026-10-18-10 (binaries/atlantis-cd-it-687104.md,
	// decision 0024 A1, C2): the Italian CD's executable (1997-07-31, eleven
	// days before fr-687104), the same size, told apart by its PE timestamp.
	// Its code is fr-687104's but the menu parser FUN_0041ecb4: a positioned
	// (`<x,y>`) item's zone is 16 px per byte (`shl ebx,4` @ 0x0041f068;
	// menuPosItemByteWidth) and a FONTMAX byte above 0xa8 is drawn as a space
	// (`cmp [ebp-0x28],0xa8` @ 0x0041f235, as atlantis-cd-en-688128
	// FUN_0041ef34 @ 0x0041f4b3), which no shipped menu reaches (no byte
	// above 0x97 in the Italian menus), so the port does not model it.
	// DGROUP (evidence/compare/it_687104_vs_fr_687104_dgroup.txt): the player
	// row `Nuovo Giocatore` (0x00486546, one string, marker = label; the
	// names array at 0x00486556) one byte longer than fr-687104's, so
	// FONTMAX 0x004865e8, FONTMIN 0x00486671 and the credits widths
	// 0x004866d6 (fr-687104's bytes up to 0xb1) one byte higher; the
	// inventory, guests, music names, newsound table, dialog FOV and
	// SUBTITLES where fr-687104 has them.  Without this entry the size alone
	// picks fr-687104's: the credits read one byte early, 22-px zones.  CON
	// texts Windows-1252 (decision 0024 D).
	{ "atlantis-cd-it-687104", 687104, 0x33e08f0f, 0x00480000, 0x6ba00,
	  0x00486788, 0x0048685c, 0x004866d6, 0x004861cc, 0x004a0000, 0x004b0000,
	  0x00486546, false,
	  false, false, true,
	  false, false, true, false, false, false, true, false, 3, 200, false, false, false, 1,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  16 },
	// atlantis-re experiment 2026-10-18-08 (binaries/atlantis-cd-nl-688128.md,
	// decision 0022 A1): the Dutch CD's executable (1997-06-05), the same
	// size as atlantis-cd-en-688128, told apart by its PE timestamp.  821 of
	// its 829 functions are en-688128's code, hence en-688128's flags but
	// the name entry: FUN_0041d70c takes 0x20..0xa7 and keeps the case
	// (`cmp 0xa7` @ 0x0041daf7, no a..z -> A..Z; en-688128 FUN_0041d6d0),
	// the reference's rule.  The player list is en-688128's (no '#' rule).
	// DGROUP: en-688128's tables up to the player row `Nieuwe speler`
	// (0x0048654a, one string, marker = label; the names array at
	// 0x00486558), then 3 bytes later for FONTMAX 0x004865ea, FONTMIN
	// 0x00486673 and the credits widths 0x004866d8, 4 for the inventory
	// 0x0048678c and the guests 0x00486860; the newsound table 0x004861d0
	// (pointers into 0x004ab37c..0x004abb90) and the music names 0x00485eec
	// where en-688128 has them (evidence/compare/nl_688128_vs_en_688128_dgroup.txt).
	// Without this entry the size alone picks en-688128's, which reads the
	// inventory and guest tables 4 bytes early and the credits 3.  The CON
	// texts are DOS code page bytes drawn by byte with the code page 437
	// FONTS.BIN; any other rendering decodes them as code page 850
	// (decision 0022 D).  It plays the R&P logo at start-up (rpLogoVideo).
	{ "atlantis-cd-nl-688128", 688128, 0x3396af27, 0x00480000, 0x6c400,
	  0x0048678c, 0x00486860, 0x004866d8, 0x004861d0, 0x004a0000, 0x004b0000,
	  0x0048654a, false,
	  false, false, false,
	  true, false, true, false, true, false, false, false, 5, 75, true, false, true, 1,
	  false, 0, false, Common::kDos850,
	  true, true,
	  false, false,
	  22 },
	// atlantis-re experiment 2026-10-18-09 (binaries/atlantis-cd-sv-688128.md,
	// decision 0023 A1): the Swedish CD's executable (1997-06-11), the same
	// size as atlantis-cd-en-688128 and atlantis-cd-nl-688128, told apart by
	// its PE timestamp.  820 of its 829 functions are en-688128's code (5 of
	// the 9 others the Dutch build's), hence en-688128's flags but: the name
	// entry FUN_0041d704 takes 0x20..0xa7 as typed (`cmp 0xa7` @
	// 0x0041daef, no upper-casing, the Dutch rule) plus the Swedish letters
	// (nameSwedishLetters); the LOAD picker FUN_0041c9b0 has the
	// reference's clamp and row + offset test (0x0041cd08..0x0041cd1e,
	// 0x0041cdb3..0x0041cdc3) with the knob still starting at y 0
	// (0x0041cc32); the subtitle breaker FUN_0042ea7c cuts backward only
	// (subtitleBreakBackOnly).  Every table is at en-688128's address with
	// en-688128's bytes; the player row 0x0048654a is `Ny spelare` (one
	// string, marker = label; the names array at 0x00486555); the newsound
	// pointers fall in 0x004ab37c..0x004abb90
	// (evidence/compare/sv_688128_vs_en_688128_dgroup.txt).  Without this
	// entry the size alone picks en-688128's: names upper-cased without the
	// Swedish letters, rows of up to 81 bytes past the screen edge, no LOAD
	// clamp.  The CON texts are DOS code page bytes drawn by byte with
	// FONTS.BIN; code page 850 for any other rendering (decision 0023 E).
	// No LOGORP start-up video (`logorp.hnm` 0x00486921 has no reference);
	// CREDITS.TXT loaded by each credits run (FUN_0041a600, `credits.txt`
	// 0x004ac1a4 @ 0x0041a71e), FONTMIN / FONTMIN2 by the LOAD picker (@
	// 0x0041c9f9 / 0x0041ca61) and the player screen (@ 0x0041d752 /
	// 0x0041d7ba), as en-688128.
	{ "atlantis-cd-sv-688128", 688128, 0x339e656f, 0x00480000, 0x6c400,
	  0x00486788, 0x0048685c, 0x004866d5, 0x004861d0, 0x004a0000, 0x004b0000,
	  0x0048654a, false,
	  false, false, false,
	  true, false, true, false, true, false, true, false, 5, 75, true, false, true, 1,
	  false, 0, false, Common::kDos850,
	  false, true,
	  true, true,
	  22 },
	// atlantis-re experiment 2026-10-18-06 (binaries/atlantis-patch-fr-714240.md):
	// the executable of Cryo's patch AT1CDINT.EXE ("1.01", 1999-12-17), the
	// same size as the reference, told apart by its PE timestamp (GOG's).
	// It is the GOG build before GOG's edits: every
	// DGROUP byte the port reads (inventory 0x004967b4, guests 0x00496888,
	// credits 0x00496700, newsound 0x004961e0, player row 0x00496570
	// `#NOUVEAU JOUEUR` / `NOUVEAU JOUEUR`) equals GOG fr-FR's; DGROUP
	// differs from it only in the drive strings at file 0x770b4 / 0x773a0
	// (atlantis-re evidence/compare/patch_fr_714240_vs_gog_and_reference.txt).
	// Its code is the reference's but FUN_00422fcc (no 0x219 / 0x21a rule),
	// FUN_004283b4 (= reference FUN_0042844c, no forced camera 2) and
	// FUN_00426570 (= reference FUN_00426590, the alternative narrations);
	// GOG's six byte patches (not reproduced for GOG either) are absent.
	{ "atlantis-patch-fr-714240", 714240, 0x385a4dc6, 0x00490000, 0x71200,
	  0x004967b4, 0x00496888, 0x00496700, 0x004961e0, 0x004b0000, 0x004d0000,
	  0x00496570, true,
	  false, false, true,
	  false, true, false, false, false, true, true, true, 3, 200, false, true, false, 2,
	  false, 0, false, Common::kWindows1252,
	  false, false,
	  false, false,
	  22 },
};

// The build of an executable of `size` bytes and PE timestamp
// `peTimestamp`; the first build of that size when the timestamp is not
// listed; the reference when the size is unknown.
inline const AtlantisExeBuild &atlantisExeBuild(uint32 size, uint32 peTimestamp) {
	for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
		if (kAtlantisExeBuilds[i].size == size && kAtlantisExeBuilds[i].peTimestamp == peTimestamp)
			return kAtlantisExeBuilds[i];
	for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
		if (kAtlantisExeBuilds[i].size == size)
			return kAtlantisExeBuilds[i];
	return kAtlantisExeBuilds[0];
}

// The build of an executable of `size` bytes (the first listed of that
// size: the reference for 714240); the reference when unknown.
inline const AtlantisExeBuild &atlantisExeBuild(uint32 size) {
	for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++)
		if (kAtlantisExeBuilds[i].size == size)
			return kAtlantisExeBuilds[i];
	return kAtlantisExeBuilds[0];
}

// The zone width per byte of a positioned menu item the port uses: the
// build's (16 for atlantis-cd-it-687104, FUN_0041ecb4 @ 0x0041f068; 22 for
// the others); 22 with the minor bug fixes enhancement, so the whole drawn
// text answers (decision 0024 C2: the Italian `OK` zone stops 11 px, the
// `Annulla` zone 18 px, before the end of the word).
inline int menuPosItemZoneByteWidth(const AtlantisExeBuild &b, bool minorBugFixes) {
	return minorBugFixes ? 22 : b.menuPosItemByteWidth;
}

// The PE TimeDateStamp of an executable: e_lfanew at 0x3c, then
// IMAGE_FILE_HEADER.TimeDateStamp 8 bytes past the "PE\0\0" signature.
// 0 when the stream is not a PE file.
inline uint32 readPeTimestamp(Common::SeekableReadStream &exe) {
	const int64 pos = exe.pos();
	uint32 stamp = 0;
	if (exe.seek(0x3c)) {
		const uint32 peOff = exe.readUint32LE();
		if (exe.seek(peOff) && exe.readUint32BE() == MKTAG('P', 'E', 0, 0)) {
			exe.skip(4);
			stamp = exe.readUint32LE();
			if (exe.eos() || exe.err())
				stamp = 0;
		}
	}
	exe.clearErr();
	exe.seek(pos);
	return stamp;
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D

#endif
