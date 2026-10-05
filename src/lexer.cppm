module;

#include <string_view>
#include <cstddef>
#include <cctype>
#include <cstdint>
#include <optional>

export module fsm.lexer;

import fsm.token;

export namespace fsm {

class Lexer {
public:
    explicit Lexer(std::string_view src) noexcept
        : src_(src), cur_(src.data()), end_(src.data() + src.size()) {}

    Token next();

private:
    std::string_view src_;
    const char*      cur_;
    const char*      end_;
    int              line_ = 1;
    int              col_  = 1;

    bool eof() const noexcept { return cur_ == end_; }

    char peek(std::size_t off = 0) const noexcept {
        return (cur_ + off < end_) ? *(cur_ + off) : '\0';
    }

    char advance() noexcept {
        char c = *cur_++;
        if (c == '\n') { ++line_; col_ = 1; }
        else           { ++col_; }
        return c;
    }

    void        skipTrivia();
    std::size_t scanIdent()  const noexcept;
    std::size_t scanNumber() const noexcept;
    std::size_t scanPunct()  const noexcept;
    std::optional<std::size_t> scanQuoted(char quote) const noexcept;
};

} // namespace fsm

module :private;

namespace fsm {

namespace {

constexpr std::string_view kKeywords[] = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto",
    "bitand", "bitor", "bool", "break",
    "case", "catch", "char", "char8_t", "char16_t", "char32_t",
    "class", "compl", "concept", "const", "consteval", "constexpr",
    "constinit", "const_cast", "continue", "co_await", "co_return",
    "co_yield",
    "decltype", "default", "delete", "do", "double", "dynamic_cast",
    "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend",
    "goto",
    "if", "inline", "int",
    "long",
    "mutable",
    "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
    "operator", "or", "or_eq",
    "private", "protected", "public",
    "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch",
    "template", "this", "thread_local", "throw", "true", "try",
    "typedef", "typeid", "typename",
    "union", "unsigned", "using",
    "virtual", "void", "volatile",
    "wchar_t", "while",
    "xor", "xor_eq",
};

bool isKeyword(std::string_view s) noexcept {
    for (auto k : kKeywords)
        if (k == s) return true;
    return false;
}

bool isIdentStart(char c) noexcept {
    auto u = static_cast<unsigned char>(c);
    return c == '_' || std::isalpha(u);
}

bool isIdentPart(char c) noexcept {
    auto u = static_cast<unsigned char>(c);
    return c == '_' || std::isalnum(u);
}

bool isSuffixChar(char c) noexcept {
    return c == 'u' || c == 'U'
        || c == 'l' || c == 'L'
        || c == 'z' || c == 'Z';
}

bool isDigitForBase(char base, char c) noexcept {
    if (base == 'x') return std::isxdigit(static_cast<unsigned char>(c)) != 0;
    if (base == 'b') return c == '0' || c == '1';
    if (base == 'o') return c >= '0' && c <= '7';
    return c >= '0' && c <= '9';
}

} // anonymous

void Lexer::skipTrivia() {
    while (!eof()) {
        char c = peek();

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }

        if (c == '/' && peek(1) == '/') {
            advance(); advance();
            while (!eof() && peek() != '\n') advance();
            continue;
        }

        if (c == '/' && peek(1) == '*') {
            advance(); advance();
            while (!eof()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
            continue;
        }

        break;
    }
}

std::size_t Lexer::scanIdent() const noexcept {
    std::size_t n = 0;
    while (cur_ + n < end_ && isIdentPart(*(cur_ + n))) ++n;
    return n;
}

std::size_t Lexer::scanNumber() const noexcept {
    auto at = [&](std::size_t i) -> char {
        return cur_ + i < end_ ? *(cur_ + i) : '\0';
    };

    char        base;
    std::size_t n;

    if (at(0) == '0' && (at(1) == 'x' || at(1) == 'X')
                     && isDigitForBase('x', at(2))) {
        base = 'x'; n = 2;
    } else if (at(0) == '0' && (at(1) == 'b' || at(1) == 'B')
                            && isDigitForBase('b', at(2))) {
        base = 'b'; n = 2;
    } else if (at(0) == '0') {
        base = 'o'; n = 1;
    } else {
        base = 'd'; n = 0;
    }

    while (true) {
        char c = at(n);
        if (isDigitForBase(base, c)) { ++n; continue; }
        if (c == '\''
            && n > 0
            && isDigitForBase(base, at(n - 1))
            && isDigitForBase(base, at(n + 1))) {
            n += 2;
            continue;
        }
        break;
    }

    while (isSuffixChar(at(n))) ++n;

    return n;
}

