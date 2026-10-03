#include "parser.hpp"

#include <algorithm>

namespace octac::parser {

    using lexer::Token;
    using lexer::TokenType;

    namespace {

        /**
         * @brief Tells whether a token type can begin a Type.
         *
         * @param type The type of the next token.
         * @return true for the tokens in FIRST(Type): a scalar type, `string`, `vector` and
         *         `matrix`.
         */
        bool startsType(TokenType type) {
            switch (type) {
            case TokenType::Scalar:
            case TokenType::String:
            case TokenType::Vector:
            case TokenType::Matrix: return true;
            default:                return false;
            }
        }

        /**
         * @brief Tells whether a token type can begin a Stmt.
         *
         * @param type The type of the next token.
         * @return true for the tokens in FIRST(Stmt): an identifier, a statement keyword or a
         *         token in FIRST(Type).
         */
        bool startsStatement(TokenType type) {
            switch (type) {
            case TokenType::Identifier:
            case TokenType::If:
            case TokenType::For:
            case TokenType::Until:
            case TokenType::Do:
            case TokenType::CaseOf:
            case TokenType::Return:
            case TokenType::Break:
            case TokenType::Continue:   return true;
            default:                    return startsType(type);
            }
        }

        /**
         * @brief Tells whether a token type can begin an Expr.
         *
         * @param type The type of the next token.
         * @return true for the tokens in FIRST(Expr).
         */
        bool startsExpression(TokenType type) {
            switch (type) {
            case TokenType::Not:
            case TokenType::LParen:
            case TokenType::LBracket:
            case TokenType::PlusMinus:
            case TokenType::Identifier:
            case TokenType::BoolLit:
            case TokenType::Int:
            case TokenType::Float:
            case TokenType::StringLit:  return true;
            default:                    return false;
            }
        }

        /**
         * @brief Tells whether a token type can follow a list of statements.
         *
         * @param type The type of the next token.
         * @return true for `}`, `case`, `default`, `function` and the end of input.
         */
        bool endsStatements(TokenType type) {
            switch (type) {
            case TokenType::RBrace:
            case TokenType::Case:
            case TokenType::Default:
            case TokenType::Function:
            case TokenType::CompilerEof: return true;
            default:                     return false;
            }
        }

        /**
         * @brief Tells whether error recovery should stop in front of a token type.
         *
         * Recovery stops at the start of a statement, and at `case` and `default`. An identifier
         * does not stop it, because a broken statement is likely to contain one.
         *
         * @param type The type of the next token.
         * @return true if skipping should stop before the token.
         */
        bool isRecoveryPoint(TokenType type) {
            switch (type) {
            case TokenType::Case:
            case TokenType::Default:    return true;
            case TokenType::Identifier: return false;
            default:                    return startsStatement(type);
            }
        }

        /**
         * @brief Describes a token for the "but found" part of an error message.
         *
         * @param token The token that was found.
         * @return "end of input" for the end of input, otherwise the lexeme in single quotes.
         */
        std::string describeFound(const Token &token) {
            if (token.type == TokenType::CompilerEof) {
                return "end of input";
            }
            return "'" + token.value + "'";
        }

    }  // namespace

    Parser::Parser(const std::vector<Token> &tokenStream, utils::DiagnosticReporter &diagnostics)
        : tokens(tokenStream), reporter(diagnostics) {
    }

    const Token &Parser::peek() const {
        return tokens[std::min(pos, tokens.size() - 1)];
    }

    bool Parser::check(TokenType type) const {
        return peek().type == type;
    }

    void Parser::shift(ParseNode &parent) {
        parent.children.emplace_back(peek());
        skip();
    }

    void Parser::expect(TokenType type, ParseNode &parent) {
        if (!check(type)) {
            fail(lexer::tokenTypeDescription(type));
        }
        shift(parent);
    }

    void Parser::reportExpected(const std::string &what) {
        ++errors;
        if (lastErrorPos == pos) {
            return;
        }
        lastErrorPos = pos;
        const Token &found = peek();
        reporter.error(found.line, found.column, std::max<std::size_t>(found.value.size(), 1),
                       "expected " + what + " but found " + describeFound(found));
    }

