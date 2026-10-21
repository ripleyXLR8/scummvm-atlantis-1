/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "cryomni3d/atlantis/subtitle_font.h"

#include "common/archive.h"
#include "common/array.h"
#include "common/compression/unzip.h"
#include "common/config-manager.h"
#include "common/endian.h"
#include "common/fs.h"
#include "common/memstream.h"
#include "common/path.h"
#include "common/ptr.h"
#include "common/stream.h"
#include "common/ustr.h"
#include "common/util.h"

#include "graphics/font.h"
#include "graphics/fonts/ttf.h"

namespace CryOmni3D {
namespace Atlantis {

const CjkSubtitleFaces *cjkSubtitleFaces(Common::CodePage codePage) {
	static const CjkSubtitleFaces kTraditional = {
		"mingliu.ttc", "MingLiU",
		{ "NotoSerifTC-Regular.otf", "NotoSerifTC-Regular.ttf" },
		"NotoSerifTC-Subset.otf",
		"NotoSansTC-Regular.otf"
	};
	static const CjkSubtitleFaces kSimplified = {
		"simsun.ttc", "SimSun",
		{ "NotoSerifSC-Regular.otf", "NotoSerifSC-Regular.ttf" },
		"NotoSerifSC-Subset.otf",
		"NotoSansSC-Regular.otf"
	};
	switch (codePage) {
	case Common::kWindows950:
		return &kTraditional;
	case Common::kWindows936:
		return &kSimplified;
	default:
		return nullptr;
	}
}

static uint32 be32At(const byte *data, uint32 size, uint32 off) {
	return off <= size && size - off >= 4 ? READ_BE_UINT32(data + off) : 0;
}

static uint16 be16At(const byte *data, uint32 size, uint32 off) {
	return off <= size && size - off >= 2 ? READ_BE_UINT16(data + off) : 0;
}

// The offset tables of the faces of `data`: a collection's (`ttcf`, its
// numFonts offsets), else the file's own at 0.
static Common::Array<uint32> sfntFaceOffsets(const byte *data, uint32 size) {
	Common::Array<uint32> faces;
	if (size < 12)
		return faces;
	if (READ_BE_UINT32(data) == MKTAG('t', 't', 'c', 'f')) {
		const uint32 n = be32At(data, size, 8);
		for (uint32 i = 0; i < n && 12 + 4 * i + 4 <= size; i++)
			faces.push_back(READ_BE_UINT32(data + 12 + 4 * i));
	} else {
		faces.push_back(0);
	}
	return faces;
}

// The table `tag` of the face whose offset table is at `face`: false if
// absent or out of the data.
static bool sfntTable(const byte *data, uint32 size, uint32 face, uint32 tag, uint32 &off, uint32 &len) {
	const uint n = be16At(data, size, face + 4);
	for (uint t = 0; t < n; t++) {
		const uint32 rec = face + 12 + 16 * t;
		if (be32At(data, size, rec) != tag)
			continue;
		off = be32At(data, size, rec + 8);
		len = be32At(data, size, rec + 12);
		return off <= size && len <= size - off;
	}
	return false;
}

int sfntFaceIndex(const byte *data, uint32 size, const char *family) {
	const Common::Array<uint32> faces = sfntFaceOffsets(data, size);
	for (uint i = 0; i < faces.size(); i++) {
		uint32 off, len;
		if (!sfntTable(data, size, faces[i], MKTAG('n', 'a', 'm', 'e'), off, len) || len < 6)
			continue;
		const byte *name = data + off;
		const uint count = READ_BE_UINT16(name + 2), strings = READ_BE_UINT16(name + 4);
		for (uint r = 0; r < count && 6 + 12 * r + 12 <= len; r++) {
			const byte *rec = name + 6 + 12 * r;
			const uint platform = READ_BE_UINT16(rec), id = READ_BE_UINT16(rec + 6);
			const uint32 slen = READ_BE_UINT16(rec + 8), soff = strings + READ_BE_UINT16(rec + 10);
			if (id != 1 || soff > len || slen > len - soff)
				continue;
			Common::String s;
			if (platform == 0 || platform == 3) {
				// UTF-16BE: an ASCII family compares on its low bytes
				bool ascii = true;
				for (uint32 c = 0; c + 1 < slen; c += 2) {
					ascii &= name[soff + c] == 0 && name[soff + c + 1] < 0x80;
					s += (char)name[soff + c + 1];
				}
				if (!ascii)
					continue;
			} else if (platform == 1) {
				s = Common::String((const char *)name + soff, slen);
			}
			if (!s.empty() && s.equalsIgnoreCase(family))
				return (int)i;
		}
	}
	return -1;
}

byte *sfntFace(const byte *data, uint32 size, uint index, uint32 &outSize) {
	outSize = 0;
	const Common::Array<uint32> faces = sfntFaceOffsets(data, size);
	if (index >= faces.size())
		return nullptr;
	const uint32 face = faces[index];
	const uint n = be16At(data, size, face + 4);
	if (!n || face > size || size - face < 12 + 16 * n)
		return nullptr;
	uint32 total = 12 + 16 * n;
	for (uint t = 0; t < n; t++) {
		const uint32 rec = face + 12 + 16 * t;
		const uint32 off = READ_BE_UINT32(data + rec + 8), len = READ_BE_UINT32(data + rec + 12);
		if (off > size || len > size - off)
			return nullptr;
		total += (len + 3) & ~3u;
	}
	byte *out = (byte *)calloc(total, 1);
	if (!out)
		return nullptr;
	memcpy(out, data + face, 12);
	uint32 at = 12 + 16 * n;
	for (uint t = 0; t < n; t++) {
		const byte *rec = data + face + 12 + 16 * t;
		const uint32 off = READ_BE_UINT32(rec + 8), len = READ_BE_UINT32(rec + 12);
		memcpy(out + 12 + 16 * t, rec, 8);   // tag, checksum
		WRITE_BE_UINT32(out + 12 + 16 * t + 8, at);
		WRITE_BE_UINT32(out + 12 + 16 * t + 12, len);
		memcpy(out + at, data + off, len);
		at += (len + 3) & ~3u;
	}
	outSize = total;
	return out;
}

#ifdef USE_FREETYPE2
// The member `name` of the zip `archiveName` as a font: the archive from the
// extra path first, then SearchMan, as Graphics::loadTTFFontFromArchive
// finds fonts.dat; the member read whole, so the archive is closed here.
static Graphics::Font *loadFontFromZip(const char *archiveName, const char *name) {
	Common::SeekableReadStream *zipStream = nullptr;
	if (ConfMan.hasKey("extrapath")) {
		Common::FSDirectory extrapath(ConfMan.getPath("extrapath"));
		zipStream = extrapath.createReadStreamForMember(Common::Path(archiveName));
	}
	if (!zipStream)
		zipStream = SearchMan.createReadStreamForMember(Common::Path(archiveName));
	Common::ScopedPtr<Common::Archive> zip(Common::makeZipArchive(zipStream));
	if (!zip)
		return nullptr;
	Common::ScopedPtr<Common::SeekableReadStream> member(
	    zip->createReadStreamForMember(Common::Path(name, Common::Path::kNoSeparator)));
	if (!member)
		return nullptr;
	Common::SeekableReadStream *copy = member->readStream((uint32)member->size());
	if (!copy || copy->size() != member->size()) {
		delete copy;
		return nullptr;
	}
	return Graphics::loadTTFFont(copy, DisposeAfterUse::YES, kCjkSubtitleEmPx, Graphics::kTTFSizeModeCharacter,
	                             0, 0, Graphics::kTTFRenderModeMonochrome);
}
#endif

Graphics::Font *loadCjkSubtitleFont(Common::CodePage codePage, CjkSubtitleFontSource &source) {
	source = kCjkSubtitleFontNone;
	const CjkSubtitleFaces *faces = cjkSubtitleFaces(codePage);
	if (!faces)
		return nullptr;
	Graphics::Font *font = nullptr;
#ifdef USE_FREETYPE2
	// 1. The player's face: the collection's face whose family is MingLiU /
	// SimSun, by name as CreateFontA picks a face.
	if (Common::SeekableReadStream *s = SearchMan.createReadStreamForMember(Common::Path(faces->playerFile))) {
		Common::Array<byte> file((uint)s->size());
		const bool read = s->read(file.data(), file.size()) == file.size();
		delete s;
		const int index = read ? sfntFaceIndex(file.data(), file.size(), faces->playerFamily) : -1;
		uint32 faceSize = 0;
		byte *face = index >= 0 ? sfntFace(file.data(), file.size(), (uint)index, faceSize) : nullptr;
		file.clear();
		if (face)
			font = Graphics::loadTTFFont(new Common::MemoryReadStream(face, faceSize, DisposeAfterUse::YES),
			                             DisposeAfterUse::YES, kCjkSubtitleEmPx, Graphics::kTTFSizeModeCharacter,
			                             0, 0, Graphics::kTTFRenderModeMonochrome);
		if (font) {
			source = kCjkSubtitleFontPlayer;
			return font;
		}
	}
	// 2. Noto Serif beside the game files, then in the fonts archives, then
	// the subset in fonts-atlantis.dat.
	for (uint i = 0; i < ARRAYSIZE(faces->serifFiles) && !font; i++) {
		if (Common::SeekableReadStream *s = SearchMan.createReadStreamForMember(Common::Path(faces->serifFiles[i])))
			font = Graphics::loadTTFFont(s, DisposeAfterUse::YES, kCjkSubtitleEmPx, Graphics::kTTFSizeModeCharacter,
			                             0, 0, Graphics::kTTFRenderModeMonochrome);
	}
	for (uint i = 0; i < ARRAYSIZE(faces->serifFiles) && !font; i++)
		font = Graphics::loadTTFFontFromArchive(faces->serifFiles[i], kCjkSubtitleEmPx,
		                                        Graphics::kTTFSizeModeCharacter, 0, 0,
		                                        Graphics::kTTFRenderModeMonochrome);
	if (!font)
		font = loadFontFromZip(kCjkSubtitleBundledArchive, faces->bundledFile);
	if (font) {
		source = kCjkSubtitleFontSerif;
		return font;
	}
	// 3. Noto Sans from fonts-cjk.dat.
	font = Graphics::loadTTFFontFromArchive(faces->sansFile, kCjkSubtitleEmPx, Graphics::kTTFSizeModeCharacter,
	                                        0, 0, Graphics::kTTFRenderModeMonochrome);
	if (font)
		source = kCjkSubtitleFontSans;
#endif
	return font;
}

int cjkSubtitleYShift(const Graphics::Font &font, CjkSubtitleFontSource source) {
	if (source != kCjkSubtitleFontSerif && source != kCjkSubtitleFontSans)
		return 0;
	return kCjkSubtitleNotoAscent - font.getFontAscent();
}

} // End of namespace Atlantis
} // End of namespace CryOmni3D
