#include "parse_tree.hpp"

#include <ostream>
#include <string>
#include <utility>

namespace octac::parser {

    namespace {

        constexpr std::size_t kIndentWidth = 2;

        void write(std::ostream &out, const ParseNode &node, std::size_t depth) {
            out << std::string(depth * kIndentWidth, ' ');
            if (node.isLeaf()) {
                out << node.token() << '\n';
                return;
            }
            out << nonTerminalName(node.symbol());
            if (node.children.empty()) {
                out << " (empty)";
            }
            out << '\n';
            for (const ParseNode &child : node.children) {
                write(out, child, depth + 1);
            }
        }

    }  // namespace

    ParseNode::ParseNode(NonTerminal symbol) : label(symbol) {
    }

    ParseNode::ParseNode(lexer::Token token) : label(std::move(token)) {
    }

    bool ParseNode::isLeaf() const {
        return std::holds_alternative<lexer::Token>(label);
    }

    NonTerminal ParseNode::symbol() const {
        return std::get<NonTerminal>(label);
    }

    const lexer::Token &ParseNode::token() const {
        return std::get<lexer::Token>(label);
    }

    const char *nonTerminalName(NonTerminal symbol) {
        switch (symbol) {
        case NonTerminal::Program:      return "Program";
        case NonTerminal::ProgramP:     return "ProgramP";
        case NonTerminal::Params:       return "Params";
        case NonTerminal::ParamList:    return "ParamList";
        case NonTerminal::ParamListP:   return "ParamListP";
        case NonTerminal::Type:         return "Type";
        case NonTerminal::TypeP:        return "TypeP";
        case NonTerminal::TypeP2:       return "TypeP2";
        case NonTerminal::Stmts:        return "Stmts";
        case NonTerminal::StmtsP:       return "StmtsP";
        case NonTerminal::Stmt:         return "Stmt";
        case NonTerminal::StmtP:        return "StmtP";
        case NonTerminal::StmtP2:       return "StmtP2";
        case NonTerminal::StmtP3:       return "StmtP3";
        case NonTerminal::StmtP4:       return "StmtP4";
        case NonTerminal::StmtP5:       return "StmtP5";
        case NonTerminal::ElseIfs:      return "ElseIfs";
        case NonTerminal::ElseIfsP:     return "ElseIfsP";
        case NonTerminal::Cases:        return "Cases";
        case NonTerminal::CasesP:       return "CasesP";
        case NonTerminal::CaseVal:      return "CaseVal";
        case NonTerminal::CaseValP:     return "CaseValP";
        case NonTerminal::Expr:         return "Expr";
        case NonTerminal::ExprP:        return "ExprP";
        case NonTerminal::AndExpr:      return "AndExpr";
        case NonTerminal::AndExprP:     return "AndExprP";
        case NonTerminal::NotExpr:      return "NotExpr";
        case NonTerminal::Comp:         return "Comp";
        case NonTerminal::CompP:        return "CompP";
        case NonTerminal::CompP2:       return "CompP2";
        case NonTerminal::Sum:          return "Sum";
        case NonTerminal::SumP:         return "SumP";
        case NonTerminal::Product:      return "Product";
        case NonTerminal::ProductP:     return "ProductP";
        case NonTerminal::Operand:      return "Operand";
        case NonTerminal::IndexedData:  return "IndexedData";
        case NonTerminal::IndexedDataP: return "IndexedDataP";
        case NonTerminal::Data:         return "Data";
        case NonTerminal::DataP:        return "DataP";
        case NonTerminal::Args:         return "Args";
        case NonTerminal::ArgList:      return "ArgList";
        case NonTerminal::ArgListP:     return "ArgListP";
        }
        return "Unknown";
    }

    std::ostream &operator<<(std::ostream &out, const ParseNode &node) {
        write(out, node, 0);
        return out;
    }

}  // namespace octac::parser
