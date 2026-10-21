// Verifier "audio": the music fades (task 24).
//
// Original: atlantis-cd-en-714240's 200 Hz timer callback FUN_00431150 (tick,
// then FUN_0042f7a8 fade down, FUN_0042f974 fade up), emulated unmodified by
// atlantis-re/tools/emu/audio_music_fade.py: the music volume 0x00495e71 and
// the mode 0x00495eac after each tick, as runs "volume,mode,count".
// The second part of FUN_0042f7a8 (the NewSound fade-out, 0x0042f89d..
// 0x0042f961) with 0x00495eb0 = 2 armed: the same run, plus the dword
// 0x006ce128 (NewSound volume) and 0x00495eb0, as runs
// "volume,mode,sfxvolume,sfxmode,count" (sfx scenarios of the harness; its
// stop path's sample-name lookup FUN_00429d20 is stubbed to slot 0, which
// does not touch the recorded variables).
// Port: AtlantisMusicFade::tick (music_fade.h).

#ifndef TEST_ATLANTIS_AUDIO_MUSIC_FADE_H
#define TEST_ATLANTIS_AUDIO_MUSIC_FADE_H

#include "cryomni3d/atlantis/music_fade.h"
#include "../../../system/null_osystem.h"

namespace AudioMusicFade {

struct FadeCase {
	const char *name;
	int cur, mode, percent, vol;
	uint32 ticks, last;
	int n;
	const char *runs;
};

// tools/emu/audio_music_fade.py
static const FadeCase kFadeCases[] = {
	{ "duck_80", 100, 2, 80, 100, 1000, 0, 450, "99,2,5;98,2,5;97,2,5;96,2,5;95,2,5;94,2,5;93,2,5;92,2,5;91,2,5;90,2,5;89,2,5;88,2,5;87,2,5;86,2,5;85,2,5;84,2,5;83,2,5;82,2,5;81,2,5;80,2,5;79,2,5;78,2,5;77,2,5;76,2,5;75,2,5;74,2,5;73,2,5;72,2,5;71,2,5;70,2,5;69,2,5;68,2,5;67,2,5;66,2,5;65,2,5;64,2,5;63,2,5;62,2,5;61,2,5;60,2,5;59,2,5;58,2,5;57,2,5;56,2,5;55,2,5;54,2,5;53,2,5;52,2,5;51,2,5;50,2,5;49,2,5;48,2,5;47,2,5;46,2,5;45,2,5;44,2,5;43,2,5;42,2,5;41,2,5;40,2,5;39,2,5;38,2,5;37,2,5;36,2,5;35,2,5;34,2,5;33,2,5;32,2,5;31,2,5;30,2,5;29,2,5;28,2,5;27,2,5;26,2,5;25,2,5;24,2,5;23,2,5;22,2,5;21,2,5;20,0,55" },
	{ "unduck", 20, 1, 80, 100, 1000, 0, 450, "21,1,5;22,1,5;23,1,5;24,1,5;25,1,5;26,1,5;27,1,5;28,1,5;29,1,5;30,1,5;31,1,5;32,1,5;33,1,5;34,1,5;35,1,5;36,1,5;37,1,5;38,1,5;39,1,5;40,1,5;41,1,5;42,1,5;43,1,5;44,1,5;45,1,5;46,1,5;47,1,5;48,1,5;49,1,5;50,1,5;51,1,5;52,1,5;53,1,5;54,1,5;55,1,5;56,1,5;57,1,5;58,1,5;59,1,5;60,1,5;61,1,5;62,1,5;63,1,5;64,1,5;65,1,5;66,1,5;67,1,5;68,1,5;69,1,5;70,1,5;71,1,5;72,1,5;73,1,5;74,1,5;75,1,5;76,1,5;77,1,5;78,1,5;79,1,5;80,1,5;81,1,5;82,1,5;83,1,5;84,1,5;85,1,5;86,1,5;87,1,5;88,1,5;89,1,5;90,1,5;91,1,5;92,1,5;93,1,5;94,1,5;95,1,5;96,1,5;97,1,5;98,1,5;99,1,5;100,0,55" },
	{ "stop_99", 127, 2, 99, 127, 50, 48, 700, "127,2,2;126,2,5;125,2,5;124,2,5;123,2,5;122,2,5;121,2,5;120,2,5;119,2,5;118,2,5;117,2,5;116,2,5;115,2,5;114,2,5;113,2,5;112,2,5;111,2,5;110,2,5;109,2,5;108,2,5;107,2,5;106,2,5;105,2,5;104,2,5;103,2,5;102,2,5;101,2,5;100,2,5;99,2,5;98,2,5;97,2,5;96,2,5;95,2,5;94,2,5;93,2,5;92,2,5;91,2,5;90,2,5;89,2,5;88,2,5;87,2,5;86,2,5;85,2,5;84,2,5;83,2,5;82,2,5;81,2,5;80,2,5;79,2,5;78,2,5;77,2,5;76,2,5;75,2,5;74,2,5;73,2,5;72,2,5;71,2,5;70,2,5;69,2,5;68,2,5;67,2,5;66,2,5;65,2,5;64,2,5;63,2,5;62,2,5;61,2,5;60,2,5;59,2,5;58,2,5;57,2,5;56,2,5;55,2,5;54,2,5;53,2,5;52,2,5;51,2,5;50,2,5;49,2,5;48,2,5;47,2,5;46,2,5;45,2,5;44,2,5;43,2,5;42,2,5;41,2,5;40,2,5;39,2,5;38,2,5;37,2,5;36,2,5;35,2,5;34,2,5;33,2,5;32,2,5;31,2,5;30,2,5;29,2,5;28,2,5;27,2,5;26,2,5;25,2,5;24,2,5;23,2,5;22,2,5;21,2,5;20,2,5;19,2,5;18,2,5;17,2,5;16,2,5;15,2,5;14,2,5;13,2,5;12,2,5;11,2,5;10,2,5;9,2,5;8,2,5;7,2,5;6,2,5;5,2,5;4,2,5;3,2,5;2,0,78" },
	{ "fade_in_from_0", 0, 1, 80, 90, 7, 5, 500, "0,1,2;1,1,5;2,1,5;3,1,5;4,1,5;5,1,5;6,1,5;7,1,5;8,1,5;9,1,5;10,1,5;11,1,5;12,1,5;13,1,5;14,1,5;15,1,5;16,1,5;17,1,5;18,1,5;19,1,5;20,1,5;21,1,5;22,1,5;23,1,5;24,1,5;25,1,5;26,1,5;27,1,5;28,1,5;29,1,5;30,1,5;31,1,5;32,1,5;33,1,5;34,1,5;35,1,5;36,1,5;37,1,5;38,1,5;39,1,5;40,1,5;41,1,5;42,1,5;43,1,5;44,1,5;45,1,5;46,1,5;47,1,5;48,1,5;49,1,5;50,1,5;51,1,5;52,1,5;53,1,5;54,1,5;55,1,5;56,1,5;57,1,5;58,1,5;59,1,5;60,1,5;61,1,5;62,1,5;63,1,5;64,1,5;65,1,5;66,1,5;67,1,5;68,1,5;69,1,5;70,1,5;71,1,5;72,1,5;73,1,5;74,1,5;75,1,5;76,1,5;77,1,5;78,1,5;79,1,5;80,1,5;81,1,5;82,1,5;83,1,5;84,1,5;85,1,5;86,1,5;87,1,5;88,1,5;89,1,5;90,0,53" },
	{ "duck_at_target", 20, 2, 80, 100, 1000, 0, 20, "20,0,20" },
	{ "duck_from_0", 0, 2, 80, 100, 1000, 0, 20, "0,0,20" },
	{ "up_at_target", 100, 1, 80, 100, 1000, 0, 20, "101,0,20" },
	{ "puzzle_50", 64, 2, 50, 64, 10, 3, 300, "63,2,5;62,2,5;61,2,5;60,2,5;59,2,5;58,2,5;57,2,5;56,2,5;55,2,5;54,2,5;53,2,5;52,2,5;51,2,5;50,2,5;49,2,5;48,2,5;47,2,5;46,2,5;45,2,5;44,2,5;43,2,5;42,2,5;41,2,5;40,2,5;39,2,5;38,2,5;37,2,5;36,2,5;35,2,5;34,2,5;33,2,5;32,0,145" },
};

struct SfxFadeCase {
	const char *name;
	int cur, mode, percent, vol;
	uint32 ticks, last;
	int sfxVol;
	uint32 sfxLast;
	int n;
	const char *runs;
};

// tools/emu/audio_music_fade.py (SFX_SCENARIOS, 0x00495eb0 = 2)
static const SfxFadeCase kSfxFadeCases[] = {
	{ "sfx_from_127", 0, 0, 80, 100, 1000, 0, 127, 1000, 330, "0,0,127,2,4;0,0,125,2,5;0,0,123,2,5;0,0,121,2,5;0,0,119,2,5;0,0,117,2,5;0,0,115,2,5;0,0,113,2,5;0,0,111,2,5;0,0,109,2,5;0,0,107,2,5;0,0,105,2,5;0,0,103,2,5;0,0,101,2,5;0,0,99,2,5;0,0,97,2,5;0,0,95,2,5;0,0,93,2,5;0,0,91,2,5;0,0,89,2,5;0,0,87,2,5;0,0,85,2,5;0,0,83,2,5;0,0,81,2,5;0,0,79,2,5;0,0,77,2,5;0,0,75,2,5;0,0,73,2,5;0,0,71,2,5;0,0,69,2,5;0,0,67,2,5;0,0,65,2,5;0,0,63,2,5;0,0,61,2,5;0,0,59,2,5;0,0,57,2,5;0,0,55,2,5;0,0,53,2,5;0,0,51,2,5;0,0,49,2,5;0,0,47,2,5;0,0,45,2,5;0,0,43,2,5;0,0,41,2,5;0,0,39,2,5;0,0,37,2,5;0,0,35,2,5;0,0,33,2,5;0,0,31,2,5;0,0,29,2,5;0,0,27,2,5;0,0,25,2,5;0,0,23,2,5;0,0,21,2,5;0,0,19,2,5;0,0,17,2,5;0,0,15,2,5;0,0,13,2,5;0,0,11,2,5;0,0,9,2,5;0,0,7,2,5;0,0,5,2,5;0,0,3,2,5;0,0,1,2,5;0,0,0,0,11" },
	{ "sfx_odd_3", 0, 0, 80, 100, 1000, 0, 3, 998, 20, "0,0,3,2,2;0,0,1,2,5;0,0,0,0,13" },
	{ "sfx_duck_at_target", 20, 2, 80, 100, 1000, 0, 10, 996, 30, "20,0,10,2,1;20,0,8,2,5;20,0,6,2,5;20,0,4,2,5;20,0,2,2,5;20,0,0,0,9" },
	{ "sfx_with_duck", 100, 2, 80, 100, 1000, 0, 50, 1000, 200, "99,2,50,2,4;99,2,48,2,1;98,2,48,2,4;98,2,46,2,1;97,2,46,2,4;97,2,44,2,1;96,2,44,2,4;96,2,42,2,1;95,2,42,2,4;95,2,40,2,1;94,2,40,2,4;94,2,38,2,1;93,2,38,2,4;93,2,36,2,1;92,2,36,2,4;92,2,34,2,1;91,2,34,2,4;91,2,32,2,1;90,2,32,2,4;90,2,30,2,1;89,2,30,2,4;89,2,28,2,1;88,2,28,2,4;88,2,26,2,1;87,2,26,2,4;87,2,24,2,1;86,2,24,2,4;86,2,22,2,1;85,2,22,2,4;85,2,20,2,1;84,2,20,2,4;84,2,18,2,1;83,2,18,2,4;83,2,16,2,1;82,2,16,2,4;82,2,14,2,1;81,2,14,2,4;81,2,12,2,1;80,2,12,2,4;80,2,10,2,1;79,2,10,2,4;79,2,8,2,1;78,2,8,2,4;78,2,6,2,1;77,2,6,2,4;77,2,4,2,1;76,2,4,2,4;76,2,2,2,1;75,2,2,2,4;75,2,0,0,1;74,2,0,0,5;73,2,0,0,5;72,2,0,0,5;71,2,0,0,5;70,2,0,0,5;69,2,0,0,5;68,2,0,0,5;67,2,0,0,5;66,2,0,0,5;65,2,0,0,5;64,2,0,0,5;63,2,0,0,5;62,2,0,0,5;61,2,0,0,5;60,2,0,0,5" },
};

} // End of namespace AudioMusicFade

class AudioMusicFadeTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	void test_fades_match_original() {
		using namespace AudioMusicFade;
		for (uint c = 0; c < ARRAYSIZE(kFadeCases); c++) {
			const FadeCase &fc = kFadeCases[c];
			CryOmni3D::Atlantis::AtlantisMusicFade f;
			f.cur = (uint8)fc.cur;
			f.mode = (uint8)fc.mode;
			f.downTarget = -1;
			f.upTarget = fc.vol;
			f.percent = (uint8)fc.percent;
			f.vol = (uint8)fc.vol;
			f.ticks = fc.ticks;
			f.lastStep = fc.last;
			Common::String runs;
			int lastCur = -1, lastMode = -1, count = 0;
			for (int i = 0; i < fc.n; i++) {
				f.tick();
				if (f.cur == lastCur && f.mode == lastMode) {
					count++;
					continue;
				}
				if (count)
					runs += Common::String::format("%s%d,%d,%d", runs.empty() ? "" : ";", lastCur, lastMode, count);
				lastCur = f.cur;
				lastMode = f.mode;
				count = 1;
			}
			runs += Common::String::format("%s%d,%d,%d", runs.empty() ? "" : ";", lastCur, lastMode, count);
			TSM_ASSERT_EQUALS(fc.name, runs, Common::String(fc.runs));
		}
	}

	// The NewSound fade-out, alone and with a music fade-down; the
	// "duck_at_target" case pins FUN_0042f7a8's early return
	// (@ 0x0042f832..0x0042f843): no NewSound step on that tick.
	void test_sfx_fades_match_original() {
		using namespace AudioMusicFade;
		for (uint c = 0; c < ARRAYSIZE(kSfxFadeCases); c++) {
			const SfxFadeCase &fc = kSfxFadeCases[c];
			CryOmni3D::Atlantis::AtlantisMusicFade f;
			f.cur = (uint8)fc.cur;
			f.mode = (uint8)fc.mode;
			f.downTarget = -1;
			f.upTarget = fc.vol;
			f.percent = (uint8)fc.percent;
			f.vol = (uint8)fc.vol;
			f.ticks = fc.ticks;
			f.lastStep = fc.last;
			f.sfxMode = 2;
			f.sfxVol = fc.sfxVol;
			f.sfxLast = fc.sfxLast;
			Common::String runs, last;
			int count = 0;
			for (int i = 0; i < fc.n; i++) {
				f.tick();
				const Common::String s = Common::String::format("%d,%d,%d,%d", f.cur, f.mode, (int)f.sfxVol, f.sfxMode);
				if (count && s == last) {
					count++;
					continue;
				}
				if (count)
					runs += Common::String::format("%s%s,%d", runs.empty() ? "" : ";", last.c_str(), count);
				last = s;
				count = 1;
			}
			runs += Common::String::format("%s%s,%d", runs.empty() ? "" : ";", last.c_str(), count);
			TSM_ASSERT_EQUALS(fc.name, runs, Common::String(fc.runs));
		}
	}
};

#endif
