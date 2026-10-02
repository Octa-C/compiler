#include "dfa.hpp"

#include <array>
#include <initializer_list>

namespace octac::lexer {

    namespace {

        using ClassTable = std::array<CharClass, 256>;
        using TransitionTable = std::array<std::array<State, kNumCharClasses>, kNumStates>;

        constexpr std::size_t index(CharClass c) {
            return static_cast<std::size_t>(c);
        }

        constexpr std::size_t index(State s) {
            return static_cast<std::size_t>(s);
        }

        constexpr ClassTable buildClassTable() {
            ClassTable table{};
            for (CharClass &entry : table) {
                entry = CharClass::Other;
            }
            for (char c = 'a'; c <= 'z'; ++c) {
                table[static_cast<unsigned char>(c)] = CharClass::Letter;
            }
            for (char c = 'A'; c <= 'Z'; ++c) {
                table[static_cast<unsigned char>(c)] = CharClass::Letter;
            }
            for (char c = '0'; c <= '9'; ++c) {
                table[static_cast<unsigned char>(c)] = CharClass::Digit;
            }
            table['e'] = CharClass::Exp;
            table['E'] = CharClass::Exp;
            table['_'] = CharClass::Underscore;

            for (char c : {' ', '\t', '\r', '\f', '\v'}) {
                table[static_cast<unsigned char>(c)] = CharClass::Whitespace;
            }
            table['\n'] = CharClass::Newline;

            table['"'] = CharClass::Quote;
            table['.'] = CharClass::Dot;
            table['+'] = CharClass::Plus;
            table['-'] = CharClass::Minus;
            table['*'] = CharClass::Star;
            table['/'] = CharClass::Slash;
            table['<'] = CharClass::Less;
            table['>'] = CharClass::Greater;
            table['='] = CharClass::Equal;

            for (char c : {'{', '}', '(', ')', '[', ']', ';', ',', ':', '%', '\''}) {
                table[static_cast<unsigned char>(c)] = CharClass::Simple;
            }
            return table;
        }

        constexpr TransitionTable buildTransitionTable() {
            TransitionTable table{};  // every entry starts as State::Dead
            for (auto &row : table) {
                for (State &entry : row) {
                    entry = State::Dead;
                }
            }

            auto on = [&table](State from, std::initializer_list<CharClass> classes, State to) {
                for (CharClass c : classes) {
                    table[index(from)][index(c)] = to;
                }
            };

            using C = CharClass;
            using S = State;

            on(S::Start, {C::Letter, C::Exp}, S::Ident);
            on(S::Start, {C::Digit}, S::Int);
            on(S::Start, {C::Whitespace, C::Newline}, S::Ws);
            on(S::Start, {C::Quote}, S::StrBody);
            on(S::Start, {C::Dot}, S::Dot);
            on(S::Start, {C::Plus, C::Star, C::Less, C::Greater}, S::OpEq);
            on(S::Start, {C::Minus}, S::Minus);
            on(S::Start, {C::Slash}, S::Slash);
            on(S::Start, {C::Equal, C::Simple}, S::OpDone);

            on(S::Ident, {C::Letter, C::Exp, C::Digit, C::Underscore}, S::Ident);

            on(S::Int, {C::Digit}, S::Int);
            on(S::Int, {C::Exp}, S::Exp);
            on(S::Int, {C::Dot}, S::IntDot);
            on(S::IntDot, {C::Digit}, S::Float);
            on(S::Float, {C::Digit}, S::Float);
            on(S::Float, {C::Exp}, S::Exp);
            on(S::Exp, {C::Plus, C::Minus}, S::ExpSign);
            on(S::Exp, {C::Digit}, S::FloatExp);
            on(S::ExpSign, {C::Digit}, S::FloatExp);
            on(S::FloatExp, {C::Digit}, S::FloatExp);

            on(S::Minus, {C::Greater, C::Equal}, S::OpDone);
            on(S::OpEq, {C::Equal}, S::OpDone);
            on(S::Slash, {C::Equal}, S::OpDone);
            on(S::Slash, {C::Slash}, S::Comment);

            for (std::size_t c = 0; c < kNumCharClasses; ++c) {
                table[index(S::Comment)][c] = S::Comment;
                table[index(S::StrBody)][c] = S::StrBody;
            }
            table[index(S::Comment)][index(C::Newline)] = S::Dead;
            table[index(S::StrBody)][index(C::Newline)] = S::Dead;
            on(S::StrBody, {C::Quote}, S::StrEnd);

            on(S::Dot, {C::Dot}, S::DotDot);
            on(S::Dot, {C::Star, C::Slash}, S::OpDone);
            on(S::DotDot, {C::Dot}, S::OpDone);

            on(S::Ws, {C::Whitespace, C::Newline}, S::Ws);

            return table;
        }

        constexpr ClassTable kClassTable = buildClassTable();
        constexpr TransitionTable kTransitions = buildTransitionTable();

    }  // namespace

    CharClass charClassOf(unsigned char c) {
        return kClassTable[c];
    }

    State nextState(State from, CharClass on) {
        return kTransitions[index(from)][index(on)];
    }

    Action actionOf(State state) {
        switch (state) {
        case State::Ident:    return Action::Word;
        case State::Int:      return Action::Int;
        case State::Float:
        case State::FloatExp: return Action::Float;
        case State::OpDone:
        case State::Minus:
        case State::OpEq:
        case State::Slash:    return Action::Op;
        case State::StrEnd:   return Action::String;
        case State::Comment:
        case State::Ws:       return Action::Skip;
        case State::Dead:
        case State::Start:
        case State::IntDot:
        case State::Exp:
        case State::ExpSign:
        case State::Dot:
        case State::DotDot:
        case State::StrBody:  return Action::None;
        }
        return Action::None;
    }

}  // namespace octac::lexer
