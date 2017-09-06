#include "defs.h"
#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "transptable.h"
#include <string>
#include <algorithm>
#include <time.h>
#include <iostream>
#include <functional>

Search::Search(const Board& board, bool logUci) {
  _logUci = logUci;
  _board = board;
  _bestScore = 0;
}

void Search::perform(int depth) {
  _rootMax(_board, depth);

  MoveList pv = _getPv(_board, depth);
  if (_logUci) {
    _logUciInfo(pv, depth, _bestMove, _bestScore);
  }
}

void Search::_logUciInfo(const MoveList& pv, int depth, Move bestMove, int bestScore) {
  std::string pvString;
  for(auto move : pv) {
    pvString += move.getNotation() + " ";
  }

  std::string scoreString;
  if (bestScore == INF) {
    scoreString = "mate " + std::to_string(pv.size());
  } else if (_bestScore == -INF) {
    scoreString = "mate -" + std::to_string(pv.size());
  } else {
    scoreString = "cp " + std::to_string(bestScore);
  }

  std::cout << "info depth " + std::to_string(depth) + " ";
  std::cout << "score " + scoreString + " ";
  std::cout << "pv " + pvString;
  std::cout << std::endl;
}

MoveList Search::_getPv(const Board& board, int depth) {
  int bestScore = INF;
  Move bestMove;
  Board bestBoard;

  bool foundBest = false;
  Board movedBoard;


  for (auto move : MoveGen(board).getLegalMoves()) {
    movedBoard = board;
    movedBoard.doMove(move);

    ZKey movedZKey = movedBoard.getZKey();

    // Due to the way negamax works, bestScore is the lowest score
    if (_tt.contains(movedZKey) && _tt.getFlag(movedZKey) == TranspTable::EXACT && _tt.getScore(movedZKey) < bestScore) {
      foundBest = true;
      bestScore = _tt.getScore(movedZKey);
      bestMove = move;
      bestBoard = movedBoard;
    }
  }

  if (!foundBest || depth == 0) {
    return MoveList();
  } else {
    MoveList pvList = _getPv(bestBoard, depth-1);
    pvList.insert(pvList.begin(), bestMove);
    return pvList;
  }
}

Move Search::getBestMove() {
  return _bestMove;
}

int Search::getBestScore() {
  return _bestScore;
}

void Search::_rootMax(const Board& board, int depth) {
  MoveGen movegen(board);
  MoveList legalMoves = movegen.getLegalMoves();
  _orderMoves(board, legalMoves);

  int bestScore = -INF;
  int currScore;

  Move bestMove;
  Board movedBoard;
  for (auto move : legalMoves) {
    movedBoard = board;
    movedBoard.doMove(move);

    currScore = -_negaMax(movedBoard, depth-1, -INF, -bestScore);

    if (currScore > bestScore) {
      bestMove = move;
      bestScore = currScore;

      // Break if we've found a checkmate
      if (bestScore == INF) {
        break;
      }
    }
  }

  // If we couldn't find a path other than checkmate, just pick the first legal move
  if (bestMove.getFlags() & Move::NULL_MOVE) {
    bestMove = legalMoves.at(0);
  }

  _tt.set(board.getZKey(), bestScore, depth, TranspTable::EXACT);

  _bestMove = bestMove;
  _bestScore = bestScore;
}

void Search::_orderMoves(const Board& board, MoveList& moveList) {
  // Order moves by tt score
  std::sort(moveList.begin(), moveList.end(), std::bind(&Search::_compareMovesTt, this, board, std::placeholders::_1, std::placeholders::_2));

  // Ending index of sorted section of moveList
  unsigned int i;

  // Order captures by MVV/LVA
  Board movedBoard;
  for (i=0;i<moveList.size();i++) {
    movedBoard = board;
    movedBoard.doMove(moveList.at(i));

    if (!_tt.contains(movedBoard.getZKey())) {
      break;
    }
  }

  std::sort(moveList.begin() + i, moveList.end(), std::bind(&Search::_compareMovesMvvLva, this, std::placeholders::_1, std::placeholders::_2));

  // Order promotions by promotion value
  for(;i<moveList.size();i++) {
    if (!(moveList.at(i).getFlags() & Move::CAPTURE)) {
      break;
    }
  }
  std::sort(moveList.begin() + i, moveList.end(), std::bind(&Search::_compareMovesPromotionValue, this, std::placeholders::_1, std::placeholders::_2));
}

void Search::_orderMovesQSearch(MoveList& moveList) {
  std::sort(moveList.begin(), moveList.end(), std::bind(&Search::_compareMovesMvvLva, this, std::placeholders::_1, std::placeholders::_2));
}

bool Search::_compareMovesTt(Board board, Move a, Move b) {
  Board aBoard = board;
  Board bBoard = board;

  aBoard.doMove(a);
  bBoard.doMove(b);

  int aScore = _tt.contains(aBoard.getZKey()) ? _tt.getScore(aBoard.getZKey()) : -INF;
  int bScore = _tt.contains(bBoard.getZKey()) ? _tt.getScore(bBoard.getZKey()) : -INF;

  return aScore > bScore;
}

