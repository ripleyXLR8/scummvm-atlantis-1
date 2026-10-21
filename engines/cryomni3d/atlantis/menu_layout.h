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

#ifndef CRYOMNI3D_ATLANTIS_MENU_LAYOUT_H
#define CRYOMNI3D_ATLANTIS_MENU_LAYOUT_H

#include "common/array.h"
#include "common/rect.h"
#include "common/str.h"
#include "common/stream.h"

namespace CryOmni3D {
namespace Atlantis {

// Parsed entry from a SPRLIST\*MENU.TXT layout file (original Atlantis menu
// data).  The format is line-oriented:
//
//   <X,Y>TEXT          — anchor a FONTMAX label at (X, Y_txt).  X may be
//                        empty, e.g. "<>,140>SELECT PLAYER NAME", meaning
//                        the label is horizontally centred on the 640-wide
//                        screen.  Y is the y_txt baseline (same convention
//                        as drawFontMaxText).
//   /spr=I,HX,HY,W,H   — composite SPRMENU.SPR sprite index I with its
//                        hotspot at screen (HX, HY).  W/H are size hints kept
//                        in w/h (slider tracks use them); the sprite still
//                        carries its real pixel size for blitting.
//   <X,Y>//A&&B[&&C…]   — a toggle: a text anchor whose value cycles through
//                        the &&-separated option labels.  Parsed as kToggle.
//   /sprj= /sprr= /sprc=I,X,Y,W,H — a word of the menu bank (bankWord):
//                        sprite I of SPRCHI1.SPR (SPRCHI2.SPR hovered) at
//                        (X, Y), the Chinese CDs' menu lines (no text line
//                        in their files).  Parsed as kText with no text, so
//                        a menu finds it by the ordinal of the label it
//                        replaces (the records are in the same order).
//   /fin or /FIN       — end of file.  Anything after is ignored.
//
// Blank lines and CR characters are tolerated.
struct MenuItem {
	enum Kind { kText, kSprite, kToggle };

	Kind kind;

	// Text item fields (anchorX/anchorY/centerX also apply to kToggle).
	Common::String text;
	bool centerX;       // true → ignore anchorX and centre on screen.
	int  anchorX;
	int  anchorY;       // y_txt baseline.
	// Character count after '>' up to end of line, BEFORE any trimming or
	// toggle splitting (for "//A&&B" toggles this counts the whole spec).
	// The original sizes a positioned text item's hit rect as 22 px per raw
	// character (atlantis.exe FUN_0041fb6c: W = chars_to_CR * 0x16; 16 in
	// atlantis-cd-it-687104, FUN_0041ecb4 @ 0x0041f068).
	int  rawLen;

	// Sprite item fields.  w/h come from the /spr size hint.
	int sprIdx;
	int hx, hy;
	int w, h;
	// A `/sprj=` `/sprr=` `/sprc=` record (kText): sprIdx, hx, hy, w, h as
	// for a sprite, drawn from the menu bank (AtlantisExeBuild::
	// menuSpriteBanks; FUN_004201e8 with 0x0048609c = 1 in
	// atlantis-cd-zh-tw-696832) and hit-tested by its record {X, Y, W, H}.
	bool bankWord;

	// Toggle item fields (kToggle).  `options` holds the label of each
	// state; `selected` indexes the currently active one.
	Common::Array<Common::String> options;
	int selected;

