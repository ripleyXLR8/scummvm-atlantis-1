// "ui" audit: Atlantis detection entry and game title.
// See atlantis-re/audit/ui/REPORT.md, section "Detection and metaengine".
//
// The detection table is plain data (detection_tables.h), included here
// directly.  The metaengine (metaengine.cpp) cannot be linked into the test
// runner (createInstance pulls in the whole engine); it is reviewed statically.

#include "atlantis_test_helper.h"
#include "../../../system/null_osystem.h"

#include "common/archive.h"
#include "common/fs.h"

#include "engines/advancedDetector.h"
#include "cryomni3d/detection.h"
#include "cryomni3d/detection_tables.h"
#include "cryomni3d/atlantis/bigfile.h"

class UiDetectionTestSuite : public CxxTest::TestSuite {
	// ScummVM's AdvancedDetector hashes the first 5000 bytes by default
	// (AdvancedMetaEngineDetection::_md5Bytes; CryOmni3D does not override it).
	static Common::String md5Head(const Common::FSNode &node, int64 &size) {
		Common::File f;
		if (!f.open(node))
			return Common::String();
		size = f.size();
		return Common::computeStreamMD5AsString(f, 5000);
	}

	static Common::FSNode findCaseless(const Common::String &dir, const char *name) {
		Common::FSNode d(Common::Path(dir, '/'));
		Common::FSList list;
		d.getChildren(list, Common::FSNode::kListFilesOnly);
		for (uint i = 0; i < list.size(); i++)
			if (list[i].getName().equalsIgnoreCase(name))
				return list[i];
		return Common::FSNode();
	}

