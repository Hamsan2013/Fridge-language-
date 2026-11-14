#pragma once
#include <string>

namespace fridge {

enum class TokenType {
    // Symbols & Brackets (v0.5.0)
    LBRACE,      // {
    RBRACE,      // }
    LPAREN,      // (
    RPAREN,      // )
    COMMA,       // ,
    PLUS,        // +
    MINUS,       // -
    STAR,        // *
    SLASH,       // /

    // Literals & Identifiers
    IDENTIFIER,
    STRING,
    NUMBER,
    BOOLEAN,
    NOTHING,

    // Core Keywords & Shorthands
    REMEMBER,    // remember, rem
    AS,          // as
    SET,         // set
    TO,          // to
    SHOW,        // show
    ASK,         // ask
    JOINED,      // joined
    WITH,        // with

    // Control Flow & Functions
    IF,          // if
    IS,          // is
    OTHERWISE,   // otherwise
    REPEAT,      // repeat
    TIMES,       // times
    WHILE,       // while
    FUNCTION,    // fnc, function
    CALLED,      // called
    RETURNING,   // returning
    END,         // end (legacy compatibility)

    // Error Handling & Critical Keywords (Red)
    ATTEMPT,     // attempt
    ERROR,       // error
    STOP,        // stop
    EXIT,        // exit
    DELETE_KW,   // delete, del

    // File System & 2D/3D Engine
    CREATE,      // create
    READ,        // read
    ADD,         // add
    FILE_KW,     // file
    INTO,        // into
    FROM,        // from
    LOAD,        // load
    MODEL,       // model
    OBJ,         // OBJ
    SPRITE,      // SPRITE
    DRAW,        // draw
    RECTANGLE,   // rectangle
    CIRCLE,      // circle
    AT,          // at
    WIDTH,       // width
    HEIGHT,      // height
    RADIUS,      // radius
    COLOR,       // color

    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line = 1;
    int column = 1;
};

} // namespace fridge
