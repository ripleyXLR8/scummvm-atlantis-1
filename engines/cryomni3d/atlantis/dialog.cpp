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

// In-game conversation playback.  This grows over several commits: it starts
// with the per-character dialog resource table (INITDIAL.TXT) and the dialog
// camera picker; per-line UBB talking-head playback, the subject (topic) menu
// and the F3DC mouth pipeline are layered on as their subsystems land.

#include "common/array.h"
#include "common/debug.h"
#include "common/events.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/rect.h"
#include "common/stream.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "audio/decoders/apc.h"
#include "audio/mixer.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/managed_surface.h"
#include "graphics/surface.h"

#include "gui/message.h"

#ifdef USE_HNM
#include "video/hnm_decoder.h"
#endif

#include "cryomni3d/omni3d.h"
#include "cryomni3d/atlantis/apc_stream.h"
#include "cryomni3d/atlantis/dialog_view.h"
#include "cryomni3d/atlantis/sprite_decode.h"
#include "cryomni3d/atlantis/engine.h"
#include "cryomni3d/atlantis/head3d.h"
#include "cryomni3d/atlantis/subject_menu.h"
#include "cryomni3d/atlantis/subtitle_layout.h"
#include "cryomni3d/atlantis/sprite_blend.h"
#include "cryomni3d/atlantis/video_skip.h"

