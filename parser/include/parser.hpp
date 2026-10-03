#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "diagnostics.hpp"
#include "parse_tree.hpp"
#include "token.hpp"

namespace octac::parser {

    /**
     * @brief A recursive descent parser for the LL(1) grammar of OctaC.
     *
     * There is one private function per nonterminal. Each one reads the production it needs by
     * looking at the next token, which is the single token of lookahead the grammar allows.
     *
     * A syntax error is reported to the DiagnosticReporter. Parsing then recovers by skipping to
     * the end of the statement, or to the next function, and carries on, so one run finds many
     * errors.
     */
    class Parser {
    public:
        /**
         * @brief Creates a parser over a token stream.
         *
         * @param tokenStream The tokens to parse, ending with the TokenType::CompilerEof token as
         *        returned by Scanner::tokenize. The caller must keep them alive for the lifetime
         *        of the parser.
         * @param diagnostics Receives one error for every syntax error.
         */
        Parser(const std::vector<lexer::Token> &tokenStream,
               utils::DiagnosticReporter &diagnostics);

        /**
         * @brief Parses the whole token stream as a Program.
         *
         * @return The parse tree, or std::nullopt when at least one syntax error was reported.
         */
        std::optional<ParseNode> parse();

    private:
        /**
         * @brief Thrown after a syntax error has been reported, to unwind to a recovery point.
         */
        struct SyntaxError {};

        const std::vector<lexer::Token> &tokens;  ///< the tokens being parsed
        utils::DiagnosticReporter &reporter;      ///< receives syntax errors
        std::size_t pos = 0;                      ///< index of the next unread token
        std::size_t errors = 0;                   ///< syntax errors reported so far
        std::size_t braceDepth = 0;               ///< `{` consumed minus `}` consumed
        std::optional<std::size_t> lastErrorPos;  ///< token index of the last reported error

        /**
         * @brief Returns the next token without consuming it.
         *
         * @return The next token, or the end of input token once the input is exhausted.
         */
        const lexer::Token &peek() const;

        /**
         * @brief Tells whether the next token has a given type.
         *
         * @param type The type to look for.
         * @return true if the next token has type @p type.
         */
        bool check(lexer::TokenType type) const;

        /**
         * @brief Consumes the next token and appends it to a node as a leaf.
         *
         * @param parent The node that receives the leaf.
         */
        void shift(ParseNode &parent);

        /**
         * @brief Consumes a token of a given type, or reports a syntax error.
         *
         * @param type The required type.
         * @param parent The node that receives the leaf.
         * @throws SyntaxError If the next token has another type.
         */
        void expect(lexer::TokenType type, ParseNode &parent);

        /**
         * @brief Reports that the next token is not what the grammar allows.
         *
         * The message reads "expected <what> but found <token>".
         *
         * @param what A description of what the grammar allows at this point.
         */
        void reportExpected(const std::string &what);

        /**
         * @brief Reports a syntax error and unwinds to the nearest recovery point.
         *
         * @param what A description of what the grammar allows at this point.
         * @throws SyntaxError Always.
         */
        [[noreturn]] void fail(const std::string &what);

        /**
         * @brief Consumes the next token without building a node, keeping braceDepth correct.
         */
        void skip();

        /**
         * @brief Skips the rest of a broken statement.
         *
         * Stops after the next `;` or the `}` that closes a block opened by the statement, or
         * before a `}` that closes the enclosing block, a keyword that starts a statement,
         * `case`, `default` or `function`.
         *
         * @param base The value of braceDepth when the broken statement started.
         */
        void recoverStatement(std::size_t base);

        /**
         * @brief Skips to the next `function` keyword, or to the end of input.
         */
        void recoverFunction();

        /**
         * @brief Parses `FUNCTION ID ( Params ) -> Type { Stmts }` into a node.
         *
         * Program and ProgramP begin with these symbols, and each one adds them as its own
         * children.
         *
         * @param parent The node that receives the symbols.
         */
        void function(ParseNode &parent);

