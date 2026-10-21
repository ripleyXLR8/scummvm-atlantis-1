#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_BREAKERS_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_UI_ZH_BREAKERS_H

// ui audit: the subtitle breakers of the Chinese CDs, FUN_0042f424 of
// atlantis-cd-zh-tw-696832 (Big5 marks) and atlantis-cd-zh-cn-692736
// (GB2312 marks), decision 0025 D2 + D3, against the original functions
// run unmodified under emulation (atlantis-re tools/emu/video_subtitles_zh.py,
// goldens evidence/golden/video_subtitles_zh_tw.txt / zh_cn.txt):
//   - the harness's synthetic texts, row by row;
//   - every CON dialog line of each release: rows per line, the longest
//     row, the rows that do not decode in the code page (zh-cn's rows
//     started one byte out of step after an ideographic space), the lines
//     of 4 rows or more, and the MD5 of every line's rows;
//   - zh-cn with the minor bug fixes (D2) against the original with its
//     skip after an ideographic space patched to 2 (@ 0x0042f5f2);
//   - zh-tw's line with no mark in its first window (CHAPI084, 80 bytes),
//     where the original's scan does not return: the port's cut at the
//     last character boundary within 75 bytes (D3).
// Each line is broken as the engine breaks it: its `[` line through
// conLineBufferText and SubtitleTextBuffer (the text buffer 0x00603cc0),
// and every line the port's CON parser gives the dialog is one of the
// lines scanned.
//
// Port code exercised directly: splitSubtitleCjk (subtitle_layout.h),
// SubtitleTextBuffer, conLineBufferText and ConScript (con_script.h),
// atlantisExeBuild / readPeTimestamp (exe_build.h: cjkBreaker,
// conCodePage).  Real files: `zhtw`, `zhcn` and `golden` in
// atlantis_test.cfg; skipped when absent.

#include "atlantis_test_helper.h"
#include "video_test_big.h"
#include "video_subtitles_en1997.h"
#include "../../../system/null_osystem.h"

#include "common/algorithm.h"
#include "common/array.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/str.h"
#include "common/ustr.h"

#include "cryomni3d/atlantis/con_script.h"
#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

