#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_DIALOG_OFF1_ZH_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_DIALOG_OFF1_ZH_H

// video audit: the off1 dialog lines of the Traditional and Simplified
// Chinese CDs (atlantis-re decision 0025 F), the only releases besides
// GOG.com's whose scripts have off1 headers (29: CHAPI021, 031, 050, 058,
// 062, 063).
//
// Expected values come from the ORIGINAL code of atlantis-cd-zh-tw-696832
// and atlantis-cd-zh-cn-692736 run unmodified under Unicorn on the discs'
// own files (atlantis-re tools/emu/video_dialog_off1_zh.py ->
// evidence/golden/video_dialog_off1_zh.txt): the off1 player (zh-tw
// FUN_0042fe7c, zh-cn FUN_0042fe58) whole, on CHAPI031 /377 A and on every
// off1 line with its own voice, and the executor's header tests (FUN_00427cc0
// @ 0x00427f0f..0x00427f97) on every `[` line of the scripts.
//
// Port code exercised: atlantisExeBuild (exe_build.h, off1FlatCopy),
// DialogView::loadLineVoice / showScreenBufferFlat (dialog_view.h), which
// playSingleConLine runs on an off1 line, ConScript::parse (con_script.cpp)
// and CryOmni3DEngine_Atlantis::castConLine (dialog.cpp, on the zero-filled
// engine of video_dialog.h), which give playConSection the off / off0 /
// off1 flags.  Real files: `zhtw`, `zhcn`, `golden` in atlantis_test.cfg;
// skipped when absent.

#include "atlantis_test_helper.h"
#include "ui_zh.h"
#include "video_dialog.h"
#include "video_test_big.h"

#include "common/algorithm.h"
#include "common/memstream.h"

#include "cryomni3d/atlantis/con_script.h"

class VideoDialogOff1ZhTestSuite : public CxxTest::TestSuite {
	static void pattern(Common::Array<byte> &out) {
		// The harness's screen buffer: byte i = (i * 7 + (i >> 11)) & 0xff.
		out.resize(640 * 480 * 2);
		for (uint i = 0; i < out.size(); i++)
			out[i] = (byte)((i * 7 + (i >> 11)) & 0xff);
	}

	struct Release {
		const char *key;      // atlantis_test.cfg
		const char *golden;   // the harness's prefix
		uint32 copyVA;        // the copy's `push 0x500` in the off1 player
	};

	static const Release *releases() {
		static const Release r[2] = {
			{ "zhtw", "tw", 0x00430074 },
			{ "zhcn", "cn", 0x00430050 },
		};
		return r;
	}

	// The build of `dir`'s atlantis.exe, or null.
	static const CryOmni3D::Atlantis::AtlantisExeBuild *buildOf(const Common::String &dir,
			Common::Array<byte> &exe) {
		if (!UiZh::readExe(dir, exe))
			return nullptr;
		Common::MemoryReadStream s(exe.data(), exe.size());
		return &CryOmni3D::Atlantis::atlantisExeBuild(exe.size(), CryOmni3D::Atlantis::readPeTimestamp(s));
	}

	static bool member(const VideoTest::Big &big, const Common::String &name, Common::Array<byte> &out) {
		uint32 size = 0;
		byte *b = big.get(name, size);
		if (!b)
			return false;
		out.resize(size);
		memcpy(out.data(), b, size);
		delete[] b;
		return true;
	}

public:
	void setUp() { Common::install_null_g_system(); }
	void tearDown() { Common::uninstall_null_g_system(); }

