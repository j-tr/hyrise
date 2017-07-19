#pragma once
#include <memory>
#include <string>
#include "storage/table.hpp"
namespace opossum {
class TableGenerator {
 public:
  std::shared_ptr<Table> get_table(const ChunkID chunk_size);

 protected:
  const size_t _num_columns = 10;
  const size_t _num_rows = 5 * 1000;
  const int _max_different_value = 1000;
};
}  // namespace opossum