	MenuItem()
	    : kind(kText), centerX(false), anchorX(0), anchorY(0), rawLen(0),
	      sprIdx(0), hx(0), hy(0), w(0), h(0), bankWord(false), selected(0) {}
};

// atlantis.exe FUN_0041fb6c, the menu walk: the text is not parsed ahead of
// time; every draw walks it byte by byte and rebuilds the menu's hit table
// (0x006126b0 + 0xa1 * menu: a count, then {s16 x, y, w, h} records) in the
// same pass.  At each position, in this order: `/fin` ends; `<>,Y>` centred
// text (FUN_004206c8); `<X,Y>` positioned text; `//` and `&&` the options
// toggles (menu 2, lines 3 and 5); `/sprj=` `/sprr=` `/sprc=` `/spr=`
// sprite records (FUN_004207c0); then the byte itself: CR (the next byte is
// consumed), space and other control bytes (pen + 22), or a FONTMAX glyph.
struct MenuWalkRecord {
	int16 x, y, w, h;
};
struct MenuWalkOp {
	bool sprite;        // a sprite of SPRMENU.SPR (or of the alternate bank)
	bool altBank;       // /sprj= /sprr= /sprc=: bank 0x006cd874
	bool hoverFont;     // glyph in FONTMAX2 (the hovered line)
	// The hovered-bank flag 0x004860a0 of atlantis-cd-zh-tw-696832's walk
	// FUN_0041f5a0 (atlantis-pe-2018-695296's): set at each byte while the
	// line is the hovered one and (line != 0 or menu 0) (@ 0x0041f6c9..
	// 0x0041f707); the options menu's lines 2 and 4 reset only the glyph
	// font (@ 0x0041f711..0x0041f72a), so a `/sprc=` label there is drawn
	// hovered.  A `/sprj=` `/sprr=` `/sprc=` record is blitted from the
	// second bank 0x006bd878 when it is set (FUN_004201e8 @ 0x004204fa..
	// 0x00420543).
	bool hoverBank;
	int16 x, y;         // pen (glyph) or record position (sprite)
	int index;          // glyph c - 0x20, or the sprite index
	int line;           // the walk's line counter at the op
	int record;         // the record the op belongs to (-1 none yet)
};
struct MenuWalk {
	// In: the records of the previous draw (a sprite record keeps a non-zero
	// x / y, FUN_004207c0); out: this draw's hit table.
	Common::Array<MenuWalkRecord> records;
	Common::Array<MenuWalkOp> ops;
};
// The original's menu record table: one static .bss table for the whole
// session (every build), 15 blocks of a count byte and `recordsPerMenu`
// records {s16 x, y, w, h}, block m at m * stride, stride = 1 + 8 x
// recordsPerMenu: 0x006126b0 / 0xa1 / 20 in the reference (FUN_0041fb6c,
// FUN_004207c0, FUN_004210c0), 0x006026b0 / 0x81 / 16 in
// atlantis-cd-en-688128 (FUN_0041ef34 `imul eax,eax,0x81` @ 0x0041ef84,
// FUN_0041fa84, FUN_00420304) and atlantis-cd-fr-688640 (FUN_0041efa4,
// FUN_0041faf4, FUN_00420374).  A walk zeroes the menu's count byte only
// (0x0041ef8a) and writes record k = count at m * stride + 1 + 8k with no
// bound, so a menu of more records writes into the next menu's block:
// SONMENU.TXT's 18 records in a 16-record build put its records 16 and 17
// over SUREMENU's count byte and records 0 and 1 (atlantis-re
// tools/emu/ui_menu_record_table.py, evidence/golden/
// ui_menu_record_table_*.txt).  Nothing else writes the table but the LOAD
// picker (record 4 of menu 1) and the sound menu's sliders (records 5..8 x
// of menu 5) (the golden's `writers` section).
class MenuRecordTable {
public:
	enum { kMenus = 15 };
	explicit MenuRecordTable(uint recordsPerMenu = 20) { reset(recordsPerMenu); }
	// A zeroed table (the .bss at start-up) of the given record count.
	void reset(uint recordsPerMenu);
	uint recordsPerMenu() const { return (_stride - 1) / 8; }
	uint8 count(int menu) const { return byteAt((uint32)menu * _stride); }
	void setCount(int menu, uint8 n);
	MenuWalkRecord record(int menu, uint k) const;
	void setRecord(int menu, uint k, const MenuWalkRecord &r);
	// The menu's records 0 .. count - 1 as stored (another menu's overflow
	// included).
	void records(int menu, Common::Array<MenuWalkRecord> &out) const;

private:
	uint8 byteAt(uint32 off) const { return off < _bytes.size() ? _bytes[off] : 0; }
	void putByte(uint32 off, uint8 v);
	uint32 _stride;
	Common::Array<byte> _bytes;
};

// hoverLine: DAT_006cd850; toggleA / toggleB: DAT_004960a8 / DAT_004960ac.
// posItemByteWidth: a positioned item's record width per byte to the CR,
// 0x16 (FUN_0041fb6c @ 0x0041fe42..) but 16 in atlantis-cd-it-687104
// (FUN_0041ecb4 @ 0x0041f068; AtlantisExeBuild::menuPosItemByteWidth).
// table: the session's record table (the original): the walk reads the
// stored records from it and writes them back, `walk.records` receives a
// copy of the menu's count records.  Null: per-menu records in
// `walk.records` only, unbounded (kEnhMinorBugFixes, or no menu number).
// toggleSprites (AtlantisExeBuild::menuToggleSprites): the sprite record's
// id rules of atlantis-cd-zh-tw-696832 FUN_004201e8, for every record kind:
//  - id 9 in menu 2 becomes 9 + toggleA (SUBTITLES: 9 off, 10 on) and the
//    parse pointer steps one byte back (@ 0x00420212..0x0042022b), then on
//    by the new id's digits (@ 0x00420245..0x00420252): right for 10, one
//    byte short for 9, so with SUBTITLES 0 the record's x is atoi(",X..")
//    = 0, y the last digits of X, w Y and h W (OPTMENU `/sprc=9,336,255,
//    50,32`: x 0, y 36, w 255, h 50; the x / y a record already holds
//    win, FUN_004201e8 @ 0x004202ff / 0x0042039f; atlantis-re
//    evidence/golden/ui_menus_zh_tw.txt `toggles`);
//  - id 12 becomes 13 when toggleB (OMNI 3D) is 1, in any menu (@
//    0x0042022e..0x00420242).
// toggleStepFix (kEnhMinorBugFixes): an id 9 left at 9 steps past its digit
// as written, so the record reads the file's x, y, w, h.
void walkMenuText(const byte *text, uint32 size, int menu, int hoverLine,
                  int toggleA, int toggleB, MenuWalk &walk, int posItemByteWidth = 22,
                  MenuRecordTable *table = nullptr, bool toggleSprites = false,
                  bool toggleStepFix = false);
// atlantis.exe FUN_004210c0: the first record strictly containing the
// mouse, coordinates read as u16 (a negative x / y is never hit), sprite
// records included; -1 for none (the original's 0xff).
int menuWalkHit(const Common::Array<MenuWalkRecord> &records, int mx, int my);
// The original's menu number of a SPRLIST menu file ("OPTMENU.TXT" → 2).
int menuNumber(const char *fileName);

struct MenuLayout {
	Common::Array<MenuItem> items;
	// The file's bytes as read, walked by walkMenuText on every draw, and the
	// menu number the original gives it (the order of FUN_0041c838's table:
	// MAINMENU 0 ... ERROR3 14; -1 unknown).  `walk` keeps the hit table
	// between draws as the original's static table does.
	Common::Array<byte> raw;
	int menu;
	mutable MenuWalk walk;
	// The session's record table the walk uses (the original's static
	// table), or null for per-menu records (kEnhMinorBugFixes).
	MenuRecordTable *table;