namespace CryOmni3D {
namespace Atlantis {

// Build the per-character dialog resource table from DIALOG\INITDIAL.TXT.  Each
// row is whitespace-separated:
//   persoId unk anim3DC anim3DA animM s3d cam2 cam3 cam4 cam5 [spf2..spf5]
// "none" tokens are the original engine's "unavailable" sentinel and map to
// empty strings here.
void CryOmni3DEngine_Atlantis::loadDialInitData() {
	Common::ScopedPtr<Common::SeekableReadStream> s(
	    openBigFileStream(kFileTypeDialog, "INITDIAL.TXT"));
	if (!s) {
		warning("loadDialInitData: cannot open DIALOG\\INITDIAL.TXT");
		return;
	}

	uint32 sz = (uint32)s->size();
	Common::Array<char> buf(sz + 1);
	s->read(buf.data(), sz);
	buf[sz] = '\0';

	const char *p = buf.data();
	while (*p) {
		while (*p && (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t')) p++;
		if (!*p) break;

		Common::Array<Common::String> tok;
		while (*p && *p != '\r' && *p != '\n') {
			while (*p && (*p == ' ' || *p == '\t')) p++;
			if (!*p || *p == '\r' || *p == '\n') break;
			const char *te = p;
			while (*te && *te != ' ' && *te != '\t' && *te != '\r' && *te != '\n') te++;
			tok.push_back(Common::String(p, (uint32)(te - p)));
			p = te;
		}

		if (tok.empty() || tok[0].equalsIgnoreCase("FIN")) break;
		if (tok.size() < 10) continue;

		int persoId = atoi(tok[0].c_str());
		if (persoId <= 0) continue;

		DialInitEntry entry;
		for (int i = 0; i < 4; i++)
			entry.camUBB[i] = tok[6 + i];      // cam2..cam5 basenames

		// Column 4 = animM basename (mouth vertex animation, DIALOG\PERSO\<base>.3DA).
		if (!tok[4].equalsIgnoreCase("none"))
			entry.animMBase = tok[4];
		// Column 5 = s3d basename (lip-sync and camera set-up, DIALOG\PERSO\<base>.S3D).
		if (!tok[5].equalsIgnoreCase("none"))
			entry.s3dBase = tok[5];
		// Column 2 = anim3DC basename (head mesh, type-1 F3DC, DIALOG\PERSO\<base>.3DC).
		if (!tok[2].equalsIgnoreCase("none"))
			entry.anim3DCBase = tok[2];
		// Column 3 = head animation (DIALOG\PERSO\<base>.3DA, FUN_00468990).
		if (!tok[3].equalsIgnoreCase("none"))
			entry.anim3DABase = tok[3];
		// Columns 10..13 = per-cam SPF overlay basenames (optional).
		for (int i = 0; i < 4; i++) {
			if (10 + i < (int)tok.size() && !tok[10 + i].equalsIgnoreCase("none"))
				entry.camSPF[i] = tok[10 + i];
		}

		_dialInitData[persoId] = entry;
	}

	debugC(1, kDebugScript, "loadDialInitData: %u entries from INITDIAL.TXT", _dialInitData.size());
}

// Per-line camera picker.  Re-implements the random-cam loop from atlantis.exe
// FUN_0042844c (atlantis-cd-en-714240 @ 0x00428852..0x004289de):
//
//   while (cam == prevCam || INITDIAL[persoId].camUBB[cam-2] is "none")
//       cam = (clock() & 3) + 2;         // 2..5, clock = FUN_00451d0c (timeGetTime), not rand
//
// An explicit `camN` from the header is used as is (clamped to 2 when out of
// 2..5), even when that cam's INITDIAL UBB is "none"; only lines without
// camN draw at random (details below).
// Returns 2..5.
int CryOmni3DEngine_Atlantis::pickDialCam(int persoId, int explicitCam, bool sethLine, bool offLine) {
	// atlantis.exe FUN_0042844c @ 0x004287c8..0x004289ea:
	//  - "camN" at the end of the header: cam = N (out of 2..5 -> 2), used
	//    even when that cam's INITDIAL UBB is "none";
	//  - an off / off0 / off1 line draws nothing: the cam stays the last
	//    one (0x0077290a);
	//  - otherwise draw (clock & 3) + 2 until the cam differs from the last
	//    one and its UBB is not "none" (0x004287f6), then draw a second
	//    time against the first draw: other lines at 0x00428896, Seth lines
	//    at 0x00428979..0x004289e3, after the 0x230 check;
	//  - persoId 0x230 (560) is forced to cam 2 (0x0042892e), before the
	//    second draw of a Seth line.
	// The draws read the millisecond clock (FUN_00451d0c), as the original's
	// do: its loop spins until the clock gives a usable cam.  The port waits
	// a millisecond between draws (64 draws all within one millisecond gave
	// the same rejected cam, e.g. garde17's cam 2 "none": no character, no
	// head).  With fewer than two usable cams the original would loop
	// forever; the port stops after 64 draws and keeps the last drawn cam
	// (kEnhGameBreakingBugFixes: hang guard, unreachable with the shipped
	// INITDIAL, whose rows all have two usable cams or more; atlantis-re
	// functions/scripts-FUN_0042844c.md, Former gaps).
	Common::HashMap<int, DialInitEntry>::const_iterator it = _dialInitData.find(persoId);
	auto usable = [&](int cam) {
		if (it == _dialInitData.end())
			return true;
		const Common::String &s = it->_value.camUBB[cam - 2];
		return !s.empty() && !s.hasPrefix("none");
	};
	auto draw = [&](int last) {
		int cam = 2;
		for (int tries = 0; tries < 64; tries++) {
			cam = (int)(gameClockMs() & 3) + 2;   // FUN_00451d0c
			if (cam != last && usable(cam))
				return cam;
			g_system->delayMillis(1);
		}
		return cam;
	};
	int cam = _lastDialCam >= 2 && _lastDialCam <= 5 ? _lastDialCam : 2;
	const bool drawn = explicitCam == 0 && !offLine;
	if (explicitCam != 0) {
		cam = (explicitCam >= 2 && explicitCam <= 5) ? explicitCam : 2;
	} else if (drawn) {
		cam = draw(_lastDialCam);
		if (!sethLine)
			cam = draw(cam);
	}
	if (persoId == 0x230 && _exeBuild->rheaCamera2)
		cam = 2;
	if (drawn && sethLine)
		cam = draw(cam);
	return cam;
}

void CryOmni3DEngine_Atlantis::drawGivenObject(Graphics::Surface &dst) const {
	if (_dialogGivenObjSprite < 0 || (uint)_dialogGivenObjSprite >= _objSprites.size())
		return;
	const SubjSprite &spr = _objSprites[(uint)_dialogGivenObjSprite];
	if (spr.pixels.empty())
		return;
	// FUN_0042ee88 @ 0x0042f52b..0x0042f555: when DAT_0049619c is set,
	// FUN_00410430(OBJETS1.SPR, frame DAT_0049619c) with its hotspot at
	// (0x136, 0x1a4) = (310, 420).
	drawSprSprite(dst, spr, 0x136, 0x1a4);
}

void CryOmni3DEngine_Atlantis::drawSubjectSprite(Graphics::Surface &dst,
        uint sprIdx, int cx, int cy) const {
	if (sprIdx >= _subjSprites.size())
		return;
	const SubjSprite &spr = _subjSprites[sprIdx];
	if (spr.pixels.empty())
		return;
	// cx,cy = centre point; xoff,yoff = hotspot offset from top-left.
	int dx = cx - spr.xoff;
	int dy = cy - spr.yoff;
	for (int y = 0; y < (int)spr.h; y++) {
		int sy = dy + y;
		if (sy < 0 || sy >= dst.h) continue;
		for (int x = 0; x < (int)spr.w; x++) {
			int sx = dx + x;
			if (sx < 0 || sx >= dst.w) continue;
			uint sidx = y * spr.w + x;
			uint16 pix = spr.pixels[sidx];
			if (spr.blend[sidx] == kSprTranspBlend) continue;
			uint16 *dp = (uint16 *)dst.getBasePtr(sx, sy);
			if (spr.blend[sidx] != kSprNoBlend)
				pix = blendSprPixel565(pix, *dp, spr.blend[sidx]);
			*dp = pix;
		}
	}
}

// Subject-choice menu, reproduced from atlantis.exe's subject grid (draw at
// FUN_004261a0, hit-test at FUN_004211f0):
//  - Each subject is two sprites: the border (SUJETS.SPR sprite 0) at the
//    cell top-left, then the icon (sprite = sujId) at the cell centre.
//  - Layout and hit test from subject_menu.h (FUN_004261a0, FUN_004211f0):
//    the clickable grid has n / 5 + 1 columns, fewer than the drawn ones
//    from nine subjects on.  No hover highlight.
//  - FUN_00425828 waits for a left click only (0x004259b6..0x004259cc: the
//    loop tests 0x0049611c, which FUN_0041a298 sets on a left-click edge; a
//    right click only sets 0x00496120, and the bar toggle @ 0x0041a305 is
//    skipped because 0x00496170 = 1).
// Returns the subject byte FUN_00425828 reads for the clicked cell
// (0x00425a87..): the subject id, 0 for a grid cell past the subjects, -1
// outside the grid.
int CryOmni3DEngine_Atlantis::showSubjectMenu(const Common::Array<int> &sujIds) {
	const int kScreenW = 640, kScreenH = 480;
	const uint nOpts = sujIds.size();
	if (nOpts == 0)
		return -1;

	const int kIconDX   = 25;   // icon anchor = cell top-left + (25,25)
	const int kIconDY   = 25;

	// FUN_004261a0's layout and FUN_00425828's grid origin (subject_menu.h).
	Common::Array<int> ids;
	for (uint i = 0; i < nOpts; i++)
		ids.push_back(sujIds[i]);
	Common::Array<SubjectMenu::Draw> draws;
	int gridX, gridY;
	SubjectMenu::draws(ids, draws, gridX, gridY);
	struct IconLayout { int bx, by; uint optIdx; };
	Common::Array<IconLayout> layout;
	for (uint i = 0; i < nOpts; i++)
		layout.push_back({draws[2 * i].x, draws[2 * i].y, i});

	Graphics::PixelFormat fmt = g_system->getScreenFormat();
	Graphics::ManagedSurface frame(kScreenW, kScreenH, fmt);

	showMouse(true);
	setArrowCursor();

	// Drain the press that opened the menu so it isn't read as an immediate
	// (and missing) selection that closes the menu the instant it appears.
	while (getCurrentMouseButton() != 0 && !shouldAbort()) {
		pollEvents();
		g_system->delayMillis(10);
	}

	int result = -1;
	bool done = false;
	while (!done && !shouldAbort()) {
		pollEvents();

		// Panoramic background → frame buffer.
		const Graphics::Surface *bg = _warpLoaded ? _omni3dMan.getSurface() : nullptr;
		if (bg)
			frame.blitFrom(*bg);
		else
			frame.fillRect(Common::Rect(kScreenW, kScreenH), 0);

		Graphics::Surface *rawPtr = frame.surfacePtr();
		for (const IconLayout &il : layout) {
			drawSubjectSprite(*rawPtr, 0, il.bx, il.by);
			int sujId = sujIds[il.optIdx];
			uint sprIdx = (sujId > 0 && (uint)sujId < _subjSprites.size())
			              ? (uint)sujId : 0;
			drawSubjectSprite(*rawPtr, sprIdx, il.bx + kIconDX, il.by + kIconDY);
		}
		// FUN_004261a0 @ 0x00426393: FUN_0042bc58, the followers not hidden
		// (FUN_00425828 hides all but the clicked one for a follower).
		drawGuests(frame);

		g_system->copyRectToScreen(frame.getPixels(), frame.pitch, 0, 0, kScreenW, kScreenH);
		g_system->updateScreen();

		if (getCurrentMouseButton() == 1) {
			Common::Point click = getMousePos();
			while (getCurrentMouseButton() != 0 && !shouldAbort()) {
				pollEvents();
				g_system->delayMillis(10);
			}
			// FUN_004211f0 from x = 450: n / 5 + 1 columns, so from nine
			// subjects on the last drawn column is not clickable; an index
			// past the subjects reads an empty slot, subject 0 (Quirk 5 of
			// atlantis-re functions/scripts-FUN_00425828.md).
			const int idx = SubjectMenu::hit((int)nOpts, gridX, gridY, click.x, click.y);
			result = SubjectMenu::subject(ids, idx);
			done = true;
		}
		g_system->delayMillis(10);
	}

	showMouse(false);
	return result;
}

// Play a fired CON section: run its /set commands and dialog lines in source
// order, then follow any /go redirect.  The game-over screen and the F3DC
// mouth pipeline layer on as their subsystems land.
void CryOmni3DEngine_Atlantis::playConSection(ConSection &sec, const DialogView::Walk *chain) {
	// Hide the cursor for the duration of the section; restore it after.
	bool cursorWasVisible = showMouse(false);

	// Drain the gesture that triggered us so the first line isn't immediately
	// skipped by a still-held click (departure /sel sections fire mid-click).
	while (getCurrentMouseButton() != 0 && !shouldAbort()) {
		pollEvents();
		g_system->delayMillis(10);
	}
	clearKeys();

	// A /set(chapter=N) reloads _conScript and frees `sec`; remember the entry
	// chapter so the loop stops the instant that happens.
	const uint entryChapter = _currentCONChapter;
	int lineIdx = 0;
	// FUN_0042844c saves ClicPerso at entry (0x004284c2) and restores it at
	// the end (0x00428cb2); in between it is cleared after every dialog line
	// (0x00428b39), so only the first line of a section entered with a
	// ClicPerso uses it, the others resolve the speaker by name.
	// ... and the view angles (0x00428483 / 0x0042848b), set back at the end.
	// A /go continues the same walk (FUN_00425c04 returns the target header
	// to FUN_0042844c), so the target keeps both, and ClicPerso stays as the
	// lines left it (0 after a line) until the end of the walk.
	DialogView::Walk walk = {{_omni3dMan.getAlpha(), _omni3dMan.getBeta()},
	                         _gameVars.getValOrDefault("clicperso", 0)};
	if (chain)
		walk = *chain;
	DialogView::Angles &entry = walk.entry;
	// A /set(gameover=N) with N != 0 ends the section (atlantis.exe FUN_0042844c
	// breaks the line loop once the GameOver variable goes non-zero); the lines
	// that follow feed the game-over screen instead of playing as dialog.
	int            pendingGameOver = 0;
	Common::String gameOverText;
	char           gameOverTrack   = 'A';
	for (uint itemIdx = 0; itemIdx < sec.items.size(); ++itemIdx) {
		const ConSectionItem &item = sec.items[itemIdx];
		if (shouldAbort())
			break;

		if (item.isSet) {
			// /set(gameover=N), N != 0: trigger the game-over screen
			// (FUN_00426590).  Its narrator commentary is the line right after
			// the command when it starts with "[Narra" (@ 0x004267ea); the APC
			// track is 'a' + the number of '[' from the section header to it,
			// which is the dialog line index for all 144 game overs shipped.
			if (scumm_strnicmp(item.setCmd.c_str(), "gameover=", 9) == 0 &&
			        atoi(item.setCmd.c_str() + 9) != 0) {
				execConSetCommand(item.setCmd, item.offset);
				pendingGameOver = atoi(item.setCmd.c_str() + 9);
				if (conGameOverNarrator(sec.items, itemIdx, gameOverText)) {
					uint narrator = itemIdx + 1;
					// The 1997 builds take the letter the executor left
					// (atlantis-cd-en-688128 0x00486198, FUN_004256a4;
					// atlantis-cd-fr-687104 0x00486194, FUN_00425430); for
					// the game overs of the data it is this count of the
					// section's lines [INFERRED] (atlantis-re experiment
					// 2026-10-18-01 item 4).
					gameOverTrack = conTrackLetter(lineIdx);
					// The 1999 build (GOG) and the French build of August
					// 1997 (atlantis-cd-fr-687104 FUN_00425430 @
					// 0x0042565f..0x00425719, counter 0x006bccac): the
					// second death in a section plays the next narrator line
					// (exe_build.h).
					if (_exeBuild->altGameOverNarration) {
						const uint pick = conAltGameOverNarration(sec.items, itemIdx,
						                                          _gameOverCount[sec.id]);
						if (pick) {
							gameOverText = sec.items[itemIdx + 1 + pick].line.text;
							gameOverTrack = (char)(gameOverTrack + pick);
							narrator += pick;
						}
					}
					// The game-over screen copies the narrator line into the
					// subtitle text buffer (atlantis-cd-en-688128
					// FUN_004256a4 @ 0x00425a31..0x00425a50), shown or not.
					_subtitleText.store(conNarratorBufferText(sec.items[narrator].line.raw));
				}
				break;
			}
			execConSetCommand(item.setCmd, item.offset);
			// A chapter reload reparsed _conScript: `sec`/`item`/the section
			// list are dangling now.  Stop at once.
			if (_currentCONChapter != entryChapter)
				break;
			continue;
		}

		// FUN_0042844c: every dialog line lowers the music when no fade runs
		// (0x004284e8..0x004284f1, at the head of the per-line loop, so also
		// a line after /set commands); the walk ends by raising it
		// (0x00428d8b, on the only exit).
		musicDuck(true);

		// The executor copies every `[` line it runs into the subtitle text
		// buffer, shown or not, before the line plays (atlantis-cd-en-688128
		// FUN_00427420 @ 0x00427707..0x00427722): the 1997 breakers read it.
		_subtitleText.store(conLineBufferText(item.line.raw));

		// Dialog line — resolve the on-screen speaker and the talking-head shot.
		// FUN_00428f58 runs only when ClicPerso is 0 (0x00428689) and writes
		// its result there; the camera choice then reads ClicPerso (0x004287bf).
		int persoId = _gameVars.getValOrDefault("clicperso", 0);
		if (persoId == 0) {
			persoId = resolveSpeakerPersoId(item.line.speaker);
			_gameVars["clicperso"] = persoId;
		}
		// The header is tested at its end, just before ']' (FUN_0042844c @
		// 0x004286a6..0x0042877e), case-sensitively: "off", "off0", "off1"
		// (no camera choice), "cam"/"Cam" + one character (explicit camera,
		// atoi of that character, 2 when out of 2..5).  "Seth" at the start
		// selects the single-draw path (0x00428608).
		const Common::String &hdrTail = item.line.params.empty() ? item.line.speaker : item.line.params;
		const uint tl = hdrTail.size();
		auto tailIs = [&](const char *lit, uint back) {
			uint n = strlen(lit);
			return tl >= back && memcmp(hdrTail.c_str() + tl - back, lit, n) == 0;
		};
		int explicitCam = 0;
		if (tailIs("cam", 4) || tailIs("Cam", 4)) {
			const char d = hdrTail[tl - 1];
			explicitCam = (d >= '0' && d <= '9') ? d - '0' : 0;
			if (explicitCam < 2 || explicitCam > 5)
				explicitCam = 2;
		}
		const bool sethLine = item.line.speaker.hasPrefix("Seth");
		const bool offFlag = tailIs("off", 3) || tailIs("off0", 4) || tailIs("off1", 4);
		int camNum = pickDialCam(persoId, explicitCam, sethLine, offFlag);

		// Off-camera variants (off / off0 / off1) play as voiceover: no head
		// shot, view at the kept angles (below).  "off" (FUN_00430258)
		// sets the warp FOV 75.137 (0x00430447), which is the port's FOV here
		// anyway (each line restores its own), so nothing is changed.
		const bool isOffCam = offFlag;
		// off0 (FUN_004305e8): the subtitle is drawn over the screen as it
		// stands, with no view redraw and no FOV reset.  off1 (FUN_00430884)
		// does the same over the screen buffer 0x004960d4 (the first IX
		// buffer), which it first copies to the back buffer and the surface
		// (0x00430a96..), with no sprite pass (FUN_00425304), no panorama
		// redraw (FUN_00451d8b) and no FOV reset (FUN_0045179a): below, the
		// screen is set from _ixVideoBuffers and the line then plays as off0.
		// The reference's CON files have no off1 header; the GOG.com
		// release's have 30 (CHAPI031, 036, 048, 058, 062).
		const bool off1 = tailIs("off1", 4);
		_dialKeepScreen = tailIs("off0", 4) || off1;

		Common::String vid;
		Common::String spfBase;
		// The INITDIAL row of the video (0x0077290c, dialog_view.h): 512 +
		// the IncPerso count for a Seth line, else 512 + ClicPerso's low
		// byte, also for a ClicPerso below 0x200 (CHAPI056 /744 Anna with
		// ClicPerso 329 plays row 585) or 0.
		const int dialRow = DialogView::dialRow(sethLine && !isOffCam, persoId, _incPersoCount);
		// FUN_0042ee88 step 3: a line whose wav\ata<NNNN><letter>.syc is
		// missing (FUN_0042f9e8 fails) is played voice-only by FUN_00430258,
		// as an `off` line: FOV 75.137 and the view redrawn at the angles left
		// in 0x00bcb59c / 0x00bcb598, which FUN_0042844c has set back to the
		// player's own unless the header gives explicit angles
		// (0x00428aec..0x00428b0e); no UBB, no head, persos shown.
		bool noSyc = false;
		if (!isOffCam) {
			Common::ScopedPtr<Common::SeekableReadStream> syc(openBigFileStream(kFileTypeSound,
			    Common::String::format("ATA%04d%c.SYC", sec.id, conTrackLetter(lineIdx))));
			noSyc = !syc;
		}
		const bool voiceOnly = isOffCam || noSyc;
		// A row without its INITDIAL record leaves an empty UBB name, and an
		// explicit camN whose INITDIAL UBB is "none" the name "none.ubb"
		// (pickDialCam keeps that cam): the original's open fails: an error
		// box, then the line is not played (DialogView::headOpenFails;
		// atlantis-re experiment 2026-10-16-15).
		// [UNKNOWN] for atlantis-cd-fr-688640: its own May CRYO.DLL returns 0,
		// not -1, for the failed open (its _GL_CreateFile @ 0x004159a6), so
		// FUN_00433038 does not see the failure and the UBB2 header reader
		// FUN_00432f10 runs on handle 0 with an uninitialised header
		// (atlantis-re experiment 2026-10-18-04 E3).  The port keeps the
		// behaviour below for this build too (decision 0020 D).
		bool headFails = false;
		// The name FUN_0042ee88 appends to "dialog\perso\" (0x0042ef9d..
		// 0x0042effb): the parser's name + ".ubb", empty without a record.
		Common::String ubbName;
		if (!voiceOnly) {
			Common::HashMap<int, DialInitEntry>::const_iterator it =
			    _dialInitData.find(dialRow);
			const bool haveRecord = it != _dialInitData.end();
			const Common::String camUBB = haveRecord ? it->_value.camUBB[camNum - 2] : Common::String();
			headFails = DialogView::headOpenFails(voiceOnly, haveRecord, camUBB);
			if (haveRecord) {
				ubbName = camUBB + ".ubb";
				Common::String fn = camUBB;
				if (!fn.empty() && !fn.equalsIgnoreCase("none")) {
					fn.toUppercase();
					vid = Common::String("PERSO\\") + fn + ".UBB";
				}
				// Per-cam SPF overlay (INITDIAL columns 10..13).  The parser
				// already strips "none" to empty, so a non-empty basename
				// here means we have an overlay to composite.
				spfBase = it->_value.camSPF[camNum - 2];
			}
		}
		// Remember for the next line so consecutive lines don't repeat a cam.
		_lastDialCam = camNum;

		// The view behind the line (FUN_0042844c @ 0x00428949..0x00428b5e,
		// dialog_view.h): a talking head turns from the angles kept in
		// 0x00bcb59c / 0x00bcb598 (+ pi for a Seth line, pitch 0) without
		// moving them; explicit X, Y header angles move them; a voice-only
		// line (FUN_00430258) redraws at them; after the line the view goes
		// back to them.  A Seth line with an off flag takes the
		// non-Seth path (0x0042896e).
		const DialogView::Angles kept = {_omni3dMan.getAlpha(), _omni3dMan.getBeta()};
		const DialogView::Line view = DialogView::lineView(kept, sethLine && !isOffCam, persoId,
		    item.line.hasAngle, item.line.angleX * M_PI / 180.0, item.line.angleY * M_PI / 180.0,
		    _omni3dMan.betaLimit());
		if (view.entryUpdate)
			entry = kept;   // 0x004289ef..0x004289fc, 0x00428aa6..0x00428ab3
		if (_warpLoaded) {
			const DialogView::Angles &shown = voiceOnly ? view.kept : view.view;
			_omni3dMan.setAlpha(shown.alpha);
			_omni3dMan.setBeta(shown.beta);
		}
		// camNum=0: the line plays as voiceover (no UBB, no head).
		const int effCamNum = voiceOnly ? 0 : camNum;
		_dialVoiceOff = voiceOnly && !_dialKeepScreen;
		debugC(2, kDebugScript, "playConSection: speaker='%s' params='%s' persoId=%d "
		         "cam=%d vid='%s' spf='%s' view=(%.2f,%.2f) kept=(%.2f,%.2f)",
		      item.line.speaker.c_str(), item.line.params.c_str(), persoId,
		      effCamNum, vid.c_str(), spfBase.c_str(), view.view.alpha, view.view.beta,
		      view.kept.alpha, view.kept.beta);
		_dialEscapeSkip = false;
		if (headFails) {
			// FUN_0042ee88 @ 0x0042f036..0x0042f048: the modal error box
			// (dialog 0x65, code 0xc), then nothing of the line; the letter
			// still counts (0x00428b21).
			debugC(1, kDebugScript, "playConSection: section %d line %c: row %d cam %d, no UBB '%s'",
			      sec.id, conTrackLetter(lineIdx), dialRow, camNum, ubbName.c_str());
			// Not shown with the UI/UX enhancements (atlantis-re
			// decisions/0016, row 7); the line is skipped either way.
			if (!enhancementEnabled(kEnhUIUX)) {
				GUI::MessageDialog box(DialogView::headOpenFailText(ubbName), Common::String("OK"));
				runDialog(box);
			}
			lineIdx++;
		} else {
			// The camera's FOV (kDialogCamFovDeg) is set only on the Seth path
			// (0x004289ff..0x00428a14) and for ClicPerso >= 0x200
			// (0x00428a9d..0x00428ada); a talking head with ClicPerso below
			// 0x200 (CHAPI056 /744 A [Anna], ClicPerso 329) keeps the FOV of
			// the last reset, 75.137 (DialogView::Line::camFov).  The
			// voice-only players draw at 75.137 whatever was set (FUN_00430258
			// @ 0x0043044c) or do not redraw the view (off0, off1).
			const double savedHFov = _omni3dMan.getHFov();
			const bool fovChanged = _warpLoaded && !voiceOnly && view.camFov;
			if (fovChanged)
				_omni3dMan.setHFov((double)kDialogCamFovDeg[camNum - 2] * M_PI / 180.0);
			// off1: FUN_00430884 shows the screen buffer before the subtitle,
			// once the voice has loaded (0x00430a12..0x00430ab8).
			if (off1 && _ixVideoBuffers) {
				Common::ScopedPtr<Common::SeekableReadStream> apc(openBigFileStream(kFileTypeSound,
				    Common::String::format("ATA%04d%c.APC", sec.id, conTrackLetter(lineIdx))));
				if (apc && g_system->getScreenFormat().bytesPerPixel == 2 &&
				        g_system->getWidth() == 640 && g_system->getHeight() == 480) {
					g_system->copyRectToScreen(_ixVideoBuffers, 640 * 2, 0, 0, 640, 480);
					g_system->updateScreen();
				}
			}
			playSingleConLine(sec.id, conTrackLetter(lineIdx++), item.line.text, vid,
			                  spfBase,
			                  /*hasAngle=*/false, 0, 0, effCamNum,
			                  /*interactiveDialog=*/sec.isCon,
			                  (!voiceOnly && !vid.empty()) ? dialRow : -1);
			if (fovChanged)
				_omni3dMan.setHFov(savedHFov);
		}
		_dialVoiceOff = false;
		_gameVars["clicperso"] = 0;   // after every line (0x00428b39)
		// FUN_00451915(0x00bcb59c, 0x00bcb598) after every line (0x00428b5e).
		if (_warpLoaded) {
			_omni3dMan.setAlpha(view.kept.alpha);
			_omni3dMan.setBeta(view.kept.beta);
		}
		// Escape during the talking head: the following lines up to the next
		// /set (or the /go, or the end) are skipped, their letters counted
		// (FUN_0042844c @ 0x00428b6d..0x00428bcd).
		if (_dialEscapeSkip) {
			_dialEscapeSkip = false;
			uint letters = (uint)lineIdx;
			itemIdx = conEscapeSkip(sec.items, itemIdx + 1, letters) - 1;
			lineIdx = (int)letters;
		}
	}
	// Game-over screen (GereGameOver, FUN_00426590): the executor calls it
	// when GAMEOVER is set after a line (0x00428c05), before the end of the
	// walk below (restores, music raised at 0x00428d8b), so the music stays
	// lowered under the narrator.
	if (pendingGameOver != 0) {
		showGameOver(sec.id, pendingGameOver, gameOverText, gameOverTrack);
		_gameVars["gameover"] = 0;   // FUN_0042de5c -> FUN_00426c68
		// The place loop then returns and the main loop reloads the player's
		// checkpoint (0x0041975d -> FUN_0042de5c): p<letter><N>.gam with N the
		// player's chapter byte 0x006ce246 (_playerChapter, set by the
		// checkpoint writer and by a row click in the load picker), not the
		// current chapter.  No file: chapter 1, place 1.
		const uint reload = (_currentPlayer >= 0) ? playerResumeSave((uint)_currentPlayer) : 0;
		if (reload != 0) {
			_loadedSave   = reload;
			_abortCommand = kAbortLoadGame;
		} else {
			warning("playConSection: game-over with no checkpoint; restarting");
			_abortCommand = kAbortNewGame;
		}
	}

	// A /go target runs in the same walk of the original (below): the end
	// of the walk, with its ClicPerso and angle restores, comes after it.
	const bool chainGo = _currentCONChapter == entryChapter && sec.gotoId > 0 && !shouldAbort() &&
	                     pendingGameOver == 0 && _abortCommand == kAbortNoAbort;
	if (!chainGo) {
		// The given-object icon (0x0049619c) ends with the walk, whatever
		// ran the section (0x00428c92, after the game-over call).
		_dialogGivenObjSprite = -1;
		_gameVars["clicperso"] = walk.clicPerso;   // 0x00428cb2
	}
	// The view goes back to the entry angles, except an axis whose
	// ANGLEWARPX / ANGLEWARPY is non-zero: that one keeps the current angle
	// and the variable is cleared (0x00428ccc..0x00428d68).  The warp loop
	// stores the angles there every frame, so a line's explicit angles stay
	// unless the player's own angle truncates to 0 degrees.  The entry
	// angles are those of the last talking-head (or perso >= 0x200) line
	// when one ran (0x004289ef, 0x00428aa6).
	auto restoreEntry = [&]() {
		if (_gameVars.getValOrDefault("anglewarpx", 0) != 0) {
			entry.alpha = _omni3dMan.getAlpha();
			_gameVars["anglewarpx"] = 0;
		}
		if (_gameVars.getValOrDefault("anglewarpy", 0) != 0) {
			entry.beta = _omni3dMan.getBeta();
			_gameVars["anglewarpy"] = 0;
		}
		_omni3dMan.setAlpha(entry.alpha);
		_omni3dMan.setBeta(entry.beta);
	};
	if (!chainGo)
		restoreEntry();
	musicUnduck();   // 0x00428d8b

	// Restore the cursor to whatever it was before the section.
	showMouse(cursorWasVisible);

	// /go N — chain to the continuation section once this body finishes.
	// Test the chapter guard FIRST: a /set(chapter=N) in the body reparsed
	// _conScript and left `sec` dangling, so reading sec.gotoId before the
	// guard would be a use-after-free.  Skip the chain on a pending game-over
	// (the reload replays the chapter) or any pending abort.
	if (chainGo) {
		// The executor resumes at the first "/N" header of the text
		// (atlantis.exe FUN_00425c04).
		ConSection target;
		if (_conScript.gotoSection(sec.gotoId, target)) {
			for (const ConSection &s : _conScript.sections()) {
				if (s.id == sec.gotoId) {
					target.isCon = s.isCon;
					break;
				}
			}
			playConSection(target, &walk);
		} else {
			_dialogGivenObjSprite = -1;   // 0x00428c92
			_gameVars["clicperso"] = walk.clicPerso;
			restoreEntry();
		}
	}

	// Drain the skip click that ended the last line so the outer warp loop
	// doesn't read it as a fresh navigation press.
	while (getCurrentMouseButton() != 0 && !shouldAbort()) {
		pollEvents();
		g_system->delayMillis(10);
	}
	clearKeys();
}

// Single dialog line: stream a UBB talking-head video over the warp panorama,
// play the matching APC speech track and overlay the subtitle.
//
// All geometry is data-driven: the UBB header gives the composite size, the
// screen format drives the CLUT8 conversion, and the GUI font drives the
// subtitle metrics.  The UBB frame is decoded into a CLUT8 buffer; the IA
// chunk's FUN_00450b08 opcode stream drives the per-pixel SKIP/RUN composite
// (SKIP leaves the cyclo panorama showing through, RUN paints palette[idx]
// with optional edge anti-aliasing).  The animated mouth mesh and the SPF
// overlays layer on top once the F3DC pipeline lands.
namespace {

// The 3D head's files, read from the BIG archives by their full path.
class BigHeadSource : public HeadFileSource {
public:
	explicit BigHeadSource(BigFileArchive &big) : _big(big) {}
	bool readFile(const Common::String &name, Common::Array<byte> &out) override {
		Common::ScopedPtr<Common::SeekableReadStream> s(_big.createReadStreamForMember(Common::Path(name, '/')));
		if (!s)
			return false;
		out.resize((uint)s->size());
		return s->read(out.data(), out.size()) == out.size();
	}
private:
	BigFileArchive &_big;
};

} // End of anonymous namespace

void CryOmni3DEngine_Atlantis::playSingleConLine(int sectionId, char track,
                                                  const Common::String &text,
                                                  const Common::String &videoName,
                                                  const Common::String &spfBaseName,
                                                  bool hasAngle,
                                                  int angleX, int angleY,
                                                  int camNum,
                                                  bool interactiveDialog,
                                                  int dialRow) {
	(void)interactiveDialog;  // used by the NPC sprite-hiding layer (later PR)
	const Graphics::PixelFormat fmt = g_system->getScreenFormat();
	const int screenW = (int)g_system->getWidth();
	const int screenH = (int)g_system->getHeight();

	// The per-cam FOV (kDialogCamFovDeg) is the caller's: FUN_0042844c sets
	// it before FUN_0042ee88, only on some paths (playConSection).

	// Per-line panorama angle (data-driven; degrees → radians).
	if (_warpLoaded && hasAngle) {
		_omni3dMan.setAlpha(((double)angleX) * M_PI / 180.0);
		_omni3dMan.setBeta(((double)angleY) * M_PI / 180.0);
	}

#ifdef USE_HNM
	Video::HNMDecoder *bgDecoder = nullptr;
	Graphics::Surface ubbConvFrame;
	// The frame count of the UBB header (hnm + 0x10): the original rewinds
	// there and the head animation's frame index restarts with it.
	int ubbFrameCount = 0;
	byte ubbPal[256 * 3];
	memset(ubbPal, 0, sizeof(ubbPal));

	if (!videoName.empty()) {
		Common::SeekableReadStream *vs = openBigFileStream(kFileTypeDialog, videoName);
		if (vs) {
			// read before the decoder takes the stream
			vs->seek(0x10);
			ubbFrameCount = (int)vs->readUint32LE();
			vs->seek(0);
		}
		if (!vs) {
			debugC(1, kDebugScript, "playSingleConLine: %s not found in BigFile",
			      videoName.c_str());
		} else {
			// UBB2 is 8-bit paletted; decode into CLUT8 and convert per-pixel so
			// we can colour-key and bleed the cyclo through SKIP runs.
			Graphics::PixelFormat clut8 = Graphics::PixelFormat::createFormatCLUT8();
			bgDecoder = new Video::HNMDecoder(clut8, true);  // loop=true
			if (!bgDecoder->loadStream(vs)) {
				delete bgDecoder;
				bgDecoder = nullptr;
			} else {
				ubbConvFrame.create(bgDecoder->getWidth(), bgDecoder->getHeight(), fmt);
				// Opt in to IA-chunk opcode capture so the decoder stops
				// discarding the FUN_00450b08 SKIP/RUN stream as audio.
				bgDecoder->setCaptureOpcodeStream(true);
				bgDecoder->start();
				debugC(1, kDebugScript, "playSingleConLine: %s loaded (%ux%u)",
				      videoName.c_str(), bgDecoder->getWidth(), bgDecoder->getHeight());
			}
		}
	}

	// Optional per-cam SPF overlay — the dialog mouth animation that the
	// original engine composites on top of the talking head every UBB frame.
	// The UBB stream has the mouth region painted as palette index 0 (the
	// canonical Cryo chroma key); the SPF fills it in.  Files live alongside
	// the UBB at `DIALOG\PERSO\<base>.SPF` (24 of them on the shipped CD),
	// NOT under `SPRITE\UBB\` (that prefix holds the 205 transition-video
	// SPFs used by playTransitionVideo for navigation walk-ons).  Only ~24
	// dialog cams have a mouth SPF — for the rest, the mouth is baked into
	// the UBB itself, so a missing file is normal and not a warning.
	Common::SeekableReadStream *spfStream = nullptr;
	uint spfFrameCount = 0;
	uint spfFrameIdx = 0;
	// UBB frames shown so far (the voice starts after the second one)
	uint ubbFramesShown = 0;
	// SPF screen bbox — the rectangle the overlay actually paints into on
	// screen.  The dialog SPF frame headers we sampled (BAPAN1K3.SPF,
	// GRDAN1C5.SPF) have ZEROED (x1, y_bot, x2, y_top) fields, unlike
	// navigation SPFs: they position themselves entirely via ROW_SEP opcodes
	// in the flat-buffer pixel stream, so the bbox only emerges from
	// rendering.  Probe by compositing frame[0] onto a scratch surface
	// seeded with a sentinel colour the SPF can never emit, then scan for
	// non-sentinel pixels.  Diagnostics only.
	int spfBboxX1 = -1, spfBboxY1 = -1, spfBboxX2 = -1, spfBboxY2 = -1;
	if (!spfBaseName.empty()) {
		Common::String spfFile = spfBaseName;
		spfFile.toUppercase();
		Common::String spfPath = Common::String("PERSO\\") + spfFile + ".SPF";
		spfStream = openBigFileStream(kFileTypeDialog, spfPath);
		if (spfStream) {
			if (spfStream->seek(4))
				spfFrameCount = spfStream->readUint32LE();
			// Composite frame[0] onto a scratch RGB565 surface seeded with
			// 0xFFFE.  SPF palettes emit RGB565 colours; 0xFFFE is taken as
			// unused (pure white is 0xFFFF), so any pixel left at 0xFFFE was
			// untouched and any other value was painted by the SPF.
			Graphics::PixelFormat probeFmt(2, 5, 6, 5, 0, 11, 5, 0, 0);
			Graphics::Surface probe;
			probe.create(640, 480, probeFmt);
			const uint16 sentinel = 0xFFFE;
			uint16 *probePix = (uint16 *)probe.getPixels();
			for (int i = 0; i < 640 * 480; i++) probePix[i] = sentinel;
			compositeSpfFrame(spfStream, 0, probe);
			for (int py = 0; py < 480; py++) {
				const uint16 *row = (const uint16 *)probe.getBasePtr(0, py);
				for (int px = 0; px < 640; px++) {
					if (row[px] != sentinel) {
						if (spfBboxX1 < 0 || px < spfBboxX1) spfBboxX1 = px;
						if (spfBboxX2 < 0 || px > spfBboxX2) spfBboxX2 = px;
						if (spfBboxY1 < 0 || py < spfBboxY1) spfBboxY1 = py;
						if (spfBboxY2 < 0 || py > spfBboxY2) spfBboxY2 = py;
					}
				}
			}
			probe.free();
			spfStream->seek(0);
		}
		debugC(1, kDebugScript, "playSingleConLine: SPF overlay '%s' %s "
		         "(%u frames, screen bbox [%d,%d]-[%d,%d])",
		      spfPath.c_str(),
		      spfStream ? "loaded" : "absent (mouth baked in UBB)",
		      spfFrameCount,
		      spfBboxX1, spfBboxY1, spfBboxX2, spfBboxY2);
	}
#endif

	// Subtitle pre-wrap.  Width budget is the composite width less symmetric
	// 5-px margins (matching the original's x=5 origin).
	const int subLeftMargin = 5, subRightMargin = 5;
	int subWidthBudget = screenW - subLeftMargin - subRightMargin;
#ifdef USE_HNM
	if (ubbConvFrame.getPixels())
		subWidthBudget = ubbConvFrame.w - subLeftMargin - subRightMargin;
#endif
	if (subWidthBudget < 32) subWidthBudget = 32;

	// The subtitle font (subtitleFont(): the player's arial.ttf or Liberation
	// Sans, at the 13 px Windows 98 renders the original's GDI Arial 16 at;
	// the 1997 build's DIALOG\FONTS.BIN, exe_build.h bitmapSubtitles).
	Common::Array<Common::String> subLines;
	int subLineH = 0;
	if (showSubtitles() && !text.empty() && subtitleFontReady()) {
		subLineH = subtitleRowStep();
		subtitleRows(text, subLines);
	}

	bool emptyBackbufferCleared = false;

	// A talking-head line (FUN_0042ee88) runs with 0x00496188 = 1 and calls
	// FUN_00425304 every frame: the place sprites animate, the persos are not
	// drawn.  The voice-only player FUN_00430258 calls FUN_00425304 once
	// (0x00430467, persos shown) before its wait loop, which does not; the
	// off0 player FUN_004305e8 never calls it.
	const bool talkingHead = (camNum != 0) && !_dialKeepScreen;
	bool voiceOffStep = _dialVoiceOff && !talkingHead;

#ifdef USE_HNM
	// The 3D head drawn over the UBB, the animated mouth (FUN_0042ee88 step
	// 4: the speaker's INITDIAL row, its first sub-record, the camera, and
	// the line's lip-sync track wav\ata<NNNN><letter>.syc read by
	// FUN_0042f9e8).  The original draws it into the RGB565 back buffer.
	Common::ScopedPtr<DialogHead> head;
	if (talkingHead && dialRow >= 0 && bgDecoder && ubbConvFrame.getPixels() &&
	        fmt == Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0) &&
	        ubbConvFrame.w == 640 && ubbConvFrame.h == 480 && camNum >= 2 && camNum <= 5) {
		Common::HashMap<int, DialInitEntry>::const_iterator it = _dialInitData.find(dialRow);
		if (it != _dialInitData.end() && !it->_value.s3dBase.empty() && !it->_value.anim3DCBase.empty() &&
		        !it->_value.anim3DABase.empty() && !it->_value.animMBase.empty()) {
			BigHeadSource src(_bigFile);
			head.reset(new DialogHead());
			const Common::String syc = Common::String::format("ATA%04d%c.SYC", sectionId, track);
			if (!head->load(src, it->_value.s3dBase, it->_value.anim3DCBase, it->_value.anim3DABase,
			                it->_value.animMBase, camNum, syc)) {
				// A head file is missing (lines without a .SYC never get
				// here: playConSection plays them voice-only).
				debugC(1, kDebugScript, "playSingleConLine: no head for row %d (%s)", dialRow, syc.c_str());
				head.reset();
			} else {
				// The rest pose of some head animations at frame 0 (an
				// original bug, decision 0016)
				head->setRestKeyFix(enhancementEnabled(kEnhMinorBugFixes));
			}
		}
	}
	bool headTimeStarted = false, headEnded = false;
	uint32 headTime0 = 0;
#endif
	// The voice has been started (FUN_0042ee88: 0x006ce108 != 0 once it is).
	bool voiceStarted = false;
	// The lip-sync ended (or failed) before the voice started: FUN_0042ee88
	// leaves its loop before drawing (0x0042f47b..0x0042f48c -> 0x0042f6e3).
	bool headCut = false;
	if (talkingHead) {
		_spritesHideNpcs = true;
		recompositeSpriteLayer();
	}

	// off0: keep a copy of the screen as it stands for the subtitle's
	// background.
	Graphics::ManagedSurface keptScreen;
	if (_dialKeepScreen) {
		Graphics::Surface *scr = g_system->lockScreen();
		if (scr) {
			keptScreen.copyFrom(*scr);
			g_system->unlockScreen();
		} else {
			_dialKeepScreen = false;
		}
	}

	auto drawFrame = [&]() {
		if (talkingHead) {
			animateSprites();
		} else if (voiceOffStep) {
			animateSprites();
			voiceOffStep = false;
		}
		const Graphics::Surface *warpView = _dialKeepScreen
		    ? &keptScreen.rawSurface()
		    : (_warpLoaded ? _omni3dMan.getSurface() : nullptr);

#ifdef USE_HNM
		if (bgDecoder && ubbConvFrame.getPixels()) {
			const Graphics::Surface *f = nullptr;
			if (bgDecoder->needsUpdate())
				f = bgDecoder->decodeNextFrame();

			if (f) {
				if (bgDecoder->hasDirtyPalette())
					memcpy(ubbPal, bgDecoder->getPalette(), 256 * 3);

				// Seed the composite with the cyclo panorama so SKIP runs show
				// it through (mirrors FUN_00451d8b); black if no panorama.
				if (warpView && warpView->w == ubbConvFrame.w &&
				                warpView->h == ubbConvFrame.h &&
				                warpView->format == fmt) {
					const uint copyRowBytes = ubbConvFrame.w * fmt.bytesPerPixel;
					for (int row = 0; row < ubbConvFrame.h; row++)
						memcpy(ubbConvFrame.getBasePtr(0, row),
						       warpView->getBasePtr(0, row), copyRowBytes);
				} else {
					ubbConvFrame.fillRect(Common::Rect(ubbConvFrame.w, ubbConvFrame.h), 0);
				}

				const int fw = f->w, fh = f->h;
				const byte *srcBase = (const byte *)f->getBasePtr(0, 0);
				uint32 opSize = 0;
				const byte *opcodes = bgDecoder->getOpcodeStream(opSize);

				uint16 palLUT[256];
				for (int i = 0; i < 256; i++)
					palLUT[i] = (uint16)fmt.RGBToColor(ubbPal[i*3+0], ubbPal[i*3+1],
					                                  ubbPal[i*3+2]);

				if (opcodes && opSize >= 4 && fmt.bytesPerPixel == 2) {
					// FUN_00450b08 (sprite_blend.h).
					composeUbbIa((uint16 *)ubbConvFrame.getPixels(), srcBase, palLUT,
					             opcodes, opSize, (uint32)fw * (uint32)fh);
				} else {
					// No opcode stream (or non-16bpp) — flat composite so the
					// head is at least visible.
					const uint bpp = fmt.bytesPerPixel;
					const int fp = f->pitch;
					for (int fy = 0; fy < fh; fy++) {
						const byte *src = srcBase + fy * fp;
						byte *dst = (byte *)ubbConvFrame.getBasePtr(0, fy);
						for (int fx = 0; fx < fw; fx++, dst += bpp) {
							const uint32 c = palLUT[src[fx]];
							if (bpp == 2)      *(uint16 *)dst = (uint16)c;
							else if (bpp == 4) *(uint32 *)dst = c;
							else               memcpy(dst, &c, bpp);
						}
					}
				}

				// The 3D head (FUN_0044dd10 with the frame index of the UBB
				// loop, FUN_0044e860 at the ms elapsed since the first frame,
				// or FUN_0044e2e0 once it has returned a negative value, then
				// FUN_0044c9c0), drawn after the UBB composite and before the
				// SPF frame as in FUN_0042ee88's loop.
				if (head) {
					const int cur = bgDecoder->getCurFrame();
					head->setFrame(ubbFrameCount > 0 ? cur % ubbFrameCount : cur);
					const uint32 now = g_system->getMillis();
					if (!headTimeStarted) {
						headTime0 = now;
						headTimeStarted = true;
					}
					if (headEnded)
						head->neutral();
					// 0x006ce0e0 is a millisecond clock advanced by 5 at
					// 200 Hz (FUN_00431150, AIL_register_timer /
					// AIL_set_timer_frequency(200) in FUN_004310ac): the
					// elapsed time is a multiple of 5
					else if (head->lipsync((int32)((now - headTime0) / 5 * 5)) < 0)
						headEnded = true;
					// A negative lip-sync result (kept once it went
					// negative) while the sample status 0x006ce108 is still
					// 0, i.e. before the voice starts, ends the line with
					// nothing drawn (0x0042f47b..0x0042f48c).  No shipped
					// .syc ends that early.
					if (headEnded && !voiceStarted) {
						headCut = true;
						return;
					}
					head->transform();
					head->render((uint16 *)ubbConvFrame.getPixels(), ubbConvFrame.pitch / 2);
				}

				// Per-cam SPF overlay (INITDIAL columns 10..13: the guards'
				// mail coif, the pilot's scarf), loaded by FUN_0042fc90 and
				// drawn by FUN_0042ff90 after the head.  The original draws it
				// into the locked screen surface (FUN_0041a254 = 0x00772968),
				// which the full copy of the back buffer (FUN_00410f36
				// @ 0x0042f57e) then overwrites: the overlay is never seen and
				// the mouth stays in front of the coif (atlantis-re experiment
				// 2026-10-17-09).  Drawn on top of the head under
				// kEnhMinorBugFixes (decision 0016 row 20).  The modulo loops
				// the SPF when the UBB outlives it (compositeSpfFrame draws
				// nothing past the last frame).
				if (spfStream && enhancementEnabled(kEnhMinorBugFixes)) {
					const uint sfi = (spfFrameCount > 0)
					    ? (spfFrameIdx % spfFrameCount)
					    : spfFrameIdx;
					compositeSpfFrame(spfStream, sfi, ubbConvFrame);
				}

				// "Just received" item icon: an NPC that gave the player an
				// object this dialog shows its icon centre-bottom over the head.
				if (fmt.bytesPerPixel == 2)
					drawGivenObject(ubbConvFrame);

				// Advance the SPF frame counter once per fresh UBB frame —
				// the same one-to-one mapping the original engine uses
				// (FUN_0042ff90).
				++spfFrameIdx;
				++ubbFramesShown;
			}
		} else
#endif
		if (warpView) {
			// No video: the panorama alone.  The given-object icon
			// (0x0049619c) is drawn only by the talking-head loop of
			// FUN_0042ee88 (0x0042f52d); the voice-only players FUN_00430258,
			// FUN_004305e8 and FUN_00430884 never read it.
			g_system->copyRectToScreen(warpView->getPixels(), warpView->pitch,
			                           0, 0, warpView->w, warpView->h);
		} else if (!emptyBackbufferCleared) {
			Graphics::ManagedSurface clr(screenW, screenH, fmt);
			clr.fillRect(Common::Rect(screenW, screenH), 0);
			g_system->copyRectToScreen(clr.getPixels(), clr.pitch, 0, 0, screenW, screenH);
			emptyBackbufferCleared = true;
		}

		// Subtitle overlay, drawn directly onto the composite (no opaque bar),
		// bottom-anchored with a 1-px drop shadow.
#ifdef USE_HNM
		if (bgDecoder && ubbConvFrame.getPixels()) {
			Graphics::ManagedSurface compose(ubbConvFrame.w, ubbConvFrame.h, fmt);
			const uint rowBytes = ubbConvFrame.w * fmt.bytesPerPixel;
			for (int row = 0; row < ubbConvFrame.h; row++)
				memcpy(compose.getBasePtr(0, row), ubbConvFrame.getBasePtr(0, row), rowBytes);

			// The original draws on the 640x480 screen (FUN_00430178); the
			// composite is that screen when it is 640x480.
			const int subOffY = (compose.h - 480) / 2;
			drawSubtitleRows(compose, subLines, subOffY, subWidthBudget);
			int blitX = (screenW - compose.w) / 2;
			int blitY = (screenH - compose.h) / 2;
			if (blitX < 0) blitX = 0;
			if (blitY < 0) blitY = 0;
			g_system->copyRectToScreen(compose.getPixels(), compose.pitch,
			                           blitX, blitY, compose.w, compose.h);
		} else
#endif
		if (!subLines.empty() && subLineH > 0) {
			// From the first line's top to below the last line (FUN_00430178).
			const Graphics::Font *guiFont = _exeBuild->bitmapSubtitles ? nullptr : subtitleFont();
			const int lastH = MAX(subLineH, guiFont ? guiFont->getFontHeight() + 1 : subLineH);
			int srcY = subtitleRowY(0, subLines.size());
			if (srcY < 0) srcY = 0;
			int subAreaH = subtitleRowY(subLines.size() - 1, subLines.size()) + lastH - srcY;
			Graphics::ManagedSurface overlay(screenW, subAreaH + 4, fmt);
			if (warpView) {
				int rowsAvail = warpView->h - srcY;
				if (rowsAvail > overlay.h) rowsAvail = overlay.h;
				const uint rowBytes = warpView->w * fmt.bytesPerPixel;
				for (int row = 0; row < rowsAvail; row++)
					memcpy(overlay.getBasePtr(0, row),
					       warpView->getBasePtr(0, srcY + row), rowBytes);
			} else {
				overlay.fillRect(Common::Rect(overlay.w, overlay.h), 0);
			}
			drawSubtitleRows(overlay, subLines, -srcY, subWidthBudget);
			g_system->copyRectToScreen(overlay.getPixels(), overlay.pitch,
			                           0, srcY, overlay.w, overlay.h);
		}
		g_system->updateScreen();
	};

	// The line lasts as long as its sound (atlantis.exe FUN_0042ee88 /
	// FUN_00430258): it ends when the sample is done or on a left click
	// (0x0049611c).  Escape is read by FUN_0041a298 (videoSkipPoll): a line
	// with a talking head runs with 0x00496188 set, so Escape ends it and
	// sets the dialog-skip flag 0x00496134 (0x0041a3d2); the voice-only
	// players (off, off0, off1, no .syc) clear 0x00496188 first, so Escape
	// ends the line only when bow mode, the bar request and OBJETENMAIN are
	// all 0 (0x00496124 = 0, 0x0041a419; their loops test it, e.g.
	// 0x00430586), without the skip flag: the following lines play, and the
	// players put 0x00496124 and 0x00496138 back (0x004305b7, 0x004305c1),
	// so no menu opens.  Without its APC the original shows nothing and
	// moves on (FUN_00430258 returns 0).
	VideoSkipState escapePoll;
	escapePoll.talkingHead = (camNum != 0);
	escapePoll.bowMode = _bowMode;
	escapePoll.objectInHand = _inventory.selectedObject() != nullptr ||
	                          _gameVars.getValOrDefault("objetenmain", 0) != 0;
	escapePoll.sectionPlay = _sectionPlay;

	// One click/key skips only one line: drain a still-held skip from the
	// previous line so this one watches for a fresh press.
	while (getCurrentMouseButton() != 0 && !shouldAbort()) {
		pollEvents();
		g_system->delayMillis(10);
	}
	clearKeys();

	// FUN_00430258 (no talking head) lowers the music when no fade runs
	// (@ 0x0043027f) and raises it once the voice has played (@ 0x004305a6);
	// without the voice file it returns before, leaving the music low.
	if (camNum == 0)
		musicDuck(true);
	Common::String apcName = Common::String::format("ATA%04d%c.APC", sectionId, track);
	Common::SeekableReadStream *apcFile = openBigFileStream(kFileTypeSound, apcName);

	if (!apcFile) {
		debugC(1, kDebugScript, "playSingleConLine: no %s, line skipped", apcName.c_str());
	} else {
		Audio::AudioStream *apc = makeAtlantisAPCStream(*apcFile);
		if (apc) {
			Audio::SoundHandle handle;
			// With a talking head (FUN_0042ee88's loop) the voice starts at
			// the end of the iteration whose frame index, once incremented,
			// is above 1: after the second UBB frame shown (0x0042f678..
			// 0x0042f6a7; every shipped UBB has 10 frames or more, so the
			// index gets there before a rewind).  The voice-only players
			// start it at once.
			auto startVoice = [&]() {
				_mixer->playStream(Audio::Mixer::kSpeechSoundType, &handle, apc);
				voiceStarted = true;
				debugC(1, kDebugScript, "playSingleConLine: %s", apcName.c_str());
			};
#ifdef USE_HNM
			const bool delayVoice = bgDecoder && talkingHead;
#else
			const bool delayVoice = false;
#endif
			if (!delayVoice)
				startVoice();

			uint prevButton = getCurrentMouseButton();
			while (!shouldAbort() && (!voiceStarted || _mixer->isSoundHandleActive(handle))) {
				drawFrame();
				if (headCut)
					break;
				if (!voiceStarted && ubbFramesShown >= 2)
					startVoice();
				pollEvents();
				// FUN_0041a298 in both line loops: a right press sets
				// 0x00496120 and toggles the bar request (bar_session.h).
				const uint button = getCurrentMouseButton();
				if (button == 2 && prevButton != 2)
					BarSessionRules::rightPressed(_barSession, _sectionPlay, false);
				prevButton = button;
				// The bar request as the right presses left it.
				escapePoll.barRequest = _barSession.request;
				bool escape = false;
				if (checkKeysPressed(1, Common::KEYCODE_ESCAPE))
					escape = videoSkipPoll(escapePoll, false, false, true);
				if (escape || getCurrentMouseButton() == 1) {
					// With a talking head Escape also sets the dialog-skip
					// flag 0x00496134 that FUN_0042844c reads after the
					// line (conEscapeSkip).
					if (escape && escapePoll.talkingHead)
						_dialEscapeSkip = true;
					if (voiceStarted)
						_mixer->stopHandle(handle);
					break;
				}
				g_system->delayMillis(10);
			}
			if (voiceStarted)
				_mixer->stopHandle(handle);
			else
				delete apc;   // never handed to the mixer
			if (voiceStarted && camNum == 0) {
				musicUnduck();
				BarSessionRules::voiceLineEnded(_barSession);   // 0x004305cb
			}
		}
		delete apcFile;
	}

#ifdef USE_HNM
	delete bgDecoder;
	delete spfStream;
	ubbConvFrame.free();
#endif
	if (talkingHead) {
		_spritesHideNpcs = false;
		recompositeSpriteLayer();
	}
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D