    void Parser::fail(const std::string &what) {
        reportExpected(what);
        throw SyntaxError{};
    }

    void Parser::skip() {
        if (check(TokenType::LBrace)) {
            ++braceDepth;
        } else if (check(TokenType::RBrace) && braceDepth > 0) {
            --braceDepth;
        }
        if (pos < tokens.size() - 1) {
            ++pos;
        }
    }

    void Parser::recoverStatement(std::size_t base) {
        while (!check(TokenType::CompilerEof) && !check(TokenType::Function)) {
            const TokenType type = peek().type;
            if (braceDepth <= base) {
                if (type == TokenType::Semicolon) {
                    skip();
                    return;
                }
                if (type == TokenType::RBrace || isRecoveryPoint(type)) {
                    return;
                }
            }
            const bool closesBlock = type == TokenType::RBrace && braceDepth == base + 1;
            skip();
            if (closesBlock) {
                return;
            }
        }
    }

    void Parser::recoverFunction() {
        while (!check(TokenType::Function) && !check(TokenType::CompilerEof)) {
            skip();
        }
        braceDepth = 0;
    }

    std::optional<ParseNode> Parser::parse() {
        ParseNode root = program();
        if (errors > 0) {
            return std::nullopt;
        }
        return root;
    }

    void Parser::function(ParseNode &parent) {
        expect(TokenType::Function, parent);
        expect(TokenType::Identifier, parent);
        expect(TokenType::LParen, parent);
        parent.children.push_back(params());
        expect(TokenType::RParen, parent);
        expect(TokenType::Arrow, parent);
        parent.children.push_back(type());
        bracedStmts(parent);
    }

    void Parser::bracedStmts(ParseNode &parent) {
        expect(TokenType::LBrace, parent);
        parent.children.push_back(stmts());
        expect(TokenType::RBrace, parent);
    }

    ParseNode Parser::program() {
        ParseNode node(NonTerminal::Program);
        try {
            function(node);
        } catch (const SyntaxError &) {
            recoverFunction();
        }
        node.children.push_back(programP());
        return node;
    }

    ParseNode Parser::programP() {
        ParseNode node(NonTerminal::ProgramP);
        if (!check(TokenType::Function) && !check(TokenType::CompilerEof)) {
            reportExpected("'function' or end of input");
            recoverFunction();
        }
        if (check(TokenType::Function)) {
            try {
                function(node);
            } catch (const SyntaxError &) {
                recoverFunction();
            }
            node.children.push_back(programP());
        }
        return node;
    }

    ParseNode Parser::params() {
        ParseNode node(NonTerminal::Params);
        if (startsType(peek().type)) {
            node.children.push_back(paramList());
        }
        return node;
    }

    ParseNode Parser::paramList() {
        ParseNode node(NonTerminal::ParamList);
        node.children.push_back(type());
        expect(TokenType::Identifier, node);
        node.children.push_back(paramListP());
        return node;
    }

    ParseNode Parser::paramListP() {
        ParseNode node(NonTerminal::ParamListP);
        if (check(TokenType::Comma)) {
            shift(node);
            node.children.push_back(type());
            expect(TokenType::Identifier, node);
            node.children.push_back(paramListP());
        }
        return node;
    }

    ParseNode Parser::type() {
        ParseNode node(NonTerminal::Type);
        switch (peek().type) {
        case TokenType::Scalar:
        case TokenType::String: shift(node); break;
        case TokenType::Vector:
            shift(node);
            expect(TokenType::Less, node);
            expect(TokenType::Scalar, node);
            node.children.push_back(typeP());
            break;
        case TokenType::Matrix:
            shift(node);
            expect(TokenType::Less, node);
            expect(TokenType::Scalar, node);
            node.children.push_back(typeP2());
            break;
        default: fail("a type");
        }
        return node;
    }