	// The field against the code: the Chinese builds' off1 player makes one
	// copy, FUN_00410f36(0x004860d4, FUN_0041a24c(), 0x1e0, 0x500), with the
	// 1997 FUN_00410f36 that copies rows x pitch contiguous bytes (`mul ebx;
	// shr eax,4` @ 0x00410f46); every other build keeps the reference's
	// two-copy player.
	void test_off1_flat_copy_on_the_bytes() {
		using namespace CryOmni3D::Atlantis;
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			const bool zh = !strcmp(b.name, "atlantis-cd-zh-tw-696832") || !strcmp(b.name, "atlantis-cd-zh-cn-692736");
			TSM_ASSERT(b.name, b.off1FlatCopy == zh);
		}
		uint checked = 0;
		for (uint r = 0; r < 2; r++) {
			const Release &rel = releases()[r];
			const Common::String dir = AtlantisTest::configValue(rel.key);
			Common::Array<byte> e;
			const AtlantisExeBuild *b = dir.empty() ? nullptr : buildOf(dir, e);
			if (!b) {
				TS_WARN(Common::String::format("%s not configured; skipped", rel.key).c_str());
				continue;
			}
			TS_ASSERT(b->off1FlatCopy);
			// push 0x500; push 0x1e0; call FUN_0041a24c; push eax; push [0x004860d4]; call FUN_00410f36
			TS_ASSERT(UiZh::codeAt(e, rel.copyVA, "68 00 05 00 00 68 e0 01 00 00 e8"));
			TS_ASSERT(UiZh::codeAt(e, rel.copyVA + 0xf, "50 ff 35 d4 60 48 00 e8"));
			TS_ASSERT(UiZh::codeAt(e, 0x00410f43, "8b 45 14 f7 e3 c1 e8 04"));
			checked++;
		}
		if (!checked)
			TS_WARN("zhtw / zhcn not configured; byte checks skipped");
	}

	// CHAPI031 /377 A ("[Commander,off1]", WAV\ATA0377A.APC from the disc's
	// archives) through DialogView::loadLineVoice with the build's copy:
	// without the file, or with a header that is not "CRYO_APC" "1.20"
	// (FUN_00417cc4, run in the original's code), nothing is drawn and the
	// player returns 0; with it, the screen buffer is on the surface before
	// the voice starts.  At a surface pitch of 0x500 (the port's) and 0x600.
	void test_off1_zh_voice_and_screen_vs_original() {
		AtlantisTest::Golden golden;
		if (!AtlantisTest::loadGolden("video_dialog_off1_zh.txt", golden)) {
			TS_WARN("video_dialog_off1_zh.txt not found; skipped");
			return;
		}
		Common::Array<byte> buf;
		pattern(buf);
		TS_ASSERT_EQUALS(AtlantisTest::md5Hex(buf.begin(), buf.size()), golden["pattern screenbuffer"]);
		const char *const kinds[] = { "ok", "absent", "badmagic", "badversion" };
		const uint pitches[] = { 0x500, 0x600 };
		const char *const subs[] = { "a1c0", "a0c2", "a0c1" };
		uint checked = 0;
		for (uint r = 0; r < 2; r++) {
			const Release &rel = releases()[r];
			const Common::String dir = AtlantisTest::configValue(rel.key);
			Common::Array<byte> e, apc;
			const CryOmni3D::Atlantis::AtlantisExeBuild *b = dir.empty() ? nullptr : buildOf(dir, e);
			if (!b) {
				TS_WARN(Common::String::format("%s not configured; skipped", rel.key).c_str());
				continue;
			}
			VideoTest::Big big(dir);
			TS_ASSERT(member(big, "WAV\\ATA0377A.APC", apc));
			TS_ASSERT_EQUALS(AtlantisTest::md5Hex(apc.begin(), apc.size()),
			                 Common::String("2f249c23e67ba7eca2fb24d997471fa0"));   // the harness's file
			for (uint k = 0; k < ARRAYSIZE(kinds); k++) {
				for (uint p = 0; p < ARRAYSIZE(pitches); p++) {
					Common::Array<byte> data = apc;
					if (!strcmp(kinds[k], "badmagic"))
						data[7] = 'X';
					else if (!strcmp(kinds[k], "badversion"))
						data[11] = '1';
					Common::MemoryReadStream file(data.begin(), data.size());
					Common::Array<byte> surface;
					surface.resize(pitches[p] * 480);
					memset(surface.begin(), 0x55, surface.size());
					TS_ASSERT_EQUALS(AtlantisTest::md5Hex(surface.begin(), surface.size()),
					                 golden[Common::String::format("pattern surface%u", pitches[p])]);
					Audio::AudioStream *stream = CryOmni3D::Atlantis::DialogView::loadLineVoice(
					        strcmp(kinds[k], "absent") ? &file : nullptr, buf.begin(), surface.begin(),
					        pitches[p], b->off1FlatCopy);
					const Common::String portRet = stream ? "1" : "0";
					delete stream;
					const Common::String port = AtlantisTest::md5Hex(surface.begin(), surface.size());
					// The subtitle states change only whether the drawer runs
					// (stubbed in the harness): the same screen in each.
					for (uint s = 0; s < ARRAYSIZE(subs); s++) {
						const Common::String key = Common::String::format("%s %s pitch%u %s ",
						        rel.golden, kinds[k], pitches[p], subs[s]);
						TS_ASSERT_EQUALS(portRet, golden[key + "ret"]);
						TS_ASSERT_EQUALS(port, golden[key + "surface"]);
						if (golden[key + "present"] != "-")
							TS_ASSERT_EQUALS(port, golden[key + "present"]);
						TS_ASSERT_EQUALS(AtlantisTest::md5Hex(buf.begin(), buf.size()), golden[key + "screenbuffer"]);
						// one copy, straight to the surface, before the voice
						if (portRet == "1")
							TS_ASSERT(golden[key + "events"].contains(
							    "lock(flags=0x21),copy(screenbuffer->surface,rows=480,pitch=0x500),unlock,"
							    "set_sample_address,start_sample"));
						checked++;
					}
				}
			}
		}
		if (checked)
			TS_ASSERT_EQUALS(checked % 24, 0u);
	}

	// Every `[` line of the scripts, parsed by ConScript and cast by
	// castConLine as playConSection does, gets the original's off / off0 /
	// off1 flag (the 29 off1 lines and the 247 off and 32 off0 ones, by CON
	// file, section, letter and header); each off1 line's voice passes the gate and
	// puts the screen buffer on the screen (pitch 0x500), as the original's
	// player does with that voice.
	void test_off1_zh_lines_reach_the_player() {
		using namespace CryOmni3D::Atlantis;
		AtlantisTest::Golden golden;
		if (!AtlantisTest::loadGolden("video_dialog_off1_zh.txt", golden)) {
			TS_WARN("video_dialog_off1_zh.txt not found; skipped");
			return;
		}
		Common::Array<Common::String> goldenLines;
		TS_ASSERT(AtlantisTest::loadGoldenLines("video_dialog_off1_zh.txt", goldenLines));
		Common::Array<byte> buf;
		pattern(buf);
		for (uint r = 0; r < 2; r++) {
			const Release &rel = releases()[r];
			const Common::String dir = AtlantisTest::configValue(rel.key);
			Common::Array<byte> exe;
			const AtlantisExeBuild *b = dir.empty() ? nullptr : buildOf(dir, exe);
			if (!b) {
				TS_WARN(Common::String::format("%s not configured; skipped", rel.key).c_str());
				continue;
			}
			VideoPeek::Eng *e = VideoPeek::rawEngine();
			e->_exeBuild = b;
			new (&e->_gameVars) Common::HashMap<Common::String, int>();
			new (&e->_dialInitData) Common::HashMap<int, VideoPeek::Eng::DialInitEntry>();

			VideoTest::Big big(dir);
			Common::Array<Common::String> cons = big.names(".CON");
			Common::sort(cons.begin(), cons.end());
			// "<CON> <section><letter> <header>" -> flag, as the port casts the lines
			Common::HashMap<Common::String, Common::String> port;
			Common::HashMap<Common::String, bool> seen;
			uint lines = 0;
			for (uint c = 0; c < cons.size(); c++) {
				Common::Array<byte> text;
				if (!member(big, cons[c], text))
					continue;
				ConScript script;
				script.parse((const char *)text.data(), text.size());
				const Common::String con = cons[c].substr(cons[c].findLastOf('\\') + 1);
				for (uint s = 0; s < script.sections().size(); s++) {
					const ConSection &sec = script.sections()[s];
					uint lineIdx = 0;
					for (uint i = 0; i < sec.items.size(); i++) {
						if (sec.items[i].isSet)
							continue;
						e->_gameVars["clicperso"] = 0;
						const VideoPeek::Eng::ConLineCast cast = e->castConLine(sec.items[i].line);
						e->_lastDialCam = cast.camNum;
						// Entries of a file can share a section number (CHAPI055
						// has five /713): the header tells their lines apart.
						const Common::String &raw = sec.items[i].line.raw;
						const size_t close = raw.findFirstOf(']');
						const Common::String hdr = raw.substr(0, close == Common::String::npos ? 0 : close + 1);
						const Common::String key = Common::String::format("%s %d%c %s", con.c_str(), sec.id,
						        conTrackLetter(lineIdx++), hdr.c_str());
						if (seen.contains(key))
							continue;
						seen[key] = true;
						lines++;
						if (cast.off1)
							port[key] = "off1";
						else if (cast.off0)
							port[key] = "off0";
						else if (cast.offFlag)
							port[key] = "off";
					}
				}
			}
			// The original's flagged lines, in order: "<rel> header <n> <CON>
			// <section><letter> <header> <flag>".
			uint count[3] = { 0, 0, 0 };
			const char *const tags[3] = { "off", "off0", "off1" };
			const Common::String prefix = Common::String(rel.golden) + " header ";
			uint n = 0, found = 0, played = 0;
			for (uint gl = 0; gl < goldenLines.size(); gl++) {
				if (!goldenLines[gl].hasPrefix(prefix.c_str()))
					continue;
				const Common::String g = goldenLines[gl].substr(prefix.size());
				TS_ASSERT_EQUALS((uint)atoi(g.c_str()), n);
				n++;
				const size_t sp = g.findLastOf(' ');
				const Common::String where = g.substr(g.findFirstOf(' ') + 1, sp - g.findFirstOf(' ') - 1), tag = g.substr(sp + 1);
				for (uint t = 0; t < 3; t++)
					count[t] += tag == tags[t];
				if (!port.contains(where) || port[where] != tag) {
					TS_FAIL(Common::String::format("%s %s: original %s, port %s", rel.key, where.c_str(), tag.c_str(),
					        port.contains(where) ? port[where].c_str() : "none").c_str());
					continue;
				}
				found++;
				if (tag != "off1")
					continue;
				// The line's voice through the port's voice step.
				const size_t sp1 = where.findFirstOf(' ');
				const Common::String sl = where.substr(sp1 + 1, where.findFirstOf(' ', sp1 + 1) - sp1 - 1);
				const int section = atoi(sl.c_str());
				const char letter = sl.lastChar();
				Common::Array<byte> apc;
				TS_ASSERT(member(big, Common::String::format("WAV\\ATA%04d%c.APC", section, letter), apc));
				Common::MemoryReadStream file(apc.begin(), apc.size());
				Common::Array<byte> surface;
				surface.resize(0x500 * 480);
				memset(surface.begin(), 0x55, surface.size());
				Audio::AudioStream *stream = DialogView::loadLineVoice(apc.empty() ? nullptr : &file,
				        buf.begin(), surface.begin(), 0x500, b->off1FlatCopy);
				const Common::String key = Common::String::format("%s line %s ", rel.golden, sl.c_str());
				TS_ASSERT_EQUALS(Common::String(stream ? "1" : "0"), golden[key + "ret"]);
				delete stream;
				TS_ASSERT_EQUALS(AtlantisTest::md5Hex(surface.begin(), surface.size()), golden[key + "surface"]);
				played++;
			}
			// No other line flagged by the port.
			uint portFlagged = 0;
			for (Common::HashMap<Common::String, Common::String>::const_iterator it = port.begin(); it != port.end(); ++it)
				portFlagged++;
			TS_ASSERT_EQUALS(portFlagged, n);
			TS_ASSERT_EQUALS(found, n);
			TS_ASSERT_EQUALS(Common::String::format("%u", count[0]), golden[Common::String(rel.golden) + " headers off"]);
			TS_ASSERT_EQUALS(Common::String::format("%u", count[1]), golden[Common::String(rel.golden) + " headers off0"]);
			TS_ASSERT_EQUALS(Common::String::format("%u", count[2]), golden[Common::String(rel.golden) + " headers off1"]);
			TS_ASSERT_EQUALS(count[2], 29u);
			TS_ASSERT_EQUALS(played, 29u);
			TS_ASSERT_EQUALS(Common::String::format("%u", lines), golden[Common::String(rel.golden) + " headers lines"]);
		}
	}
};

#endif
