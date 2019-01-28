#pragma once



#include "storage/chunk_encoder.hpp" // NEEDEDINCLUDE
#include "storage/table.hpp" // NEEDEDINCLUDE
#include "utils/load_table.hpp" // NEEDEDINCLUDE // NEEDEDINCLUDE

namespace opossum {

// Base Class for tests that should be run with various encodings
class EncodingTest : public ::testing::TestWithParam<SegmentEncodingSpec> {
 public:
  std::shared_ptr<Table> load_table_with_encoding(const std::string& path, ChunkOffset max_chunk_size) {
    const auto table = load_table(path, max_chunk_size);
    ChunkEncoder::encode_all_chunks(table, GetParam());
    return table;
  }
};

const SegmentEncodingSpec all_segment_encoding_specs[]{
    {EncodingType::Unencoded},
    {EncodingType::Dictionary, VectorCompressionType::FixedSizeByteAligned},
    {EncodingType::Dictionary, VectorCompressionType::SimdBp128},
    {EncodingType::RunLength}};

}  // namespace opossum
