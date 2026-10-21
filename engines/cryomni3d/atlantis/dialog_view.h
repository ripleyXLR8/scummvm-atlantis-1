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

#ifndef CRYOMNI3D_ATLANTIS_DIALOG_VIEW_H
#define CRYOMNI3D_ATLANTIS_DIALOG_VIEW_H

#include "common/scummsys.h"
#include "common/str.h"

#include "cryomni3d/atlantis/apc_stream.h"
#include "cryomni3d/atlantis/omni3d.h"

namespace CryOmni3D {
namespace Atlantis {

// The view behind a dialog line, from the section executor FUN_0042844c of
// atlantis.exe (atlantis-re functions/scripts-FUN_0042844c.md, "View
// angles"; emulated by tools/emu/video_dialog_line_view.py).
//
// The view angles 0x00bcb59c / 0x00bcb598 ("kept" here) only move on a line
// with explicit header angles.  The turn of a talking-head line is drawn
// into the projection grid alone: the globals are put back right after it
// (0x00428a7a..0x00428a85, 0x00428aec..0x00428af7), and after every line
// the view is set back to them (0x00428b52..0x00428b5e).  So each line
// turns from the same kept angles, never from the previous line's turn.
namespace DialogView {

struct Angles {
	double alpha, beta;   // radians
};

struct Line {
	Angles view;          // the panorama behind the talking head
	Angles kept;          // 0x00bcb59c / 0x00bcb598 during and after the line
	bool entryUpdate;     // the section-end angles become `kept` before the line
	bool camFov;          // the camera's FOV is set (FUN_0045179a(FOV[cam])); else
	                      // no FOV call: the FOV in effect stays (for the four
	                      // shipped lines, each the first of its walk, the
	                      // FOV the walk started with; see LineFov)
};

// What FUN_0042844c carries along one walk, through /go targets too
// (FUN_00425c04 hands the target back to the same loop).
struct Walk {
	Angles entry;         // EBP-0xc / EBP-8, restored at the end of the walk
	int clicPerso;        // ClicPerso at the start of the walk (0x004284c2), restored at its end
};

// The INITDIAL row (0x0077290c) of a line: the IncPerso count for a Seth
// line (0x00428a91); otherwise ClicPerso's low byte (0x00428924, the byte
// store of `local_40`), so a ClicPerso below 0x200 still picks row
// 0x200 + (ClicPerso & 0xff).  FUN_0042ee88 indexes the table with it.
inline int dialRow(bool sethPath, int clicPerso, uint8 incPerso) {
	return 0x200 + (sethPath ? incPerso : (clicPerso & 0xff));
}

inline double clampBeta(double b, double betaMax) {
	// FUN_00451915: beta kept when -max <= beta <= max, else the limit.
	if (b > betaMax)
		return betaMax;
	if (b < -betaMax)
		return -betaMax;
	return b;
}

inline double wrapAlpha(double a) {
	// FUN_00451915 @ 0x00451915: alpha >= 2 pi -> alpha - 2 pi, < 0 -> + 2 pi.
	const double twoPi = 2.0 * M_PI;
	if (a >= twoPi)
		return a - twoPi;
	if (a < 0.0)
		return a + twoPi;
	return a;
}

// kept: the angles before the line.  sethPath: the header starts with "Seth"
// and has no off / off0 / off1 flag (0x00428949..0x0042896c).  clicPerso:
// ClicPerso after the speaker lookup.  hasAngle, x, y: explicit header angles
// (radians); betaMax: FUN_00451915's pitch limit.
inline Line lineView(const Angles &kept, bool sethPath, int clicPerso,
                     bool hasAngle, double x, double y, double betaMax) {
	Line l;
	l.view = kept;
	l.kept = kept;
	l.entryUpdate = false;
	l.camFov = false;
	if (sethPath) {
		// 0x004289ef..0x00428a75: entry angles := globals; FOV of the cam
		// (0x004289ff..0x00428a14); grid at (alpha + pi, 0).
		l.entryUpdate = true;
		l.camFov = true;
		l.view.alpha = wrapAlpha(kept.alpha + M_PI);
		l.view.beta = 0.0;
	} else if (clicPerso >= 0x200) {
		// 0x00428a9d..0x00428ae7 (off lines too): entry angles := globals;
		// FOV 75.137 then the cam's (0x00428ab6..0x00428ada); grid at
		// (alpha, 0).  Below 0x200 (JL 0x00428afc) none of this runs: no
		// FOV call.
		l.entryUpdate = true;
		l.camFov = true;
		l.view.beta = 0.0;
	}
	if (hasAngle) {
		// 0x00428afc..0x00428b0e: FUN_00451915(X, Y) writes the globals too.
		l.view.alpha = wrapAlpha(x);
		l.view.beta = clampBeta(y, betaMax);
		l.kept = l.view;
	}
	return l;
}

// The panorama's FOV over one talking-head line, as playConSection plays
// it: the cam's (kDialogCamFovDeg, the table 0x00496080) when the line sets
// one (Line::camFov: FUN_0042844c @ 0x00428a14 / 0x00428ada), not for a
// voice-only line (FUN_00430258 draws at 75.137 whatever was set, 0x0043044c;
// off0 / off1 do not redraw the view) nor without a panorama; else the FOV
// in effect stays.  The original resets 75.137 after every line
// (0x00428b43..0x00428b4d); the port puts back the FOV it had before the
// line, which is kWarpFovDeg 75.137 (engine.cpp init; every other FOV change
// of the port is put back the same way).  For the four shipped lines with no
// FOV call (CHAPI056 /744 A, CHAPI065 /901 A, CHAPI068 /998 A, CHAPI069
// /1019 A, each the first line of its walk) the original's FOV is the one the
// walk started with, 75.137 [CONFIRMED-STATIC] (atlantis-re experiment
// 2026-10-21-05: FUN_00423ad8 0x00423ed4 -> FUN_0044a424 0x0044a706, or a
// previous walk's 0x00428cc2).
class LineFov {
public:
	LineFov(Omni3DManagerAtlantis &omni, bool warpLoaded, const Line &l, bool voiceOnly, int camNum)
	    : _omni(omni), _saved(omni.getHFov()), _set(warpLoaded && !voiceOnly && l.camFov) {
		if (_set)
			_omni.setHFov((double)kDialogCamFovDeg[camNum - 2] * M_PI / 180.0);
	}
	~LineFov() {
		if (_set)
			_omni.setHFov(_saved);
	}

private:
	LineFov(const LineFov &);
	LineFov &operator=(const LineFov &);

