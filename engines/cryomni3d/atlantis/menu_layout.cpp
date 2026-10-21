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

#include "cryomni3d/atlantis/menu_layout.h"

#include "common/str-enc.h"
#include "common/textconsole.h"
#include "common/ustr.h"
#include "common/util.h"

namespace CryOmni3D {
namespace Atlantis {

// Parse one ASCII integer starting at *p, advance *p past it.
// Empty (no digits) → returns false; *outVal untouched.
static bool consumeInt(const char *&p, int &outVal) {
	bool neg = false;
	if (*p == '-') { neg = true; ++p; }
	if (*p < '0' || *p > '9')
		return false;
	int v = 0;
	while (*p >= '0' && *p <= '9') {
		v = v * 10 + (*p - '0');
		++p;
	}
	outVal = neg ? -v : v;
	return true;
}

static bool startsWithIgnoreCase(const Common::String &s, const char *prefix) {
	uint n = strlen(prefix);
	if (s.size() < n) return false;
	for (uint i = 0; i < n; i++) {
		char a = s[i], b = prefix[i];
		if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
		if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
		if (a != b) return false;
	}
	return true;
}

bool MenuLayout::loadFromStream(Common::ReadStream &s) {
	items.clear();
	raw.clear();
	walk.records.clear();
	walk.ops.clear();

	// Read the whole stream into a heap buffer.  Menu files are tiny
	// (< 500 bytes typical, < 5 KB for credits), so this is fine.
	Common::Array<byte> buf;
	byte chunk[256];
	while (!s.eos()) {
		uint32 got = s.read(chunk, sizeof(chunk));
		if (got == 0)
			break;
		for (uint32 i = 0; i < got; i++)
			buf.push_back(chunk[i]);
	}
	raw = buf;
	buf.push_back(0); // null-terminate so c_str scans are safe.

	const char *cur = (const char *)buf.data();
	while (*cur) {
		// Extract one line (terminator-tolerant).
		Common::String line;
		while (*cur && *cur != '\n' && *cur != '\r') {
			line += *cur;
			++cur;
		}
		while (*cur == '\n' || *cur == '\r') ++cur;

		if (line.empty())
			continue;

		// /fin terminator (case-insensitive).
		if (startsWithIgnoreCase(line, "/fin"))
			break;

		// /spr=I,HX,HY,W,H
		if (startsWithIgnoreCase(line, "/spr=")) {
			const char *p = line.c_str() + 5;
			int idx, hx, hy, w, h;
			if (!consumeInt(p, idx)) continue;
			if (*p != ',') continue;
			++p;
			if (!consumeInt(p, hx))  continue;
			if (*p != ',') continue;
			++p;
			if (!consumeInt(p, hy))  continue;
			if (*p != ',') continue;
			++p;
			if (!consumeInt(p, w))   continue;
			if (*p != ',') continue;
			++p;
			if (!consumeInt(p, h))   continue;
			MenuItem it;
			it.kind = MenuItem::kSprite;
			it.sprIdx = idx;
			it.hx = hx;
			it.hy = hy;
			it.w = w;
			it.h = h;
			items.push_back(it);
			continue;
		}

		// <X,Y>TEXT — X may be empty meaning "centered".
		// The original game encodes the empty form as a literal "<>", then
		// continues with ",Y>TEXT" (so the full line is e.g. "<>,230>LOAD
		// GAME").  Confirmed against shipped MAINMENU.TXT / SELEMENU.TXT.
		if (line[0] == '<') {
			const char *p = line.c_str() + 1;
			bool centerX = false;
			int  ax = 0, ay = 0;
			if (*p == '>') {
				centerX = true;
				++p;
			} else if (*p == ',') {
				// Legacy/permissive: bare "<,Y>" with no opening number.
				centerX = true;
			} else {
				if (!consumeInt(p, ax))
					continue;
			}
			if (*p != ',') continue;
			++p;
			if (!consumeInt(p, ay)) continue;
			if (*p != '>') continue;
			++p;

			MenuItem it;
			it.kind    = MenuItem::kText;
			it.centerX = centerX;
			it.anchorX = ax;
			it.anchorY = ay;
			it.text    = Common::String(p);
			it.rawLen  = (int)it.text.size();
			// Strip stray trailing whitespace.
			while (!it.text.empty()) {
				char c = it.text[it.text.size() - 1];
				if (c == ' ' || c == '\t')
					it.text.deleteLastChar();
				else
					break;
			}

			// A "//A&&B" value is a toggle: split the &&-separated option
			// labels into MenuItem::options and mark the item as kToggle.
			if (it.text.hasPrefix("//") && it.text.contains("&&")) {
				const Common::String body(it.text.c_str() + 2);
				uint segStart = 0;
				for (uint q = 0; q < body.size(); ) {
					if (q + 1 < body.size() &&
					    body[q] == '&' && body[q + 1] == '&') {
						it.options.push_back(
						    Common::String(body.c_str() + segStart, q - segStart));
						q += 2;
						segStart = q;
					} else {
						++q;
					}
				}
				it.options.push_back(Common::String(body.c_str() + segStart));
				it.kind = MenuItem::kToggle;
				it.text.clear();
			}

			items.push_back(it);
			continue;
		}

		// Unknown directive — silently skip (credits use $NN section markers,
		// \ italics, etc.).
	}

	return true;
}

int menuVolumeFromKnob(int x) {
	int a = (int)(((double)(x - 305)) / 213.0 * 127.0);
	return CLIP(a, 0, 127);
}

void subtitleToggleClick(bool &on, uint8 &counter) {
	on = !on;
	counter++;
	if (counter == 2)
		on = false;
	if (counter == 3)
		counter = 0;
}

bool omniToggleClick(MenuItem &tog) {
	if (!tog.options.empty())
		tog.selected = (tog.selected + 1) % (int)tog.options.size();
	return tog.selected == 1;
}

// FONTMIN advances, 0x00496697 (atlantis-cd-en-688128 and
// atlantis-cd-fr-687104: 0x00486670), 101 bytes.
static const uint8 kFontMinAdvance[101] = {
	15, 8,13, 6,13,20,17, 6,13,20,11, 6, 4,20,15,12,17, 8,16,16,
	14,13,15,13,16,15, 7, 6, 6, 6, 6,20, 6,17,19,18,19,16,16,19,
	18,12,12,17,16,24,20,23,17,22,17,15,19,20,17,23,22,18,17, 7,
	16, 6,10, 6, 7,16,16,15,15,16,12,16,17, 8, 9,16, 7,19,16,17,
	16,15,13,12,10,17,15,19,17,15,14,16, 6,17,11,16,17,17,15,16,
	15
};

// The reference's credits advances 0x004966fc (FONTMIN + 101), bytes
// 0x00..0x7a: what a FONTMIN index 101..223 reads there.
static const uint8 kRefCreditAdvance[123] = {
	16,16,16,15,15,15,11,10,10,18,18,16,17,17,17,17,15,23,19,15,
	19, 0,21,17,16,10,17,17,10,20,14,14,13, 3, 5, 8, 8,12,10, 3,
	 5, 5, 6, 7, 4, 5, 3, 6, 8, 5, 7, 8, 8, 7, 7, 7, 8, 7, 3, 3,
	 8, 7, 8, 7,11, 9, 7,10, 9, 7, 6,12, 8, 4, 7, 8, 5,12, 8,11,
	 6,11, 7, 7, 7, 8, 9,12, 9, 9, 7, 4, 7, 5, 8, 7, 4, 9, 9, 8,
	 9, 9, 5, 9, 7, 3, 4, 8, 3,13, 8, 8, 9, 8, 5, 7, 6, 7, 8,12,
	 8, 8, 7
};

int fontMinAdvance(uint idx, const byte *creditWidths) {
	if (idx < ARRAYSIZE(kFontMinAdvance))
		return kFontMinAdvance[idx];
	const uint c = idx - ARRAYSIZE(kFontMinAdvance);
	if (creditWidths)
		return c < 256 ? creditWidths[c] : 0;
	return c < ARRAYSIZE(kRefCreditAdvance) ? kRefCreditAdvance[c] : 0;
}

Common::String sprTextSharpSAsSs(const Common::String &text) {
	Common::String out;
	for (uint i = 0; i < text.size(); i++) {
		if ((byte)text[i] == kSprTextSharpS)
			out += "ss";
		else
			out += text[i];
	}
	return out;
}

void fontMinBlits(FontMinRenderer r, const Common::String &textIn, int x, int y,
                  Common::Array<FontMinBlit> &out, bool hashRule, const byte *creditWidths,
                  bool sharpSAsSs) {
	const Common::String text = sharpSAsSs ? sprTextSharpSAsSs(textIn) : textIn;
	out.clear();
	int cx = x;
	for (uint i = 0; i < text.size(); i++) {
		const byte c = (byte)text[i];
		if (r != kFontMinLoadList && c == ' ') {
			cx += 20;
			continue;
		}
		if (r == kFontMinPlayerList && hashRule && c == '#')
			continue;
		const uint8 idx = (c < 0x20) ? 0x20 : (uint8)(c - 0x20);
		if (r == kFontMinLoadList && idx == 0x20) {
			cx += 6;
			continue;
		}
		FontMinBlit b = { cx, y, idx };
		out.push_back(b);
		cx += (r == kFontMinNameField && idx == 0x2e) ? 20 : fontMinAdvance(idx, creditWidths);
	}
}

int fontMinLoadListWidth(const Common::String &textIn, const byte *creditWidths, bool sharpSAsSs) {
	const Common::String text = sharpSAsSs ? sprTextSharpSAsSs(textIn) : textIn;
	// FUN_00420b48 @ 0x00420c57..0x00420c7e: width[target] += the advance of
	// each byte, 6 for glyph 0x20.
	int w = 0;
	for (uint i = 0; i < text.size(); i++) {
		const byte c = (byte)text[i];
		const uint8 idx = (c < 0x20) ? 0x20 : (uint8)(c - 0x20);
		w += (idx == 0x20) ? 6 : fontMinAdvance(idx, creditWidths);
	}
	return w;
}

void loadPickerScrollInit(int count, LoadPickerScroll &s, bool knobAtBar) {
	s.offset = 0;
	s.knobY = knobAtBar ? 0x7b : 0;
	s.knobX = 0;
	if (count <= 11) {
		s.knobX = (int16)0xffce;
		s.dragOn = false;
		s.step = 1;
	} else {
		s.dragOn = true;
		s.step = 0xd7 / (count - 10);
	}
}

void loadPickerScrollFrame(LoadPickerScroll &s, bool trackLatched, bool buttonDown, int mouseY,
                           bool clamp) {
	if (!trackLatched || !buttonDown || !s.dragOn)
		return;
	if (mouseY <= 0x7b || mouseY >= 0x152)
		return;
	s.knobY = (uint16)(mouseY - 10);
	s.offset = (uint8)((s.knobY - 0x7b) / s.step);
	if (clamp && mouseY - 10 < 0x7b) {
		s.knobY = 0x7b;
		s.offset = 0;
	}
}

int menuGridHit(int x0, int y0, int cellW, int cellH, int cols, int rows, bool colMajor,
                int mx, int my) {
	if (mx < x0 || mx > x0 + cols * cellW || my < y0 || my > y0 + rows * cellH)
		return -1;
	const int col = (mx - x0) / cellW, row = (my - y0) / cellH;
	return colMajor ? col * rows + row : row * cols + col;
}

// FONTMAX *measuring* width table (atlantis.exe @ 0x4965ee, glyph idx = char).
// The original uses it only to centre "<>,Y>" labels and size their hit rects
// (FUN_004206c8); the same table advances the pen while drawing (0x004202cc).
const uint8 kFontMaxMeasureWidths[137] = {
	22,10,15,15,16,24,20, 8,16,24,13, 8, 8,24, 7,14,20, 9,19,19,
	17,16,18,17,20,18, 7, 8, 8, 8, 8,18, 8,21,23,21,23,19,20,22,
	22,14,15,20,19,28,24,28,20,27,21,19,24,23,21,28,26,22,21, 8,
	19, 8,11, 8, 8,18,19,17,19,18,14,19,19,10,11,18,10,23,20,20,
	19,19,15,15,12,20,18,23,20,19,17,18, 7,20,13,18,21,20,18,18,
	18,18,18,19,18,18,18,13,11,13,21,21,19,26,32,20,20,20,20,20,
	19,28,23,17,24, 0,25,20,18,13,20,20,20,24, 2,12,16
};

int fontMaxMeasureWidth(const Common::String &text) {
	int w = 0;
	for (uint i = 0; i < text.size(); ++i) {
		int idx = (int)(unsigned char)text[i] - 0x20;
		if (idx >= 0 && idx < (int)ARRAYSIZE(kFontMaxMeasureWidths))
			w += kFontMaxMeasureWidths[idx];
	}
	return w;
}

// Exe-exact menu text hit rect (FUN_0041fb6c).  A text record's clickable box
// is X-11 / Y-28, height 28; width is the measured string for centred items
// but 22 px per raw spec character for positioned ones (16 in
// atlantis-cd-it-687104: `shl ebx,4` @ 0x0041f068).  The original tests
// strict inside (left < x < left+W), which a Common::Rect reproduces with its
// bounds shrunk by one on the left/top.
Common::Rect menuTextHitRect(const MenuItem &it, int drawX, int posItemByteWidth) {
	int w;
	if (it.centerX)
		w = fontMaxMeasureWidth(it.text);
	else
		w = it.rawLen * posItemByteWidth;
	int left = drawX - 11;
	int top  = it.anchorY - 28;
	return Common::Rect(left + 1, top + 1, left + w, top + 28);
}

int menuTextDrawX(const MenuItem &it, const Common::String &label) {
	return it.centerX ? (640 - fontMaxMeasureWidth(label)) / 2 : it.anchorX;
}

void sprFontPen(const Common::String &text, int anchorX, int spaceAdvance,
                const Common::Array<int> &glyphWidths, const uint8 *advanceTable,
                uint advanceCount, Common::Array<SprFontGlyph> &out) {
	out.clear();
	int cx = anchorX;
	for (uint i = 0; i < text.size(); ++i) {
		const unsigned char c = (unsigned char)text[i];
		if (c == ' ') {
			cx += spaceAdvance;
			continue;
		}
		const int idx = (int)c - 0x20;
		if (idx < 0 || idx >= (int)glyphWidths.size())
			continue;
		SprFontGlyph g;
		g.x = cx;
		g.idx = idx;
		out.push_back(g);
		cx += (advanceTable && (uint)idx < advanceCount) ? advanceTable[idx] : glyphWidths[idx];
	}
}

namespace {

// FUN_00451b7a (atoi): leading spaces, a sign, digits.
int walkAtoi(const byte *t, uint32 size, uint32 p) {
	while (p < size && (t[p] == ' ' || t[p] == '\t'))
		p++;
	bool neg = false;
	if (p < size && (t[p] == '-' || t[p] == '+'))
		neg = (t[p++] == '-');
	int v = 0;
	while (p < size && t[p] >= '0' && t[p] <= '9')
		v = v * 10 + (t[p++] - '0');
	return neg ? -v : v;
}

// The digit steps of FUN_0041fb6c / FUN_004207c0: base, + 1 above 9,
// + 1 above 99 (a 4-digit or negative number desynchronises the walk).
uint32 walkStep(uint32 p, int v, int base, bool threeDigits = true) {
	p += base;
	if (v > 9)
		p++;
	if (threeDigits && v > 99)
		p++;
	return p;
}

bool walkAt(const byte *t, uint32 size, uint32 p, const char *word) {
	const uint32 n = strlen(word);
	return p + n <= size && memcmp(t + p, word, n) == 0;
}

MenuWalkRecord &walkSlot(MenuWalk &walk, uint n) {
	while (walk.records.size() <= n) {
		MenuWalkRecord r = { 0, 0, 0, 0 };
		walk.records.push_back(r);
	}
	return walk.records[n];
}

int16 tableWord(uint8 lo, uint8 hi) {
	return (int16)(uint16)(lo | (hi << 8));
}

} // End of anonymous namespace

void MenuRecordTable::reset(uint recordsPerMenu) {
	_stride = 1 + 8 * recordsPerMenu;
	_bytes.clear();
	_bytes.resize(kMenus * _stride);
	for (uint32 i = 0; i < _bytes.size(); i++)
		_bytes[i] = 0;
}

// Past the 15 blocks lies other .bss of the original ([UNKNOWN] what): no
// shipped menu writes or reads there (counts at most 18, the stale count
// 119 of menu 6 ends in menu 13's block; ui_menu_record_table_*.txt), so
// the port keeps zeros there.
void MenuRecordTable::putByte(uint32 off, uint8 v) {
	while (_bytes.size() <= off)
		_bytes.push_back(0);
	_bytes[off] = v;
}

void MenuRecordTable::setCount(int menu, uint8 n) {
	putByte((uint32)menu * _stride, n);
}

MenuWalkRecord MenuRecordTable::record(int menu, uint k) const {
	const uint32 o = (uint32)menu * _stride + 1 + 8 * k;
	MenuWalkRecord r;
	r.x = tableWord(byteAt(o), byteAt(o + 1));
	r.y = tableWord(byteAt(o + 2), byteAt(o + 3));
	r.w = tableWord(byteAt(o + 4), byteAt(o + 5));
	r.h = tableWord(byteAt(o + 6), byteAt(o + 7));
	return r;
}

void MenuRecordTable::setRecord(int menu, uint k, const MenuWalkRecord &r) {
	const uint32 o = (uint32)menu * _stride + 1 + 8 * k;
	const int16 v[4] = { r.x, r.y, r.w, r.h };
	for (int i = 0; i < 4; i++) {
		putByte(o + 2 * i, (uint8)((uint16)v[i] & 0xff));
		putByte(o + 2 * i + 1, (uint8)((uint16)v[i] >> 8));
	}
}

void MenuRecordTable::records(int menu, Common::Array<MenuWalkRecord> &out) const {
	const uint n = count(menu);
	out.resize(n);
	for (uint k = 0; k < n; k++)
		out[k] = record(menu, k);
}

MenuWalkRecord MenuLayout::storedRecord(uint k) const {
	if (table && menu >= 0 && menu < MenuRecordTable::kMenus)
		return table->record(menu, k);
	if (k < walk.records.size())
		return walk.records[k];
	MenuWalkRecord r = { 0, 0, 0, 0 };
	return r;
}

void MenuLayout::setStoredRecord(uint k, const MenuWalkRecord &r) {
	if (table && menu >= 0 && menu < MenuRecordTable::kMenus)
		table->setRecord(menu, k, r);
	else
		walkSlot(walk, k) = r;
}

int menuWalkHit(const Common::Array<MenuWalkRecord> &records, int mx, int my) {
	for (uint k = 0; k < records.size(); k++) {
		const int x = (uint16)records[k].x, y = (uint16)records[k].y;
		const int w = (uint16)records[k].w, h = (uint16)records[k].h;
		if (x < mx && mx < x + w && y < my && my < y + h)
			return (int)k;
	}
	return -1;
}

int menuNumber(const char *fileName) {
	static const char *const kMenus[] = {
		"MAINMENU", "LOADMENU", "OPTMENU", "NEWMENU", "SELEMENU", "SONMENU",
		"SUREMENU", "CD1MENU", "CD2MENU", "CD3MENU", "CD4MENU", "WAIT",
		"ERROR1", "ERROR2", "ERROR3" };
	Common::String base(fileName);
	const size_t dot = base.findFirstOf('.');
	if (dot != Common::String::npos)
		base = Common::String(base.c_str(), dot);
	for (uint k = 0; k < ARRAYSIZE(kMenus); k++)
		if (base.equalsIgnoreCase(kMenus[k]))
			return (int)k;
	return -1;
}

void walkMenuText(const byte *t, uint32 size, int menu, int hoverLine,
                  int toggleA, int toggleB, MenuWalk &walk, int posItemByteWidth,
                  MenuRecordTable *table) {
	walk.ops.clear();
	if (menu < 0 || menu >= MenuRecordTable::kMenus)
		table = nullptr;
	uint count = 0;               // 0x006126b0 + 0xa1 * menu = 0
	if (table)
		table->setCount(menu, 0);   // the count byte only (en-688128 @ 0x0041ef8a)
	// Record `count`: the table's (whatever an earlier walk, another menu's
	// overflow or the LOAD picker left there) or the per-menu one; written
	// back and counted by `put` (the count byte is a byte: `inc byte ptr`).
	auto get = [&]() {
		if (table)
			return table->record(menu, count);
		return walkSlot(walk, count);
	};
	auto put = [&](const MenuWalkRecord &r) {
		if (table) {
			table->setRecord(menu, count, r);
			table->setCount(menu, (uint8)(count + 1));
			count = table->count(menu);
		} else {
			walkSlot(walk, count) = r;
			count++;
		}
	};
	uint32 p = 0;
	int line = 0;                 // EBP-0x14
	int penX = 0, penY = 0;       // EBP-0x24, EBP-0x20
	auto toCr = [&](uint32 q) {
		while (q < size && t[q] != 0x0d)
			q++;
		return q;
	};
	while (p < size && !walkAt(t, size, p, "/fin")) {
		bool draw = true;         // EBP-0x18
		bool hover = (line == hoverLine && line != 0) || (line == hoverLine && menu == 0);
		if ((line == 2 || line == 4) && menu == 2)
			hover = false;
		if (walkAt(t, size, p, "<>")) {             // 0x0041fd08
			p += 3;
			const int y = walkAtoi(t, size, p);
			p = walkStep(p, y, 2);
			// FUN_004206c8 reads 0x004965ee[byte] with no bound (bytes < 0x20
			// and >= 0xa9 read the strings before the table and the FONTMIN
			// table after it); the port counts them 0, as the pen advance below
			// (0x004202cc).  Unreachable with the shipped menus: atlantis-re
			// functions/ui-FUN_0041fb6c.md, Differences from the original.
			int w = 0;                               // FUN_004206c8
			for (uint32 q = p; q < size && t[q] != 0x0d; q++)
				w += (t[q] >= 0x20 && t[q] - 0x20 < (int)ARRAYSIZE(kFontMaxMeasureWidths)) ? kFontMaxMeasureWidths[t[q] - 0x20] : 0;
			penX = (640 - w) / 2;
			penY = y;
			MenuWalkRecord r;
			r.x = (int16)(penX - 11);
			r.y = (int16)(y - 28);
			r.w = (int16)w;
			r.h = 28;
			put(r);
		}
		if (walkAt(t, size, p, "<")) {              // 0x0041fe42
			p++;
			const int x = walkAtoi(t, size, p);
			p = walkStep(p, x, 2);
			const int y = walkAtoi(t, size, p);
			p = walkStep(p, y, 2);
			penX = x;
			penY = y;
			MenuWalkRecord r;
			r.x = (int16)(x - 11);
			r.y = (int16)(y - 28);
			// bytes to the CR x 0x16 (atlantis-cd-it-687104 FUN_0041ecb4:
			// x 16, `shl ebx,4` @ 0x0041f068, stored @ 0x0041f098)
			r.w = (int16)(posItemByteWidth * (int)(toCr(p) - p));
			r.h = 28;
			put(r);
		}
		if (walkAt(t, size, p, "//")) {             // 0x0041ffb7
			p += 2;
			for (int k = 0; k < 2; k++) {
				const bool on = (menu == 2) && (k == 0 ? (line == 3 && toggleA == 0) : (line == 5 && toggleB == 0));
				if (!on)
					continue;
				uint32 q = p;                         // strchr(p, '&') + 2
				while (q < size && t[q] != '&')
					q++;
				p = q + 2;
			}
		}
		if (walkAt(t, size, p, "&&")) {             // 0x0042002d
			if (menu == 2 && ((line == 3 && toggleA == 1) || (line == 5 && toggleB == 1))) {
				p = toCr(p) + 1;
				line++;
			}
		}
		static const char *const kSpr[] = { "/sprj=", "/sprr=", "/sprc=", "/spr=" };
		for (int v = 0; v < 4; v++) {               // 0x0042009d..0x004201be
			if (!walkAt(t, size, p, kSpr[v]))
				continue;
			p += (v < 3) ? 6 : 5;
			// FUN_004207c0: I (1-2 digits), X, Y, W (1-3 digits), H
			const int idx = walkAtoi(t, size, p);
			p = walkStep(p, idx, 2, false);
			const int x = walkAtoi(t, size, p);
			p = walkStep(p, x, 2);
			const int y = walkAtoi(t, size, p);
			p = walkStep(p, y, 2);
			const int w = walkAtoi(t, size, p);
			p = walkStep(p, w, 2);
			const int h = walkAtoi(t, size, p);
			// FUN_004207c0 stores the parsed x / y only into a zero field and
			// otherwise reloads the stored one (x @ 0x00420910, y @
			// 0x004209b0); the blits take both (0x00420a7f..0x00420a95), so a
			// slider keeps its knob and the LOAD knob (record 4, written by
			// FUN_0041cf78) is drawn where it was put.
			MenuWalkRecord r = get();
			int bx = x, by = y;
			if (r.x == 0)
				r.x = (int16)x;
			else
				bx = r.x;
			if (r.y == 0)
				r.y = (int16)y;
			else
				by = r.y;
			r.w = (int16)w;
			r.h = (int16)h;
			MenuWalkOp op = { true, v < 3, false, (int16)bx, (int16)by, idx, line, (int)count };
			walk.ops.push_back(op);
			put(r);
			p = toCr(p) + 1;                          // past the CR
			draw = false;
			line++;
		}
		if (p >= size)
			break;
		int c = t[p];                                 // 0x004201c1
		if (c == 0x0d) {
			p++;
			penX = 0;
			penY += 28;
			draw = false;
			line++;
		}
		if (c < 0x20 && c != 0x0d)
			c = 0x20;
		if (c == 0x20) {
			penX += 0x16;
			draw = false;
		}
		c -= 0x20;
		if (draw) {
			MenuWalkOp op = { false, false, hover, (int16)penX, (int16)penY, c, line, (int)count - 1 };
			walk.ops.push_back(op);
			penX += (c >= 0 && c < (int)ARRAYSIZE(kFontMaxMeasureWidths)) ? kFontMaxMeasureWidths[c] : 0;
		}
		p++;
	}
	if (table)
		table->records(menu, walk.records);
	else
		walk.records.resize(count);
}

void loadPickerKnobRecord(MenuLayout &layout, const LoadPickerScroll &s) {
	MenuWalkRecord r = layout.storedRecord(4);
	if (s.knobX != 0)
		r.x = (int16)s.knobX;
	r.y = (int16)s.knobY;
	layout.setStoredRecord(4, r);
}

void soundMenuTableWalk(MenuLayout &layout, const int knobX[4], int posItemByteWidth) {
	MenuRecordTable *table = layout.table;
	if (!table || layout.menu < 0 || layout.menu >= MenuRecordTable::kMenus || layout.raw.empty())
		return;
	for (int t = 0; t < 4; t++) {
		MenuWalkRecord r = table->record(layout.menu, 5 + t);
		r.x = (int16)knobX[t];
		table->setRecord(layout.menu, 5 + t, r);
	}
	walkMenuText(layout.raw.data(), layout.raw.size(), layout.menu, -1, 0, 0, layout.walk,
	             posItemByteWidth, table);
}

uint nameKeyCode(uint16 unicode) {
	if (unicode < 0x80 || (unicode >= 0xa0 && unicode <= 0xff))
		return unicode;
	const Common::u32char_type_t ch = unicode;
	const Common::String enc = Common::U32String(&ch, 1).encode(Common::kWindows1252);
	if (enc.size() != 1 || (byte)enc[0] < 0x80)
		return 0;   // not in the code page ('?')
	return (byte)enc[0];
}

int nameEntryChar(uint code, bool upperCase, bool swedishLetters) {
	if (swedishLetters) {
		// atlantis-cd-sv-688128 FUN_0041d704 @ 0x0041dafa..0x0041db44
		// (accepted) and 0x0041db4a..0x0041dbe9 (stored as)
		switch (code) {
		case 0xc4: return 0x8e;   // A diaeresis   @ 0x0041dbb2
		case 0xc5: return 0x8f;   // A ring        @ 0x0041dbbd
		case 0xd6: return 0x99;   // O diaeresis   @ 0x0041dbc8
		case 0xe4: return 0x84;   // a diaeresis   @ 0x0041dbd3
		case 0xe5: return 0x86;   // a ring        @ 0x0041dbde
		case 0xf6: return 0x94;   // o diaeresis   @ 0x0041dbe9
		default: break;
		}
	}
	if (!upperCase)
		return (code >= 0x20 && code <= 0xa7) ? (int)code : -1;
	if (code < 0x20 || code > 0x7a)
		return -1;
	return (code >= 'a' && code <= 'z') ? (int)(code - 0x20) : (int)code;
}

// Byte i of the name field buffer 0x00612080 (the field always holds 16
// bytes when the name entry writes it).
static void putFieldByte(Common::String &field, uint i, char c) {
	if (i < field.size())
		field.setChar(c, i);
	else if (i == field.size())
		field += c;
}

bool PlayerScreenState::isNewRow(const Common::Array<Common::String> &names,
                                 const Common::String &marker, int r) {
	return r >= 0 && r == (int)names.size() - 1 && names[r] == marker;
}

bool PlayerScreenState::fieldValid() const {
	return !field.empty() && field[0] != ' ' && field[0] != '.';
}

bool PlayerScreenState::click(bool clicked, int record, int grid,
                              const Common::Array<Common::String> &names,
                              const Common::String &marker) {
	// 0x0041e0a5: the click flag and a record other than OK (5).
	if (!clicked || record == 5)
		return false;
	if (record != 6)                    // 0x0041e0bc..0x0041e0ea
		row = grid;
	len = 0;                            // 0x0041e0ed..0x0041e0fd
	key = 0;
	full = false;
	if (row != -1) {
		// 0x0041e10d..0x0041e165.  The caller gives no row past the list.
		field = row < (int)names.size() ? names[row] : Common::String();
		if (isNewRow(names, marker, row))
			field = Common::String(kNameLen, '.');   // 0x0041e153
		current = row;
	} else {
		field.clear();                  // 0x0041e175
		dblFlag = false;                // 0x0041e184
	}
	// 0x0041e18e..0x0041e1c7: DELETE, a field not starting with ' ', a name
	// other than the marker and a row.
	if (record == 6 && (field.empty() || field[0] != ' ') && row != -1 &&
	    row < (int)names.size() && names[row] != marker) {
		dblFlag = false;                // 0x0041e1cb
		return true;
	}
	return false;
}

void PlayerScreenState::deleteDone(bool confirmed) {
	if (confirmed)
		field.clear();
	dblFlag = false;                    // FUN_0041e8a8 @ 0x0041e8db
	row = -1;
}

bool PlayerScreenState::nameEntry(const Common::Array<Common::String> &names,
                                  const Common::String &marker,
                                  bool upperCase, bool swedishLetters) {
	const int count = (int)names.size();
	if (isNewRow(names, marker, row)) {           // 0x0041e212..0x0041e23c
		if (key != 0 && len < kNameLen) {           // 0x0041e243..0x0041e254
			dblFlag = false;                        // 0x0041e25b
			const uint k = (key == 0x20 && len == 0) ? 0 : key;   // 0x0041e265..0x0041e27a
			const int c = (k < 0x20) ? -1 : nameEntryChar(k, upperCase, swedishLetters);
			if (c >= 0) {
				// 0x0041e29d..0x0041e302
				if (len == 0)
					field = Common::String(kNameLen, '.');
				putFieldByte(field, len, (char)c);
				lastKey = '.';
				if (len == kNameLen - 1) {
					full = true;
					lastKey = (char)c;
				} else {
					len++;
				}
			} else if (k == 8) {
				// 0x0041e310..0x0041e361
				if (len != 0) {
					putFieldByte(field, len, '.');
					if (len == kNameLen - 1 && lastKey != '.') {
						lastKey = '.';
					} else {
						putFieldByte(field, len - 1, '.');
						len--;
					}
				}
			} else if (k == 0x0d && fieldValid()) {
				// 0x0041e39e..0x0041e44c: the key stays 0x0d.
				field = typedName();
				row = -1;
				return true;
			}
			key = 0;
		} else {
			// 0x0041e45c..0x0041e4b0
			putFieldByte(field, len, blinkOn ? (full ? lastKey : '.') : ' ');
			blinkOn = !blinkOn;
		}
	}
	if (row < count - 1) {                        // 0x0041e4b3..0x0041e4cc
		key = 0;
		len = 0;
	}
	return false;
}

bool PlayerScreenState::exitCommits(int record, int count) const {
	return record == 5 && key != 0x0d && row == count - 1;
}

Common::String PlayerScreenState::typedName() const {
	Common::String name(field.c_str(), MIN<uint>(len, field.size()));
	if (full)
		name += lastKey;
	return name;
}

void PlayerScreenState::commitAtExit(int count) {
	field = typedName();
	row = count - 1;
}

} // namespace Atlantis
} // namespace CryOmni3D
