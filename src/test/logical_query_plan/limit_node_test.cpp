

#include "base_test.hpp" // NEEDEDINCLUDE

#include "logical_query_plan/limit_node.hpp" // NEEDEDINCLUDE

using namespace opossum::expression_functional;  // NOLINT

namespace opossum {

class LimitNodeTest : public ::testing::Test {
 protected:
  void SetUp() override { _limit_node = LimitNode::make(value_(10)); }

  std::shared_ptr<LimitNode> _limit_node;
};

TEST_F(LimitNodeTest, Description) { EXPECT_EQ(_limit_node->description(), "[Limit] 10"); }

TEST_F(LimitNodeTest, Equals) {
  EXPECT_EQ(*_limit_node, *_limit_node);
  EXPECT_EQ(*LimitNode::make(value_(10)), *_limit_node);
  EXPECT_NE(*LimitNode::make(value_(11)), *_limit_node);
}

TEST_F(LimitNodeTest, Copy) { EXPECT_EQ(*_limit_node->deep_copy(), *_limit_node); }

TEST_F(LimitNodeTest, NodeExpressions) {
  ASSERT_EQ(_limit_node->node_expressions.size(), 1u);
  EXPECT_EQ(*_limit_node->node_expressions.at(0u), *value_(10));
}

}  // namespace opossum
