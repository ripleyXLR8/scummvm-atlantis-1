#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_FRMAY1997_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_FRMAY1997_H

// ui audit: the French CD's first pressing of May 1997
// (atlantis-cd-fr-688640), as atlantis-re experiment 2026-10-18-04 read it
// (STATIC, EMULATED, DATA) and decision 0020 chose.  The build is
// en-688128's code (its rules are checked in ui_en1997.h and
// video_subtitles_en1997.h; the flags in exe_builds.h), so here:
//   M1 LoadMusik all or nothing on its first track (FUN_0041e830)
//   M2 LOADMUSIK only with a current music volume of 5 or more
//      (FUN_00429184 @ 0x00429c5e), and how soon a StartMusik reaches it
//   B  the per-track copy kept for kEnhMinorBugFixes (the other builds' rule)
//
// Port code exercised directly: conMusicApply / conLoadMusikVolumeOk
// (con_eval.cpp), AtlantisMusicFade::tick (music_fade.h), atlantisExeBuild
// (exe_build.h).

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/array.h"

#include "cryomni3d/atlantis/con_eval.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/music_fade.h"

namespace UiFrMay1997 {

using namespace CryOmni3D::Atlantis;

inline const AtlantisExeBuild &may() { return atlantisExeBuild(688640); }

} // End of namespace UiFrMay1997

class UiFrMay1997TestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// M1: FUN_0041e830 tests only the first listed track: its copy there
	// (FUN_0041ee40 @ 0x0041e86e) or playing (0x0041e878..0x0041e891), then
	// nothing; else every listed track is copied without a test (loop
	// 0x0041e926..0x0041e96e), present or playing ones included.
	void test_music_load_all_or_nothing() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		Common::Array<uint8> list;
		list.push_back(4);   // tracks 5, 7, 9
		list.push_back(6);
		list.push_back(8);
		Common::Array<uint8> copies;
		// the first one there: nothing, the missing ones included
		uint32 disk = 1u << 4;
		conMusicApply(true, list, 0, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 1u << 4);
		TS_ASSERT(copies.empty());
		// the other builds copy the missing ones
		conMusicApply(true, list, 0, disk, copies, true);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6) | (1u << 8));
		TS_ASSERT_EQUALS(copies.size(), 2u);
		// the first one missing: all three copied, the present one again
		copies.clear();
		disk = 1u << 6;
		conMusicApply(true, list, 0, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6) | (1u << 8));
		TS_ASSERT_EQUALS(copies.size(), 3u);
		if (copies.size() == 3)
			TS_ASSERT(copies[0] == 4 && copies[1] == 6 && copies[2] == 8);
		// the playing track copied too when it is not first
		copies.clear();
		disk = 0;
		conMusicApply(true, list, 7, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(copies.size(), 3u);
		// ... and nothing when it is first
		copies.clear();
		disk = 0;
		conMusicApply(true, list, 5, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 0u);
		TS_ASSERT(copies.empty());
		// DelMusik keeps en-688128's rule (FUN_0041eee8 deletes the playing
		// track when it is not first)
		disk = (1u << 4) | (1u << 6) | (1u << 8);
		conMusicApply(false, list, 7, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 0u);
		// a first index of 0x20 or more: nothing, as in every build
		Common::Array<uint8> high;
		high.push_back(0x20);
		high.push_back(4);
		disk = 0;
		conMusicApply(true, high, 0, disk, copies, b.musicFirstValueRule, b.musicLoadAllOrNothing);
		TS_ASSERT_EQUALS(disk, 0u);
	}

	// M2: FUN_00429184 @ 0x00429c5e runs LoadMusik only when 0x00485e71 is
	// 5 or more; the other builds always.  0x00485e71 starts at 0 with each
	// StartMusik and gains 1 every 5 ticks of the 200 Hz timer
	// (AtlantisMusicFade::tick): 5 is reached after 25 ticks (125 ms).
	void test_music_load_volume_rule() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		for (uint v = 0; v < 5; v++)
			TSM_ASSERT(v, !conLoadMusikVolumeOk((uint8)v, b.musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(5, b.musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(126, b.musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(0, atlantisExeBuild(688128).musicLoadMinVolume));
		TS_ASSERT(conLoadMusikVolumeOk(0, atlantisExeBuild(714240).musicLoadMinVolume));
		// the initial value (0x6e in the data) lets the first LOADMUSIK run
		AtlantisMusicFade initial;
		TS_ASSERT(conLoadMusikVolumeOk(initial.cur, b.musicLoadMinVolume));
		// after a StartMusik (musicSwitchTrack: cur 0, fade-in to vol)
		AtlantisMusicFade f;
		f.vol = 126;
		f.cur = 0;
		f.mode = 1;
		f.upTarget = f.vol;
		f.ticks = 0;
		f.lastStep = 0;
		int ticks = 0;
		while (!conLoadMusikVolumeOk(f.cur, b.musicLoadMinVolume) && ticks < 1000) {
			f.tick();
			ticks++;
		}
		TS_ASSERT_EQUALS(ticks, 25);
		// with a music volume option below 5 it is never reached
		AtlantisMusicFade low;
		low.vol = 4;
		low.cur = 0;
		low.mode = 1;
		low.upTarget = low.vol;
		for (int i = 0; i < 1000; i++)
			low.tick();
		TS_ASSERT(!conLoadMusikVolumeOk(low.cur, b.musicLoadMinVolume));
	}

	// Decision 0020 B: with kEnhMinorBugFixes the port copies per track
	// (music.cpp execMusicList: allOrNothing off, no volume test): the
	// rule of FUN_0041f1d0, each listed track not on disk and not playing.
	void test_music_minor_bug_fix_rule() {
		using namespace CryOmni3D::Atlantis;
		const AtlantisExeBuild &b = UiFrMay1997::may();
		Common::Array<uint8> list;
		list.push_back(4);
		list.push_back(6);
		list.push_back(8);
		Common::Array<uint8> copies;
		uint32 disk = 1u << 4;
		conMusicApply(true, list, 9, disk, copies, b.musicFirstValueRule, false);
		TS_ASSERT_EQUALS(disk, (1u << 4) | (1u << 6));
		TS_ASSERT_EQUALS(copies.size(), 1u);
		if (copies.size() == 1)
			TS_ASSERT_EQUALS(copies[0], 6);
		TS_ASSERT(conLoadMusikVolumeOk(0, 0));
	}
};

#endif