namespace UiZhBreakers {

using namespace CryOmni3D::Atlantis;

struct Release {
	const char *key;     // atlantis_test.cfg
	const char *golden;
	CjkBreaker breaker;
	Common::CodePage codePage;
};

static const Release kReleases[2] = {
	{ "zhtw", "video_subtitles_zh_tw.txt", kCjkBreakerBig5, Common::kWindows950 },
	{ "zhcn", "video_subtitles_zh_cn.txt", kCjkBreakerGb2312, Common::kWindows936 },
};

// The harness's samples (video_subtitles_zh.py SAMPLES) in their code page.
// zh-tw: 短句。; two CHAPI001 lines joined (我在這裡放你下來，…道路！
// 你先順著階梯往上走，然後再問水池邊的守衛。); 一二三四五六七八九十 x 4,
// 。, 甲乙丙丁 x 6 (no mark in the first window).
static const char *const kSamplesTw[3] = {
	"\xb5\x75\xa5\x79\xa1\x43",
	"\xa7\xda\xa6\x62\xb3\x6f\xb8\xcc\xa9\xf1\xa7\x41\xa4\x55\xa8\xd3\xa1\x41"
	"\xc0\x48\xb1\x71\xaa\xba\xa6\xed\xb3\x42\xab\xdc\xae\x65\xa9\xf6\xb4\x4e"
	"\xa5\x69\xa5\x48\xa7\xe4\xa8\xec\xa1\x43\xc4\x40\xa8\xc8\xbf\x70\xa4\xa7"
	"\xa5\xfa\xb7\xd3\xc4\xa3\xb5\xdb\xa7\x41\xaa\xba\xb9\x44\xb8\xf4\xa1\x49"
	"\xa7\x41\xa5\xfd\xb6\xb6\xb5\xdb\xb6\xa5\xb1\xe8\xa9\xb9\xa4\x57\xa8\xab"
	"\xa1\x41\xb5\x4d\xab\xe1\xa6\x41\xb0\xdd\xa4\xf4\xa6\xc0\xc3\xe4\xaa\xba"
	"\xa6\x75\xbd\xc3\xa1\x43",
	"\xa4\x40\xa4\x47\xa4\x54\xa5\x7c\xa4\xad\xa4\xbb\xa4\x43\xa4\x4b\xa4\x45"
	"\xa4\x51\xa4\x40\xa4\x47\xa4\x54\xa5\x7c\xa4\xad\xa4\xbb\xa4\x43\xa4\x4b"
	"\xa4\x45\xa4\x51\xa4\x40\xa4\x47\xa4\x54\xa5\x7c\xa4\xad\xa4\xbb\xa4\x43"
	"\xa4\x4b\xa4\x45\xa4\x51\xa4\x40\xa4\x47\xa4\x54\xa5\x7c\xa4\xad\xa4\xbb"
	"\xa4\x43\xa4\x4b\xa4\x45\xa4\x51\xa1\x43\xa5\xd2\xa4\x41\xa4\xfe\xa4\x42"
	"\xa5\xd2\xa4\x41\xa4\xfe\xa4\x42\xa5\xd2\xa4\x41\xa4\xfe\xa4\x42\xa5\xd2"
	"\xa4\x41\xa4\xfe\xa4\x42\xa5\xd2\xa4\x41\xa4\xfe\xa4\x42\xa5\xd2\xa4\x41"
	"\xa4\xfe\xa4\x42"
};
// zh-cn: the same, Simplified, each ending in an ideographic space
// (短句。　; 我在这里…道路。　你先顺着…守卫。　; … 甲乙丙丁 x 6　).
static const char *const kSamplesCn[3] = {
	"\xb6\xcc\xbe\xe4\xa1\xa3\xa1\xa1",
	"\xce\xd2\xd4\xda\xd5\xe2\xc0\xef\xb7\xc5\xc4\xe3\xcf\xc2\xc0\xb4\xa3\xac"
	"\xcb\xe6\xb4\xd3\xb5\xc4\xd7\xa1\xb4\xa6\xba\xdc\xc8\xdd\xd2\xd7\xbe\xcd"
	"\xbf\xc9\xd2\xd4\xd5\xd2\xb5\xbd\xa1\xa3\xd4\xb8\xd1\xc7\xc4\xc2\xd6\xae"
	"\xb9\xe2\xd5\xd5\xd2\xab\xd7\xc5\xc4\xe3\xb5\xc4\xb5\xc0\xc2\xb7\xa1\xa3"
	"\xa1\xa1\xc4\xe3\xcf\xc8\xcb\xb3\xd7\xc5\xbd\xd7\xcc\xdd\xcd\xf9\xc9\xcf"
	"\xd7\xdf\xa3\xac\xc8\xbb\xba\xf3\xd4\xd9\xce\xca\xcb\xae\xb3\xd8\xb1\xdf"
	"\xb5\xc4\xca\xd8\xce\xc0\xa1\xa3\xa1\xa1",
	"\xd2\xbb\xb6\xfe\xc8\xfd\xcb\xc4\xce\xe5\xc1\xf9\xc6\xdf\xb0\xcb\xbe\xc5"
	"\xca\xae\xd2\xbb\xb6\xfe\xc8\xfd\xcb\xc4\xce\xe5\xc1\xf9\xc6\xdf\xb0\xcb"
	"\xbe\xc5\xca\xae\xd2\xbb\xb6\xfe\xc8\xfd\xcb\xc4\xce\xe5\xc1\xf9\xc6\xdf"
	"\xb0\xcb\xbe\xc5\xca\xae\xd2\xbb\xb6\xfe\xc8\xfd\xcb\xc4\xce\xe5\xc1\xf9"
	"\xc6\xdf\xb0\xcb\xbe\xc5\xca\xae\xa1\xa3\xbc\xd7\xd2\xd2\xb1\xfb\xb6\xa1"
	"\xbc\xd7\xd2\xd2\xb1\xfb\xb6\xa1\xbc\xd7\xd2\xd2\xb1\xfb\xb6\xa1\xbc\xd7"
	"\xd2\xd2\xb1\xfb\xb6\xa1\xbc\xd7\xd2\xd2\xb1\xfb\xb6\xa1\xbc\xd7\xd2\xd2"
	"\xb1\xfb\xb6\xa1\xa1\xa1"
};

inline Common::String utf8(const Common::String &bytes, Common::CodePage cp) {
	return Common::U32String(bytes, cp).encode(Common::kUtf8);
}

// Whether the row decodes in the code page (Python's strict cp950 / gbk in
// the harness): no U+FFFD from the port's decoder.
inline bool decodes(const Common::String &row, Common::CodePage cp) {
	const Common::U32String u(row, cp);
	for (uint i = 0; i < u.size(); i++)
		if (u[i] == 0xfffd)
			return false;
	return true;
}

// The rows as the engine breaks them: the text buffer filled from the `[`
// line (" " + text when the line has "] text"), broken up to its NUL.
inline void engineRows(const Common::String &text, CjkBreaker breaker, bool fixes,
                       Common::Array<Common::String> &rows) {
	SubtitleTextBuffer buf;
	buf.store(conLineBufferText("[Seth] " + text));
	splitSubtitleCjk(buf.text(), rows, breaker, fixes);
}

// The lines of a golden file as they are (not trimmed).
inline bool goldenLines(const char *file, Common::Array<Common::String> &out) {
	Common::File f;
	if (!f.open(Common::FSNode(Common::Path(AtlantisTest::goldenDir(), '/').appendComponent(file))))
		return false;
	while (!f.eos()) {
		const Common::String line = f.readLine();
		if (!line.empty())
			out.push_back(line);
	}
	return true;
}

inline const Common::String *lineWithPrefix(const Common::Array<Common::String> &lines, const Common::String &prefix) {
	for (uint i = 0; i < lines.size(); i++)
		if (lines[i].hasPrefix(prefix))
			return &lines[i];
	return nullptr;
}

// "len 114 -> 2 rows: 72:<row> | 42:<row>": each row's byte count and text
// (UTF-8, as the harness decoded it with errors replaced).
struct GoldenRow { uint bytes; Common::String text; };
inline bool parseSampleRows(const Common::String &line, Common::Array<GoldenRow> &rows) {
	size_t p = line.find("rows: ");
	if (p == Common::String::npos)
		return false;
	Common::String rest = line.substr(p + 6);
	for (;;) {
		const size_t colon = rest.find(':');
		if (colon == Common::String::npos)
			return false;
		const size_t bar = rest.find(" | ");
		GoldenRow r;
		r.bytes = (uint)atoi(rest.substr(0, colon).c_str());
		r.text = rest.substr(colon + 1, bar == Common::String::npos ? Common::String::npos : bar - colon - 1);
		rows.push_back(r);
		if (bar == Common::String::npos)
			return true;
		rest = rest.substr(bar + 3);
	}
}

// A Python bytes literal of \xNN escapes and printable characters.
inline Common::String pyBytes(const Common::String &lit) {
	Common::String out;
	size_t p = lit.find("b'");
	if (p == Common::String::npos)
		return out;
	for (p += 2; p < lit.size() && lit[p] != '\''; p++) {
		if (lit[p] == '\\' && p + 3 < lit.size() && lit[p + 1] == 'x') {
			out += (char)strtol(lit.substr(p + 2, 2).c_str(), nullptr, 16);
			p += 3;
		} else if (lit[p] == '\\' && p + 1 < lit.size()) {
			out += lit[++p];
		} else {
			out += lit[p];
		}
	}
	return out;
}

// The harness's scan of every dialog line (video_subtitles_zh.py main and
// scan_digest), with the port's rows.
struct Scan {
	int lines = 0, faults = 0, hist[9] = { 0 }, over8 = 0, longest = 0;
	Common::String longestName, longestRow;
	int broken = 0, linesBroken = 0, lone = 0;
	Common::Array<Common::String> brokenNames, brokenRows;
	Common::Array<Common::String> many;   // "N rows: NAME (L bytes)", 4 rows or more
	Common::String md5;
};

// `runawayName` / `runawayLen`: the line the golden lists as not returning
// (its rows FAULT in the digest, left out of the counts as there).
inline void scan(const VideoTest::Big &big, CjkBreaker breaker, Common::CodePage cp, bool fixes,
                 const Common::String &runawayName, uint runawayLen, Scan &out,
                 Common::Array<Common::String> *runawayRows = nullptr) {
	Common::Array<Common::String> cons = big.names(".CON");
	Common::sort(cons.begin(), cons.end());
	Common::String digest;
	struct Many { int rows; Common::String name; uint len; };
	Common::Array<Many> many;
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
			engineRows(texts[t], breaker, fixes, rows);
			if (out.lines++)
				digest += '\n';
			if (cons[f] == runawayName && texts[t].size() == runawayLen) {
				digest += "FAULT";
				out.faults++;
				if (runawayRows)
					*runawayRows = rows;
				continue;
			}
			for (uint r = 0; r < rows.size(); r++) {
				if (r)
					digest += ',';
				for (uint i = 0; i < rows[r].size(); i++)
					digest += Common::String::format("%02x", (byte)rows[r][i]);
			}
			if (rows.size() < ARRAYSIZE(out.hist))
				out.hist[rows.size()]++;
			out.over8 += rows.size() > 8;
			if (rows.size() >= 4) {
				Many m = { (int)rows.size(), cons[f], texts[t].size() };
				many.push_back(m);
			}
			bool bad = false;
			for (uint r = 0; r < rows.size(); r++) {
				if (!decodes(rows[r], cp)) {
					bad = true;
					out.broken++;
					out.lone += rows[r].size() == 1 && (byte)rows[r][0] >= 0x80;
					out.brokenNames.push_back(cons[f]);
					out.brokenRows.push_back(rows[r]);
				}
				if ((int)rows[r].size() > out.longest) {
					out.longest = rows[r].size();
					out.longestName = cons[f];
					out.longestRow = rows[r];
				}
			}
			out.linesBroken += bad;
		}
	}
	// sorted(many, reverse=True)
	Common::sort(many.begin(), many.end(), [](const Many &a, const Many &b) {
		if (a.rows != b.rows)
			return a.rows > b.rows;
		if (a.name != b.name)
			return b.name < a.name;
		return a.len > b.len;
	});
	for (uint i = 0; i < many.size() && i < 12; i++)
		out.many.push_back(Common::String::format("# %d rows: %s (%u bytes)", many[i].rows, many[i].name.c_str(),
		                                          many[i].len));
	out.md5 = AtlantisTest::md5Hex((const byte *)digest.c_str(), digest.size());
}