	static Common::String readSource(const char *rel) {
		Common::String src = AtlantisTest::configValue("src");
		Common::File f;
		if (src.empty() || !f.open(Common::FSNode(Common::Path(src + "/" + rel, '/'))))
			return Common::String();
		Common::String text;
		while (!f.eos())
			text += f.readLine() + "\n";
		return text;
	}

public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// Every file of the Atlantis CD entry: size and 5000-byte MD5 against the
	// reference files (atlantis.exe 714240 B full md5 9c850caeb212b3096442222606b3d392).
	void test_cd_entry_matches_reference_files() {
		if (!AtlantisTest::haveData()) { TS_WARN("data not configured; skipped"); return; }
		int checked = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || (g->desc.flags & ADGF_DEMO) ||
			        strcmp(g->desc.extra, "") || g->desc.filesDescriptions[0].fileSize != 714240)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(AtlantisTest::dataDir(), fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				Common::String md5 = md5Head(n, size);
				TSM_ASSERT_EQUALS(fd->fileName, md5, Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
				checked++;
			}
			TS_ASSERT_EQUALS(g->desc.language, Common::EN_ANY);
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
		}
		TS_ASSERT_EQUALS(checked, 2);
	}

	// The English retail CD of May 1997 (atlantis-cd-en-688128): its own
	// executable with the reference's archives.  `en1997` in
	// atlantis_test.cfg names a folder with that atlantis.exe and the four
	// BIGCD*.BIG (atlantis-re binaries/atlantis-cd-en-688128.md).
	void test_en1997_entry_matches_files() {
		const Common::String dir = AtlantisTest::configValue("en1997");
		if (dir.empty()) { TS_WARN("en1997 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			// the Dutch CD's executable has the same size (its own entry)
			if (strcmp(g->desc.gameId, "atlantis") || g->desc.filesDescriptions[0].fileSize != 688128 ||
			    g->desc.language != Common::EN_ANY)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(dir, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 1);
		// The whole executable, as binaries/atlantis-cd-en-688128.md lists it.
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("147a95f74bb75815343f0e57979e342d"));
	}

	// The French retail CD of August 1997 (atlantis-cd-fr-687104): its own
	// executable with the French archives.  `fr1997` in atlantis_test.cfg
	// names a folder with that ATLANTIS.EXE and the four BIGCD*.BIG
	// (atlantis-re binaries/atlantis-cd-fr-687104.md).
	void test_fr1997_entry_matches_files() {
		const Common::String dir = AtlantisTest::configValue("fr1997");
		if (dir.empty()) { TS_WARN("fr1997 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			// the German CD's executable has the same size (its own entry)
			if (strcmp(g->desc.gameId, "atlantis") || g->desc.filesDescriptions[0].fileSize != 687104 ||
			    g->desc.language != Common::FR_FRA)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(dir, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			TS_ASSERT_EQUALS(Common::String(g->desc.extra), "");
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 1);
		// The whole executable, as binaries/atlantis-cd-fr-687104.md lists
		// it (the 350..640 MB archives: their sizes, and BIGCD1's head md5
		// above).
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("099f685e3805ca7ce18376a485d44639"));
		static const struct { const char *name; int64 size; } kBigs[] = {
			{ "BIGCD1.BIG", 636020326 }, { "BIGCD2.BIG", 589253694 },
			{ "BIGCD3.BIG", 637627700 }, { "BIGCD4.BIG", 354358792 } };
		for (uint i = 0; i < ARRAYSIZE(kBigs); i++) {
			Common::File big;
			TSM_ASSERT(kBigs[i].name, big.open(findCaseless(dir, kBigs[i].name)));
			TSM_ASSERT_EQUALS(kBigs[i].name, (int64)big.size(), kBigs[i].size);
		}
	}

	// The German CD of August 1997 (atlantis-cd-de-687104, decision 0021
	// B): its own executable, fr-687104's size, with the German archives.
	// `de1997` in atlantis_test.cfg names a folder with that ATLANTIS.EXE
	// and the four BIGCD*.BIG (atlantis-re binaries/atlantis-cd-de-687104.md).
	void test_de1997_entry_matches_files() {
		const Common::String dir = AtlantisTest::configValue("de1997");
		if (dir.empty()) { TS_WARN("de1997 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || g->desc.filesDescriptions[0].fileSize != 687104 ||
			    g->desc.language != Common::DE_DEU)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(dir, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			TS_ASSERT_EQUALS(Common::String(g->desc.extra), "");
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 1);
		// The whole executable and the four archives' sizes, as the binary
		// note lists them.
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("4674137e782b37a8a77b4c8a17ae1588"));
		static const struct { const char *name; int64 size; } kBigs[] = {
			{ "BIGCD1.BIG", 638118150 }, { "BIGCD2.BIG", 590591758 },
			{ "BIGCD3.BIG", 638720191 }, { "BIGCD4.BIG", 354709470 } };
		for (uint i = 0; i < ARRAYSIZE(kBigs); i++) {
			Common::File big;
			TSM_ASSERT(kBigs[i].name, big.open(findCaseless(dir, kBigs[i].name)));
			TSM_ASSERT_EQUALS(kBigs[i].name, (int64)big.size(), kBigs[i].size);
		}
	}

	// The Dutch CD of June 1997 (atlantis-cd-nl-688128, decision 0022 B1):
	// its own executable, en-688128's size, with the Dutch archives.
	// `nl1997` in atlantis_test.cfg names a folder with that ATLANTIS.EXE and
	// the four BIGCD*.BIG (atlantis-re binaries/atlantis-cd-nl-688128.md).
	void test_nl1997_entry_matches_files() {
		const Common::String dir = AtlantisTest::configValue("nl1997");
		if (dir.empty()) { TS_WARN("nl1997 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || g->desc.filesDescriptions[0].fileSize != 688128 ||
			    g->desc.language != Common::NL_NLD)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(dir, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			TS_ASSERT_EQUALS(Common::String(g->desc.extra), "");
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 1);
		// The whole executable and the four archives' sizes and first 5000
		// bytes, as the binary note lists them.
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("41684f794183eff0421659d3de7d7e37"));
		static const struct { const char *name; int64 size; const char *head; } kBigs[] = {
			{ "BIGCD1.BIG", 633096108, "88bb8611b5bcb0f3cc7087ce6347a274" },
			{ "BIGCD2.BIG", 587451694, "e22d8487be983c85234b73b9d7cab74e" },
			{ "BIGCD3.BIG", 635452202, "3f7571d472bb25399817d6ec3eff4c94" },
			{ "BIGCD4.BIG", 354628561, "01ed853f0f67ddbe5c9e85496135f963" } };
		for (uint i = 0; i < ARRAYSIZE(kBigs); i++) {
			int64 size = -1;
			TSM_ASSERT_EQUALS(kBigs[i].name, md5Head(findCaseless(dir, kBigs[i].name), size), Common::String(kBigs[i].head));
			TSM_ASSERT_EQUALS(kBigs[i].name, size, kBigs[i].size);
		}
	}

	// The Swedish CD of June 1997 (atlantis-cd-sv-688128, decision 0023 B1):
	// its own executable, en-688128's size, with the Swedish archives.
	// `sv1997` in atlantis_test.cfg names a folder with that ATLANTIS.EXE and
	// the four BIGCD*.BIG (atlantis-re binaries/atlantis-cd-sv-688128.md).
	void test_sv1997_entry_matches_files() {
		const Common::String dir = AtlantisTest::configValue("sv1997");
		if (dir.empty()) { TS_WARN("sv1997 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || g->desc.filesDescriptions[0].fileSize != 688128 ||
			    g->desc.language != Common::SV_SWE)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(dir, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			TS_ASSERT_EQUALS(Common::String(g->desc.extra), "");
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 1);
		// The whole executable and the four archives' sizes and first 5000
		// bytes, as the binary note lists them.
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("f429f2a6863f6e9f6d11d9ab03d2b2d9"));
		static const struct { const char *name; int64 size; const char *head; } kBigs[] = {
			{ "BIGCD1.BIG", 635706318, "50c6868f8b959323e4cddb36641fb7dc" },
			{ "BIGCD2.BIG", 589142889, "47631238312dfcd276cae998c1fc0b1c" },
			{ "BIGCD3.BIG", 636401610, "72fa032d9fd0e936fa7b9432ebac4673" },
			{ "BIGCD4.BIG", 355111981, "3384a5256952621331447eb0d754e617" } };
		for (uint i = 0; i < ARRAYSIZE(kBigs); i++) {
			int64 size = -1;
			TSM_ASSERT_EQUALS(kBigs[i].name, md5Head(findCaseless(dir, kBigs[i].name), size), Common::String(kBigs[i].head));
			TSM_ASSERT_EQUALS(kBigs[i].name, size, kBigs[i].size);
		}
	}

	// The French CD's first pressing of May 1997 (atlantis-cd-fr-688640):
	// its own executable with the May French archives.  `frmay1997` in
	// atlantis_test.cfg names a folder with that ATLANTIS.EXE and the four
	// BIGCD*.BIG (atlantis-re binaries/atlantis-cd-fr-688640.md).
	void test_frmay1997_entry_matches_files() {
		const Common::String dir = AtlantisTest::configValue("frmay1997");
		if (dir.empty()) { TS_WARN("frmay1997 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || g->desc.filesDescriptions[0].fileSize != 688640)
				continue;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(dir, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.language, Common::FR_FRA);
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			TS_ASSERT_EQUALS(Common::String(g->desc.extra), "");
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 1);
		// The whole executable and the four archives' sizes, as the binary
		// note lists them (the archives' md5 is too long to compute here).
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("34ffe7e36b75b1836bb314e6258eb427"));
		static const struct { const char *name; int64 size; } kBigs[] = {
			{ "BIGCD1.BIG", 636020222 }, { "BIGCD2.BIG", 589253616 },
			{ "BIGCD3.BIG", 637992061 }, { "BIGCD4.BIG", 354358899 } };
		for (uint i = 0; i < ARRAYSIZE(kBigs); i++) {
			Common::File big;
			TSM_ASSERT(kBigs[i].name, big.open(findCaseless(dir, kBigs[i].name)));
			TSM_ASSERT_EQUALS(kBigs[i].name, (int64)big.size(), kBigs[i].size);
		}
	}

	// The GOG.com entries (one per language) against the files of GOG's
	// installer: `gog` in atlantis_test.cfg names a directory with one
	// sub-directory per language (en-US, fr-FR, de-DE, es-ES, it-IT), as
	// extracted in atlantis-re experiment 2026-10-17-07.
	void test_gog_entries_match_gog_files() {
		const Common::String gog = AtlantisTest::configValue("gog");
		if (gog.empty()) { TS_WARN("gog not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || strcmp(g->desc.extra, "GOG.com"))
				continue;
			const char *sub = g->desc.language == Common::EN_ANY ? "en-US" :
			                  g->desc.language == Common::FR_FRA ? "fr-FR" :
			                  g->desc.language == Common::DE_DEU ? "de-DE" :
			                  g->desc.language == Common::ES_ESP ? "es-ES" :
			                  g->desc.language == Common::IT_ITA ? "it-IT" : "";
			TS_ASSERT(*sub);
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName; fd++) {
				Common::FSNode n = findCaseless(gog + "/" + sub, fd->fileName);
				TSM_ASSERT(fd->fileName, n.exists());
				if (!n.exists())
					continue;
				int64 size = -1;
				TSM_ASSERT_EQUALS(fd->fileName, md5Head(n, size), Common::String(fd->md5));
				TSM_ASSERT_EQUALS(fd->fileName, size, (int64)fd->fileSize);
			}
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			entries++;
		}
		TS_ASSERT_EQUALS(entries, 5);
	}

	// The Atlantis entries of the detection table whose every file is found
	// with its size and 5000-byte MD5, the files of each name taken from
	// exeDir (atlantis.exe) or bigDir (the archives) (an AdvancedDetector full
	// match, case-insensitive names).
	static Common::Array<const CryOmni3D::CryOmni3DGameDescription *> fullMatches(
	        const Common::String &exeDir, const Common::String &bigDir) {
		Common::Array<const CryOmni3D::CryOmni3DGameDescription *> out;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis"))
				continue;
			bool all = true;
			for (const ADGameFileDescription *fd = g->desc.filesDescriptions; fd->fileName && all; fd++) {
				const bool exe = Common::String(fd->fileName).equalsIgnoreCase("atlantis.exe");
				Common::FSNode n = findCaseless(exe ? exeDir : bigDir, fd->fileName);
				int64 size = -1;
				all = n.exists() && md5Head(n, size) == fd->md5 && size == (int64)fd->fileSize;
			}
			if (all)
				out.push_back(g);
		}
		return out;
	}

	// Cryo's patch AT1CDINT (atlantis-patch-fr-714240, atlantis-re
	// experiment 2026-10-18-06): one entry per known CD's archives, the
	// patch executable in each.  `patch101` in atlantis_test.cfg names a
	// folder with that atlantis.exe and the reference's (English CD's)
	// BIGCD*.BIG; the French archives are taken from `fr1997` and
	// `frmay1997`, the German ones from `de1997`, the Dutch ones from
	// `nl1997`, the Swedish ones from `sv1997`, when configured.
	void test_patch101_entries_match_files() {
		const Common::String dir = AtlantisTest::configValue("patch101");
		if (dir.empty()) { TS_WARN("patch101 not configured; skipped"); return; }
		int entries = 0;
		for (const CryOmni3D::CryOmni3DGameDescription *g = CryOmni3D::gameDescriptions; g->desc.gameId; g++) {
			if (strcmp(g->desc.gameId, "atlantis") || strcmp(g->desc.extra, "AT1CDINT patch"))
				continue;
			entries++;
			TS_ASSERT_EQUALS(g->desc.platform, Common::kPlatformWindows);
			const ADGameFileDescription *exe = &g->desc.filesDescriptions[0];
			const ADGameFileDescription *big = &g->desc.filesDescriptions[1];
			TS_ASSERT_EQUALS(Common::String(exe->fileName), "atlantis.exe");
			TS_ASSERT_EQUALS(Common::String(big->fileName), "BIGCD1.BIG");
			int64 size = -1;
			TS_ASSERT_EQUALS(md5Head(findCaseless(dir, "atlantis.exe"), size), Common::String(exe->md5));
			TS_ASSERT_EQUALS(size, (int64)exe->fileSize);
			// the archives: the English CD's, the French CDs' of August
			// and May 1997, the German, Dutch or Swedish CD's
			const Common::String bigDir = g->desc.language == Common::EN_ANY ? dir :
			        !strcmp(big->md5, "f3184be3e2d2a52693d7fc5311b2ed7d") ? AtlantisTest::configValue("fr1997") :
			        !strcmp(big->md5, "24acad3df3d968292ea12ba026d84cd1") ? AtlantisTest::configValue("frmay1997") :
			        !strcmp(big->md5, "08214298a01ac06c7dc23b9f93d2dd1b") ? AtlantisTest::configValue("de1997") :
			        !strcmp(big->md5, "88bb8611b5bcb0f3cc7087ce6347a274") ? AtlantisTest::configValue("nl1997") :
			        !strcmp(big->md5, "50c6868f8b959323e4cddb36641fb7dc") ? AtlantisTest::configValue("sv1997") :
			        Common::String();
			TSM_ASSERT(big->md5, g->desc.language == Common::EN_ANY || g->desc.language == Common::FR_FRA ||
			                     (g->desc.language == Common::DE_DEU && !strcmp(big->md5, "08214298a01ac06c7dc23b9f93d2dd1b")) ||
			                     (g->desc.language == Common::NL_NLD && !strcmp(big->md5, "88bb8611b5bcb0f3cc7087ce6347a274")) ||
			                     (g->desc.language == Common::SV_SWE && !strcmp(big->md5, "50c6868f8b959323e4cddb36641fb7dc")));
			if (bigDir.empty()) {
				TS_WARN(Common::String::format("archives %s not configured", big->md5).c_str());
				continue;
			}
			size = -1;
			TSM_ASSERT_EQUALS(big->md5, md5Head(findCaseless(bigDir, "BIGCD1.BIG"), size), Common::String(big->md5));
			TSM_ASSERT_EQUALS(big->md5, size, (int64)big->fileSize);
		}
		TS_ASSERT_EQUALS(entries, 6);
		Common::File f;
		TS_ASSERT(f.open(findCaseless(dir, "atlantis.exe")));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("e6a49004ea6160566fb01ed2ab6ab034"));
	}

	// Each install matches one entry only: the reference folder its
	// reference entry (the patch entries share its archives and its
	// executable's size, not its executable's head), the patch executable
	// with each CD's archives that CD's patch entry, and the other builds
	// their own entries.
	void test_each_install_matches_one_entry() {
		const Common::String patch = AtlantisTest::configValue("patch101");
		const Common::String fr = AtlantisTest::configValue("fr1997");
		const Common::String frMay = AtlantisTest::configValue("frmay1997");
		const Common::String en97 = AtlantisTest::configValue("en1997");
		const Common::String de = AtlantisTest::configValue("de1997");
		const Common::String nl = AtlantisTest::configValue("nl1997");
		const Common::String sv = AtlantisTest::configValue("sv1997");
		struct Case { Common::String exeDir, bigDir; const char *extra; Common::Language lang; uint32 exeSize; };
		Common::Array<Case> cases;
		if (AtlantisTest::haveData())
			cases.push_back({ AtlantisTest::dataDir(), AtlantisTest::dataDir(), "", Common::EN_ANY, 714240 });
		if (!en97.empty())
			cases.push_back({ en97, en97, "", Common::EN_ANY, 688128 });
		if (!fr.empty())
			cases.push_back({ fr, fr, "", Common::FR_FRA, 687104 });
		if (!frMay.empty())
			cases.push_back({ frMay, frMay, "", Common::FR_FRA, 688640 });
		if (!de.empty())
			cases.push_back({ de, de, "", Common::DE_DEU, 687104 });
		// the French and German executables of August 1997 share a size:
		// each with the other's archives matches nothing
		if (!fr.empty() && !de.empty()) {
			TS_ASSERT(fullMatches(fr, de).empty());
			TS_ASSERT(fullMatches(de, fr).empty());
		}
		if (!nl.empty())
			cases.push_back({ nl, nl, "", Common::NL_NLD, 688128 });
		// the English and Dutch executables of 1997 share a size too
		if (!en97.empty() && !nl.empty()) {
			TS_ASSERT(fullMatches(en97, nl).empty());
			TS_ASSERT(fullMatches(nl, en97).empty());
		}
		if (!sv.empty())
			cases.push_back({ sv, sv, "", Common::SV_SWE, 688128 });
		// and the Swedish one: each with the other's archives matches nothing
		if (!sv.empty()) {
			if (!en97.empty()) {
				TS_ASSERT(fullMatches(en97, sv).empty());
				TS_ASSERT(fullMatches(sv, en97).empty());
			}
			if (!nl.empty()) {
				TS_ASSERT(fullMatches(nl, sv).empty());
				TS_ASSERT(fullMatches(sv, nl).empty());
			}
		}
		if (!patch.empty()) {
			cases.push_back({ patch, patch, "AT1CDINT patch", Common::EN_ANY, 714240 });
			if (!fr.empty())
				cases.push_back({ patch, fr, "AT1CDINT patch", Common::FR_FRA, 714240 });
			if (!frMay.empty())
				cases.push_back({ patch, frMay, "AT1CDINT patch", Common::FR_FRA, 714240 });
			if (!de.empty())
				cases.push_back({ patch, de, "AT1CDINT patch", Common::DE_DEU, 714240 });
			if (!nl.empty())
				cases.push_back({ patch, nl, "AT1CDINT patch", Common::NL_NLD, 714240 });
			if (!sv.empty())
				cases.push_back({ patch, sv, "AT1CDINT patch", Common::SV_SWE, 714240 });
		}
		if (cases.empty()) { TS_WARN("no install configured; skipped"); return; }
		for (uint i = 0; i < cases.size(); i++) {
			const Common::String what = cases[i].exeDir + " + " + cases[i].bigDir;
			Common::Array<const CryOmni3D::CryOmni3DGameDescription *> m = fullMatches(cases[i].exeDir, cases[i].bigDir);
			TSM_ASSERT_EQUALS(what.c_str(), m.size(), 1u);
			if (m.size() != 1)
				continue;
			TSM_ASSERT_EQUALS(what.c_str(), Common::String(m[0]->desc.extra), cases[i].extra);
			TSM_ASSERT_EQUALS(what.c_str(), m[0]->desc.language, cases[i].lang);
			TSM_ASSERT_EQUALS(what.c_str(), m[0]->desc.filesDescriptions[0].fileSize, cases[i].exeSize);
		}
	}

	// The full reference executable (not only its first 5000 bytes).
	void test_reference_exe_full_md5() {
		if (!AtlantisTest::haveData()) { TS_WARN("data not configured; skipped"); return; }
		Common::File f;
		Common::FSNode n = findCaseless(AtlantisTest::dataDir(), "atlantis.exe");
		TS_ASSERT(f.open(n));
		TS_ASSERT_EQUALS(Common::computeStreamMD5AsString(f), Common::String("9c850caeb212b3096442222606b3d392"));
	}

	// The game names itself "ATLANTIS / The Lost Tales" (first two centred
	// lines of SPRLIST\CREDITS.TXT).  Every user-visible title string in the
	// port must say "The Lost Tales".
	void test_title_consistency() {
		if (!AtlantisTest::haveData()) { TS_WARN("data not configured; skipped"); return; }
		SearchMan.addDirectory("ui_det_data", Common::FSNode(Common::Path(AtlantisTest::dataDir(), '/')));
		CryOmni3D::Atlantis::BigFileArchive big;
		bool opened = big.open(Common::Path("BIGCD1.BIG"));
		SearchMan.remove("ui_det_data");
		TS_ASSERT(opened);
		if (!opened)
			return;
		Common::SeekableReadStream *s = big.createReadStreamForMember(Common::String("SPRLIST\\CREDITS.TXT"));
		TS_ASSERT(s);
		if (!s)
			return;
		Common::Array<Common::String> centred;
		while (!s->eos() && centred.size() < 2) {
			Common::String line = s->readLine();
			if (line.hasPrefix("<>"))
				centred.push_back(line.substr(2));
		}
		delete s;
		TS_ASSERT_EQUALS(centred.size(), 2u);
		if (centred.size() < 2)
			return;
		TS_ASSERT_EQUALS(centred[0], Common::String("ATLANTIS"));
		TS_ASSERT_EQUALS(centred[1], Common::String("The Lost Tales"));

		static const char *const files[] = {
			"engines/cryomni3d/configure.engine",
			"engines/cryomni3d/detection.cpp",
			"engines/cryomni3d/detection_tables.h",
			"engines/cryomni3d/metaengine.cpp",
			"engines/cryomni3d/atlantis/engine.h" };
		for (uint i = 0; i < ARRAYSIZE(files); i++) {
			Common::String text = readSource(files[i]);
			if (text.empty()) { TS_WARN(Common::String::format("%s not readable (src=)", files[i]).c_str()); continue; }
			int bad = 0;
			for (const char *p = strstr(text.c_str(), "Lost Tale"); p; p = strstr(p + 1, "Lost Tale"))
				if (p[9] != 's')
					bad++;
			TSM_ASSERT_EQUALS(Common::String::format("'Lost Tale' without 's' in %s", files[i]).c_str(), bad, 0);
		}
	}

	// The demo entry (ATLANTIS.UBB) cannot be checked: no demo files here.
	void test_demo_entry_not_verifiable() {
		TS_WARN("Atlantis demo entry (ATLANTIS.UBB f5b41b85..., 106611456 B) NOT VERIFIABLE: no demo files available");
	}
};
