#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_SUBTITLES_ZH_FONT_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_SUBTITLES_ZH_FONT_H

// video audit: the Chinese CDs' subtitle font and drawing (atlantis-re
// decision 0025 C1 + C2 + C3; atlantis-cd-zh-tw-696832 and
// atlantis-cd-zh-cn-692736 Z1, Z3):
//   - what the original asks for, on the bytes: FUN_0041ad5c @ 0x0041ad77
//     CreateFontA(16, ..., DEFAULT_CHARSET, ..., "MingLi") (height 16 from
//     WinMain @ 0x00418f89), FUN_0041ae20 SetBkMode TRANSPARENT then TextOutA
//     of the bytes, no SetTextAlign imported (TA_TOP); the drawer
//     (zh-tw FUN_0042f798, zh-cn FUN_0042f774) black at (6, y + 1), white
//     0xffffff at (5, y), y = 0x1ce - (n - i + 1) x 18;
//   - the port's font for code page 950 / 936 (subtitle_font.h): the
//     player's mingliu.ttc / simsun.ttc face MingLiU / SimSun first, else
//     Noto Serif TC / SC, else Noto Sans TC / SC from fonts-cjk.dat, at 16
//     pixels per em, the Noto faces' ideographs in the 16-pixel cell under
//     the row's y; every other build keeps Arial and its 14-px rows;
//   - drawSubtitleRows of a zh-tw / zh-cn engine on the rows of the goldens
//     (evidence/golden/video_subtitles_zh_tw.txt / zh_cn.txt, CHAPI001's
//     first line, emulated): white and black only (monochrome), the shadow
//     at (+1, +1), the glyphs from x 5 in the rows' 16-pixel cells 18 px
//     apart, 16 px per ideograph.
//
// Port code exercised directly: cjkSubtitleFaces, loadCjkSubtitleFont,
// cjkSubtitleYShift (subtitle_font.cpp), subtitleLineY (subtitle_layout.cpp),
// CryOmni3DEngine_Atlantis::subtitleFont / subtitleRowY / subtitleRowStep /
// drawSubtitleRows (engine.cpp) on zero-filled engine storage.  Fonts:
// `src` in atlantis_test.cfg (dists/engine-data/fonts.dat, fonts-cjk.dat);
// the player's face is a copy of Liberation Sans renamed MingLiU / SimSun
// (no MingLiU / SimSun file is at hand), the player's Noto Serif a copy of
// Noto Sans under Noto Serif's file name.  Real files: `zhtw`, `zhcn` for
// the code bytes; `golden`.  Skipped where absent.

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/compression/unzip.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/ptr.h"
#include "common/ustr.h"

#include "graphics/font.h"
#include "graphics/managed_surface.h"

#include "cryomni3d/atlantis/exe_build.h"
#include "cryomni3d/atlantis/subtitle_font.h"
#include "cryomni3d/atlantis/subtitle_layout.h"

// The first test header that includes engine.h decides its access, hence
// the same defines as video_dialog.h.
#define private public
#define protected public
#include "cryomni3d/atlantis/engine.h"
#undef private
#undef protected

#include <new>