	Omni3DManagerAtlantis &_omni;
	const double _saved;
	const bool _set;
};

// A talking-head line (not voice-only) whose UBB does not open.  A row with
// no INITDIAL record has an empty UBB name, so FUN_0042ee88 opens
// "dialog\perso\" (FUN_00434814 @ 0x0042f005, cryo.dll _GL_CreateFile@28 ->
// _GL_FileOpen@8, mode 1); an explicit camN whose INITDIAL UBB is "none"
// (FUN_0042844c keeps it: the draw loops are skipped, 0x004287cf) has the
// name "none.ubb", which no archive holds.  With the modes WinMain sets
// (_GL_SetErrorMode(3) @ 0x00418f1e, _SetBigFileDebug(1) @ 0x00418f30) the
// open returns -1 without quitting; FUN_0042ee88 shows error dialog 0x65
// code 0xc ("Fichier introuvable : ") and returns before the subtitle and
// the voice (0x0042f00f..0x0042f056); FUN_0042844c ignores the result and
// goes on with the next line, the letter counted (0x00428b21).  camUBB: the
// record's name for the line's cam (the port's parser keeps "none" there).
// No line of the shipped CON files takes the "none" case (0 of 248
// explicit-cam lines land on a "none" cam).
inline bool headOpenFails(bool voiceOnly, bool haveRecord, const Common::String &camUBB) {
	return !voiceOnly && (!haveRecord || camUBB.empty() || camUBB.equalsIgnoreCase("none"));
}

// The text of that error box: the procedure 0x00419808 of dialog 0x65
// builds, for code 0xc (@ 0x004199e9), "Fichier introuvable : " (0x004bbf18)
// + the path 0x006cd9d0, copied from 0x006cd8c0 = the string 0x0049619f
// (empty: never written) + "dialog\perso\" (0x004bbbfc) + the UBB name
// (FUN_0042ee88 @ 0x0042ef9d..0x0042f031).  The box has an OK button.
static const char *const kHeadOpenFailText = "Fichier introuvable : dialog\\perso\\";

// The box text for the UBB name the original appends: "" without a
// record, "none.ubb" for a "none" cam (the parser appends ".ubb").
inline Common::String headOpenFailText(const Common::String &ubbName) {
	return Common::String(kHeadOpenFailText) + ubbName;
}

// The screen of an "off1" line (FUN_00430884; atlantis-re
// functions/video-FUN_00430258.md#fun_00430884--off1-line, emulated by
// tools/emu/video_dialog_off1.py, golden video_dialog_off1.txt).  Once the
// voice is loaded (an APC 1.20 header, FUN_00417cb4; a missing file or a
// bad header returns 0 before anything is drawn, 0x0043099d..0x004309b2),
// the surface is locked and FUN_00410f36 copies the screen buffer
// 0x004960d4 to the back buffer 0x004960d8 (0x00430a86..0x00430a9c), then
// the back buffer to the surface at its pitch (0x00430aa4..0x00430abb):
// 480 rows of 0x500 bytes, read contiguously, written every `pitch` bytes;
// the bytes of a wider row past 0x500 keep what they had.  The voice starts
// after, then the subtitle is drawn over that screen.  screenBuffer: 640 x
// 480 RGB565 (the port's first IX buffer, _ixVideoBuffers).
static const uint kOff1Rows = 0x1e0;
static const uint kOff1RowBytes = 0x500;

inline void showScreenBuffer(const byte *screenBuffer, byte *surface, uint pitch) {
	for (uint y = 0; y < kOff1Rows; y++)
		memcpy(surface + y * pitch, screenBuffer + y * kOff1RowBytes, kOff1RowBytes);
}

// The voice step of a dialog line, as playSingleConLine runs it once the
// .APC has been looked up: apcFile is the file, or null when it is absent
// (FUN_00418920 returned 0).  The voice is decoded by makeAtlantisAPCStream,
// which checks the "CRYO_APC" "1.20" header as FUN_00417cb4; an absent file
// or a bad header returns null with nothing drawn (FUN_00430884 returns 0,
// 0x0043099d..0x004309b2).  With a stream, an off1 line (screen not null)
// gets the screen buffer on its screen (showScreenBuffer) here, so before
// the caller can start the voice with the returned stream (FUN_00430884
// copies at 0x00430a86..0x00430abb, starts the sample after).  screen null:
// any other line, nothing drawn.
inline Audio::AudioStream *loadLineVoice(Common::SeekableReadStream *apcFile,
		const byte *screenBuffer, byte *screen, uint pitch) {
	if (!apcFile)
		return nullptr;
	Audio::AudioStream *apc = makeAtlantisAPCStream(*apcFile);
	if (apc && screen && screenBuffer)
		showScreenBuffer(screenBuffer, screen, pitch);
	return apc;
}

} // End of namespace DialogView

} // End of namespace Atlantis
} // End of namespace CryOmni3D

#endif
