// dfa.hpp

#pragma once

enum CharClass
{
    CC_LETTER,     // a-z A-Z
    CC_EXP,        //e E
    CC_DIGIT,      // 0-9
    CC_UNDERSCORE, //_
    CC_WS,         // space, \t ,\r ,\f ,\v
    CC_NEWLINE,    //\n
    CC_QUOTE,      // "
    CC_DOT,        // .
    CC_PLUS,       // +
    CC_MINUS,      // -
    CC_STAR,       // *
    CC_SLASH,      // /
    CC_LT,         // <
    CC_GT,         // >
    CC_EQ,         // =
    CC_SIMPLE,     // { } ( ) [ ] ; , : ' % single char that cannot be extended to a longer operator
    CC_OTHER,      // any other character not in the above classes
    NUM_CLASSES    // sentinel=17, sizes the table's columns
};

enum State
{
    S_DEAD = 0,     // 0.  error state
    S_START,        // 1.  nothing consumed yet
    S_IDENT,        // 2.  [a-zA-Z][a-zA-Z0-9_]* accepting
    S_INT,          // 3.  [0-9]+ accepting
    S_INT_DOT,      // 4.  [0-9]+\. not accepting
    S_FLOAT,        // 5.  [0-9]+\.[0-9]+ accepting
    S_EXP,          // 6.  [0-9]+(\.[0-9]+)?[eE] not accepting
    S_EXP_SIGN,     // 7.  [0-9]+(\.[0-9]+)?[eE][+-] not accepting
    S_FLOAT_EXP,    // 8.  [0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+ accepting
    S_OP_DONE,      // 9.  any operator /punctuator that cannot be extended
                    //     {} () ... -> <= >= .* ./ = * /
                    //     accepting resolved by lookup operator ()
    S_MINUS,        // 10. - accepting, can extend  to -> or -=
    S_OP_EQ,        // 11. accepting  < > + * may extend to with = (<= >= += *=)
    S_SLASH,        // 12. accepting  / may extend to /= or start a comment //
    S_COMMENT,      // 13. accepting (skip) up to the newline
    S_DOT,          // 14. . not accepting, waits for * / or .
    S_DOT_DOT,      // 15. .. not accepting, waits for . to form ...
    S_STR_BODY,     // 16. " followed by anything but " or \n, not accepting,
    S_STR_END,      // 17. closing " seen, accepting"
    S_WS,           // 18. run of whitespaces  accepting but produces no token
    NUM_STATES      // 19. sentinel, sizes the table's rows
};

enum Action
{
    A_NONE,   // not an accepting state, go back to last accepting state
    A_WORD,   // lookupKeyword(lexeme), falls back to IDENTIFIER
    A_INT,    // INT
    A_FLOAT,  // FLOAT
    A_OP,     // lookupOperator(lexeme)
    A_STRING, // STRING_LIT
    A_SKIP    // whitespace: discard, scan again
};

CharClass charClassOf(unsigned char c);

extern const State TRANSITION[NUM_STATES][NUM_CLASSES];
extern const Action ACTION[NUM_STATES];
extern const char *STATE_NAMES[];  // NUM_STATES entries, in State order
extern const char *CLASS_NAMES[];  // NUM_CLASSES entries, in CharClass order