namespace ZhFont {

using namespace CryOmni3D::Atlantis;

typedef CryOmni3DEngine_Atlantis Eng;

static const uint32 kTwTimestamp = 0x35af1f09;
static const uint32 kCnTimestamp = 0x36f11f20;

// CHAPI001's first line as FUN_0042f424 cuts it (the goldens' `len 114` /
// `len 118` rows; zh-cn's second row is out of step, so only the first).
static const char kTwRow1[] =
	"\xa7\xda\xa6\x62\xb3\x6f\xb8\xcc\xa9\xf1\xa7\x41\xa4\x55\xa8\xd3\xa1\x41\xc0\x48\xb1\x71\xaa\xba"
	"\xa6\xed\xb3\x42\xab\xdc\xae\x65\xa9\xf6\xb4\x4e\xa5\x69\xa5\x48\xa7\xe4\xa8\xec\xa1\x43\xc4\x40"
	"\xa8\xc8\xbf\x70\xa4\xa7\xa5\xfa\xb7\xd3\xc4\xa3\xb5\xdb\xa7\x41\xaa\xba\xb9\x44\xb8\xf4\xa1\x49";
static const char kTwRow2[] =
	"\xa7\x41\xa5\xfd\xb6\xb6\xb5\xdb\xb6\xa5\xb1\xe8\xa9\xb9\xa4\x57\xa8\xab\xa1\x41\xb5\x4d\xab\xe1"
	"\xa6\x41\xb0\xdd\xa4\xf4\xa6\xc0\xc3\xe4\xaa\xba\xa6\x75\xbd\xc3\xa1\x43";
static const char kCnRow1[] =
	"\xce\xd2\xd4\xda\xd5\xe2\xc0\xef\xb7\xc5\xc4\xe3\xcf\xc2\xc0\xb4\xa3\xac\xcb\xe6\xb4\xd3\xb5\xc4"
	"\xd7\xa1\xb4\xa6\xba\xdc\xc8\xdd\xd2\xd7\xbe\xcd\xbf\xc9\xd2\xd4\xd5\xd2\xb5\xbd\xa1\xa3\xd4\xb8"
	"\xd1\xc7\xc4\xc2\xd6\xae\xb9\xe2\xd5\xd5\xd2\xab\xd7\xc5\xc4\xe3\xb5\xc4\xb5\xc0\xc2\xb7\xa1\xa3";

// A read-only archive of files held in memory.
class MemArchive : public Common::Archive {
public:
	void add(const char *name, const Common::Array<byte> &data) {
		_names.push_back(Common::String(name));
		_data.push_back(data);
	}
	bool hasFile(const Common::Path &path) const override { return find(path) >= 0; }
	int listMembers(Common::ArchiveMemberList &list) const override {
		for (uint i = 0; i < _names.size(); i++)
			list.push_back(Common::ArchiveMemberPtr(new Common::GenericArchiveMember(_names[i], *this)));
		return (int)_names.size();
	}
	const Common::ArchiveMemberPtr getMember(const Common::Path &path) const override {
		const int i = find(path);
		return i < 0 ? Common::ArchiveMemberPtr()
		             : Common::ArchiveMemberPtr(new Common::GenericArchiveMember(_names[i], *this));
	}
	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override {
		const int i = find(path);
		if (i < 0)
			return nullptr;
		byte *copy = (byte *)malloc(_data[i].size());
		memcpy(copy, _data[i].data(), _data[i].size());
		return new Common::MemoryReadStream(copy, _data[i].size(), DisposeAfterUse::YES);
	}

private:
	int find(const Common::Path &path) const {
		for (uint i = 0; i < _names.size(); i++)
			if (path.toString().equalsIgnoreCase(_names[i]))
				return (int)i;
		return -1;
	}
	Common::Array<Common::String> _names;
	Common::Array<Common::Array<byte> > _data;
};

inline bool engineDataInSearchMan() {
	const Common::String src = AtlantisTest::configValue("src");
	if (src.empty())
		return false;
	Common::FSNode dir(Common::Path(src + "/dists/engine-data", '/'));
	if (!dir.getChild("fonts.dat").exists() || !dir.getChild("fonts-cjk.dat").exists())
		return false;
	if (!SearchMan.hasArchive("zh_font_engine_data"))
		SearchMan.addDirectory("zh_font_engine_data", dir);
	return true;
}

// The member `name` of the zip archive `dat` (found through SearchMan).
inline Common::Array<byte> datMember(const char *dat, const char *name) {
	Common::Array<byte> out;
	Common::ScopedPtr<Common::Archive> zip(Common::makeZipArchive(SearchMan.createReadStreamForMember(Common::Path(dat))));
	if (!zip)
		return out;
	Common::ScopedPtr<Common::SeekableReadStream> s(zip->createReadStreamForMember(Common::Path(name)));
	if (!s)
		return out;
	out.resize((uint)s->size());
	s->read(out.data(), out.size());
	return out;
}

inline uint16 be16(const Common::Array<byte> &b, uint32 o) { return (uint16)(b[o] << 8 | b[o + 1]); }
inline uint32 be32(const Common::Array<byte> &b, uint32 o) { return (uint32)be16(b, o) << 16 | be16(b, o + 2); }

// A copy of the TrueType font `ttf` whose family names (name IDs 1 and 16)
// read `family` (each record shortened in place).
inline Common::Array<byte> renamed(const Common::Array<byte> &ttf, const char *family) {
	Common::Array<byte> b = ttf;
	if (b.size() < 12)
		return b;
	const uint n = be16(b, 4);
	for (uint t = 0; t < n; t++) {
		const uint32 rec = 12 + 16 * t;
		if (memcmp(&b[rec], "name", 4))
			continue;
		const uint32 table = be32(b, rec + 8);
		const uint count = be16(b, table + 2), strings = table + be16(b, table + 4);
		for (uint r = 0; r < count; r++) {
			const uint32 nr = table + 6 + 12 * r;
			const uint platform = be16(b, nr), id = be16(b, nr + 6), len = be16(b, nr + 8);
			const uint32 at = strings + be16(b, nr + 10);
			if (id != 1 && id != 16)
				continue;
			const uint wide = (platform == 1) ? 1 : 2;
			const uint want = strlen(family) * wide;
			if (want > len)
				continue;
			for (uint c = 0; c < strlen(family); c++) {
				if (wide == 2)
					b[at + 2 * c] = 0;
				b[at + wide * c + wide - 1] = (byte)family[c];
			}
			b[nr + 8] = (byte)(want >> 8);
			b[nr + 9] = (byte)want;
		}
	}
	return b;
}

// A TrueType collection of `fonts` (each copied whole, its table offsets
// moved by where it lands).
inline Common::Array<byte> collection(const Common::Array<Common::Array<byte> > &fonts) {
	Common::Array<byte> b(12 + 4 * fonts.size(), 0);
	memcpy(&b[0], "ttcf\0\1\0\0", 8);
	b[11] = (byte)fonts.size();
	for (uint i = 0; i < fonts.size(); i++) {
		while (b.size() % 4)
			b.push_back(0);
		const uint32 base = b.size();
		for (int k = 0; k < 4; k++)
			b[12 + 4 * i + k] = (byte)(base >> (24 - 8 * k));
		for (uint j = 0; j < fonts[i].size(); j++)
			b.push_back(fonts[i][j]);
		const uint n = be16(b, base + 4);
		for (uint t = 0; t < n; t++) {
			const uint32 rec = base + 12 + 16 * t, off = be32(b, rec + 8) + base;
			for (int k = 0; k < 4; k++)
				b[rec + 8 + k] = (byte)(off >> (24 - 8 * k));
		}
	}
	return b;
}

// atlantis.exe of the folder `key` of atlantis_test.cfg.
inline bool readExe(const char *key, Common::Array<byte> &out) {
	const Common::String dir = AtlantisTest::configValue(key);
	if (dir.empty())
		return false;
	Common::FSList list;
	if (!Common::FSNode(Common::Path(dir, '/')).getChildren(list, Common::FSNode::kListFilesOnly))
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

// BEGTEXT: VA 0x00410000 at file offset 0x400 in both executables.
inline bool codeAt(const Common::Array<byte> &e, uint32 va, const char *hex) {
	uint32 off = va - 0x00410000 + 0x400;
	for (const char *p = hex; *p; ) {
		if (*p == ' ') {
			p++;
			continue;
		}
		uint v = 0;
		for (int k = 0; k < 2; k++, p++)
			v = v * 16 + (*p <= '9' ? *p - '0' : (*p | 0x20) - 'a' + 10);
		if (off >= e.size() || e[off++] != v)
			return false;
	}
	return true;
}

inline bool contains(const Common::Array<byte> &e, const char *s, uint n = 0) {
	if (!n)
		n = strlen(s);
	for (uint i = 0; i + n <= e.size(); i++)
		if (!memcmp(&e[i], s, n))
			return true;
	return false;
}

inline Eng *engineOf(const AtlantisExeBuild &b) {
	void *mem = ::operator new(sizeof(Eng));
	memset(mem, 0, sizeof(Eng));
	Eng *e = static_cast<Eng *>(mem);
	e->_exeBuild = &b;
	new (&e->_subtitleBitmapFont) Common::Array<byte>();
	return e;
}

inline void freeEngine(Eng *e) {
	delete e->_subtitleTTF;
	::operator delete(e);
}

} // End of namespace ZhFont

class VideoSubtitlesZhFontTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override {
		SearchMan.remove("zh_font_player");
		SearchMan.remove("zh_font_engine_data");
		Common::uninstall_null_g_system();
	}

