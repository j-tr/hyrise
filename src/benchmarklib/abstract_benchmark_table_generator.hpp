#pragma once

#include <map> // NEEDEDINCLUDE
#include <numeric> // NEEDEDINCLUDE

#include "resolve_data_type.hpp" // NEEDEDINCLUDE
#include "storage/table.hpp" // NEEDEDINCLUDE
#include "storage/value_segment.hpp" // NEEDEDINCLUDE

namespace opossum {

class AbstractBenchmarkTableGenerator {
 public:
  explicit AbstractBenchmarkTableGenerator(const opossum::ChunkOffset chunk_size) : _chunk_size(chunk_size) {}

  virtual ~AbstractBenchmarkTableGenerator() = default;

  virtual std::map<std::string, std::shared_ptr<opossum::Table>> generate_all_tables() = 0;

 protected:
  const opossum::ChunkOffset _chunk_size;

  /**
   * In TPCC and TPCH table sizes are usually defined relatively to each other.
   * E.g. the specification defines that there are 10 districts for each warehouse.
   *
   * A trivial approach to implement this in our table generator would be to iterate in nested loops and add all rows.
   * However, this makes it hard to take care of a certain chunk_size. With nested loops
   * chunks only contain as many rows as there are iterations in the most inner loop.
   *
   * In this method we basically generate the whole column in a single loop,
   * so that we can easily split when a Chunk is full. To do that we have all the cardinalities of the influencing
   * tables:
   * E.g. for the CUSTOMER table we have the following cardinalities:
   * indices[0] = warehouse_size = 1
   * indices[1] = district_size = 10
   * indices[2] = customer_size = 3000
   * So in total we have to generate 1*10*3000 = 30 000 customers.
   *
   * @tparam T                  the type of the column
   * @param table               the column shall be added to this table as well as column metadata
   * @param name                the name of the column
   * @param cardinalities       the cardinalities of the different 'nested loops',
   *                            e.g. 10 districts per warehouse results in {1, 10}
   * @param generator_function  a lambda function to generate a vector of values for this column
   */
  template <typename T>
  void add_column(std::vector<opossum::Segments>& segments_by_chunk,
                  opossum::TableColumnDefinitions& column_definitions, std::string name,
                  std::shared_ptr<std::vector<size_t>> cardinalities,
                  const std::function<std::vector<T>(std::vector<size_t>)>& generator_function) {
    bool is_first_column = column_definitions.size() == 0;

    auto data_type = opossum::data_type_from_type<T>();
    column_definitions.emplace_back(name, data_type);

    /**
     * Calculate the total row count for this column based on the cardinalities of the influencing tables.
     * For the CUSTOMER table this calculates 1*10*3000
     */
    auto loop_count =
        std::accumulate(std::begin(*cardinalities), std::end(*cardinalities), 1u, std::multiplies<size_t>());

    tbb::concurrent_vector<T> data;
    data.reserve(_chunk_size);

    /**
     * The loop over all records that the final column of the table will contain, e.g. loop_count = 30 000 for CUSTOMER
     */
    size_t row_index = 0;

    for (size_t loop_index = 0; loop_index < loop_count; loop_index++) {
      std::vector<size_t> indices(cardinalities->size());

      /**
       * Calculate indices for internal loops
       *
       * We have to take care of writing IDs for referenced table correctly, e.g. when they are used as foreign key.
       * In that case the 'generator_function' has to be able to access the current index of our loops correctly,
       * which we ensure by defining them here.
       *
       * For example for CUSTOMER:
       * WAREHOUSE_ID | DISTRICT_ID | CUSTOMER_ID
       * indices[0]   | indices[1]  | indices[2]
       */
      for (size_t loop = 0; loop < cardinalities->size(); loop++) {
        auto divisor = std::accumulate(std::begin(*cardinalities) + loop + 1, std::end(*cardinalities), 1u,
                                       std::multiplies<size_t>());
        indices[loop] = (loop_index / divisor) % cardinalities->at(loop);
      }

      /**
       * Actually generating and adding values.
       * Pass in the previously generated indices to use them in 'generator_function',
       * e.g. when generating IDs.
       * We generate a vector of values with variable length
       * and iterate it to add to the output segment.
       */
      auto values = generator_function(indices);
      for (T& value : values) {
        data.push_back(value);

        // write output chunks if segment size has reached chunk_size
        if (row_index % _chunk_size == _chunk_size - 1) {
          auto value_segment = std::make_shared<opossum::ValueSegment<T>>(std::move(data));

          if (is_first_column) {
            segments_by_chunk.emplace_back();
            segments_by_chunk.back().push_back(value_segment);
          } else {
            opossum::ChunkID chunk_id{static_cast<uint32_t>(row_index / _chunk_size)};
            segments_by_chunk[chunk_id].push_back(value_segment);
          }

          // reset data
          data.clear();
          data.reserve(_chunk_size);
        }
        row_index++;
      }
    }

    // write partially filled last chunk
    if (row_index % _chunk_size != 0) {
      auto value_segment = std::make_shared<opossum::ValueSegment<T>>(std::move(data));

      // add Chunk if it is the first column, e.g. WAREHOUSE_ID in the example above
      if (is_first_column) {
        segments_by_chunk.emplace_back();
        segments_by_chunk.back().push_back(value_segment);
      } else {
        opossum::ChunkID chunk_id{static_cast<uint32_t>(row_index / _chunk_size)};
        segments_by_chunk[chunk_id].push_back(value_segment);
      }
    }
  }

  /**
   * This method simplifies the interface for columns
   * where only a single element is added in the inner loop.
   *
   * @tparam T                  the type of the column
   * @param table               the column shall be added to this table as well as column metadata
   * @param name                the name of the column
   * @param cardinalities       the cardinalities of the different 'nested loops',
   *                            e.g. 10 districts per warehouse results in {1, 10}
   * @param generator_function  a lambda function to generate a value for this column
   */
  template <typename T>
  void add_column(std::vector<opossum::Segments>& segments_by_chunk,
                  opossum::TableColumnDefinitions& column_definitions, std::string name,
                  std::shared_ptr<std::vector<size_t>> cardinalities,
                  const std::function<T(std::vector<size_t>)>& generator_function) {
    const std::function<std::vector<T>(std::vector<size_t>)> wrapped_generator_function =
        [generator_function](std::vector<size_t> indices) { return std::vector<T>({generator_function(indices)}); };
    add_column(segments_by_chunk, column_definitions, name, cardinalities, wrapped_generator_function);
  }
};
}  // namespace opossum
