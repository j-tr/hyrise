#include "calibration_query_generator.hpp"

#include <algorithm>
#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>
#include <experimental/iterator>
#include <iterator>
#include <iostream>
#include <random>
#include <utils/assert.hpp>

namespace opossum {

    const std::vector<std::string> CalibrationQueryGenerator::generate_queries(const std::vector<CalibrationTableSpecification>& table_definitions) {

      std::vector<std::string> queries;

      for (const auto & table_definition : table_definitions) {
//        std::cout << "Generating queries for " << table_definition.table_name << std::endl;

// TODO: Add queries for Aggregates, Joins
        queries.push_back(CalibrationQueryGenerator::_generate_table_scans(table_definition));
      }

      return queries;
    }

    const std::string CalibrationQueryGenerator::_generate_table_scans(const CalibrationTableSpecification& table_definition) {
      // TODO:
      auto string_template = "SELECT %1% FROM %2% WHERE %3% %4% %5%;";
//      auto and_or = " %1% ";
//      auto additional_predicates =  "%1% %2% %3%";

      auto select_columns = _generate_select_columns(table_definition.columns);
      auto table_name = table_definition.table_name;

      auto column_definitions = table_definition.columns;
      std::random_device random_device;
      std::mt19937 engine{random_device()};
      std::uniform_int_distribution<long> dist(0, column_definitions.size() - 1);

      auto filter_column = std::next(column_definitions.begin(), dist(engine));
      auto filter_column_name = filter_column->first;
      auto filter_column_value = _generate_table_scan_predicate(filter_column->second);

      return boost::str(boost::format(string_template) % select_columns % table_name % filter_column_name % "=" % filter_column_value);
    }

    const std::string CalibrationQueryGenerator::_generate_table_scan_predicate(const CalibrationColumnSpecification& column_definition) {
      auto column_type = column_definition.type;
      if (column_type == "int") return "10";
      if (column_type == "string") return "'Aaron Anderson'";
      if (column_type == "float") return "0.5";

      Fail("Unsupported data type in CalibrationQueryGenerator");
    }

    const std::vector<std::string> CalibrationQueryGenerator::_get_column_names(const std::map<std::string, CalibrationColumnSpecification>& column_definitions) {
      std::vector<std::string> column_names;
      column_names.reserve(column_definitions.size());

      for (const auto &elem : column_definitions) {
        column_names.push_back(elem.first);
      }

      return column_names;
    }

    const std::string CalibrationQueryGenerator::_generate_select_columns(const std::map<std::string, CalibrationColumnSpecification>& column_definitions) {
      auto column_names = CalibrationQueryGenerator::_get_column_names(column_definitions);

      // Random device for shuffling
      std::random_device rd;
      std::mt19937 g(rd());
      std::shuffle(column_names.begin(), column_names.end(), g);

      // Random device for sampling
      std::random_device random_device;
      std::mt19937 engine{random_device()};
      std::uniform_int_distribution<long> dist(0, column_names.size() - 1);

      long number_of_columns = dist(engine);
      if (number_of_columns == 0) return "*";

      std::vector<std::string> out;
      std::sample(column_names.begin(), column_names.end(), std::back_inserter(out), number_of_columns, std::mt19937{std::random_device{}()});
      return boost::algorithm::join(out, ", ");
    }
}