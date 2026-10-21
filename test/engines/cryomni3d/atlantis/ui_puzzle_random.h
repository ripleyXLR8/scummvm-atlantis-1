// Verifier "ui": the original's random generators (task 23).
//
// Original: atlantis-cd-en-714240's C rand @ 0x00453cd1 (seed 1: srand has
// no caller) and the ogre queen placement FUN_004422b8 on the puzzle LCG
// FUN_00447524, emulated unmodified by atlantis-re/tools/emu/puzzle_rng.py.
// The rand is shared by the rat (FUN_00438fd4 @ 0x00439418), crab
// (FUN_0043ede8 @ 0x0043ee25) and ogre (FUN_00442a0c) puzzles and never
// reseeded, so one stream runs for the whole launch.
// Port: crtRandNext, kCrtRandStartSeed, puzzleLcgNext, ogreQueenCell
// (random.h); CryOmni3DEngine_Atlantis::crtRand, RatPuzzle::randomValue and
// CrabPuzzle::rand32767 on zero-filled storage (the engine peek of
// video_dialog.h: the engine cannot be constructed in the runner, and only
// _crtRandSeed and the puzzles' _engine are touched).

#ifndef TEST_ATLANTIS_UI_PUZZLE_RANDOM_H
#define TEST_ATLANTIS_UI_PUZZLE_RANDOM_H

#include "common/array.h"
#include "common/random.h"
#include "audio/mixer.h"
#include "graphics/managed_surface.h"

#include "cryomni3d/atlantis/random.h"

// The first test header that includes engine.h decides its access, hence
// the same defines as video_dialog.h.
#define private public
#define protected public
#include "cryomni3d/atlantis/engine.h"
#include "cryomni3d/atlantis/puzzle_crab.h"
#include "cryomni3d/atlantis/puzzle_rat.h"
#undef private
#undef protected

#include <new>

namespace UiPuzzleRandom {

// Seed 1, golden of puzzle_rng.py.
static const int kWant[] = { 16838, 5758, 10113, 17515, 31051, 5627, 23010, 7419, 16212, 4086, 2749, 12767 };

// Zero-filled storage for T, never constructed or destroyed.
template<class T>
inline T *rawStorage() {
	void *mem = ::operator new(sizeof(T));
	memset(mem, 0, sizeof(T));
	return static_cast<T *>(mem);
}

} // End of namespace UiPuzzleRandom

class UiPuzzleRandomTestSuite : public CxxTest::TestSuite {
public:
	void test_crt_rand_from_seed_1() {
		using UiPuzzleRandom::kWant;
		uint32 seed = CryOmni3D::Atlantis::kCrtRandStartSeed;
		for (uint i = 0; i < ARRAYSIZE(kWant); i++)
			TS_ASSERT_EQUALS(CryOmni3D::Atlantis::crtRandNext(seed), kWant[i]);
	}

	// The engine's one stream: the rat, the crab and the engine itself draw
	// in turn from _crtRandSeed, set as the constructor sets it, and the
	// values keep following the seed-1 sequence with no restart.
	void test_crt_rand_shared_stream() {
		using namespace UiPuzzleRandom;
		using CryOmni3D::Atlantis::CryOmni3DEngine_Atlantis;
		using CryOmni3D::Atlantis::RatPuzzle;
		using CryOmni3D::Atlantis::CrabPuzzle;
		CryOmni3DEngine_Atlantis *e = rawStorage<CryOmni3DEngine_Atlantis>();
		e->_crtRandSeed = CryOmni3D::Atlantis::kCrtRandStartSeed;
		RatPuzzle *rat = rawStorage<RatPuzzle>();
		rat->_engine = e;
		CrabPuzzle *crab = rawStorage<CrabPuzzle>();
		crab->_engine = e;
		for (uint i = 0; i < ARRAYSIZE(kWant); i++) {
			int got;
			if (i % 3 == 0)
				got = rat->RatPuzzle::randomValue();
			else if (i % 3 == 1)
				got = crab->CrabPuzzle::rand32767();
			else
				got = e->crtRand();
			TS_ASSERT_EQUALS(got, kWant[i]);
		}
		::operator delete(crab);
		::operator delete(rat);
		::operator delete(e);
	}

	void test_ogre_queen_placement() {
		static const int kCases[][4] = {
			{ 0, 6, 4, 43930 }, { 1, 4, 6, 14011 }, { 1234, 6, 4, 15577 },
			{ 32767, 6, 4, 15477 }, { 32768, 5, 5, 13800 }, { 48879, 5, 5, 35852 }
		};
		for (uint i = 0; i < ARRAYSIZE(kCases); i++) {
			uint16 seed = (uint16)kCases[i][0];
			int x, y;
			CryOmni3D::Atlantis::ogreQueenCell(seed, x, y);
			TS_ASSERT_EQUALS(x, kCases[i][1]);
			TS_ASSERT_EQUALS(y, kCases[i][2]);
			TS_ASSERT_EQUALS((int)seed, kCases[i][3]);
		}
	}
};

#endif
