#include <string>
#include <string_view>
#include <memory>
#include <vector>

import fsm.test;
import fsm.lexer;
import fsm.token;
import fsm.regex;
import fsm.nfa;
import fsm.dfa;
import fsm.parallel;

namespace {

bool same(const fsm::Token& t, fsm::Tok k, std::string_view text) {
    return t.kind == k && t.text == text;
}

bool lex_one(std::string_view src, fsm::Tok k, std::string_view text) {
    fsm::Lexer lex(src);
    return same(lex.next(), k, text);
}

} // anonymous

int main() {
    {
        fsm::Lexer lex("");
        fsm::test::check(lex.next().kind == fsm::Tok::End, "empty -> End");
    }

    {
        fsm::Lexer lex("x");
        fsm::test::check(same(lex.next(), fsm::Tok::Ident, "x"),
                         "single ident x");
    }

    {
        fsm::Lexer lex("foo_bar123");
        fsm::test::check(same(lex.next(), fsm::Tok::Ident, "foo_bar123"),
                         "long ident");
    }

    {
        fsm::Lexer lex("_private");
        fsm::test::check(same(lex.next(), fsm::Tok::Ident, "_private"),
                         "ident starts with _");
    }

    {
        fsm::Lexer lex("int");
        fsm::test::check(same(lex.next(), fsm::Tok::Keyword, "int"),
                         "keyword int");
    }

    {
        fsm::Lexer lex("integer");
        fsm::test::check(same(lex.next(), fsm::Tok::Ident, "integer"),
                         "integer is not keyword");
    }

    {
        fsm::Lexer lex("  int x");
        auto t = lex.next();
        fsm::test::check(same(t, fsm::Tok::Keyword, "int"),
                         "leading spaces skipped");
        fsm::test::check(t.line == 1 && t.col == 3,
                         "int at line 1, col 3");
    }

    {
        fsm::Lexer lex("42");
        fsm::test::check(same(lex.next(), fsm::Tok::IntLit, "42"),
                        "decimal 42");
    }

    {
        fsm::Lexer lex("0");
        fsm::test::check(same(lex.next(), fsm::Tok::IntLit, "0"),
                        "zero");
    }

    {
        fsm::Lexer lex("052");
        fsm::test::check(same(lex.next(), fsm::Tok::IntLit, "052"),
                        "octal 052");
    }

    {
        fsm::Lexer lex("0x2a");
        fsm::test::check(same(lex.next(), fsm::Tok::IntLit, "0x2a"),
                        "hex lower");
    }

    {
        fsm::Lexer lex("0XFF");
        fsm::test::check(same(lex.next(), fsm::Tok::IntLit, "0XFF"),
                        "hex upper");
    }

    {
        fsm::Lexer lex("0b1010");
        fsm::test::check(same(lex.next(), fsm::Tok::IntLit, "0b1010"),
                        "binary");
    }

    {
        fsm::Lexer lex("42abc");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::IntLit, "42"), "42 before ident");
        fsm::test::check(same(b, fsm::Tok::Ident,  "abc"), "ident after 42");
    }

    {
        fsm::Lexer lex("0x");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::IntLit, "0"), "0x -> just 0");
        fsm::test::check(same(b, fsm::Tok::Ident,  "x"), "x after 0");
    }

    {
        fsm::Lexer lex("09");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::IntLit, "0"), "09 -> 0");
        fsm::test::check(same(b, fsm::Tok::IntLit, "9"), "09 -> 9");
    }

    fsm::test::check(lex_one("+",   fsm::Tok::Punct, "+"),   "plus");
    fsm::test::check(lex_one("=",   fsm::Tok::Punct, "="),   "eq");
    fsm::test::check(lex_one("==",  fsm::Tok::Punct, "=="),  "eqeq not two eq");
    fsm::test::check(lex_one("<",   fsm::Tok::Punct, "<"),   "lt");
    fsm::test::check(lex_one("<=",  fsm::Tok::Punct, "<="),  "le not lt+eq");
    fsm::test::check(lex_one("<<",  fsm::Tok::Punct, "<<"),  "shl");
    fsm::test::check(lex_one("<<=", fsm::Tok::Punct, "<<="), "shl-eq wins over <<");
    fsm::test::check(lex_one("->",  fsm::Tok::Punct, "->"),  "arrow");
    fsm::test::check(lex_one("::",  fsm::Tok::Punct, "::"),  "scope");
    fsm::test::check(lex_one("...", fsm::Tok::Punct, "..."), "ellipsis");
    fsm::test::check(lex_one("++",  fsm::Tok::Punct, "++"),  "plusplus");

    {
        fsm::Lexer lex("+ =");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::Punct, "+"), "plus alone");
        fsm::test::check(same(b, fsm::Tok::Punct, "="), "eq alone");
    }

    {
        fsm::Lexer lex("a+b");
        auto a  = lex.next();
        auto op = lex.next();
        auto b  = lex.next();
        fsm::test::check(same(a,  fsm::Tok::Ident, "a"), "ident a");
        fsm::test::check(same(op, fsm::Tok::Punct, "+"), "op plus");
        fsm::test::check(same(b,  fsm::Tok::Ident, "b"), "ident b");
    }

    {
        fsm::Lexer lex("..");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::Punct, "."), "first dot");
        fsm::test::check(same(b, fsm::Tok::Punct, "."), "second dot");
    }

    fsm::test::check(lex_one("42u",    fsm::Tok::IntLit, "42u"),    "u suffix");
    fsm::test::check(lex_one("42ull",  fsm::Tok::IntLit, "42ull"),  "ull suffix");
    fsm::test::check(lex_one("42ULL",  fsm::Tok::IntLit, "42ULL"),  "ULL suffix");
    fsm::test::check(lex_one("123z",   fsm::Tok::IntLit, "123z"),   "z suffix");
    fsm::test::check(lex_one("0xFFu",  fsm::Tok::IntLit, "0xFFu"),  "hex + suffix");

    fsm::test::check(lex_one("1'000'000", fsm::Tok::IntLit, "1'000'000"),
                    "decimal separators");
    fsm::test::check(lex_one("0xFF'FF",   fsm::Tok::IntLit, "0xFF'FF"),
                    "hex separators");
    fsm::test::check(lex_one("0b1010'1010", fsm::Tok::IntLit, "0b1010'1010"),
                    "binary separators");

    {
        fsm::Lexer lex("1'");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::IntLit, "1"),   "1 before stray apostrophe");
        fsm::test::check(b.kind == fsm::Tok::Unknown,      "stray apostrophe -> Unknown");
    }

    {
        fsm::Lexer lex("42abc");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::IntLit, "42"), "42 unaffected by suffix rule");
        fsm::test::check(same(b, fsm::Tok::Ident,  "abc"), "abc after 42");
    }

    fsm::test::check(lex_one("\"hello\"", fsm::Tok::StringLit, "\"hello\""),
                    "simple string");
    fsm::test::check(lex_one("\"\"",      fsm::Tok::StringLit, "\"\""),
                    "empty string");
    fsm::test::check(lex_one("\"a\\\"b\"", fsm::Tok::StringLit, "\"a\\\"b\""),
                    "escaped quote inside");

    fsm::test::check(lex_one("'a'",  fsm::Tok::CharLit, "'a'"),  "char a");
    fsm::test::check(lex_one("'\\n'", fsm::Tok::CharLit, "'\\n'"), "escaped newline char");
    fsm::test::check(lex_one("'\\''", fsm::Tok::CharLit, "'\\''"), "escaped quote char");

    {
        fsm::Lexer lex("\"abc");
        auto t = lex.next();
        fsm::test::check(t.kind == fsm::Tok::Unknown, "unterminated string -> Unknown");
        fsm::test::check(t.text == "\"abc", "consumed whole rest");
    }

    {
        fsm::Lexer lex("std::print(\"hi\")");
        auto a = lex.next();
        auto b = lex.next();
        auto c = lex.next();
        auto d = lex.next();
        auto e = lex.next();
        fsm::test::check(same(a, fsm::Tok::Ident,     "std"),     "std");
        fsm::test::check(same(b, fsm::Tok::Punct,     "::"),      "::");
        fsm::test::check(same(c, fsm::Tok::Ident,     "print"),   "print");
        fsm::test::check(same(d, fsm::Tok::Punct,     "("),       "(");
        fsm::test::check(same(e, fsm::Tok::StringLit, "\"hi\""),  "string arg");
    }

    {
        fsm::Lexer lex("// comment\n42");
        auto t = lex.next();
        fsm::test::check(same(t, fsm::Tok::IntLit, "42"), "// skipped, 42 read");
        fsm::test::check(t.line == 2, "42 on line 2");
    }

    {
        fsm::Lexer lex("// no newline at end");
        fsm::test::check(lex.next().kind == fsm::Tok::End, "// to EOF");
    }

    {
        fsm::Lexer lex("/* multi\nline */ x");
        auto t = lex.next();
        fsm::test::check(same(t, fsm::Tok::Ident, "x"), "/* */ skipped");
        fsm::test::check(t.line == 2, "x on line 2");
    }

    {
        fsm::Lexer lex("1 /* c */ 2");
        auto a = lex.next();
        auto b = lex.next();
        fsm::test::check(same(a, fsm::Tok::IntLit, "1"), "1 before comment");
        fsm::test::check(same(b, fsm::Tok::IntLit, "2"), "2 after comment");
    }

    {
        fsm::Lexer lex("/* unterminated");
        fsm::test::check(lex.next().kind == fsm::Tok::End, "unterminated /* -> End");
    }

    {
        fsm::Lexer lex("a / b * c");
        auto a  = lex.next();
        auto s1 = lex.next();
        auto b  = lex.next();
        auto s2 = lex.next();
        auto c  = lex.next();
        fsm::test::check(same(a,  fsm::Tok::Ident, "a"), "regression: a");
        fsm::test::check(same(s1, fsm::Tok::Punct, "/"), "regression: /");
        fsm::test::check(same(b,  fsm::Tok::Ident, "b"), "regression: b");
        fsm::test::check(same(s2, fsm::Tok::Punct, "*"), "regression: *");
        fsm::test::check(same(c,  fsm::Tok::Ident, "c"), "regression: c");
    }

    fsm::test::check(lex_one("constexpr",  fsm::Tok::Keyword, "constexpr"),
                    "constexpr keyword");
    fsm::test::check(lex_one("co_await",   fsm::Tok::Keyword, "co_await"),
                    "co_await keyword");
    fsm::test::check(lex_one("nullptr",    fsm::Tok::Keyword, "nullptr"),
                    "nullptr keyword");
    fsm::test::check(lex_one("and",        fsm::Tok::Keyword, "and"),
                    "alternative operator and");
    fsm::test::check(lex_one("xor_eq",     fsm::Tok::Keyword, "xor_eq"),
                    "alternative operator xor_eq");
    fsm::test::check(lex_one("char8_t",    fsm::Tok::Keyword, "char8_t"),
                    "char8_t keyword");

    fsm::test::check(lex_one("null",    fsm::Tok::Ident, "null"),
                    "null is not a keyword");
    fsm::test::check(lex_one("module",  fsm::Tok::Ident, "module"),
                    "module is contextual, not keyword");
    fsm::test::check(lex_one("override", fsm::Tok::Ident, "override"),
                    "override is contextual, not keyword");
    fsm::test::check(lex_one("final",   fsm::Tok::Ident, "final"),
                    "final is contextual, not keyword");

    using fsm::regex::NodePtr;

    auto lit = [](char c) -> NodePtr {
        return std::make_unique<fsm::regex::Literal>(c);
    };

    {
        auto n = lit('a');
        fsm::test::check(n->toString() == "a", "literal a");
    }

    {
        auto n = std::make_unique<fsm::regex::Any>();
        fsm::test::check(n->toString() == ".", "any");
    }

    {
        auto n = std::make_unique<fsm::regex::Star>(lit('a'));
        fsm::test::check(n->toString() == "(star a)", "star a");
    }

    {
        auto n = std::make_unique<fsm::regex::Plus>(lit('a'));
        fsm::test::check(n->toString() == "(plus a)", "plus a");
    }

    {
        auto n = std::make_unique<fsm::regex::Opt>(lit('a'));
        fsm::test::check(n->toString() == "(opt a)", "opt a");
    }

    {
        auto n = std::make_unique<fsm::regex::Concat>(lit('a'), lit('b'));
        fsm::test::check(n->toString() == "(cat a b)", "concat ab");
    }

    {
        auto n = std::make_unique<fsm::regex::Alt>(lit('a'), lit('b'));
        fsm::test::check(n->toString() == "(alt a b)", "alt a|b");
    }

    {
        auto n = std::make_unique<fsm::regex::Concat>(
            std::make_unique<fsm::regex::Star>(lit('a')),
            lit('b'));
        fsm::test::check(n->toString() == "(cat (star a) b)", "a*b");
    }

    {
        auto n = std::make_unique<fsm::regex::Alt>(
            lit('a'),
            std::make_unique<fsm::regex::Star>(lit('b')));
        fsm::test::check(n->toString() == "(alt a (star b))", "a|b*");
    }

    {
        auto n = std::make_unique<fsm::regex::CharClass>();
        n->ranges.push_back({'a', 'z'});
        fsm::test::check(n->toString() == "[a-z]", "class a-z");
    }

    {
        auto n = std::make_unique<fsm::regex::CharClass>(true);
        n->ranges.push_back({'0', '9'});
        fsm::test::check(n->toString() == "[^0-9]", "negated class");
    }

    {
        auto n = std::make_unique<fsm::regex::CharClass>();
        n->ranges.push_back({'a', 'a'});
        n->ranges.push_back({'b', 'b'});
        n->ranges.push_back({'c', 'c'});
        fsm::test::check(n->toString() == "[abc]", "class abc");
    }

    auto R = [](std::string_view s) {
        auto n = fsm::regex::parse(s);
        return n ? n->toString() : std::string("<null>");
    };

    fsm::test::check(R("a")     == "a",          "parse literal");
    fsm::test::check(R(".")     == ".",          "parse any");
    fsm::test::check(R("ab")    == "(cat a b)",  "parse concat");
    fsm::test::check(R("a|b")   == "(alt a b)",  "parse alt");
    fsm::test::check(R("a*")    == "(star a)",   "parse star");
    fsm::test::check(R("a+")    == "(plus a)",   "parse plus");
    fsm::test::check(R("a?")    == "(opt a)",    "parse opt");
    fsm::test::check(R("(a)")   == "a",          "parse group");

    fsm::test::check(R("ab|c")  == "(alt (cat a b) c)", "concat binds tighter than alt");
    fsm::test::check(R("a|bc")  == "(alt a (cat b c))", "alt then concat");
    fsm::test::check(R("a|b*")  == "(alt a (star b))",  "star binds to b, not to alt");
    fsm::test::check(R("ab*")   == "(cat a (star b))",  "star binds to b, not concat");
    fsm::test::check(R("a**")   == "(star (star a))",   "double star ok (star of star)");

    fsm::test::check(R("(a|b)*") == "(star (alt a b))", "group with alt, then star");
    fsm::test::check(R("(ab)+")  == "(plus (cat a b))", "group then plus");

    fsm::test::check(R("[a-z]")  == "[a-z]", "char class range");
    fsm::test::check(R("[abc]")  == "[abc]", "char class list");
    fsm::test::check(R("[^0-9]") == "[^0-9]", "negated class");

    fsm::test::check(R("")     == "<null>", "empty is error");
    fsm::test::check(R("*a")   == "<null>", "leading star is error");
    fsm::test::check(R("a|")   == "<null>", "trailing alt is error");
    fsm::test::check(R("(a")   == "<null>", "unclosed paren is error");
    fsm::test::check(R("a)")   == "<null>", "stray close paren is error");

    auto accepts = [](std::string_view re, std::string_view input) {
        auto ast = fsm::regex::parse(re);
        if (!ast) return false;
        auto nfa = fsm::nfa::build(*ast);
        return nfa.accepts(input);
    };

    fsm::test::check( accepts("a", "a"),    "a accepts a");
    fsm::test::check(!accepts("a", "b"),    "a rejects b");
    fsm::test::check(!accepts("a", ""),     "a rejects empty");

    fsm::test::check( accepts("ab", "ab"),  "ab accepts ab");
    fsm::test::check(!accepts("ab", "a"),   "ab rejects a");
    fsm::test::check(!accepts("ab", "abc"), "ab rejects abc");

    fsm::test::check( accepts("a|b", "a"),  "a|b accepts a");
    fsm::test::check( accepts("a|b", "b"),  "a|b accepts b");
    fsm::test::check(!accepts("a|b", "c"),  "a|b rejects c");
    fsm::test::check(!accepts("a|b", "ab"), "a|b rejects ab");

    fsm::test::check( accepts("a*", ""),     "a* accepts empty");
    fsm::test::check( accepts("a*", "a"),    "a* accepts a");
    fsm::test::check( accepts("a*", "aaa"),  "a* accepts aaa");
    fsm::test::check(!accepts("a*", "b"),    "a* rejects b");

    fsm::test::check(!accepts("a+", ""),    "a+ rejects empty");
    fsm::test::check( accepts("a+", "a"),   "a+ accepts a");
    fsm::test::check( accepts("a+", "aaa"), "a+ accepts aaa");

    fsm::test::check( accepts("a?", ""),   "a? accepts empty");
    fsm::test::check( accepts("a?", "a"),  "a? accepts a");
    fsm::test::check(!accepts("a?", "aa"), "a? rejects aa");

    fsm::test::check( accepts("(a|b)*", "abab"),  "(a|b)* accepts abab");
    fsm::test::check( accepts("(a|b)*", ""),      "(a|b)* accepts empty");
    fsm::test::check(!accepts("(a|b)*", "abc"),   "(a|b)* rejects abc");

    fsm::test::check( accepts(".", "x"),   ". accepts x");
    fsm::test::check(!accepts(".", ""),    ". rejects empty");
    fsm::test::check(!accepts(".", "xy"),  ". rejects xy");

    fsm::test::check( accepts("[a-z]", "m"),  "[a-z] accepts m");
    fsm::test::check(!accepts("[a-z]", "M"),  "[a-z] rejects M");
    fsm::test::check(!accepts("[a-z]", "5"),  "[a-z] rejects 5");
    fsm::test::check( accepts("[^0-9]", "a"), "[^0-9] accepts a");
    fsm::test::check(!accepts("[^0-9]", "5"), "[^0-9] rejects 5");

    fsm::test::check( accepts("[0-9]+", "42"), "[0-9]+ accepts 42");
    fsm::test::check(!accepts("[0-9]+", "4a"), "[0-9]+ rejects 4a");
    fsm::test::check( accepts("[0-9]+[.][0-9]+", "3.14"), "float-like pattern");

    auto accepts_both = [](std::string_view re, std::string_view input) {
        auto ast = fsm::regex::parse(re);
        if (!ast) return std::pair{false, false};
        auto nfa = fsm::nfa::build(*ast);
        auto dfa = fsm::dfa::build(nfa);
        return std::pair{ nfa.accepts(input), dfa.accepts(input) };
    };

    auto same_result = [&](std::string_view re, std::string_view input) {
        auto [n, d] = accepts_both(re, input);
        return n == d;
    };

    fsm::test::check(same_result("a", "a"),    "DFA=NFA for a/a");
    fsm::test::check(same_result("a", "b"),    "DFA=NFA for a/b");
    fsm::test::check(same_result("a", ""),     "DFA=NFA for a/empty");

    fsm::test::check(same_result("ab", "ab"),  "DFA=NFA for ab/ab");
    fsm::test::check(same_result("ab", "a"),   "DFA=NFA for ab/a");
    fsm::test::check(same_result("abc", "abd"), "DFA=NFA for abc/abd");

    fsm::test::check(same_result("a|b", "a"),  "DFA=NFA for a|b/a");
    fsm::test::check(same_result("a|b", "c"),  "DFA=NFA for a|b/c");

    fsm::test::check(same_result("a*", ""),    "DFA=NFA for a*/empty");
    fsm::test::check(same_result("a*", "aaa"), "DFA=NFA for a*/aaa");
    fsm::test::check(same_result("a*", "b"),   "DFA=NFA for a*/b");

    fsm::test::check(same_result("(a|b)*", "abab"),  "DFA=NFA for (a|b)*/abab");
    fsm::test::check(same_result("(a|b)*", "abc"),   "DFA=NFA for (a|b)*/abc");
    fsm::test::check(same_result("[0-9]+", "42"),    "DFA=NFA for [0-9]+/42");
    fsm::test::check(same_result("[0-9]+", "4a"),    "DFA=NFA for [0-9]+/4a");

    {
        auto ast = fsm::regex::parse("(a|b)*abb");
        auto nfa = fsm::nfa::build(*ast);
        auto dfa = fsm::dfa::build(nfa);

        fsm::test::check( dfa.accepts("abb"),     "DFA accepts abb");
        fsm::test::check( dfa.accepts("aabb"),    "DFA accepts aabb");
        fsm::test::check( dfa.accepts("ababb"),   "DFA accepts ababb");
        fsm::test::check(!dfa.accepts("ab"),      "DFA rejects ab");
        fsm::test::check(!dfa.accepts("abba"),    "DFA rejects abba");
        fsm::test::check(!dfa.accepts(""),        "DFA rejects empty");
    }

    {
        auto ast = fsm::regex::parse("(a|b)*abb");
        auto nfa = fsm::nfa::build(*ast);
        auto dfa = fsm::dfa::build(nfa);

        std::vector<std::string> inputs;
        for (int i = 0; i < 1000; ++i) {
            std::string s;
            unsigned x = i * 2654435761u;
            for (int k = 0; k < 20; ++k) {
                s.push_back('a' + (x % 2));
                x = x * 1103515245u + 12345u;
            }
            inputs.push_back(std::move(s));
        }
        inputs.push_back("abb");
        inputs.push_back("ab");
        inputs.push_back("aabb");

        auto seq = fsm::parallel::accepts_many(dfa, inputs, 1);
        auto par = fsm::parallel::accepts_many(dfa, inputs, 4);

        fsm::test::check(seq.size() == inputs.size(), "seq size matches");
        fsm::test::check(par.size() == inputs.size(), "par size matches");
        fsm::test::check(seq == par,                  "seq and par agree");

        fsm::test::check(par[1000] == 1, "par: abb accepted");
        fsm::test::check(par[1001] == 0, "par: ab rejected");
        fsm::test::check(par[1002] == 1, "par: aabb accepted");
    }

    return fsm::test::summary();
}