    ParseNode Parser::typeP() {
        ParseNode node(NonTerminal::TypeP);
        switch (peek().type) {
        case TokenType::Greater: shift(node); break;
        case TokenType::Comma:
            shift(node);
            expect(TokenType::Int, node);
            expect(TokenType::Greater, node);
            break;
        default: fail("'>' or ','");
        }
        return node;
    }

    ParseNode Parser::typeP2() {
        ParseNode node(NonTerminal::TypeP2);
        switch (peek().type) {
        case TokenType::Greater: shift(node); break;
        case TokenType::Comma:
            shift(node);
            expect(TokenType::Int, node);
            expect(TokenType::Comma, node);
            expect(TokenType::Int, node);
            expect(TokenType::Greater, node);
            break;
        default: fail("'>' or ','");
        }
        return node;
    }

    ParseNode Parser::stmts() {
        ParseNode node(NonTerminal::Stmts);
        node.children.push_back(stmtsP());
        return node;
    }

    ParseNode Parser::stmtsP() {
        ParseNode node(NonTerminal::StmtsP);
        if (startsStatement(peek().type)) {
            const std::size_t base = braceDepth;
            try {
                node.children.push_back(stmt());
            } catch (const SyntaxError &) {
                recoverStatement(base);
            }
            node.children.push_back(stmtsP());
        } else if (!endsStatements(peek().type)) {
            reportExpected("a statement or '}'");
            recoverStatement(braceDepth);
            node.children.push_back(stmtsP());
        }
        return node;
    }