inline bool readExe(const Common::String &dir, Common::Array<byte> &out) {
	Common::FSNode d(Common::Path(dir, '/'));
	Common::FSList list;
	if (!d.getChildren(list, Common::FSNode::kListFilesOnly))
		return false;
	for (uint i = 0; i < list.size(); i++) {
		if (!list[i].getName().equalsIgnoreCase("atlantis.exe"))
			continue;
		Common::File f;
		if (!f.open(list[i]))
			return false;
		out.resize((uint)f.size());
		return f.read(out.data(), out.size()) == out.size();
	}
	return false;
}

} // End of namespace UiZhBreakers

class UiZhBreakersTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// The golden's samples: a short line (one row, its space included); two
	// CHAPI001 lines joined (zh-tw cut after `！`; zh-cn cut before the
	// ideographic space, the second row read from the space's second byte,
	// in step with the minor bug fixes); a text with no mark in its first
	// window, where the original's scan does not return and the port cuts
	// at the last character boundary within 75 bytes (D3).
	void test_samples_vs_golden() {
		using namespace UiZhBreakers;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		for (uint k = 0; k < 2; k++) {
			const Release &rel = kReleases[k];
			const char *const *samples = k ? kSamplesCn : kSamplesTw;
			Common::Array<Common::String> golden;
			TS_ASSERT(goldenLines(rel.golden, golden));
			for (uint s = 0; s < 3; s++) {
				const Common::String text(samples[s]);
				const Common::String prefix = Common::String::format("len %3u -> ", text.size());
				const Common::String *line = lineWithPrefix(golden, prefix);
				TSM_ASSERT(prefix.c_str(), line);
				if (!line)
					continue;
				Common::Array<Common::String> rows;
				for (int fixes = 0; fixes < 2; fixes++) {
					engineRows(text, rel.breaker, fixes, rows);
					if (s == 2) {
						// the original: no return
						TS_ASSERT(line->contains("-> no return"));
						TS_ASSERT_EQUALS(rows.size(), 2u);
						if (rows.size() != 2)
							continue;
						// 37 characters (74 bytes; a 38th would end at 76),
						// then the rest
						TS_ASSERT_EQUALS(rows[0].size(), 74u);
						TS_ASSERT_EQUALS(rows[0] + rows[1], text);
						TS_ASSERT(decodes(rows[0], rel.codePage) && decodes(rows[1], rel.codePage));
						continue;
					}
					if (k == 1 && s == 1 && fixes) {
						// D2: the second row after the whole space
						TS_ASSERT_EQUALS(rows.size(), 2u);
						if (rows.size() == 2) {
							TS_ASSERT_EQUALS(rows[0].size(), 72u);
							TS_ASSERT_EQUALS(rows[1].size(), 44u);
							TS_ASSERT(text.hasSuffix(rows[1]));
							TS_ASSERT(decodes(rows[1], rel.codePage));
						}
						continue;
					}
					Common::Array<GoldenRow> want;
					TS_ASSERT(parseSampleRows(*line, want));
					TSM_ASSERT_EQUALS(line->c_str(), rows.size(), want.size());
					for (uint r = 0; r < rows.size() && r < want.size(); r++) {
						TSM_ASSERT_EQUALS(line->c_str(), rows[r].size(), want[r].bytes);
						const Common::String got = utf8(rows[r], rel.codePage);
						const size_t bad = want[r].text.find("\xef\xbf\xbd");
						if (bad == Common::String::npos) {
							TSM_ASSERT_EQUALS(line->c_str(), got, want[r].text);
						} else {
							// zh-cn's out-of-step row (the last): the
							// characters up to the pair that does not decode
							// (Python's decoder and the port's resume
							// differently after it), and the bytes, the end
							// of the text
							TS_ASSERT(!decodes(rows[r], rel.codePage));
							TSM_ASSERT(got.c_str(), got.hasPrefix(want[r].text.substr(0, bad)));
							TS_ASSERT_EQUALS(r + 1, rows.size());
							TS_ASSERT(text.hasSuffix(rows[r]));
						}
					}
				}
			}
		}
	}

	// Every dialog line of each release, broken as the engine breaks it,
	// against the golden's scan of the original (and, for zh-cn under the
	// minor bug fixes, of the original with the skip after A1 A1 patched to
	// 2); zh-tw's runaway line cut by D3; every line the port's CON parser
	// hands to the dialog is one of the lines scanned.
	void test_scan_vs_golden() {
		using namespace UiZhBreakers;
		if (AtlantisTest::goldenDir().empty()) { TS_WARN("golden not configured; skipped"); return; }
		for (uint k = 0; k < 2; k++) {
			const Release &rel = kReleases[k];
			const Common::String dir = AtlantisTest::configValue(rel.key);
			if (dir.empty()) { TS_WARN(Common::String::format("%s not configured; skipped", rel.key).c_str()); continue; }
			// the build entry gives the breaker and the code page
			Common::Array<byte> e;
			TS_ASSERT(readExe(dir, e));
			if (e.empty())
				continue;
			Common::MemoryReadStream es(e.data(), e.size());
			const AtlantisExeBuild &b = atlantisExeBuild(e.size(), readPeTimestamp(es));
			TS_ASSERT_EQUALS(b.cjkBreaker, rel.breaker);
			TS_ASSERT_EQUALS(b.conCodePage, rel.codePage);

			Common::Array<Common::String> golden;
			TS_ASSERT(goldenLines(rel.golden, golden));
			// zh-tw: "# no return (no cut point in a window): ('SCENAR\\CHAPI084.CON', 80, ...)"
			Common::String runawayName;
			uint runawayLen = 0, goldenFaults = 0;
			for (uint i = 0; i < golden.size(); i++) {
				if (!golden[i].hasPrefix("# no return (no cut point in a window): ('"))
					continue;
				goldenFaults++;
				Common::String rest = golden[i].substr(golden[i].find("('") + 2);
				const size_t q = rest.find("', ");
				for (uint c = 0; c < q; c++)
					if (!(rest[c] == '\\' && c + 1 < q && rest[c + 1] == '\\'))
						runawayName += rest[c];
				runawayLen = (uint)atoi(rest.c_str() + q + 3);
			}
			TS_ASSERT_EQUALS(goldenFaults, k ? 0u : 1u);

			VideoTest::Big big(dir);
			TS_ASSERT(!big.empty());
			for (int fixes = 0; fixes < 2; fixes++) {
				Scan sc;
				Common::Array<Common::String> runawayRows;
				scan(big, b.cjkBreaker, b.conCodePage, fixes, runawayName, runawayLen, sc, &runawayRows);
				TS_ASSERT_EQUALS(sc.lines, 2542);
				TS_ASSERT_EQUALS(sc.faults, (int)goldenFaults);
				if (k == 1 && fixes) {
					const Common::String want = Common::String::format(
					        "# skip after A1 A1 patched to 2 (@ 0x0042f5f2): rows md5 %s; rows that do not "
					        "decode as gbk: %d in %d lines", sc.md5.c_str(), sc.broken, sc.linesBroken);
					TSM_ASSERT(want.c_str(), lineWithPrefix(golden, want));
					TS_ASSERT_EQUALS(sc.broken, 45);
					TS_ASSERT_EQUALS(sc.linesBroken, 23);
					continue;
				}
				// the counts line, the md5 line
				Common::String hist = "{";
				for (uint n = 0; n < ARRAYSIZE(sc.hist); n++)
					if (sc.hist[n])
						hist += Common::String::format("%s%u: %d", hist.size() > 1 ? ", " : "", n, sc.hist[n]);
				hist += "}";
				const Common::String counts = Common::String::format(
				        "# CON dialog lines: %d; rows per line %s; lines over 8 rows: %d; faults: %d", sc.lines,
				        hist.c_str(), sc.over8, sc.faults);
				TSM_ASSERT(counts.c_str(), lineWithPrefix(golden, counts));
				const Common::String md5 = "# rows md5 (each line's rows as hex joined by ',', FAULT for no "
				                           "return; lines by newline, sorted CON names): " + sc.md5;
				TSM_ASSERT(md5.c_str(), lineWithPrefix(golden, md5));
				// the longest row
				const Common::String longest = Common::String::format("# longest row: %s %d bytes: ",
				        sc.longestName.c_str(), sc.longest) + utf8(sc.longestRow, b.conCodePage);
				TSM_ASSERT(longest.c_str(), lineWithPrefix(golden, longest));
				TS_ASSERT_EQUALS(sc.longest, 75);
				// the rows that do not decode, the first six as bytes
				const Common::String brokenLine = Common::String::format(
				        "# rows that do not decode as %s (a row starting or ending inside a double-byte character): "
				        "%d in %d lines, of which rows of one lone byte >= 0x80: %d", k ? "gbk" : "cp950", sc.broken,
				        sc.linesBroken, sc.lone);
				TSM_ASSERT(brokenLine.c_str(), lineWithPrefix(golden, brokenLine));
				TS_ASSERT_EQUALS(sc.broken, k ? 72 : 0);
				uint listed = 0;
				for (uint i = 0; i < golden.size(); i++) {
					if (!golden[i].hasPrefix("#   SCENAR"))
						continue;
					TS_ASSERT(listed < sc.brokenRows.size());
					if (listed >= sc.brokenRows.size())
						break;
					TS_ASSERT(golden[i].hasPrefix("#   " + sc.brokenNames[listed] + ": b'"));
					TS_ASSERT_EQUALS(pyBytes(golden[i]), sc.brokenRows[listed]);
					listed++;
				}
				TS_ASSERT_EQUALS(listed, k ? 6u : 0u);
				// the lines of 4 rows or more
				uint manyListed = 0;
				for (uint i = 0; i < golden.size(); i++) {
					if (!golden[i].hasPrefix("# ") || !golden[i].contains(" rows: SCENAR"))
						continue;
					TS_ASSERT(manyListed < sc.many.size());
					if (manyListed < sc.many.size())
						TS_ASSERT_EQUALS(golden[i], sc.many[manyListed]);
					manyListed++;
				}
				TS_ASSERT_EQUALS(manyListed, sc.many.size());
				TS_ASSERT_EQUALS(manyListed, 6u);
				// D3: zh-tw's runaway line (塞特沒有武器可以對抗那頭怪物...
				// 他永遠也無法得知亞特蘭提斯將會面臨什麼樣的命運！ and a
				// space; its first mark ends at byte 79): 75 bytes (36
				// characters and the `...`, up to 命), then 運！ and the space,
				// each row decoding
				if (!k) {
					TS_ASSERT_EQUALS(runawayName, Common::String("SCENAR\\CHAPI084.CON"));
					TS_ASSERT_EQUALS(runawayLen, 80u);
					TS_ASSERT_EQUALS(runawayRows.size(), 2u);
					if (runawayRows.size() == 2) {
						TS_ASSERT_EQUALS(runawayRows[0].size(), 75u);
						TS_ASSERT_EQUALS(runawayRows[1].size(), 5u);
						for (uint r = 0; r < 2; r++)
							TS_ASSERT(decodes(runawayRows[r], b.conCodePage));
					}
				}
			}

			// the lines the port's CON parser gives the dialog (the executor
			// copies `[` lines with a non-empty text into the buffer) are
			// lines of the scan: the buffer is " " + one of them
			Common::Array<Common::String> cons = big.names(".CON");
			int parsed = 0;
			for (uint f = 0; f < cons.size(); f++) {
				uint32 size = 0;
				byte *raw = big.get(cons[f], size);
				if (!raw)
					continue;
				Common::Array<Common::String> texts;
				VideoSubEn1997::conDialogTexts(raw, size, texts);
				ConScript script;
				script.parse((const char *)raw, size);
				delete[] raw;
				for (uint s = 0; s < script.sections().size(); s++) {
					const ConSection &sec = script.sections()[s];
					for (uint i = 0; i < sec.items.size(); i++) {
						const ConSectionItem &it = sec.items[i];
						if (it.isSet || it.line.text.empty())
							continue;
						const Common::String bufText = conLineBufferText(it.line.raw);
						bool found = false;
						for (uint t = 0; t < texts.size() && !found; t++)
							found = bufText == " " + texts[t];
						TSM_ASSERT((cons[f] + ": " + it.line.raw).c_str(), found);
						parsed++;
					}
				}
			}
			TS_ASSERT(parsed > 2000);
		}
	}

	// Every other build keeps its breaker: no CJK breaker in its entry.
	void test_other_builds_have_no_cjk_breaker() {
		using namespace CryOmni3D::Atlantis;
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			const bool zh = !strcmp(b.name, "atlantis-cd-zh-tw-696832") || !strcmp(b.name, "atlantis-cd-zh-cn-692736");
			TSM_ASSERT(b.name, zh == (b.cjkBreaker != kCjkBreakerNone));
		}
	}
};

#endif
