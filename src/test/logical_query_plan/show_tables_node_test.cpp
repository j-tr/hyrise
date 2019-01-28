

#include "base_test.hpp" // NEEDEDINCLUDE

#include "logical_query_plan/show_tables_node.hpp" // NEEDEDINCLUDE

using namespace opossum::expression_functional;  // NOLINT

namespace opossum {

class ShowTablesNodeTest : public BaseTest {
 protected:
  void SetUp() override { _show_tables_node = ShowTablesNode::make(); }

  std::shared_ptr<ShowTablesNode> _show_tables_node;
};

TEST_F(ShowTablesNodeTest, Columns) {
  ASSERT_EQ(_show_tables_node->column_expressions().size(), 1u);
  EXPECT_EQ(*_show_tables_node->column_expressions().at(0), *lqp_column_({_show_tables_node, ColumnID{0}}));
}

TEST_F(ShowTablesNodeTest, Description) { EXPECT_EQ(_show_tables_node->description(), "[ShowTables]"); }
TEST_F(ShowTablesNodeTest, Equals) { EXPECT_EQ(*_show_tables_node, *_show_tables_node); }
TEST_F(ShowTablesNodeTest, Copy) { EXPECT_EQ(*_show_tables_node->deep_copy(), *_show_tables_node); }

TEST_F(ShowTablesNodeTest, NodeExpressions) { ASSERT_EQ(_show_tables_node->node_expressions.size(), 0u); }

}  // namespace opossum