	MenuLayout() : menu(-1), table(nullptr) {}

	// Record k as the next walk will find it: in the table when there is one
	// (and a menu number), else in `walk.records` (zero when absent).
	MenuWalkRecord storedRecord(uint k) const;
	void setStoredRecord(uint k, const MenuWalkRecord &r);

	// Parse the menu-text stream.  Returns false only when the buffer is
	// unreadable; an empty / truncated file yields an empty layout and true.
	bool loadFromStream(Common::ReadStream &s);
};

// The FONTMIN pen advance of glyph index c - 0x20 (control characters use
// index 0x20): the 101-byte table at 0x00496697.  The drawers index it with
// no bound (FUN_00420d8c, FUN_00420f60, FUN_00420b48; the 1997 builds the
// same), so an index of 101 or more reads the credits advance table that
// follows (0x004966fc = 0x00496697 + 101; atlantis-cd-fr-687104 0x004866d5
// = 0x00486670 + 101; atlantis-cd-de-687104 0x004866d4 = 0x0048666f + 101;
// atlantis-cd-fr-688640 0x004866d9 = 0x00486674 + 101,
// with E b d g n t one less) at index - 101: `creditWidths` is the build's table
// as read from atlantis.exe (the reference's bytes when null).  Bytes 0xa1,
// 0xa3, 0xa4 thus advance 10, 14, 14 in the reference and 17, 2, 10 in the
// 1997 builds (atlantis-re experiment 2026-10-18-03, decision 0019 E).
int fontMinAdvance(uint idx, const byte *creditWidths = nullptr);

// One FONTMIN glyph blit: pen x, baseline y, glyph index.
struct FontMinBlit {
	int x, y;
	uint8 idx;
};

// The three FONTMIN text renderers of atlantis.exe:
//  - player list (FUN_00420d8c): a space advances 20, not drawn; '#' is
//    neither drawn nor advanced; other glyphs are drawn and advance by the
//    table;
//  - name field (FUN_00420f60): a space advances 20, not drawn; other
//    glyphs are drawn and advance by the table, glyph 0x2e by 20;
//  - load list (FUN_00420b48): glyph 0x20 is not drawn and advances 6;
//    other glyphs (the space included) are drawn and advance by the table.
// The 1997 builds' player list (atlantis-cd-en-688128 FUN_0041fff4,
// atlantis-cd-fr-687104 FUN_0041fd6c) has no '#' rule: hashRule false
// draws '#' like any glyph.  Advances: fontMinAdvance(idx, creditWidths).
enum FontMinRenderer {
	kFontMinPlayerList,
	kFontMinNameField,
	kFontMinLoadList
};
//
// The German CD's EPI.TXT (atlantis-cd-de-687104) writes `ß` as the DOS
// code page byte 0xe1: glyph 193, past FONTMIN's 137 sprites, so the
// original draws nothing (the glyph-count test of FUN_00410430) and moves
// the pen by credits byte 0x5c (7 px; atlantis-re experiment
// 2026-10-18-07 item 4).  The port does the same by default (the overflow
// rule above, the drawer skipping a glyph it does not have).
// sharpSAsSs (kEnhMinorBugFixes, decision 0021 C3): byte 0xe1 is drawn and
// advanced as two 's' glyphs of the same font instead, in every FONTMIN
// renderer and in the load-list width, so a centred title stays centred.
void fontMinBlits(FontMinRenderer r, const Common::String &text, int x, int y,
                  Common::Array<FontMinBlit> &out, bool hashRule = true,
                  const byte *creditWidths = nullptr, bool sharpSAsSs = false);

// Width FUN_00420b48 records for a load-list row (0x00611e80): the sum of
// the pen advances of fontMinBlits(kFontMinLoadList), the +6 steps included.
int fontMinLoadListWidth(const Common::String &text, const byte *creditWidths = nullptr,
                         bool sharpSAsSs = false);

// The DOS code page byte of `ß` in the SPRLIST texts (EPI.TXT of
// atlantis-cd-de-687104) and the text with each one replaced by "ss": the
// sharpSAsSs rendering above.
enum { kSprTextSharpS = 0xe1 };
Common::String sprTextSharpSAsSs(const Common::String &text);

// atlantis.exe FUN_0041cf78, the LOAD picker's scrollbar (0x0041d1f3..
// 0x0041d24b at entry, 0x0041d273..0x0041d2e6 each frame).  The knob
// (record 4, /spr=13) starts at y 0x7b, the list at offset 0.  With 11 files
// or fewer the knob x is 0xffce (off-screen) and the knob never moves; else
// step = 0xd7 / (count - 10).  While the track (record 1) is latched (the
// record under the pointer at WM_LBUTTONDOWN), the button is down and
// 0x7b < mouse y < 0x152: knob y = mouse y - 10, offset = (knob y - 0x7b) /
// step as a byte, and knob y 0x7b, offset 0 when mouse y - 10 < 0x7b.  The
// offset is not clamped to count - 11.
// knobAtBar false: the knob starts at y 0 (atlantis-cd-en-688128
// FUN_0041c9a8 @ 0x0041cc23, atlantis-cd-fr-687104 FUN_0041c708 @
// 0x0041c98a).  clamp false (atlantis-cd-en-688128, atlantis-cd-fr-688640
// FUN_0041ca00, atlantis-cd-nl-688128 FUN_0041c9e0): no clamp at 0x7b, so
// mouse y 0x7c..0x84 leaves the knob above the bar and the offset (knob
// y - 0x7b) / step truncated toward zero, as a byte (atlantis-cd-en-688128
// @ 0x0041ccd6..0x0041ccf4); atlantis-cd-fr-687104 (and the de / es / it
// 687104 builds) clamps as the reference (0x0041ca60..0x0041ca76), and
// atlantis-cd-sv-688128 too (0x0041cd08..0x0041cd1e).
// See atlantis-re: functions/ui-FUN_0041cf78.md step 4,
// experiments/2026-10-16-04-load-picker.md, 2026-10-18-01 item 7,
// 2026-10-18-03 item 7.
struct LoadPickerScroll {
	bool dragOn = false;   // EBP-0x14
	int step = 1;          // EBP-0x28
	int knobX = 0;         // record 4 x when moved off-screen (0xffce), else 0 = the text's
	int knobY = 0x7b;      // record 4 y, 0x00612774
	uint8 offset = 0;      // 0x00496605
};
void loadPickerScrollInit(int count, LoadPickerScroll &s, bool knobAtBar = true);
void loadPickerScrollFrame(LoadPickerScroll &s, bool trackLatched, bool buttonDown, int mouseY,
                           bool clamp = true);
// The picker's writes of the knob, record 4 of menu 1, before a walk: y
// always (0x0041d1fa / 0x0041d2b0; en-688128 0x0041cc23 / 0x0041ccd9), x
// 0xffce only with 11 files or fewer (0x0041d21a; en-688128 0x0041cc43),
// never back: with the session's table (layout.table) a later visit with
// more files keeps x 0xffce, the knob off screen (atlantis-re
// ui_menu_record_table_*.txt `loadknob`); per-menu records start from zero
// at each visit (kEnhMinorBugFixes).
void loadPickerKnobRecord(MenuLayout &layout, const LoadPickerScroll &s);
// The sound menu's walk into the session's table (en-688128 FUN_0041d218
// @ 0x0041d2b9, the reference FUN_0041d940): its knob records 5..8 at the
// knobs' x (the sliders' writes, en-688128 @ 0x0041d2b2), then the walk of
// SONMENU.TXT, whose records 16 and 17 overwrite menu 6's count byte and
// records 0-1 in a 16-record build.  Nothing without a table (the port
// draws and hit-tests this menu itself).
void soundMenuTableWalk(MenuLayout &layout, const int knobX[4], int posItemByteWidth = 22);

// atlantis.exe FUN_004211f0: the cell under (mx, my) of a cols x rows grid
// at (x0, y0) with cells cellW x cellH, bounds INCLUSIVE on both sides (so
// the far edge column / row gives an index past the grid); column-major
// (index = col * rows + row) or row-major; -1 outside.
int menuGridHit(int x0, int y0, int cellW, int cellH, int cols, int rows, bool colMajor,
                int mx, int my);

// atlantis.exe sound menu OK: a knob at x gives the AIL volume
// (x - 305) / 213 * 127, rounded toward zero (FUN_00451d6e), 0..127.
int menuVolumeFromKnob(int x);

// The port keeps those AIL volumes (0..127) in ScummVM's 0..maxMixer scale:
// the menu stores ceil(a * maxMixer / 127) and the readers (place.cpp
// effectsVolume127, music.cpp musicSyncVolume) truncate v * 127 / maxMixer,
// so a value set in the menu reads back unchanged (127 -> 256 -> 127).
inline int menuMixerFromAilVolume(int a, int maxMixer) { return (a * maxMixer + 126) / 127; }
inline int menuAilFromMixerVolume(int v, int maxMixer) { return v * 127 / maxMixer; }

// atlantis.exe options menu, SUBTITLES item: on = !on; counter + 1; when
// the counter is 2, on = 0; when it is 3, it returns to 0 (the byte counter
// DAT_00496098).  Subtitles are drawn when on is set (FUN_00430178).
void subtitleToggleClick(bool &on, uint8 &counter);

// atlantis.exe options menu, OMNI 3D item (FUN_0041d5f4 @ 0x0041d764..
// 0x0041d794, hover record 5):
// DAT_004960ac = (DAT_004960ac == 0).  The port steps the toggle to its next
// option, (selected + 1) % options; with the file's two options this is the
// same flip.  Returns true when option 1 (inverted look, the original's
// DAT_004960ac == 0) is selected.
bool omniToggleClick(MenuItem &tog);

// FONTMAX measuring widths (atlantis.exe @ 0x0049660e indexed by char - 0x20,
// i.e. 0x004965ee indexed by the raw byte):
// FUN_004206c8 centres labels and sizes hit rects with it, and the pen
// advances by it while drawing (0x004202cc).
extern const uint8 kFontMaxMeasureWidths[137];
int fontMaxMeasureWidth(const Common::String &text);

// Draw x of a text item's label: (640 - measured width) / 2 when centred
// (FUN_004206c8), else its anchor.
int menuTextDrawX(const MenuItem &it, const Common::String &label);

// Hit box of a text record (FUN_0041fb6c): x - 11, y - 28, height 28; width
// is the measured string when centred, posItemByteWidth (22; 16 in
// atlantis-cd-it-687104, FUN_0041ecb4 @ 0x0041f068) per raw spec character
// otherwise.  The original tests strict inside, hence the left/top + 1.
Common::Rect menuTextHitRect(const MenuItem &it, int drawX, int posItemByteWidth = 22);

// Pen of a glyph-sprite font (FONTMAX, FONTMIN): a space advances
// spaceAdvance and draws nothing; glyph c - 0x20 (below glyphWidths.size())
// is drawn at the pen, which then advances by advanceTable when given, else
// by the glyph's width.
struct SprFontGlyph {
	int x, idx;
};
void sprFontPen(const Common::String &text, int anchorX, int spaceAdvance,
                const Common::Array<int> &glyphWidths, const uint8 *advanceTable,
                uint advanceCount, Common::Array<SprFontGlyph> &out);

// The code the original's window procedure stores from WM_CHAR (0x00419c9a):
// its window is ANSI (RegisterClassA, PeekMessageA + TranslateMessage), so a
// typed character arrives in the Windows-1252 code page; ScummVM gives
// Unicode. They differ only for 0x80..0x9f (e.g. U+2026 is 0x85, drawn as
// FONTMIN's 'a grave'; atlantis-re experiments/2026-10-14-15). 0 when the
// character is not in the code page.
uint nameKeyCode(uint16 unicode);

// The byte a typed code adds to a new player's name, -1 when refused (the
// leading-space rule is the caller's): FUN_0041dea4 @ 0x0041e283 takes
// 0x20..0xa7 (the FONTMIN glyphs) as typed, as does atlantis-cd-fr-687104
// (FUN_0041d45c @ 0x0041d847); the English build of 1997
// (atlantis-cd-en-688128 FUN_0041d6d0 @ 0x0041dab1..0x0041daf8, upperCase)
// takes 0x20..0x7a and stores a..z as A..Z.  swedishLetters: the Swedish
// build (atlantis-cd-sv-688128 FUN_0041d704 @ 0x0041dae5..0x0041dbe9,
// exe_build.h nameSwedishLetters, decision 0023 C1) also takes the codes
// 0xc4 0xc5 0xd6 0xe4 0xe5 0xf6 (Windows-1252 `Ä Å Ö ä å ö`, what
// nameKeyCode gives for those Unicode letters) and stores them as the DOS
// code page bytes 0x8e 0x8f 0x99 0x84 0x86 0x94, the FONTMIN glyphs.
int nameEntryChar(uint code, bool upperCase, bool swedishLetters = false);

// The SELECT PLAYER NAME loop's state (atlantis.exe FUN_0041dea4) and its
// per-frame blocks, in the original's order: click (0x0041e0a5..0x0041e1e4),
// the field draw (shown when not empty, 0x0041e1e9), name entry
// (0x0041e212..0x0041e4cc), the double-click exit (0x0041e4d3), the OK exit
// (0x0041e5bf) and, on any exit, the commit test (0x0041e6af..0x0041e6d4).
// There is no typing mode: a click is handled the same way while a name is
// typed.  The rows are names[0..count) as the original keeps them
// (0x0049657c, count 0x00496606): the players, then the New Player marker
// (0x00496570) while there are fewer than five; records are SELEMENU's (5
// OK, 6 DELETE).  See atlantis-re: functions/ui-FUN_0041dea4.md steps 3-5,
// golden ui_player_screen.txt (tools/emu/ui_player_screen.py).
struct PlayerScreenState {
	static const uint kNameLen = 16;
	// The record of a frame whose pointer is on neither OK nor DELETE (the
	// port hit-tests only those two; the original's FUN_004210c0 gives
	// another record or 0xff there, which no block tells from each other).
	static const int kNoRecord = -1;

