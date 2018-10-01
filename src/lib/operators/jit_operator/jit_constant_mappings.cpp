#include "jit_constant_mappings.hpp"

#include <boost/bimap.hpp>
#include <boost/hana/fold.hpp>

#include <string>
#include <unordered_map>

#include "jit_types.hpp"
#include "utils/make_bimap.hpp"

namespace opossum {

const boost::bimap<JitExpressionType, std::string> jit_expression_type_to_string =
    make_bimap<JitExpressionType, std::string>({{JitExpressionType::Addition, "+"},
                                                {JitExpressionType::Column, "<COLUMN>"},
                                                {JitExpressionType::Subtraction, "-"},
                                                {JitExpressionType::Multiplication, "*"},
                                                {JitExpressionType::Division, "/"},
                                                {JitExpressionType::Modulo, "%"},
                                                {JitExpressionType::Power, "^"},
                                                {JitExpressionType::Equals, "="},
                                                {JitExpressionType::NotEquals, "<>"},
                                                {JitExpressionType::GreaterThan, ">"},
                                                {JitExpressionType::GreaterThanEquals, ">="},
                                                {JitExpressionType::LessThan, "<"},
                                                {JitExpressionType::LessThanEquals, "<="},
                                                {JitExpressionType::Like, "LIKE"},
                                                {JitExpressionType::NotLike, "NOT LIKE"},
                                                {JitExpressionType::And, "AND"},
                                                {JitExpressionType::Or, "OR"},
                                                {JitExpressionType::Not, "NOT"},
                                                {JitExpressionType::IsNull, "IS NULL"},
                                                {JitExpressionType::IsNotNull, "IS NOT NULL"}});

const boost::bimap<JitOperatorType, std::string> jit_operator_type_to_string =
    make_bimap<JitOperatorType, std::string>({{JitOperatorType::Read, "JitRead"},
                                              {JitOperatorType::Write, "JitWrite"},
                                              {JitOperatorType::Aggregate, "JitAggregate"},
                                              {JitOperatorType::Filter, "JitFilter"},
                                              {JitOperatorType::Compute, "JitCompute"},
                                              {JitOperatorType::Validate, "JitValidate"},
                                              {JitOperatorType::Limit, "JitLimit"},
                                              {JitOperatorType::WriteOffset, "WriteOffset"},
                                              {JitOperatorType::ReadValue, "JitReadValue"}});

}  // namespace opossum
