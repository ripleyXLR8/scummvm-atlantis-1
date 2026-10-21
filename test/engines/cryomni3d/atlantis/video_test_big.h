#ifndef TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_TEST_BIG_H
#define TEST_ENGINES_CRYOMNI3D_ATLANTIS_VIDEO_TEST_BIG_H

// Verifier "video": minimal, port-independent reader of BIGCD1..4.BIG, the
// same layout as atlantis-re/tools/emu/video_big.py (entry count at +16,
// data base at +24, then 32-byte records followed by the name; file data is
// stored back to back from the data base). First archive wins for duplicates.

#include "atlantis_test_helper.h"

#include "common/array.h"

namespace VideoTest {

struct BigEntry {
	int cd;
	uint32 offset;
	uint32 size;
};

class Big {
public:
	// The archives of `dir` (the reference's, `data`, by default).
	explicit Big(const Common::String &dir = AtlantisTest::dataDir()) : _dir(dir) {
		for (int cd = 1; cd <= 4; cd++) {
			Common::File f;
			if (!f.open(archive(cd)))
				continue;
			f.seek(16);
			uint32 n = f.readUint32LE();
			f.seek(24);
			uint32 base = f.readUint32LE();
			f.seek(32);
			uint32 seq = 0;
			for (uint32 i = 0; i < n; i++) {
				uint32 rec[8];
				for (int k = 0; k < 8; k++)
					rec[k] = f.readUint32LE();
				Common::String name;
				for (uint32 k = 0; k < rec[0]; k++) {
					char c = (char)f.readByte();
					if (c)
						name += c;
				}
				name.toUppercase();
				if (!_entries.contains(name)) {
					BigEntry e = { cd, base + seq, rec[2] };
					_entries[name] = e;
					_names.push_back(name);
				}
				seq += rec[2];
			}
		}
	}

	bool empty() const { return _entries.empty(); }

	Common::Array<Common::String> names(const char *suffix) const {
		Common::Array<Common::String> out;
		for (uint i = 0; i < _names.size(); i++)
			if (_names[i].hasSuffix(suffix))
				out.push_back(_names[i]);
		return out;
	}

	// Caller owns the returned buffer (delete[]).
	byte *get(const Common::String &name, uint32 &size) const {
		if (!_entries.contains(name))
			return nullptr;
		const BigEntry &e = _entries[name];
		Common::File f;
		if (!f.open(archive(e.cd)))
			return nullptr;
		f.seek(e.offset);
		byte *buf = new byte[e.size];
		f.read(buf, e.size);
		size = e.size;
		return buf;
	}

private:
	// BIGCD<cd>.BIG in any case: the Chinese discs name some of them in
	// lower case (`bigcd2.big`; all four on the Simplified Chinese discs).
	Common::FSNode archive(int cd) const {
		const Common::String name = Common::String::format("BIGCD%d.BIG", cd);
		Common::FSNode dir(Common::Path(_dir, '/'));
		Common::FSList list;
		if (dir.getChildren(list, Common::FSNode::kListFilesOnly))
			for (uint i = 0; i < list.size(); i++)
				if (list[i].getName().equalsIgnoreCase(name))
					return list[i];
		return dir.getChild(name);
	}

	Common::String _dir;
	Common::HashMap<Common::String, BigEntry> _entries;
	Common::Array<Common::String> _names;
};

} // End of namespace VideoTest

#endif