	// How a frame leaves the loop: no exit, an Enter commit (0x0041e44c),
	// the double-click exit (0x0041e4fd) or the OK exit (0x0041e5f2).
	enum Exit { kExitNone, kExitEnter, kExitDouble, kExitOk };

	int row = -1;            // [EBP-8]: the selected row, -1 none
	Common::String field;    // 0x00612080: the name field, "" at entry
	uint len = 0;            // 0x00772910: typed length, at most 15
	bool full = false;       // [EBP-0x18]: a 16th character was typed
	char lastKey = '.';      // [EBP-4]: the 16th character
	bool blinkOn = true;     // [EBP-0x10]: insertion point phase, 1 at entry
	int current = -1;        // 0x00496607: the current player's row
	bool dblFlag = false;    // 0x00496118: set by WM_LBUTTONDBLCLK
	uint key = 0;            // 0x006ce262: the last WM_CHAR code, 0 none

	static bool isNewRow(const Common::Array<Common::String> &names,
	                     const Common::String &marker, int r);
	// First byte neither NUL, ' ' nor '.' (0x0041e37c, 0x0041e4dc, 0x0041e5d3).
	bool fieldValid() const;
	// The click block, for a frame whose click flag 0x0049611c is set on a
	// record other than OK: unless on DELETE the row is the grid's
	// (FUN_004211f0, -1 off the rows); length, key and 16th-character flag
	// reset; a row gives its name (16 dots on the New Player row) and
	// becomes current, no row empties the field and clears the double-click
	// flag.  True when FUN_0041e8a8 (ARE YOU SURE, then delete) runs for
	// row: DELETE on a row whose name is not the marker; the caller asks,
	// deletes and calls deleteDone.
	bool click(bool clicked, int record, int grid, const Common::Array<Common::String> &names,
	           const Common::String &marker);
	// After FUN_0041e8a8: a confirmed delete empties the field (0x0041ec58);
	// the flag is cleared and the row is -1 either way (0x0041e1e3).
	void deleteDone(bool confirmed);
	// Name entry on the New Player row: a key (0x20..0xa7 through
	// nameEntryChar; from the 16th on the length stays 15 and the key
	// replaces the 16th character), Backspace, Enter, or the blink when no
	// key; then key and length reset off the last row (0x0041e4b3).  True on
	// an Enter commit: field is then the name and row -1 (0x0041e3a2..
	// 0x0041e43c), the loop ends as on OK.
	bool nameEntry(const Common::Array<Common::String> &names, const Common::String &marker,
	               bool upperCase, bool swedishLetters);
	bool dblExit() const { return dblFlag && fieldValid(); }
	bool okExit(bool clicked, int record) const { return clicked && record == 5 && fieldValid(); }
	// The frame up to the draw: name entry, then the double-click exit test
	// (0x0041e212..0x0041e4fd).  kExitEnter on an Enter commit, kExitDouble
	// when the flag is set with a valid name whatever the record
	// (0x0041e4d3..0x0041e4fd test no record), kExitNone otherwise: the
	// frame is then drawn and exitAfterDraw tested.
	Exit exitBeforeDraw(const Common::Array<Common::String> &names, const Common::String &marker,
	                    bool upperCase, bool swedishLetters);
	// The loop test after the draw (0x0041e5bf..0x0041e5f2): kExitOk or
	// kExitNone.
	Exit exitAfterDraw(bool clicked, int record) const {
		return okExit(clicked, record) ? kExitOk : kExitNone;
	}
	// The record 0x006cd850 the exit's commit test reads: 5 after Enter
	// (0x0041e442) and OK, the frame's own after a double click.
	static int exitRecord(Exit e, int record) {
		return e == kExitDouble ? record : 5;
	}
	// The exit (0x0041e5f7..0x0041e6d4): the double-click flag cleared,
	// then the commit test.  True when the exit stores a typed name into
	// the last row: after an Enter commit (field holds the name), or when
	// exitCommits holds (commitAtExit, field then holds the name).
	bool leave(Exit e, int record, int count);
	// On any exit: record 5, no pending Enter and the last row selected run
	// the commit of a typed name (0x0041e6af..0x0041e6d4), whatever that
	// row holds.
	bool exitCommits(int record, int count) const;
	// The name the commits store: the typed bytes, then the 16th character
	// when one was typed (0x0041e39e..0x0041e3cc, 0x0041e6db..0x0041e709).
	Common::String typedName() const;
	// The exit commit's state change: field = the name, row = count - 1.
	void commitAtExit(int count);
};

// The port's side of the player screen's rows: its player slots (empty =
// unused, kept packed by storePlayerName and removePlayerName) give the rows
// as the original's table 0x0049657c holds them, the players then marker
// while there are fewer than n; rowSlot[r] is row r's slot, newRow for the
// marker's row.
void playerScreenRows(const Common::String *players, uint n, const Common::String &marker, int newRow,
                      Common::Array<Common::String> &names, int *rowSlot);
// A stored name goes into the first free slot, which is the New Player
// row's (0x0041e3fa..0x0041e436, 0x0041e71c..0x0041e758); -1 when none.
int storePlayerName(Common::String *players, uint n, const Common::String &name);
// The names part of FUN_0041e8a8's delete: the later players move up one
// slot and the last is freed.
void removePlayerName(Common::String *players, uint n, uint slot);
// The end of an exit on the port's slots: st.leave (commit: the original
// stores the typed name into the last row), then the name goes into a free
// slot when the last row is the New Player row (stored); with five players
// the original's store of the typed name over the fifth name is left out
// (no empty-named player, functions/ui-FUN_0041dea4.md differences).
// chosen: the stored slot, else the current player's, -1 none.
struct PlayerScreenExitResult {
	bool commit = false;
	bool stored = false;
	int chosen = -1;
};
PlayerScreenExitResult playerScreenExit(PlayerScreenState &st, PlayerScreenState::Exit e, int record,
                                        Common::String *players, uint n, const int *rowSlot,
                                        int count, int newRow);

} // namespace Atlantis
} // namespace CryOmni3D

#endif // CRYOMNI3D_ATLANTIS_MENU_LAYOUT_H
