#pragma once

#include <cstddef>

namespace octac::lexer {

    /**
     * @brief The class an input byte belongs to. These are the columns of the transition table.
     */
    enum class CharClass {
        Letter,      ///< a-z A-Z, except e and E
        Exp,         ///< e E, kept apart from Letter so exponents can be recognized
        Digit,       ///< 0-9
        Underscore,  ///< _
        Whitespace,  ///< space, \t, \r, \f, \v
        Newline,     ///< \n
        Quote,       ///< "
        Dot,         ///< .
        Plus,        ///< +
        Minus,       ///< -
        Star,        ///< *
        Slash,       ///< /
        Less,        ///< <
        Greater,     ///< >
        Equal,       ///< =
        Simple,  ///< { } ( ) [ ] ; , : ' % single characters that never extend to a longer operator
        Other    ///< any byte not in the classes above.
    };

    /**
     * @brief The number of character classes, the column count of the transition table.
     */
    inline constexpr std::size_t kNumCharClasses = static_cast<std::size_t>(CharClass::Other) + 1;

    /**
     * @brief A state of the lexer DFA. These are the rows of the transition table.
     */
    enum class State {
        Dead,      ///< no transition exists, the error state
        Start,     ///< nothing consumed yet
        Ident,     ///< [a-zA-Z][a-zA-Z0-9_]*, accepting
        Int,       ///< [0-9]+, accepting
        IntDot,    ///< [0-9]+\., not accepting
        Float,     ///< [0-9]+\.[0-9]+, accepting
        Exp,       ///< a number followed by e or E, not accepting
        ExpSign,   ///< a number, e or E, then + or -, not accepting
        FloatExp,  ///< a number with a complete exponent, accepting
        OpDone,    ///< an operator or punctuator that cannot be extended, accepting
        Minus,     ///< -, accepting, may extend to -> or -=
        OpEq,      ///< < > + *, accepting, may extend with = (<= >= += *=)
        Slash,     ///< /, accepting, may extend to /= or start a // comment
        Comment,   ///< a // comment up to the newline, accepting but produces no token
        Dot,       ///< ., not accepting, waits for * / or .
        DotDot,    ///< .., not accepting, waits for . to form ...
        StrBody,   ///< an opening " followed by anything but " or a newline, not accepting
        StrEnd,    ///< a closing " was seen, accepting
        Ws         ///< a run of whitespace, accepting but produces no token.
    };

    /**
     * @brief The number of states, the row count of the transition table.
     */
    inline constexpr std::size_t kNumStates = static_cast<std::size_t>(State::Ws) + 1;

    /**
     * @brief What the lexer does when the DFA stops in a given state.
     */
    enum class Action {
        None,    ///< not an accepting state, fall back to the last accepting state
        Word,    ///< keyword lookup, falling back to an identifier
        Int,     ///< an integer literal
        Float,   ///< a float literal
        Op,      ///< operator lookup
        String,  ///< a string literal
        Skip     ///< discard the lexeme and scan again
    };

    /**
     * @brief Maps an input byte to its character class.
     *
     * @param c The byte to classify.
     * @return The class of @p c.
     */
    CharClass charClassOf(unsigned char c);

    /**
     * @brief Looks up a transition of the DFA.
     *
     * @param from The current state.
     * @param on The class of the next input byte.
     * @return The next state, or State::Dead when the DFA cannot continue.
     */
    State nextState(State from, CharClass on);

    /**
     * @brief Returns what the lexer does when it stops in a state.
     *
     * @param state The state the DFA stopped in.
     * @return Action::None for states that do not accept, otherwise the action for that state.
     */
    Action actionOf(State state);

}  // namespace octac::lexer