    ParseNode Parser::stmt() {
        ParseNode node(NonTerminal::Stmt);
        switch (peek().type) {
        case TokenType::Scalar:
        case TokenType::String:
        case TokenType::Vector:
        case TokenType::Matrix:
            node.children.push_back(type());
            expect(TokenType::Identifier, node);
            expect(TokenType::Assignment, node);
            node.children.push_back(expr());
            expect(TokenType::Semicolon, node);
            break;
        case TokenType::Identifier:
            shift(node);
            node.children.push_back(stmtP());
            break;
        case TokenType::If:
            shift(node);
            expect(TokenType::LParen, node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            bracedStmts(node);
            node.children.push_back(elseIfs());
            node.children.push_back(stmtP2());
            break;
        case TokenType::For:
            shift(node);
            expect(TokenType::LParen, node);
            node.children.push_back(type());
            expect(TokenType::Identifier, node);
            node.children.push_back(stmtP3());
            break;
        case TokenType::Until:
            shift(node);
            expect(TokenType::LParen, node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            bracedStmts(node);
            break;
        case TokenType::Do:
            shift(node);
            bracedStmts(node);
            expect(TokenType::Until, node);
            expect(TokenType::LParen, node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            expect(TokenType::Semicolon, node);
            break;
        case TokenType::CaseOf:
            shift(node);
            expect(TokenType::LParen, node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            expect(TokenType::LBrace, node);
            node.children.push_back(cases());
            node.children.push_back(stmtP4());
            break;
        case TokenType::Return:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::Semicolon, node);
            break;
        case TokenType::Break:
        case TokenType::Continue:
            shift(node);
            expect(TokenType::Semicolon, node);
            break;
        default: fail("a statement");
        }
        return node;
    }

    ParseNode Parser::stmtP() {
        ParseNode node(NonTerminal::StmtP);
        switch (peek().type) {
        case TokenType::Assignment:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::Semicolon, node);
            break;
        case TokenType::LBracket:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::RBracket, node);
            node.children.push_back(stmtP5());
            break;
        case TokenType::LParen:
            shift(node);
            node.children.push_back(args());
            expect(TokenType::RParen, node);
            expect(TokenType::Semicolon, node);
            break;
        default: fail("'(', '[' or an assignment operator");
        }
        return node;
    }

    ParseNode Parser::stmtP2() {
        ParseNode node(NonTerminal::StmtP2);
        if (check(TokenType::Else)) {
            shift(node);
            bracedStmts(node);
        }
        return node;
    }

    ParseNode Parser::stmtP3() {
        ParseNode node(NonTerminal::StmtP3);
        switch (peek().type) {
        case TokenType::Assignment:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::Ellipsis, node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            bracedStmts(node);
            break;
        case TokenType::In:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            bracedStmts(node);
            break;
        default: fail("an assignment operator or 'in'");
        }
        return node;
    }

    ParseNode Parser::stmtP4() {
        ParseNode node(NonTerminal::StmtP4);
        switch (peek().type) {
        case TokenType::RBrace: shift(node); break;
        case TokenType::Default:
            shift(node);
            expect(TokenType::Colon, node);
            node.children.push_back(stmts());
            expect(TokenType::RBrace, node);
            break;
        default: fail("'default' or '}'");
        }
        return node;
    }

    ParseNode Parser::stmtP5() {
        ParseNode node(NonTerminal::StmtP5);
        switch (peek().type) {
        case TokenType::Assignment:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::Semicolon, node);
            break;
        case TokenType::LBracket:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::RBracket, node);
            expect(TokenType::Assignment, node);
            node.children.push_back(expr());
            expect(TokenType::Semicolon, node);
            break;
        default: fail("an assignment operator or '['");
        }
        return node;
    }

    ParseNode Parser::elseIfs() {
        ParseNode node(NonTerminal::ElseIfs);
        node.children.push_back(elseIfsP());
        return node;
    }

    ParseNode Parser::elseIfsP() {
        ParseNode node(NonTerminal::ElseIfsP);
        if (check(TokenType::ElseIf)) {
            shift(node);
            expect(TokenType::LParen, node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            bracedStmts(node);
            node.children.push_back(elseIfsP());
        }
        return node;
    }

    ParseNode Parser::cases() {
        ParseNode node(NonTerminal::Cases);
        node.children.push_back(casesP());
        return node;
    }

    ParseNode Parser::casesP() {
        ParseNode node(NonTerminal::CasesP);
        if (check(TokenType::Case)) {
            shift(node);
            node.children.push_back(caseVal());
            expect(TokenType::Colon, node);
            node.children.push_back(stmts());
            node.children.push_back(casesP());
        }
        return node;
    }

    ParseNode Parser::caseVal() {
        ParseNode node(NonTerminal::CaseVal);
        switch (peek().type) {
        case TokenType::Int:
        case TokenType::Float:
        case TokenType::StringLit:
        case TokenType::BoolLit:   shift(node); break;
        case TokenType::PlusMinus:
            shift(node);
            node.children.push_back(caseValP());
            break;
        default: fail("a case value");
        }
        return node;
    }

    ParseNode Parser::caseValP() {
        ParseNode node(NonTerminal::CaseValP);
        if (check(TokenType::Int) || check(TokenType::Float)) {
            shift(node);
        } else {
            fail("an integer or float literal");
        }
        return node;
    }

    ParseNode Parser::expr() {
        ParseNode node(NonTerminal::Expr);
        node.children.push_back(andExpr());
        node.children.push_back(exprP());
        return node;
    }

    ParseNode Parser::exprP() {
        ParseNode node(NonTerminal::ExprP);
        if (check(TokenType::Or)) {
            shift(node);
            node.children.push_back(andExpr());
            node.children.push_back(exprP());
        }
        return node;
    }

    ParseNode Parser::andExpr() {
        ParseNode node(NonTerminal::AndExpr);
        node.children.push_back(notExpr());
        node.children.push_back(andExprP());
        return node;
    }

    ParseNode Parser::andExprP() {
        ParseNode node(NonTerminal::AndExprP);
        if (check(TokenType::And)) {
            shift(node);
            node.children.push_back(notExpr());
            node.children.push_back(andExprP());
        }
        return node;
    }

    ParseNode Parser::notExpr() {
        ParseNode node(NonTerminal::NotExpr);
        if (check(TokenType::Not)) {
            shift(node);
            node.children.push_back(notExpr());
        } else {
            node.children.push_back(comp());
        }
        return node;
    }

    ParseNode Parser::comp() {
        ParseNode node(NonTerminal::Comp);
        node.children.push_back(sum());
        node.children.push_back(compP());
        return node;
    }

    ParseNode Parser::compP() {
        ParseNode node(NonTerminal::CompP);
        switch (peek().type) {
        case TokenType::Less:
        case TokenType::Greater:
        case TokenType::LessEq:
        case TokenType::GreaterEq:
            shift(node);
            node.children.push_back(sum());
            break;
        case TokenType::Is:
            shift(node);
            node.children.push_back(compP2());
            break;
        default: break;
        }
        return node;
    }

    ParseNode Parser::compP2() {
        ParseNode node(NonTerminal::CompP2);
        if (check(TokenType::Not)) {
            shift(node);
        }
        node.children.push_back(sum());
        return node;
    }

    ParseNode Parser::sum() {
        ParseNode node(NonTerminal::Sum);
        node.children.push_back(product());
        node.children.push_back(sumP());
        return node;
    }

    ParseNode Parser::sumP() {
        ParseNode node(NonTerminal::SumP);
        if (check(TokenType::PlusMinus)) {
            shift(node);
            node.children.push_back(product());
            node.children.push_back(sumP());
        }
        return node;
    }

    ParseNode Parser::product() {
        ParseNode node(NonTerminal::Product);
        node.children.push_back(operand());
        node.children.push_back(productP());
        return node;
    }

    ParseNode Parser::productP() {
        ParseNode node(NonTerminal::ProductP);
        switch (peek().type) {
        case TokenType::StarSlashMod:
        case TokenType::DotStar:
        case TokenType::DotSlash:
            shift(node);
            node.children.push_back(operand());
            node.children.push_back(productP());
            break;
        default: break;
        }
        return node;
    }

    ParseNode Parser::operand() {
        ParseNode node(NonTerminal::Operand);
        if (check(TokenType::PlusMinus)) {
            shift(node);
            node.children.push_back(operand());
        } else {
            node.children.push_back(indexedData());
        }
        return node;
    }

    ParseNode Parser::indexedData() {
        ParseNode node(NonTerminal::IndexedData);
        node.children.push_back(data());
        node.children.push_back(indexedDataP());
        return node;
    }

    ParseNode Parser::indexedDataP() {
        ParseNode node(NonTerminal::IndexedDataP);
        switch (peek().type) {
        case TokenType::Transpose:
            shift(node);
            node.children.push_back(indexedDataP());
            break;
        case TokenType::LBracket:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::RBracket, node);
            node.children.push_back(indexedDataP());
            break;
        default: break;
        }
        return node;
    }

    ParseNode Parser::data() {
        ParseNode node(NonTerminal::Data);
        switch (peek().type) {
        case TokenType::Identifier:
            shift(node);
            node.children.push_back(dataP());
            break;
        case TokenType::Int:
        case TokenType::Float:
        case TokenType::StringLit:
        case TokenType::BoolLit:   shift(node); break;
        case TokenType::LParen:
            shift(node);
            node.children.push_back(expr());
            expect(TokenType::RParen, node);
            break;
        case TokenType::LBracket:
            shift(node);
            node.children.push_back(args());
            expect(TokenType::RBracket, node);
            break;
        default: fail("an expression");
        }
        return node;
    }

    ParseNode Parser::dataP() {
        ParseNode node(NonTerminal::DataP);
        if (check(TokenType::LParen)) {
            shift(node);
            node.children.push_back(args());
            expect(TokenType::RParen, node);
        }
        return node;
    }

    ParseNode Parser::args() {
        ParseNode node(NonTerminal::Args);
        if (startsExpression(peek().type)) {
            node.children.push_back(argList());
        }
        return node;
    }

    ParseNode Parser::argList() {
        ParseNode node(NonTerminal::ArgList);
        node.children.push_back(expr());
        node.children.push_back(argListP());
        return node;
    }

    ParseNode Parser::argListP() {
        ParseNode node(NonTerminal::ArgListP);
        if (check(TokenType::Comma)) {
            shift(node);
            node.children.push_back(expr());
            node.children.push_back(argListP());
        }
        return node;
    }

}  // namespace octac::parser
