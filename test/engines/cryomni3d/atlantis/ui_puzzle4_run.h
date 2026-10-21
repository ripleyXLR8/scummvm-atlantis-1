#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/fs.h"
#include "common/ptr.h"
#include "common/stream.h"

#include "cryomni3d/atlantis/bigfile.h"
#include "cryomni3d/atlantis/rat_game.h"

// ui audit — puzzle 4 (the rat's run), whole runs.
//
// Expected values come from the ORIGINAL FUN_00438bf0 of
// atlantis-cd-en-714240 run unmodified under Unicorn with four scripted
// players (lose_all, abort, win, random_play):
// atlantis-re/tools/emu/ui_puzzle4_run.py -> evidence/golden/ui_puzzle4_run.txt.
// Per frame: the input, the sounds, the md5 of the draws and of the state.
//
// Port code exercised directly: RatGame (rat_game.cpp) over RatRules.
//
// The compared state is 0x004969c0..0x00496cd7, the result, 0x00772d90 /
// 0x00772d94 and the 0x68 animation entry 0x00496eea..0x00496eef.  The
// scenario abort_then_lose_all plays twice on one image ("play 2" in the
// golden): the second play starts from the state the first left, as
// RatPuzzle's g_ratMemory carries RatRules::mem and clockTicked (Fable pass 2
// rat#0 / rat#7).
//
// Not covered: the harness writes the click and held flags together at the
// frame boundary, so RatPuzzle::readInput's one-frame lag of the held flag
// is not tested.

namespace UiRatRun {

using CryOmni3D::Atlantis::RatGame;
using CryOmni3D::Atlantis::RatRules;

class Replay : public RatGame {
public:
	const Common::Array<Common::String> *lines;
	uint next;          // the next frame line
	uint end;           // one past the scenario's last line
	int mismatches;
	Common::String firstMismatch;
	uint32 _clock;
	Common::Array<int> rands;
	uint randPos;
	Common::String draws, sounds;
	bool open;
	uint frames;

	Replay() : lines(nullptr), next(0), end(0), mismatches(0), _clock(1000), randPos(0), open(false), frames(0) {}

	void closeFrame() {
		if (!open)
			return;
		open = false;
		frames++;
		const Common::String &l = (*lines)[next - 1];
		const uint b1 = l.find('|'), b2 = l.find('|', b1 + 1), b3 = l.find('|', b2 + 1);
		const Common::String wantSnd = l.substr(b1 + 2, b2 - b1 - 3);
		const Common::String wantDraws = l.substr(b2 + 2, b3 - b2 - 3);
		const Common::String wantState = l.substr(b3 + 2);
		byte st[RatRules::kSize + 12 + 6];
		memcpy(st, rules.mem, RatRules::kSize);
		const uint32 extra[3] = { (uint32)rules.result, rules.clockTicked, (uint32)wheelY };
		for (int k = 0; k < 3; k++)
			for (int j = 0; j < 4; j++)
				st[RatRules::kSize + 4 * k + j] = (byte)(extra[k] >> (8 * j));
		// the 0x68 animation entry 0x00496eea..0x00496eef
		memcpy(st + RatRules::kSize + 12, rules.mem + (0x00496eea - RatRules::kBase), 6);
		const Common::String gotDraws = AtlantisTest::md5Hex((const byte *)draws.c_str(), draws.size());
		const Common::String gotState = AtlantisTest::md5Hex(st, sizeof(st));
		if (gotDraws != wantDraws || gotState != wantState || sounds != wantSnd) {
			if (mismatches == 0)
				firstMismatch = Common::String::format("frame line '%s': sounds '%s' draws %s state %s",
				                                       l.c_str(), sounds.c_str(),
				                                       gotDraws == wantDraws ? "ok" : "differ",
				                                       gotState == wantState ? "ok" : "differ");
			mismatches++;
		}
	}

protected:
	void readInput(int &mx, int &my, bool &click, bool &held) override {
		closeFrame();
		draws.clear();
		sounds.clear();
		if (next >= end || mismatches) {
			// Past the recorded frames: stop the run.
			rules.result = 99;
			mx = my = 0;
			click = held = false;
			return;
		}
		const Common::String &l = (*lines)[next++];
		open = true;
		Common::Array<Common::String> tok;
		Common::String cur;
		for (uint k = 0; k < l.find('|'); k++) {
			if (l[k] == ' ') {
				if (!cur.empty())
					tok.push_back(cur);
				cur.clear();
			} else {
				cur += l[k];
			}
		}
		mx = atoi(tok[1].c_str());
		my = atoi(tok[2].c_str());
		click = tok[3] == "1";
		held = tok[4] == "1";
		rands.clear();
		randPos = 0;
		if (tok.size() > 5 && tok[5].hasPrefix("rand=")) {
			const char *p = tok[5].c_str() + 5;
			while (*p) {
				rands.push_back(atoi(p));
				while (*p && *p != ',')
					p++;
				if (*p == ',')
					p++;
			}
		}
	}
	void add(const Common::String &s) {
		if (!draws.empty())
			draws += "\n";
		draws += s;
	}
	void drawBackground(int) override { add("bg"); }
	void drawSprite(char bank, int x, int y, int frame) override {
		add(Common::String::format("%c %d %d %d", bank, x, y, frame));
	}
	void present() override { add("present"); }
	void playSound(int slot) override {
		sounds += (sounds.empty() ? "" : " ") + Common::String::format("%d", slot);
	}
	int randomValue() override { return randPos < rands.size() ? rands[randPos++] : 0; }
	uint32 ticks() override { return _clock; }
	void waitUntil(uint32 t) override { _clock = t; }
};

inline bool loadTrajectories(RatGame &g) {
	SearchMan.addDirectory("ui_rat_data", Common::FSNode(Common::Path(AtlantisTest::dataDir(), '/')));
	CryOmni3D::Atlantis::BigFileArchive big;
	for (int cd = 1; cd <= 4; cd++)
		big.open(Common::Path(Common::String::format("BIGCD%d.BIG", cd)));
	SearchMan.remove("ui_rat_data");
	for (int k = 0; k < RatGame::kNumSlots; k++) {
		Common::String name = Common::String("PUZZLES\\") + RatGame::kSlotFiles[k];
		name.toUppercase();
		Common::ScopedPtr<Common::SeekableReadStream> s(big.createReadStreamForMember(name));
		if (!s)
			return false;
		Common::Array<byte> d((uint)s->size());
		s->read(d.data(), d.size());
		if (!g.loadSlot(k, d.data(), d.size()))
			return false;
	}
	return true;
}

} // End of namespace UiRatRun

