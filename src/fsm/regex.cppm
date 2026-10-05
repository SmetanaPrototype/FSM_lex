module;

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <utility>
#include <cstddef>

export module fsm.regex;

export namespace fsm::regex {

class Node {
public:
    virtual ~Node() = default;
    virtual std::string toString() const = 0;
};

using NodePtr = std::unique_ptr<Node>;

class Literal : public Node {
public:
    char c;
    explicit Literal(char c) noexcept : c(c) {}
    std::string toString() const override {
        return std::string(1, c);
    }
};

class Any : public Node {
public:
    std::string toString() const override { return "."; }
};

class CharClass : public Node {
public:
    using Range = std::pair<char, char>;

    bool               negated = false;
    std::vector<Range> ranges;

    CharClass() = default;
    explicit CharClass(bool neg) noexcept : negated(neg) {}

    std::string toString() const override {
        std::string s = negated ? "[^" : "[";
        for (auto [lo, hi] : ranges) {
            s.push_back(lo);
            if (hi != lo) { s.push_back('-'); s.push_back(hi); }
        }
        s.push_back(']');
        return s;
    }
};

class Concat : public Node {
public:
    NodePtr left, right;

    Concat(NodePtr l, NodePtr r)
        : left(std::move(l)), right(std::move(r)) {}

    std::string toString() const override {
        return "(cat " + left->toString() + " " + right->toString() + ")";
    }
};

class Alt : public Node {
public:
    NodePtr left, right;

    Alt(NodePtr l, NodePtr r)
        : left(std::move(l)), right(std::move(r)) {}

    std::string toString() const override {
        return "(alt " + left->toString() + " " + right->toString() + ")";
    }
};

class Star : public Node {
public:
    NodePtr child;
    explicit Star(NodePtr c) : child(std::move(c)) {}
    std::string toString() const override {
        return "(star " + child->toString() + ")";
    }
};

class Plus : public Node {
public:
    NodePtr child;
    explicit Plus(NodePtr c) : child(std::move(c)) {}
    std::string toString() const override {
        return "(plus " + child->toString() + ")";
    }
};

class Opt : public Node {
public:
    NodePtr child;
    explicit Opt(NodePtr c) : child(std::move(c)) {}
    std::string toString() const override {
        return "(opt " + child->toString() + ")";
    }
};

class Parser {
public:
    explicit Parser(std::string_view src) noexcept : src_(src) {}
    NodePtr parse();

private:
    std::string_view src_;
    std::size_t      pos_ = 0;

    bool eof() const noexcept { return pos_ >= src_.size(); }
    char peek() const noexcept { return eof() ? '\0' : src_[pos_]; }
    char get()  noexcept { return eof() ? '\0' : src_[pos_++]; }

    NodePtr parseAlternation();
    NodePtr parseConcat();
    NodePtr parseRepeat();
    NodePtr parseAtom();
    NodePtr parseCharClass();
};

inline NodePtr parse(std::string_view src) {
    return Parser(src).parse();
}

} // namespace fsm::regex

// ─────────────────────────────────────────────────────────
module :private;

namespace fsm::regex {

NodePtr Parser::parseAtom() {
    char c = peek();

    if (c == '(') {
        get();
        auto inner = parseAlternation();
        if (!inner) return nullptr;
        if (peek() != ')') return nullptr;
        get();
        return inner;
    }
    if (c == '.') { get(); return std::make_unique<Any>(); }
    if (c == '[') { return parseCharClass(); }
    if (c == '\0' || c == '|' || c == ')' || c == '*' || c == '+' || c == '?')
        return nullptr;

    get();
    return std::make_unique<Literal>(c);
}

NodePtr Parser::parseCharClass() {
    get();   // '['
    auto cls = std::make_unique<CharClass>();
    if (peek() == '^') { get(); cls->negated = true; }

    while (!eof() && peek() != ']') {
        char lo = get();
        if (peek() == '-' && pos_ + 1 < src_.size() && src_[pos_ + 1] != ']') {
            get();   // '-'
            char hi = get();
            cls->ranges.push_back({lo, hi});
        } else {
            cls->ranges.push_back({lo, lo});
        }
    }
    if (peek() != ']') return nullptr;
    get();   // ']'
    return cls;
}

NodePtr Parser::parseRepeat() {
    auto atom = parseAtom();
    if (!atom) return nullptr;

    while (true) {
        char c = peek();
        if      (c == '*') { get(); atom = std::make_unique<Star>(std::move(atom)); }
        else if (c == '+') { get(); atom = std::make_unique<Plus>(std::move(atom)); }
        else if (c == '?') { get(); atom = std::make_unique<Opt >(std::move(atom)); }
        else break;
    }
    return atom;
}

NodePtr Parser::parseConcat() {
    NodePtr left = parseRepeat();
    if (!left) return nullptr;

    while (!eof() && peek() != '|' && peek() != ')') {
        auto right = parseRepeat();
        if (!right) return nullptr;
        left = std::make_unique<Concat>(std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parseAlternation() {
    auto left = parseConcat();
    if (!left) return nullptr;

    while (peek() == '|') {
        get();
        auto right = parseConcat();
        if (!right) return nullptr;
        left = std::make_unique<Alt>(std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parse() {
    if (eof()) return nullptr;
    auto n = parseAlternation();
    if (!n) return nullptr;
    if (!eof()) return nullptr;
    return n;
}

} // namespace fsm::regex