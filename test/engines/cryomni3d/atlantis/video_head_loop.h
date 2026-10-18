// Verifier "video": the 3D talking head across the loop of its UBB video.
//
// Original: atlantis-cd-en-714240 FUN_0042ee88 step 4, then per frame
// FUN_0044dd10 (the UBB frame index of the loop: 0..30, then 0 again),
// FUN_0044e860 and FUN_0044c9c0, run unmodified under emulation by
// atlantis-re/tools/emu/video_head_loop.py into video_head_loop.txt
// (experiments/2026-10-18-02-head-anim-rest-key.md).  Frame 0 uses the
// first key of each head-animation track; in some *A.3DA files that key
// is a rest pose outside the loop, so the original draws the lower face
// away from the video's face on frame 0 (12 pixels for Meljanz at cam 2,
// 35 for Actyon at cam 3).  kEnhMinorBugFixes draws frame 0 with frame
// 1's pose (DialogHead::setRestKeyFix, decision 0016).
//
// Port: DialogHead (head3d.h), called directly.

#include "atlantis_test_helper.h"
#include "video_test_big.h"
#include "../../../system/null_osystem.h"

#include "cryomni3d/atlantis/head3d.h"

#include <math.h>

namespace VideoHeadLoop {

class BigSource : public CryOmni3D::Atlantis::HeadFileSource {
public:
	explicit BigSource(const VideoTest::Big &big) : _big(big) {}
	bool readFile(const Common::String &name, Common::Array<byte> &out) override {
		uint32 size = 0;
		byte *buf = _big.get(name, size);
		if (!buf)
			return false;
		out.resize(size);
		memcpy(out.data(), buf, size);
		delete[] buf;
		return true;
	}
private:
	const VideoTest::Big &_big;
};

// INITDIAL.TXT rows: persoId unk 3dc A.3da M.3da s3d cam2..cam5 ...
struct Row {
	int id;
	Common::String names[4];   // s3d, 3dc, A.3da, M.3da
};

static void initdialRows(const VideoTest::Big &big, Common::Array<Row> &rows) {
	uint32 size = 0;
	byte *buf = big.get("DIALOG\\INITDIAL.TXT", size);
	if (!buf)
		return;
	Common::String text((const char *)buf, size);
	delete[] buf;
	uint pos = 0;
	while (pos < text.size()) {
		uint end = pos;
		while (end < text.size() && text[end] != '\n')
			end++;
		Common::Array<Common::String> tok;
		Common::String cur;
		for (uint k = pos; k <= end; k++) {
			const char c = k < end ? text[k] : ' ';
			if (c == ' ' || c == '\t' || c == '\r') {
				if (!cur.empty())
					tok.push_back(cur);
				cur.clear();
			} else {
				cur += c;
			}
		}
		pos = end + 1;
		if (!tok.empty() && tok[0].equalsIgnoreCase("FIN"))
			break;
		if (tok.size() < 6 || tok[0].empty() || tok[0][0] < '0' || tok[0][0] > '9')
			continue;
		Row r;
		r.id = atoi(tok[0].c_str());
		r.names[0] = tok[5];
		r.names[1] = tok[2];
		r.names[2] = tok[3];
		r.names[3] = tok[4];
		bool none = false;
		for (int i = 0; i < 4; i++)
			none |= r.names[i].equalsIgnoreCase("none");
		if (!none)
			rows.push_back(r);
	}
}

static const Row *findRow(const Common::Array<Row> &rows, int id) {
	for (uint i = 0; i < rows.size(); i++)
		if (rows[i].id == id)
			return &rows[i];
	return nullptr;
}

static void centroid(const Common::Array<uint16> &buf, double &x, double &y) {
	double n = 0, sx = 0, sy = 0;
	for (uint i = 0; i < buf.size(); i++) {
		if (buf[i]) {
			n++;
			sx += i % 640;
			sy += i / 640;
		}
	}
	if (n == 0)
		n = 1;
	x = sx / n;
	y = sy / n;
}

static double dist(const double *a, const double *b) {
	return sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]));
}

} // End of namespace VideoHeadLoop

class VideoHeadLoopTestSuite : public CxxTest::TestSuite {
public:
	void setUp() override { Common::install_null_g_system(); }
	void tearDown() override { Common::uninstall_null_g_system(); }

