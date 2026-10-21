// Verifier "video": what ends a video (transition, Cinema=, start-up).
//
// Original: atlantis-cd-en-714240 FUN_0041bc74 with the real input poll
// FUN_0041a298 and key poll FUN_0042e624, run unmodified under Unicorn by
// atlantis-re/tools/emu/video_skip_input.py (golden video_skip_input.txt,
// experiment 2026-10-16-06): 7 inputs x 128 flag states, a 10-frame video.
//
// Port: VideoSkipRun (video_skip.h), the frame control playTransitionVideo
// runs: framePresented() after each presented frame, poll() after each frame
// but the last, finish() after the loop.

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "cryomni3d/atlantis/bar_session.h"
#include "cryomni3d/atlantis/menu_layout.h"
#include "cryomni3d/atlantis/video_skip.h"

class VideoSkipInputTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	void test_skip_input_vs_original() {
		using namespace CryOmni3D::Atlantis;
		if (!AtlantisTest::haveData()) {
			TS_WARN("video skip: no golden configured, skipped");
			return;
		}
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_skip_input.txt", lines));
		const uint kFrames = 10;
		uint rows = 0, bad = 0;
		for (uint li = 0; li < lines.size(); li++) {
			const Common::String &l = lines[li];
			if (!l.hasPrefix("V "))
				continue;
			char input[32];
			int bow, bar, objet, head, section, cinema, menu;
			int frames, ended, keep, menuAfter, barAfter, fade, click, skipflag;
			if (sscanf(l.c_str(), "V %31s bow %d bar %d objet %d head %d section %d cinema %d menu %d: "
			           "frames %d ended %d keep %d menu %d bar %d fade %d click %d skipflag %d",
			           input, &bow, &bar, &objet, &head, &section, &cinema, &menu,
			           &frames, &ended, &keep, &menuAfter, &barAfter, &fade, &click, &skipflag) != 16) {
				TS_FAIL(l.c_str());
				continue;
			}
			rows++;
			const Common::String in(input);
			VideoSkipRun run;
			run.s.bowMode = bow;
			run.s.barRequest = bar;
			run.s.objectInHand = objet;
			run.s.talkingHead = head;
			run.s.sectionPlay = section;
			run.s.menuRequest = menu;
			BarSession barSession;
			barSession.request = bar;
			// The presses come at polls 2 (and 4 for the second key of
			// rbutton+escape); poll k follows frame k - 1.
			for (uint f = 0; f < kFrames; f++) {
				if (run.framePresented(f + 1 == kFrames))
					break;
				const uint poll = f + 1;
				const bool left = in == "lbutton" && poll == 2;
				const bool right = (in == "rbutton" || in == "rbutton+escape") && poll == 2;
				const bool esc = (in == "escape" && poll == 2) || (in == "rbutton+escape" && poll == 4);
				run.poll(left, right, esc);
			}
			const int gotFade = run.finish(cinema != 0, barSession) ? 2 : 0;
			if ((int)run.presented != frames || (int)run.stopAfterFrame != ended || gotFade != fade ||
			        (int)run.s.keepRunning != keep || (int)run.s.menuRequest != menuAfter ||
			        (int)barSession.request != barAfter || (int)run.s.click != click ||
			        (int)run.s.skipFlag != skipflag) {
				bad++;
				if (bad < 10)
					warning("video skip: %s: port frames %u ended %d fade %d", l.c_str(), run.presented,
					        (int)run.stopAfterFrame, gotFade);
			}
		}
		TS_ASSERT_EQUALS(rows, 7u * 128u);
		TS_ASSERT_EQUALS(bad, 0u);
	}

	// Escape in the warp loop and the in-game menu FUN_0041c838 (golden
	// ui_ingame_menu.txt, atlantis-re tools/emu/ui_ingame_menu.py,
	// experiment 2026-10-16-07):
	//  - O: the real FUN_0041a298 polled with Escape (and a right press in
	//    the same poll), then the warp loop's test 0x0041c5f7, over 32 flag
	//    states: whether the menu opens and the flags after the poll;
	//  - G: the record FUN_004210c0(0) gives a click at every pointer
	//    position, against the port's menuWalkHit on MAINMENU.TXT.
	void test_ingame_menu_vs_original() {
		using namespace CryOmni3D::Atlantis;
		if (!AtlantisTest::haveData()) {
			TS_WARN("in-game menu: no golden configured, skipped");
			return;
		}
		Common::Array<Common::String> lines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("ui_ingame_menu.txt", lines));
		// MAINMENU.TXT as shipped (records 0..2: LOAD GAME, OPTIONS MENU,
		// QUIT GAME), walked by the port.
		static const char kMainMenu[] = "<>,230>LOAD GAME\r\n<>,270>OPTIONS MENU\r\n<>,310>QUIT GAME\r\n/fin\r\n";
		MenuWalk walk;
		walkMenuText((const byte *)kMainMenu, sizeof(kMainMenu) - 1, 0, -1, 0, 0, walk);
		uint oRows = 0, gRows = 0, bad = 0;
		for (uint li = 0; li < lines.size(); li++) {
			const Common::String &l = lines[li];
			if (l.hasPrefix("O ")) {
				char input[32];
				int bow, bar, objet, head, section, menu, request, keep, click, barAfter;
				if (sscanf(l.c_str(), "O %31s bow %d bar %d objet %d head %d section %d: menu %d request %d keep %d click %d bar %d",
				           input, &bow, &bar, &objet, &head, &section, &menu, &request, &keep, &click, &barAfter) != 11) {
					TS_FAIL(l.c_str());
					continue;
				}
				oRows++;
				VideoSkipState s;
				s.bowMode = bow;
				s.barRequest = bar;
				s.objectInHand = objet;
				s.talkingHead = head;
				s.sectionPlay = section;
				videoSkipPoll(s, false, !strcmp(input, "escape+rbutton"), true);
				if ((int)inGameMenuOpens(s) != menu || (int)s.menuRequest != request ||
				        (int)s.keepRunning != keep || (int)s.click != click || (int)s.barRequest != barAfter) {
					bad++;
					warning("in-game menu: %s", l.c_str());
				}
			} else if (l.hasPrefix("G ")) {
				int y = 0;
				if (sscanf(l.c_str(), "G %d", &y) != 1)
					continue;
				gRows++;
				// "<x0>-<x1>:<record>" runs
				for (const char *p = strchr(l.c_str() + 2, ' '); p && *p;) {
					int x0, x1, rec;
					if (sscanf(p, " %d-%d:%d", &x0, &x1, &rec) != 3)
						break;
					for (int x = x0; x <= x1; x++) {
						const int got = menuWalkHit(walk.records, x, y);
						if ((got < 0 ? 255 : got) != rec) {
							bad++;
							break;
						}
					}
					p = strchr(p + 1, ' ');
				}
			}
		}
		TS_ASSERT_EQUALS(oRows, 64u);
		TS_ASSERT_EQUALS(gRows, 480u);
		TS_ASSERT_EQUALS(bad, 0u);
	}
};
