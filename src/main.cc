#include "uci.h"
#include "psquaretable.h"
#include "zkey.h"
#include "attacktable.h"
#include "raytable.h"
#include "movepicker.h"
#include "eval.h"

int main() {
  RayTable::init();
  PSquareTable::init();
  ZKey::init();
  MovePicker::init();
  AttackTable::init();
  Eval::init();

  Uci uci;
  uci.start();

  return 0;
}
