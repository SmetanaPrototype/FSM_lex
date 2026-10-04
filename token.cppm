// src/token.cppm
module;

#include <string_view>
#include <cstdint>

export module fsm.token;

export namespace fsm {

enum class Tok : std::uint8_t {
    End,
};

struct Token {
    Tok              kind;
    std::string_view text;
    int              line;
    int              col;
};

} // namespace fsm