        /**
         * @brief Parses `{ Stmts }` into a node.
         *
         * The braces and the Stmts node become children of @p parent, as they are written in
         * the productions of Stmt, StmtP2, StmtP3 and ElseIfsP.
         *
         * @param parent The node that receives the symbols.
         */
        void bracedStmts(ParseNode &parent);

        /**
         * @brief Parses `Program -> FUNCTION ID ( Params ) -> Type { Stmts } ProgramP`.
         *
         * @return The Program node.
         */
        ParseNode program();

        /**
         * @brief Parses `ProgramP -> FUNCTION ID ( Params ) -> Type { Stmts } ProgramP | empty`.
         *
         * @return The ProgramP node.
         */
        ParseNode programP();

        /**
         * @brief Parses `Params -> ParamList | empty`.
         *
         * @return The Params node.
         */
        ParseNode params();

        /**
         * @brief Parses `ParamList -> Type ID ParamListP`.
         *
         * @return The ParamList node.
         */
        ParseNode paramList();

        /**
         * @brief Parses `ParamListP -> , Type ID ParamListP | empty`.
         *
         * @return The ParamListP node.
         */
        ParseNode paramListP();

        /**
         * @brief Parses `Type -> SCALAR | STRING | VECTOR < SCALAR TypeP | MATRIX < SCALAR TypeP2`.
         *
         * @return The Type node.
         */
        ParseNode type();

        /**
         * @brief Parses `TypeP -> > | , INT >`.
         *
         * @return The TypeP node.
         */
        ParseNode typeP();

        /**
         * @brief Parses `TypeP2 -> > | , INT , INT >`.
         *
         * @return The TypeP2 node.
         */
        ParseNode typeP2();

        /**
         * @brief Parses `Stmts -> StmtsP`.
         *
         * @return The Stmts node.
         */
        ParseNode stmts();

        /**
         * @brief Parses `StmtsP -> Stmt StmtsP | empty`.
         *
         * @return The StmtsP node.
         */
        ParseNode stmtsP();

        /**
         * @brief Parses `Stmt -> Type ID = Expr ; | ID StmtP | IF ( Expr ) { Stmts } ElseIfs StmtP2
         * | FOR ( Type ID StmtP3 | UNTIL ( Expr ) { Stmts } | DO { Stmts } UNTIL ( Expr ) ; |
         * CASEOF ( Expr ) { Cases StmtP4 | RETURN Expr ; | BREAK ; | CONTINUE ;`.
         *
         * @return The Stmt node.
         */
        ParseNode stmt();

        /**
         * @brief Parses `StmtP -> = Expr ; | [ Expr ] StmtP5 | ( Args ) ;`.
         *
         * @return The StmtP node.
         */
        ParseNode stmtP();

        /**
         * @brief Parses `StmtP2 -> ELSE { Stmts } | empty`.
         *
         * @return The StmtP2 node.
         */
        ParseNode stmtP2();

        /**
         * @brief Parses `StmtP3 -> = Expr ... Expr ) { Stmts } | IN Expr ) { Stmts }`.
         *
         * @return The StmtP3 node.
         */
        ParseNode stmtP3();

        /**
         * @brief Parses `StmtP4 -> } | DEFAULT : Stmts }`.
         *
         * @return The StmtP4 node.
         */
        ParseNode stmtP4();

        /**
         * @brief Parses `StmtP5 -> = Expr ; | [ Expr ] = Expr ;`.
         *
         * @return The StmtP5 node.
         */
        ParseNode stmtP5();

        /**
         * @brief Parses `ElseIfs -> ElseIfsP`.
         *
         * @return The ElseIfs node.
         */
        ParseNode elseIfs();

        /**
         * @brief Parses `ElseIfsP -> ELSEIF ( Expr ) { Stmts } ElseIfsP | empty`.
         *
         * @return The ElseIfsP node.
         */
        ParseNode elseIfsP();

        /**
         * @brief Parses `Cases -> CasesP`.
         *
         * @return The Cases node.
         */
        ParseNode cases();