class UiPuzzle4RunTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	void test_runs_vs_original() {
		using namespace UiRatRun;
		if (AtlantisTest::goldenDir().empty() || AtlantisTest::dataDir().empty()) {
			TS_WARN("ui_puzzle4_run: no golden or data directory configured, skipped");
			return;
		}
		Common::Array<Common::String> golden;
		TS_ASSERT(AtlantisTest::loadGoldenLines("ui_puzzle4_run.txt", golden));
		int scenarios = 0;
		for (uint i = 0; i < golden.size(); i++) {
			if (!golden[i].hasPrefix("scenario "))
				continue;
			// "scenario NAME frames N result R"
			Common::Array<Common::String> tok;
			Common::String cur;
			for (uint k = 0; k <= golden[i].size(); k++) {
				if (k == golden[i].size() || golden[i][k] == ' ') {
					tok.push_back(cur);
					cur.clear();
				} else {
					cur += golden[i][k];
				}
			}
			const uint frames = (uint)atoi(tok[3].c_str());
			const int want = atoi(tok[5].c_str());
			Replay g;
			TS_ASSERT(loadTrajectories(g));
			g.lines = &golden;
			g.next = i + 1;
			g.end = i + 1 + frames;
			const int got = g.run();
			g.closeFrame();
			TS_ASSERT_EQUALS(g.firstMismatch, Common::String());
			TS_ASSERT_EQUALS(g.mismatches, 0);
			TS_ASSERT_EQUALS(g.frames, frames);
			TS_ASSERT_EQUALS(got, want);
			scenarios++;
			// "play 2 frames N result R": a second play on the same image,
			// from the state the first left (RatPuzzle::play's g_ratMemory:
			// RatRules::mem and clockTicked); the clock goes on.
			const uint p2 = i + 1 + frames;
			if (p2 < golden.size() && golden[p2].hasPrefix("play 2 ")) {
				Common::Array<Common::String> t2;
				Common::String c2;
				for (uint k = 0; k <= golden[p2].size(); k++) {
					if (k == golden[p2].size() || golden[p2][k] == ' ') {
						t2.push_back(c2);
						c2.clear();
					} else {
						c2 += golden[p2][k];
					}
				}
				const uint frames2 = (uint)atoi(t2[3].c_str());
				const int want2 = atoi(t2[5].c_str());
				Replay g2;
				TS_ASSERT(loadTrajectories(g2));
				memcpy(g2.rules.mem, g.rules.mem, sizeof(g2.rules.mem));
				g2.rules.clockTicked = g.rules.clockTicked;
				g2._clock = g._clock;
				g2.lines = &golden;
				g2.next = p2 + 1;
				g2.end = p2 + 1 + frames2;
				const int got2 = g2.run();
				g2.closeFrame();
				TS_ASSERT_EQUALS(g2.firstMismatch, Common::String());
				TS_ASSERT_EQUALS(g2.mismatches, 0);
				TS_ASSERT_EQUALS(g2.frames, frames2);
				TS_ASSERT_EQUALS(got2, want2);
				scenarios++;
			}
		}
		TS_ASSERT_EQUALS(scenarios, 6);
	}
};
