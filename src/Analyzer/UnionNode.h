#pragma once

#include <Core/NamesAndTypes.h>
#include <Core/Field.h>

#include <Analyzer/Identifier.h>
#include <Analyzer/IQueryTreeNode.h>
#include <Analyzer/ListNode.h>
#include <Analyzer/TableExpressionModifiers.h>

#include <Parsers/SelectUnionMode.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int UNSUPPORTED_METHOD;
}

/** Union node represents union of queries in query tree.
  *
  * Example: (SELECT id FROM test_table) UNION ALL (SELECT id FROM test_table_2);
  * Example: (SELECT id FROM test_table) UNION DISTINCT (SELECT id FROM test_table_2);
  * Example: (SELECT id FROM test_table) EXCEPT ALL (SELECT id FROM test_table_2);
  * Example: (SELECT id FROM test_table) EXCEPT DISTINCT (SELECT id FROM test_table_2);
  * Example: (SELECT id FROM test_table) INTERSECT ALL (SELECT id FROM test_table_2);
  * Example: (SELECT id FROM test_table) INTERSECT DISTINCT (SELECT id FROM test_table_2);
  *
  * Union node can be used as CTE.
  * Example: WITH cte_subquery AS ((SELECT id FROM test_table) UNION ALL (SELECT id FROM test_table_2)) SELECT * FROM cte_subquery;
  *
  * Union node can be used as scalar subquery.
  * Example: SELECT (SELECT 1 UNION DISTINCT SELECT 1);
  *
  * During query analysis pass union node queries must be resolved.
  */
class UnionNode;
using UnionNodePtr = std::shared_ptr<UnionNode>;

class UnionNode final : public IQueryTreeNode
{
public:
    explicit UnionNode();

    /// Returns true if union node is subquery, false otherwise
    bool isSubquery() const
    {
        return is_subquery;
    }

    /// Set union node is subquery value
    void setIsSubquery(bool is_subquery_value)
    {
        is_subquery = is_subquery_value;
    }

    /// Returns true if union node is CTE, false otherwise
    bool isCTE() const
    {
        return is_cte;
    }

    /// Set union node is CTE
    void setIsCTE(bool is_cte_value)
    {
        is_cte = is_cte_value;
    }

    /// Get union node CTE name
    const std::string & getCTEName() const
    {
        return cte_name;
    }

    /// Set union node CTE name
    void setCTEName(std::string cte_name_value)
    {
        cte_name = std::move(cte_name_value);
    }

    /// Get union mode
    SelectUnionMode getUnionMode() const
    {
        return union_mode;
    }

    /// Set union mode value
    void setUnionMode(SelectUnionMode union_mode_value)
    {
        union_mode = union_mode_value;
    }

    /// Get union modes
    const SelectUnionModes & getUnionModes() const
    {
        return union_modes;
    }

    /// Set union modes value
    void setUnionModes(const SelectUnionModes & union_modes_value)
    {
        union_modes = union_modes_value;
        union_modes_set = SelectUnionModesSet(union_modes.begin(), union_modes.end());
    }

    /// Get union node queries
    const ListNode & getQueries() const
    {
        return children[queries_child_index]->as<const ListNode &>();
    }

    /// Get union node queries
    ListNode & getQueries()
    {
        return children[queries_child_index]->as<ListNode &>();
    }

    /// Get union node queries node
    const QueryTreeNodePtr & getQueriesNode() const
    {
        return children[queries_child_index];
    }

    /// Get union node queries node
    QueryTreeNodePtr & getQueriesNode()
    {
        return children[queries_child_index];
    }

    /// Return true if union node has table expression modifiers, false otherwise
    bool hasTableExpressionModifiers() const
    {
        return table_expression_modifiers.has_value();
    }

    /// Get table expression modifiers
    const std::optional<TableExpressionModifiers> & getTableExpressionModifiers() const
    {
        return table_expression_modifiers;
    }

    /// Set table expression modifiers
    void setTableExpressionModifiers(TableExpressionModifiers table_expression_modifiers_value)
    {
        table_expression_modifiers = std::move(table_expression_modifiers_value);
    }

    /// Compute union node projection columns
    NamesAndTypes computeProjectionColumns() const;

    QueryTreeNodeType getNodeType() const override
    {
        return QueryTreeNodeType::UNION;
    }

    DataTypePtr getResultType() const override
    {
        if (constant_value)
            return constant_value->getType();

        throw Exception(ErrorCodes::UNSUPPORTED_METHOD, "Method getResultType is not supported for non scalar union node");
    }

    /// Perform constant folding for scalar union node
    void performConstantFolding(ConstantValuePtr constant_folded_value)
    {
        constant_value = std::move(constant_folded_value);
    }

    ConstantValuePtr getConstantValueOrNull() const override
    {
        return constant_value;
    }

    void dumpTreeImpl(WriteBuffer & buffer, FormatState & format_state, size_t indent) const override;

protected:
    bool isEqualImpl(const IQueryTreeNode & rhs) const override;

    void updateTreeHashImpl(HashState &) const override;

    QueryTreeNodePtr cloneImpl() const override;

    ASTPtr toASTImpl() const override;

private:
    bool is_subquery = false;
    bool is_cte = false;
    std::string cte_name;
    SelectUnionMode union_mode;
    SelectUnionModes union_modes;
    SelectUnionModesSet union_modes_set;
    ConstantValuePtr constant_value;
    std::optional<TableExpressionModifiers> table_expression_modifiers;

    static constexpr size_t queries_child_index = 0;
    static constexpr size_t children_size = queries_child_index + 1;
};

}