std::size_t Lexer::scanPunct() const noexcept {
    auto at = [&](std::size_t i) -> char {
        return cur_ + i < end_ ? *(cur_ + i) : '\0';
    };

    static constexpr std::string_view k3[] = {
        "<<=", ">>=", "...", "<=>"
    };
    static constexpr std::string_view k2[] = {
        "==", "!=", "<=", ">=", "&&", "||", "++", "--",
        "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
        "<<", ">>", "->", "::", ".*"
    };

    for (auto op : k3)
        if (at(0) == op[0] && at(1) == op[1] && at(2) == op[2]) return 3;
    for (auto op : k2)
        if (at(0) == op[0] && at(1) == op[1]) return 2;

    switch (at(0)) {
        case '+': case '-': case '*': case '/': case '%':
        case '=': case '<': case '>': case '!': case '&':
        case '|': case '^': case '~': case '?': case ':':
        case ';': case ',': case '.': case '(': case ')':
        case '[': case ']': case '{': case '}':
            return 1;
    }
    return 0;
}

Token Lexer::next() {
    skipTrivia();

    const int sl = line_;
    const int sc = col_;

    if (eof()) return { Tok::End, {}, sl, sc };

    if (isIdentStart(peek())) {
        auto n    = scanIdent();
        auto text = std::string_view(cur_, n);
        Tok  kind = isKeyword(text) ? Tok::Keyword : Tok::Ident;

        for (std::size_t i = 0; i < n; ++i) advance();
        return { kind, text, sl, sc };
    }

    if (std::isdigit(static_cast<unsigned char>(peek()))) {
        auto n    = scanNumber();
        auto text = std::string_view(cur_, n);
        for (std::size_t i = 0; i < n; ++i) advance();
        return { Tok::IntLit, text, sl, sc };
    }

    if (auto n = scanPunct(); n > 0) {
        auto text = std::string_view(cur_, n);
        for (std::size_t i = 0; i < n; ++i) advance();
        return { Tok::Punct, text, sl, sc };
    }

    if (peek() == '"') {
        auto r = scanQuoted('"');
        if (r) {
            auto text = std::string_view(cur_, *r);
            for (std::size_t i = 0; i < *r; ++i) advance();
            return { Tok::StringLit, text, sl, sc };
        }
        auto n    = static_cast<std::size_t>(end_ - cur_);
        auto text = std::string_view(cur_, n);
        for (std::size_t i = 0; i < n; ++i) advance();
        return { Tok::Unknown, text, sl, sc };
    }

    if (peek() == '\'') {
        auto r = scanQuoted('\'');
        if (r) {
            auto text = std::string_view(cur_, *r);
            for (std::size_t i = 0; i < *r; ++i) advance();
            return { Tok::CharLit, text, sl, sc };
        }
        auto n    = static_cast<std::size_t>(end_ - cur_);
        auto text = std::string_view(cur_, n);
        for (std::size_t i = 0; i < n; ++i) advance();
        return { Tok::Unknown, text, sl, sc };
    }

    const char* start = cur_;
    advance();
    return { Tok::Unknown, std::string_view(start, 1), sl, sc };
}

std::optional<std::size_t>
Lexer::scanQuoted(char quote) const noexcept {
    auto at = [&](std::size_t i) -> char {
        return cur_ + i < end_ ? *(cur_ + i) : '\0';
    };

    std::size_t n = 1;
    while (true) {
        char c = at(n);
        if (c == '\0')  return std::nullopt;
        if (c == quote) return n + 1;
        if (c == '\\') {
            if (at(n + 1) == '\0') return std::nullopt;
            n += 2;
        } else {
            ++n;
        }
    }
}

} // namespace fsm