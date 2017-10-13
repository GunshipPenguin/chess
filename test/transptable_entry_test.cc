#include "catch.hpp"
#include "transptable_entry.h"
#include "defs.h"

TEST_CASE("Transposition table entries work as expected") {
  Board board;

  SECTION("Transposition table entries store depth, score and flags") {
    board.setToStartPos();

    Move move(a2, a3, PAWN);
    TranspTableEntry ttEntry(1, 2, TranspTableEntry::EXACT, move);

    REQUIRE(ttEntry.getScore() == 1);
    REQUIRE(ttEntry.getDepth() == 2);
    REQUIRE(ttEntry.getFlag() == TranspTableEntry::EXACT);
    REQUIRE(ttEntry.getBestMove() == move);
  }
}