	// Two loops of the UBB (frame index 0..30, then 0 and 1) for Meljanz
	// (row 522, cams 2..5) and Actyon (row 523, cams 3 and 4), without the
	// fix: the value FUN_0044e860 returned and the pixels, every frame.
	void test_loop_vs_original() {
		using namespace VideoHeadLoop;
		if (!AtlantisTest::haveData()) {
			TS_WARN("Atlantis data not configured: head loop test skipped");
			return;
		}
		Common::Array<Common::String> lines;
		if (!AtlantisTest::loadGoldenLines("video_head_loop.txt", lines)) {
			TS_FAIL("video_head_loop.txt missing from the configured golden directory");
			return;
		}
		VideoTest::Big big;
		BigSource src(big);
		Common::Array<Row> rows;
		initdialRows(big, rows);
		Common::Array<uint16> buf(640 * 480);
		Common::Array<byte> le(640 * 480 * 2);
		Common::ScopedPtr<CryOmni3D::Atlantis::DialogHead> head;
		int curRow = -1, curCam = -1, checked = 0;
		for (uint l = 0; l < lines.size(); l++) {
			if (lines[l][0] == '#')
				continue;
			int row, cam, k, idx, ms, r;
			char md5[33];
			if (sscanf(lines[l].c_str(), "%d %d %d %d %d %d %32s", &row, &cam, &k, &idx, &ms, &r, md5) != 7) {
				TS_FAIL(lines[l].c_str());
				continue;
			}
			if (row != curRow || cam != curCam) {
				const Row *rr = findRow(rows, row);
				TS_ASSERT(rr);
				if (!rr)
					return;
				head.reset(new CryOmni3D::Atlantis::DialogHead());
				TS_ASSERT(head->load(src, rr->names[0], rr->names[1], rr->names[2], rr->names[3], cam, "ATA0041A.SYC"));
				curRow = row;
				curCam = cam;
			}
			head->setFrame(idx);
			const int got = head->lipsync(ms);
			head->transform();
			memset(buf.data(), 0, buf.size() * 2);
			head->render(buf.data(), 640);
			for (uint i = 0; i < buf.size(); i++)
				WRITE_LE_UINT16(&le[2 * i], buf[i]);
			TSM_ASSERT_EQUALS(lines[l].c_str(), got, r);
			TSM_ASSERT_EQUALS(lines[l].c_str(), AtlantisTest::md5Hex(le.data(), le.size()), Common::String(md5));
			checked++;
		}
		TS_ASSERT_EQUALS(checked, 198);
	}

	// The fix on every head of the INITDIAL table at each camera (neutral
	// mouth, one loop of the animation, ATA0001A.SYC): without it the
	// heads whose first key is a rest pose jump at frame 0 (by 2 pixels or
	// more, and more than their largest step elsewhere in the loop); with
	// it frame 0 is drawn as frame 1 and no head moves between frames 30,
	// 0 and 1 by more than a pixel over its largest step.
	void test_rest_key_fix_every_head() {
		using namespace VideoHeadLoop;
		if (!AtlantisTest::haveData()) {
			TS_WARN("Atlantis data not configured: head loop test skipped");
			return;
		}
		VideoTest::Big big;
		BigSource src(big);
		Common::Array<Row> rows;
		initdialRows(big, rows);
		Common::Array<uint16> buf(640 * 480), f0(640 * 480);
		Common::Array<Common::String> jumped;
		int heads = 0;
		for (uint ri = 0; ri < rows.size(); ri++) {
			for (int cam = 2; cam <= 5; cam++) {
				double c[2][31][2];
				bool frame0IsFrame1 = false;
				bool ok = true;
				for (int fix = 0; fix < 2 && ok; fix++) {
					CryOmni3D::Atlantis::DialogHead head;
					if (!head.load(src, rows[ri].names[0], rows[ri].names[1], rows[ri].names[2], rows[ri].names[3], cam, "ATA0001A.SYC")) {
						ok = false;
						break;
					}
					head.setRestKeyFix(fix == 1);
					for (int f = 0; f < 31; f++) {
						head.setFrame(f);
						head.neutral();
						head.transform();
						memset(buf.data(), 0, buf.size() * 2);
						head.render(buf.data(), 640);
						centroid(buf, c[fix][f][0], c[fix][f][1]);
						if (fix == 1 && f == 0)
							f0 = buf;
						if (fix == 1 && f == 1)
							frame0IsFrame1 = f0 == buf;
					}
				}
				if (!ok)
					continue;
				heads++;
				double step = 0;
				for (int i = 1; i < 30; i++)
					step = MAX(step, dist(c[0][i], c[0][i + 1]));
				const double jump = MIN(dist(c[0][0], c[0][1]), dist(c[0][30], c[0][0]));
				Common::String a3da = rows[ri].names[2];
				a3da.toUppercase();
				if (jump >= 2.0 && jump > step) {
					bool seen = false;
					for (uint j = 0; j < jumped.size(); j++)
						seen |= jumped[j] == a3da;
					if (!seen)
						jumped.push_back(a3da);
				}
				const Common::String what = Common::String::format("row %d cam %d", rows[ri].id, cam);
				TSM_ASSERT(what.c_str(), frame0IsFrame1);
				TSM_ASSERT(what.c_str(), dist(c[1][30], c[1][0]) <= step + 1.0);
			}
		}
		TS_ASSERT_LESS_THAN_EQUALS(100, heads);
		Common::sort(jumped.begin(), jumped.end());
		Common::String got;
		for (uint j = 0; j < jumped.size(); j++)
			got += (j ? " " : "") + jumped[j];
		TS_ASSERT_EQUALS(got, Common::String("GROAN1A MELJAN1A PECHAN2A RHEAAN1A RPTQAN1A SEVAN1A"));
	}
};