bool Search::_compareMovesMvvLva(Move a, Move b) {
  bool aIsCapture = a.getFlags() & Move::CAPTURE;
  bool bIsCapture = b.getFlags() & Move::CAPTURE;

  if (aIsCapture && !bIsCapture) {
    return true;
  } else if (bIsCapture && !aIsCapture) {
    return false;
  } else { // Both captures
    int aCaptureValue = _getPieceValue(a.getCapturedPieceType());
    int bCaptureValue = _getPieceValue(b.getCapturedPieceType());

    if (aCaptureValue != bCaptureValue) {
      return aCaptureValue > bCaptureValue;
    } else { // Captured piece type value is the same, order by attacker value
      int aPieceValue = _getPieceValue(a.getPieceType());
      int bPieceValue = _getPieceValue(b.getPieceType());

      return aPieceValue < bPieceValue;
    }
  }
}

bool Search::_compareMovesPromotionValue(Move a, Move b) {
  bool aIsPromotion = a.getFlags() & Move::PROMOTION;
  bool bIsPromotion = b.getFlags() & Move::PROMOTION;

  if (aIsPromotion && !bIsPromotion) {
    return true;
  } else if (bIsPromotion && !aIsPromotion) {
    return false;
  } else { // Both promotions
    int aPromotionValue = _getPieceValue(a.getPromotionPieceType());
    int bPromotionValue = _getPieceValue(b.getPromotionPieceType());

    return aPromotionValue > bPromotionValue;
  }
}

int Search::_getPieceValue(PieceType pieceType) {
  int score = 0;

  switch(pieceType) {
    case PAWN: score = 1;
      break;
    case KNIGHT: score = 3;
      break;
    case BISHOP: score = 3;
      break;
    case ROOK: score = 5;
      break;
    case QUEEN: score = 9;
      break;
    default: break;
  }

  return score;
}

int Search::_negaMax(const Board& board, int depth, int alpha, int beta) {
  int alphaOrig = alpha;

  ZKey zKey = board.getZKey();
  // Check transposition table cache
  if (_tt.contains(zKey) && (_tt.getDepth(zKey) >= depth)) {
    switch(_tt.getFlag(zKey)) {
      case TranspTable::EXACT:
        return _tt.getScore(zKey);
      case TranspTable::UPPER_BOUND:
        if (_tt.getScore(zKey) <= alpha) {
          return alpha;
        }
        break;
      case TranspTable::LOWER_BOUND:
        if (_tt.getScore(zKey) >= beta) {
          return beta;
        }
        break;
    }
  }

  // Transposition table lookups are inconclusive, generate moves and recurse
  MoveGen movegen(board);
  MoveList legalMoves = movegen.getLegalMoves();

  // Check for checkmate and stalemate
  if (legalMoves.size() == 0) {
    int score = board.colorIsInCheck(board.getActivePlayer()) ? -INF : 0; // -INF = checkmate, 0 = stalemate (draw)

    _tt.set(board.getZKey(), score, depth, TranspTable::EXACT);
    return score;
  }

  // Eval if depth is 0
  if (depth == 0) {
    int score = _qSearch(board, alpha, beta);
    _tt.set(board.getZKey(), score, 0, TranspTable::EXACT);
    return score;
  }

  _orderMoves(board, legalMoves);

  int bestScore = -INF;
  Board movedBoard;
  for (auto move : legalMoves) {
    movedBoard = board;
    movedBoard.doMove(move);

    bestScore = std::max(bestScore, -_negaMax(movedBoard, depth-1, -beta, -alpha));

    alpha = std::max(alpha, bestScore);
    if (alpha > beta) {
      break;
    }
  }

  // Store bestScore in transposition table
  TranspTable::Flag flag;
  if (bestScore < alphaOrig) {
    flag = TranspTable::UPPER_BOUND;
  } else if (bestScore >= beta) {
    flag = TranspTable::LOWER_BOUND;
  } else {
    flag = TranspTable::EXACT;
  }
  _tt.set(zKey, bestScore, depth, flag);

  return bestScore;
}

int Search::_qSearch(const Board& board, int alpha, int beta) {
  MoveGen movegen(board);
  MoveList legalMoves = movegen.getLegalMoves();

  // Check for checkmate / stalemate
  if (legalMoves.size() == 0) {
    if (board.colorIsInCheck(board.getActivePlayer())) { // Checkmate
      return -INF;
    } else { // Stalemate
      return 0;
    }
  }

  _orderMovesQSearch(legalMoves);
  int standPat = Eval(board, board.getActivePlayer()).getScore();

  // If node is quiet, just return eval
  if (!(legalMoves.at(0).getFlags() & Move::CAPTURE)) {
    return standPat;
  }

  if (standPat >= beta) {
    return beta;
  }
  if (alpha < standPat) {
    alpha = standPat;
  }

  Board movedBoard;
  for (auto move : legalMoves) {
    if ((move.getFlags() & Move::CAPTURE) == 0) {
      break;
    }

    movedBoard = board;
    movedBoard.doMove(move);

    int score = -_qSearch(movedBoard, -beta, -alpha);

    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }
  return alpha;
}
