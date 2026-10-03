#pragma once

#include <iosfwd>
#include <variant>
#include <vector>

#include "token.hpp"

namespace octac::parser {

    /**
     * @brief The nonterminals of the LL(1) grammar.
     *
     * Each name is the grammar's name for the symbol, with each prime written as P and the number
     * of primes after it from two upwards: Stmt' is StmtP, Stmt'' is StmtP2, and so on.
     */
    enum class NonTerminal {
        Program,     ///< FUNCTION ID ( Params ) -> Type { Stmts } ProgramP
        ProgramP,    ///< FUNCTION ID ( Params ) -> Type { Stmts } ProgramP | empty
        Params,      ///< ParamList | empty
        ParamList,   ///< Type ID ParamListP
        ParamListP,  ///< , Type ID ParamListP | empty

        Type,    ///< SCALAR | STRING | VECTOR < SCALAR TypeP | MATRIX < SCALAR TypeP2
        TypeP,   ///< > | , INT >
        TypeP2,  ///< > | , INT , INT >

        Stmts,   ///< StmtsP
        StmtsP,  ///< Stmt StmtsP | empty
        /// Type ID = Expr ; | ID StmtP | IF ( Expr ) { Stmts } ElseIfs StmtP2
        /// | FOR ( Type ID StmtP3 | UNTIL ( Expr ) { Stmts } | DO { Stmts } UNTIL ( Expr ) ;
        /// | CASEOF ( Expr ) { Cases StmtP4 | RETURN Expr ; | BREAK ; | CONTINUE ;
        Stmt,
        StmtP,     ///< = Expr ; | [ Expr ] StmtP5 | ( Args ) ;
        StmtP2,    ///< ELSE { Stmts } | empty
        StmtP3,    ///< = Expr ... Expr ) { Stmts } | IN Expr ) { Stmts }
        StmtP4,    ///< } | DEFAULT : Stmts }
        StmtP5,    ///< = Expr ; | [ Expr ] = Expr ;
        ElseIfs,   ///< ElseIfsP
        ElseIfsP,  ///< ELSEIF ( Expr ) { Stmts } ElseIfsP | empty
        Cases,     ///< CasesP
        CasesP,    ///< CASE CaseVal : Stmts CasesP | empty
        CaseVal,   ///< INT | FLOAT | STRING_LIT | BOOL_LIT | PLUSMINUS CaseValP
        CaseValP,  ///< INT | FLOAT

        Expr,      ///< AndExpr ExprP
        ExprP,     ///< OR AndExpr ExprP | empty
        AndExpr,   ///< NotExpr AndExprP
        AndExprP,  ///< AND NotExpr AndExprP | empty
        NotExpr,   ///< NOT NotExpr | Comp
        Comp,      ///< Sum CompP
        CompP,     ///< < Sum | > Sum | <= Sum | >= Sum | IS CompP2 | empty
        CompP2,    ///< Sum | NOT Sum
        Sum,       ///< Product SumP
        SumP,      ///< PLUSMINUS Product SumP | empty
        Product,   ///< Operand ProductP
        /// STAR_SLASH_MOD Operand ProductP | .* Operand ProductP | ./ Operand ProductP | empty
        ProductP,
        Operand,       ///< PLUSMINUS Operand | IndexedData
        IndexedData,   ///< Data IndexedDataP
        IndexedDataP,  ///< ' IndexedDataP | [ Expr ] IndexedDataP | empty
        Data,          ///< ID DataP | INT | FLOAT | STRING_LIT | BOOL_LIT | ( Expr ) | [ Args ]
        DataP,         ///< ( Args ) | empty
        Args,          ///< ArgList | empty
        ArgList,       ///< Expr ArgListP
        ArgListP       ///< , Expr ArgListP | empty
    };

    /**
     * @brief A node of the parse tree.
     *
     * A node is either a nonterminal, whose children are the symbols of the production that
     * derived it, or a leaf holding the token that matched a terminal. A nonterminal without
     * children derived the empty string.
     */
    struct ParseNode {
        std::variant<NonTerminal, lexer::Token> label;  ///< the nonterminal, or the matched token
        std::vector<ParseNode> children;                ///< the derived symbols, in order

        /**
         * @brief Creates a nonterminal node with no children.
         *
         * @param symbol The nonterminal the node stands for.
         */
        explicit ParseNode(NonTerminal symbol);

        /**
         * @brief Creates a leaf for a matched terminal.
         *
         * @param token The token that matched.
         */
        explicit ParseNode(lexer::Token token);

        /**
         * @brief Tells whether the node is a leaf holding a token.
         *
         * @return true for a terminal, false for a nonterminal.
         */
        bool isLeaf() const;

        /**
         * @brief Returns the nonterminal of the node.
         *
         * @return The nonterminal. The node should not be a leaf.
         */
        NonTerminal symbol() const;

        /**
         * @brief Returns the token of the node.
         *
         * @return The token. The node should be a leaf.
         */
        const lexer::Token &token() const;
    };

    /**
     * @brief Returns the name of a nonterminal as written in the enum, for example "StmtP2".
     *
     * @param symbol The nonterminal.
     * @return A static string naming @p symbol.
     */
    const char *nonTerminalName(NonTerminal symbol);

    /**
     * @brief Writes a parse tree, one node per line, indented two spaces per level.
     *
     * A nonterminal is written as its name, followed by " (empty)" when it derived the empty
     * string. A leaf is written as its token, in the form used by the token stream.
     */
    std::ostream &operator<<(std::ostream &out, const ParseNode &node);

}  // namespace octac::parser