        /**
         * @brief Parses `CasesP -> CASE CaseVal : Stmts CasesP | empty`.
         *
         * @return The CasesP node.
         */
        ParseNode casesP();

        /**
         * @brief Parses `CaseVal -> INT | FLOAT | STRING_LIT | BOOL_LIT | PLUSMINUS CaseValP`.
         *
         * @return The CaseVal node.
         */
        ParseNode caseVal();

        /**
         * @brief Parses `CaseValP -> INT | FLOAT`.
         *
         * @return The CaseValP node.
         */
        ParseNode caseValP();

        /**
         * @brief Parses `Expr -> AndExpr ExprP`.
         *
         * @return The Expr node.
         */
        ParseNode expr();

        /**
         * @brief Parses `ExprP -> OR AndExpr ExprP | empty`.
         *
         * @return The ExprP node.
         */
        ParseNode exprP();

        /**
         * @brief Parses `AndExpr -> NotExpr AndExprP`.
         *
         * @return The AndExpr node.
         */
        ParseNode andExpr();

        /**
         * @brief Parses `AndExprP -> AND NotExpr AndExprP | empty`.
         *
         * @return The AndExprP node.
         */
        ParseNode andExprP();

        /**
         * @brief Parses `NotExpr -> NOT NotExpr | Comp`.
         *
         * @return The NotExpr node.
         */
        ParseNode notExpr();

        /**
         * @brief Parses `Comp -> Sum CompP`.
         *
         * @return The Comp node.
         */
        ParseNode comp();

        /**
         * @brief Parses `CompP -> < Sum | > Sum | <= Sum | >= Sum | IS CompP2 | empty`.
         *
         * @return The CompP node.
         */
        ParseNode compP();

        /**
         * @brief Parses `CompP2 -> Sum | NOT Sum`.
         *
         * @return The CompP2 node.
         */
        ParseNode compP2();

        /**
         * @brief Parses `Sum -> Product SumP`.
         *
         * @return The Sum node.
         */
        ParseNode sum();

        /**
         * @brief Parses `SumP -> PLUSMINUS Product SumP | empty`.
         *
         * @return The SumP node.
         */
        ParseNode sumP();

        /**
         * @brief Parses `Product -> Operand ProductP`.
         *
         * @return The Product node.
         */
        ParseNode product();

        /**
         * @brief Parses `ProductP -> STAR_SLASH_MOD Operand ProductP | .* Operand ProductP | ./
         * Operand ProductP | empty`.
         *
         * @return The ProductP node.
         */
        ParseNode productP();

        /**
         * @brief Parses `Operand -> PLUSMINUS Operand | IndexedData`.
         *
         * @return The Operand node.
         */
        ParseNode operand();

        /**
         * @brief Parses `IndexedData -> Data IndexedDataP`.
         *
         * @return The IndexedData node.
         */
        ParseNode indexedData();

        /**
         * @brief Parses `IndexedDataP -> ' IndexedDataP | [ Expr ] IndexedDataP | empty`.
         *
         * @return The IndexedDataP node.
         */
        ParseNode indexedDataP();

        /**
         * @brief Parses `Data -> ID DataP | INT | FLOAT | STRING_LIT | BOOL_LIT | ( Expr ) | [ Args
         * ]`.
         *
         * @return The Data node.
         */
        ParseNode data();

        /**
         * @brief Parses `DataP -> ( Args ) | empty`.
         *
         * @return The DataP node.
         */
        ParseNode dataP();

        /**
         * @brief Parses `Args -> ArgList | empty`.
         *
         * @return The Args node.
         */
        ParseNode args();

        /**
         * @brief Parses `ArgList -> Expr ArgListP`.
         *
         * @return The ArgList node.
         */
        ParseNode argList();

        /**
         * @brief Parses `ArgListP -> , Expr ArgListP | empty`.
         *
         * @return The ArgListP node.
         */
        ParseNode argListP();
    };

}  // namespace octac::parser