	// The faces by code page: the two Chinese builds get MingLiU / SimSun,
	// Noto Serif and Noto Sans TC / SC, every other build none (Arial).
	void test_faces_by_code_page() {
		using namespace ZhFont;
		const CjkSubtitleFaces *tc = cjkSubtitleFaces(Common::kWindows950);
		const CjkSubtitleFaces *sc = cjkSubtitleFaces(Common::kWindows936);
		TS_ASSERT(tc && sc);
		if (!tc || !sc)
			return;
		TS_ASSERT_EQUALS(Common::String(tc->playerFile), "mingliu.ttc");
		TS_ASSERT_EQUALS(Common::String(tc->playerFamily), "MingLiU");
		TS_ASSERT_EQUALS(Common::String(tc->serifFiles[0]), "NotoSerifTC-Regular.otf");
		TS_ASSERT_EQUALS(Common::String(tc->sansFile), "NotoSansTC-Regular.otf");
		TS_ASSERT_EQUALS(Common::String(sc->playerFile), "simsun.ttc");
		TS_ASSERT_EQUALS(Common::String(sc->playerFamily), "SimSun");
		TS_ASSERT_EQUALS(Common::String(sc->serifFiles[0]), "NotoSerifSC-Regular.otf");
		TS_ASSERT_EQUALS(Common::String(sc->sansFile), "NotoSansSC-Regular.otf");
		TS_ASSERT(!cjkSubtitleFaces(Common::kWindows1252));
		TS_ASSERT(!cjkSubtitleFaces(Common::kDos850));
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			const bool chinese = b.peTimestamp == kTwTimestamp || b.peTimestamp == kCnTimestamp;
			TSM_ASSERT(b.name, (cjkSubtitleFaces(b.conCodePage) != nullptr) == chinese);
		}
		TS_ASSERT_EQUALS(kCjkSubtitleEmPx, 16);
		TS_ASSERT_EQUALS(kCjkSubtitleNotoAscent, 14);
	}

	// What the original asks GDI for, and its drawer, on the bytes.
	void test_font_and_drawer_on_the_bytes() {
		using namespace ZhFont;
		const char *const keys[2] = { "zhtw", "zhcn" };
		for (uint k = 0; k < 2; k++) {
			Common::Array<byte> e;
			if (!readExe(keys[k], e)) {
				TS_WARN(Common::String::format("%s not configured; skipped", keys[k]).c_str());
				continue;
			}
			// WinMain: `mov eax,0x10; call FUN_0041ad5c`
			TS_ASSERT(codeAt(e, 0x00418f89, "b8 10 00 00 00 e8 c9 1d 00 00"));
			// FUN_0041ad5c: the face 0x004ac2e8, then DEFAULT_QUALITY (0)
			// ... DEFAULT_CHARSET (1) ... height, CreateFontA (IAT 0x00c20108)
			TS_ASSERT(codeAt(e, 0x0041ad77, "b8 e8 c2 4a 00 50 6a 00 6a 00 6a 00 6a 00 6a 01 6a 00 6a 00 6a 00 "
			                                "6a 00 6a 00 6a 00 6a 00 8b 45 f8 50 2e ff 15 08 01 c2 00"));
			TS_ASSERT(contains(e, "MingLi\0\0erreur Texte", 20));
			TS_ASSERT(contains(e, "CreateFontA") && contains(e, "TextOutA") && !contains(e, "SetTextAlign"));
			// FUN_0041ae20: SetBkMode(dc, 1) then TextOutA(dc, x, y, s, strlen(s))
			TS_ASSERT(codeAt(e, 0x0041aeb3, "6a 01 ff 35 50 d8 6b 00 2e ff 15 2c 01 c2 00"));
			TS_ASSERT(codeAt(e, 0x0041aed7, "ff 35 50 d8 6b 00 2e ff 15 40 01 c2 00"));
			// the drawer: y0 0x1ce; black (ecx 0) at (6, y + 1), white at (5, y)
			const uint32 d = k ? 0x0042f774 : 0x0042f798;
			TS_ASSERT(codeAt(e, d + 0x1b, "c7 45 f8 ce 01 00 00"));
			TS_ASSERT(codeAt(e, d + 0x4a, "31 c9"));
			TS_ASSERT(codeAt(e, d + 0x60, "6b c0 12"));
			TS_ASSERT(codeAt(e, d + 0x6a, "8d 50 01 b8 06 00 00 00"));
			TS_ASSERT(codeAt(e, d + 0x77, "b9 ff ff ff 00"));
			TS_ASSERT(codeAt(e, d + 0x90, "6b c0 12"));
			TS_ASSERT(codeAt(e, d + 0x98, "b8 05 00 00 00"));
		}
	}

	// Row y: 18 px apart in the Chinese builds, 14 in the other GDI builds,
	// the FONTS.BIN step in the 1997 bitmap builds.
	void test_row_y() {
		using namespace ZhFont;
		TS_ASSERT_EQUALS(subtitleLineY(0, 1), 0x1ce - 2 * 14);
		TS_ASSERT_EQUALS(subtitleLineY(1, 3, 18), 0x1ce - 3 * 18);
		for (uint i = 0; i < ARRAYSIZE(kAtlantisExeBuilds); i++) {
			const AtlantisExeBuild &b = kAtlantisExeBuilds[i];
			const bool chinese = b.peTimestamp == kTwTimestamp || b.peTimestamp == kCnTimestamp;
			Eng *e = engineOf(b);
			const int step = b.bitmapSubtitles ? kSubtitleBitmapRowStep : (chinese ? 18 : 14);
			TSM_ASSERT_EQUALS(b.name, e->subtitleRowStep(), step);
			for (uint n = 1; n <= 5; n++)
				for (uint r = 0; r < n; r++)
					TSM_ASSERT_EQUALS(b.name, e->subtitleRowY(r, n),
					                  b.bitmapSubtitles ? subtitleLineYBitmap(r, n) : 0x1ce - (int)(n - r + 1) * step);
			freeEngine(e);
		}
	}

	// The sources in order: the player's face, Noto Serif, Noto Sans.
	void test_font_sources_in_order() {
		using namespace ZhFont;
		if (!engineDataInSearchMan()) { TS_WARN("src (dists/engine-data) not configured; skipped"); return; }
		const Common::CodePage pages[2] = { Common::kWindows950, Common::kWindows936 };
		const char *const players[2] = { "mingliu.ttc", "simsun.ttc" };
		const char *const families[2] = { "MingLiU", "SimSun" };
		const char *const serifs[2] = { "NotoSerifTC-Regular.otf", "NotoSerifSC-Regular.otf" };
		const char *const sans[2] = { "NotoSansTC-Regular.otf", "NotoSansSC-Regular.otf" };
		const Common::Array<byte> liberation = datMember("fonts.dat", "LiberationSans-Regular.ttf");
		const Common::Array<byte> mono = datMember("fonts.dat", "LiberationMono-Regular.ttf");
		TS_ASSERT(!liberation.empty() && !mono.empty());
		for (uint k = 0; k < 2; k++) {
			const Common::Array<byte> noto = datMember("fonts-cjk.dat", sans[k]);
			TS_ASSERT(!noto.empty());
			CjkSubtitleFontSource source;
			// fonts-cjk.dat alone: Noto Sans, ascent ceil(1160 x 16 / 1000) = 19,
			// its ideographs moved 5 px up
			Common::ScopedPtr<Graphics::Font> f(loadCjkSubtitleFont(pages[k], source));
			TS_ASSERT(f);
			TS_ASSERT_EQUALS(source, kCjkSubtitleFontSans);
			if (f) {
				TS_ASSERT_EQUALS(f->getFontAscent(), 19);
				TS_ASSERT_EQUALS(cjkSubtitleYShift(*f, source), -5);
				// 16 px per em: an ideograph advances 16
				TS_ASSERT_EQUALS(f->getCharWidth(0x6211), 16);
			}
			// a player's file of that name without the family: not taken
			MemArchive *player = new MemArchive();
			player->add(players[k], noto);
			SearchMan.add("zh_font_player", player);
			f.reset(loadCjkSubtitleFont(pages[k], source));
			TS_ASSERT_EQUALS(source, kCjkSubtitleFontSans);
			SearchMan.remove("zh_font_player");
			// Noto Serif beside the game files before Noto Sans
			player = new MemArchive();
			player->add(serifs[k], noto);
			SearchMan.add("zh_font_player", player);
			f.reset(loadCjkSubtitleFont(pages[k], source));
			TS_ASSERT(f);
			TS_ASSERT_EQUALS(source, kCjkSubtitleFontSerif);
			if (f)
				TS_ASSERT_EQUALS(cjkSubtitleYShift(*f, source), 14 - f->getFontAscent());
			// the player's MingLiU / SimSun before both, by its family in the
			// collection (here a Liberation Mono face after a Liberation Sans
			// one named PMingLiU / NSimSun), at its own ascent; the other
			// release's file is not read
			Common::Array<Common::Array<byte> > other, own;
			other.push_back(renamed(mono, families[1 - k]));
			player->add(players[1 - k], collection(other));
			f.reset(loadCjkSubtitleFont(pages[k], source));
			TS_ASSERT_EQUALS(source, kCjkSubtitleFontSerif);
			own.push_back(renamed(liberation, k ? "NSimSun" : "PMingLiU"));
			own.push_back(renamed(mono, families[k]));
			const Common::Array<byte> ttc = collection(own);
			TS_ASSERT_EQUALS(sfntFaceIndex(ttc.data(), ttc.size(), families[k]), 1);
			TS_ASSERT_EQUALS(sfntFaceIndex(ttc.data(), ttc.size(), k ? "nsimsun" : "pmingliu"), 0);
			TS_ASSERT_EQUALS(sfntFaceIndex(ttc.data(), ttc.size(), families[1 - k]), -1);
			player->add(players[k], ttc);
			f.reset(loadCjkSubtitleFont(pages[k], source));
			TS_ASSERT(f);
			TS_ASSERT_EQUALS(source, kCjkSubtitleFontPlayer);
			if (f) {
				TS_ASSERT_EQUALS(cjkSubtitleYShift(*f, source), 0);
				// 16 px per em: Liberation Mono's advance 1229 / 2048 em (its
				// Sans face's '0' would be 9)
				TS_ASSERT_EQUALS(f->getCharWidth('0'), 10);
				TS_ASSERT_EQUALS(f->getCharWidth('i'), 10);
			}
			SearchMan.remove("zh_font_player");
		}
		// no CJK font for the other code pages
		CjkSubtitleFontSource source;
		TS_ASSERT(!loadCjkSubtitleFont(Common::kWindows1252, source));
		TS_ASSERT_EQUALS(source, kCjkSubtitleFontNone);
	}

	// drawSubtitleRows of the Chinese builds on CHAPI001's first line.
	void test_draw_rows() {
		using namespace ZhFont;
		if (!engineDataInSearchMan()) { TS_WARN("src (dists/engine-data) not configured; skipped"); return; }
		const AtlantisExeBuild *builds[2] = { &atlantisExeBuild(696832, kTwTimestamp),
		                                      &atlantisExeBuild(692736, kCnTimestamp) };
		const char *const goldens[2] = { "video_subtitles_zh_tw.txt", "video_subtitles_zh_cn.txt" };
		const char *const prefixes[2] = { "len 114 -> 2 rows: 72:", "len 118 -> 2 rows: 72:" };
		for (uint k = 0; k < 2; k++) {
			Common::Array<Common::String> rows;
			rows.push_back(Common::String(k ? kCnRow1 : kTwRow1));
			if (!k)
				rows.push_back(Common::String(kTwRow2));
			// the rows are the golden's (decoded with the build's code page)
			if (!AtlantisTest::goldenDir().empty()) {
				Common::Array<Common::String> lines;
				TS_ASSERT(AtlantisTest::loadGoldenLines(goldens[k], lines));
				bool found = false;
				for (uint i = 0; i < lines.size(); i++) {
					if (!lines[i].hasPrefix(prefixes[k]))
						continue;
					found = true;
					const Common::String golden = lines[i].substr(strlen(prefixes[k]));
					Common::String want = Common::U32String(rows[0], builds[k]->conCodePage).encode(Common::kUtf8);
					if (!k)
						want += " | 42:" + Common::U32String(rows[1], builds[k]->conCodePage).encode(Common::kUtf8);
					TSM_ASSERT(goldens[k], k ? golden.hasPrefix(want + " | ") : golden == want);
				}
				TS_ASSERT(found);
			}
			Eng *e = engineOf(*builds[k]);
			const Graphics::PixelFormat fmt(2, 5, 6, 5, 0, 11, 5, 0, 0);
			Graphics::ManagedSurface dst(640, 480, fmt);
			const uint32 bg = fmt.RGBToColor(0x40, 0x80, 0x40), white = fmt.RGBToColor(255, 255, 255),
			             black = 0;
			dst.fillRect(Common::Rect(640, 480), bg);
			e->drawSubtitleRows(dst, rows, 0, 630);
			TS_ASSERT(e->_subtitleTTF);
			TS_ASSERT_EQUALS(e->_subtitleYShift, -5);   // Noto Sans from fonts-cjk.dat
			const uint n = rows.size();
			uint whites = 0;
			bool onlyThree = true, shadowed = true;
			int minX = 640;
			Common::Array<int> rowMaxX(n, -1);
			bool outside = false;
			for (int y = 0; y < 480; y++) {
				for (int x = 0; x < 640; x++) {
					const uint32 c = dst.getPixel(x, y);
					if (c == bg)
						continue;
					if (c != white && c != black) {
						onlyThree = false;
						continue;
					}
					// which row's cell (y .. y + 15, its shadow one lower)
					int row = -1;
					for (uint r = 0; r < n; r++) {
						const int top = 0x1ce - (int)(n - r + 1) * 18;
						if (y >= top && y <= top + 16)
							row = (int)r;
					}
					if (row < 0) {
						outside = true;
						continue;
					}
					if (c != white)
						continue;
					whites++;
					minX = MIN(minX, x);
					rowMaxX[row] = MAX(rowMaxX[row], x);
					const uint32 s = dst.getPixel(x + 1, y + 1);
					if (s != white && s != black)
						shadowed = false;
				}
			}
			TSM_ASSERT(goldens[k], whites > 1000);
			TSM_ASSERT(goldens[k], onlyThree);       // monochrome
			TSM_ASSERT(goldens[k], shadowed);        // the black copy at (+1, +1)
			TSM_ASSERT(goldens[k], !outside);      // nothing outside the rows' cells
			TSM_ASSERT(goldens[k], minX >= 5 && minX < 5 + 16);
			// 36 and 21 characters of 16 px from x 5
			const uint chars[2] = { 36, 21 };
			for (uint r = 0; r < n; r++)
				TSM_ASSERT(goldens[k], rowMaxX[r] > 5 + 16 * ((int)chars[r] - 1) && rowMaxX[r] < 5 + 16 * (int)chars[r]);
			freeEngine(e);
		}
	}

	// The other builds keep Arial / Liberation Sans at 13 px and no y shift.
	void test_other_builds_keep_their_font() {
		using namespace ZhFont;
		if (!engineDataInSearchMan()) { TS_WARN("src (dists/engine-data) not configured; skipped"); return; }
		Eng *e = engineOf(kAtlantisExeBuilds[0]);
		const Graphics::Font *f = e->subtitleFont();
		TS_ASSERT(f && f == e->_subtitleTTF);
		TS_ASSERT_EQUALS(e->_subtitleYShift, 0);
		if (f) {
			// Liberation Sans at 13 px: ascent ceil(1854 x 13 / 2048) = 12
			TS_ASSERT_EQUALS(f->getFontAscent(), 12);
			TS_ASSERT_EQUALS(f->getCharWidth('0'), 7);
		}
		freeEngine(e);
	}
};

#endif
