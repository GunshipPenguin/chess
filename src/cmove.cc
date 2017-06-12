#include "cmove.h"
#include "defs.h"
#include <string>
#include <iostream>
#include <algorithm>

const char CMove::RANKS[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
const char CMove::FILES[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

CMove::CMove(unsigned int from, unsigned int to, unsigned int flags) {
  // Moves are stored as packed integers with the first 6 least signifigant bits
  // representing the from square, the next 6 bits representing the to square and
  // the next 8 bits representing flags

  // Squares are numbered 0-63 with a1 being 0 and h8 being 63

  // 0x3f = 63 = 6 set bits
  // 0xff = 255 = 8 set bits

  _move = ((flags & 0xff) << 12) | ((to & 0x3f) << 6) | (from & 0x3f);
}

CMove::CMove(std::string notation) {
  std::string fromSquare = notation.substr(0, 2);
  std::string toSquare = notation.substr(2, 2);

  unsigned int from = notationToIndex(fromSquare);
  unsigned int to = notationToIndex(toSquare);
  unsigned int flags = 0;

  // Promotions
  if (notation.size() == 5) {
    char promotion = tolower(notation[4]);

    switch(promotion) {
      case 'q':
        flags |= QUEEN_PROMOTION;
      case 'r':
        flags |= ROOK_PROMOTION;
      case 'b':
        flags |= BISHOP_PROMOTION;
      case 'n':
        flags |= KNIGHT_PROMOTION;
    }
  }

  _move = ((flags & 0xff) << 12) | ((to & 0x3f) << 6) | (from & 0x3f);
}

unsigned int CMove::getFrom() {
  return _move & 0x3f;
}

unsigned int CMove::getTo() {
  return ((_move >> 6) & 0x3f);
}

int CMove::getFlags() {
  return ((_move >> 12) & 0xff);
}

std::string CMove::getNotation() {
  int fromIndex = getFrom();
  int toIndex = getTo();

  std::string moveNotation = indexToNotation(fromIndex) + indexToNotation(toIndex);

  if (getFlags() & QUEEN_PROMOTION) {
    moveNotation += 'Q';
  } else if (getFlags() & KNIGHT_PROMOTION) {
    moveNotation += 'N';
  } else if (getFlags() & BISHOP_PROMOTION) {
    moveNotation += 'B';
  } else if (getFlags() & ROOK_PROMOTION) {
    moveNotation += 'R';
  }

  return moveNotation;
}

std::string CMove::indexToNotation (int index) {
  return std::string({FILES[index%8], RANKS[index/8]});
}

unsigned int CMove::notationToIndex (std::string notation) {
  int file = std::find(FILES, FILES+8, notation[0]) - std::begin(FILES);
  int rank = std::find(RANKS, RANKS+8, notation[1]) - std::begin(RANKS);

  return rank * 8 + file;
}
