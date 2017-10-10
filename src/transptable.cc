#include "transptable.h"
#include <iostream>
#include <utility>

void TranspTable::set(const ZKey& key, TranspTableEntry entry) {
  _table.insert(std::make_pair(key.getValue(), entry));
}

void TranspTable::clear() {
  _table.clear();
}

const TranspTableEntry* TranspTable::getEntry(const ZKey& key) const {
  std::unordered_map<U64, TranspTableEntry >::const_iterator got = _table.find(key.getValue());
  if (got != _table.end()) {
    return &_table.at(key.getValue());
  } else {
    return nullptr;
  }
}
