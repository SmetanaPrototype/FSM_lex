module;

#include <string_view>
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <algorithm>

export module fsm.nfa;

import fsm.regex;

export namespace fsm::nfa {

struct CharSet {
    enum Kind : std::uint8_t { Char, Any, Class };

    Kind kind = Char;
    char c    = 0;
    std::vector<std::pair<char,char>> ranges;
    bool negated = false;

    bool match(char x) const noexcept {
        if (kind == Any)  return true;
        if (kind == Char) return x == c;
        bool in = false;
        for (auto [lo, hi] : ranges)
            if (x >= lo && x <= hi) { in = true; break; }
        return negated ? !in : in;
    }
};

struct State {
    std::vector<int>                     eps;
    std::vector<std::pair<CharSet, int>> trans;
};

struct NFA {
    std::vector<State> states;
    int start  = -1;
    int accept = -1;

    bool accepts(std::string_view input) const;
};

NFA build(const regex::Node& root);

} // namespace fsm::nfa

module :private;

namespace fsm::nfa {

namespace {

class Builder {
public:
    NFA nfa;

    int newState() {
        nfa.states.push_back({});
        return static_cast<int>(nfa.states.size()) - 1;
    }

    struct Frag { int start; int accept; };

    Frag build(const regex::Node& n) {
        using namespace regex;

        if (auto* l = dynamic_cast<const Literal*>(&n)) {
            int s = newState(), e = newState();
            CharSet cs; cs.kind = CharSet::Char; cs.c = l->c;
            nfa.states[s].trans.push_back({cs, e});
            return {s, e};
        }
        if (dynamic_cast<const Any*>(&n)) {
            int s = newState(), e = newState();
            CharSet cs; cs.kind = CharSet::Any;
            nfa.states[s].trans.push_back({cs, e});
            return {s, e};
        }
        if (auto* c = dynamic_cast<const CharClass*>(&n)) {
            int s = newState(), e = newState();
            CharSet cs; cs.kind = CharSet::Class;
            cs.ranges = c->ranges; cs.negated = c->negated;
            nfa.states[s].trans.push_back({cs, e});
            return {s, e};
        }
        if (auto* cat = dynamic_cast<const Concat*>(&n)) {
            auto l = build(*cat->left);
            auto r = build(*cat->right);
            nfa.states[l.accept].eps.push_back(r.start);
            return {l.start, r.accept};
        }
        if (auto* alt = dynamic_cast<const Alt*>(&n)) {
            auto l = build(*alt->left);
            auto r = build(*alt->right);
            int s = newState(), e = newState();
            nfa.states[s].eps.push_back(l.start);
            nfa.states[s].eps.push_back(r.start);
            nfa.states[l.accept].eps.push_back(e);
            nfa.states[r.accept].eps.push_back(e);
            return {s, e};
        }
        if (auto* st = dynamic_cast<const Star*>(&n)) {
            auto inner = build(*st->child);
            int s = newState(), e = newState();
            nfa.states[s].eps.push_back(inner.start);
            nfa.states[s].eps.push_back(e);
            nfa.states[inner.accept].eps.push_back(inner.start);
            nfa.states[inner.accept].eps.push_back(e);
            return {s, e};
        }
        if (auto* pl = dynamic_cast<const Plus*>(&n)) {
            auto inner = build(*pl->child);
            int s = newState(), e = newState();
            nfa.states[s].eps.push_back(inner.start);
            nfa.states[inner.accept].eps.push_back(inner.start);
            nfa.states[inner.accept].eps.push_back(e);
            return {s, e};
        }
        if (auto* op = dynamic_cast<const Opt*>(&n)) {
            auto inner = build(*op->child);
            int s = newState(), e = newState();
            nfa.states[s].eps.push_back(inner.start);
            nfa.states[s].eps.push_back(e);
            nfa.states[inner.accept].eps.push_back(e);
            return {s, e};
        }
        return {-1, -1};
    }
};

} // anonymous

NFA build(const regex::Node& root) {
    Builder b;
    auto f = b.build(root);
    b.nfa.start  = f.start;
    b.nfa.accept = f.accept;
    return std::move(b.nfa);
}

bool NFA::accepts(std::string_view input) const {
    if (start < 0) return false;

    std::vector<int>  cur;
    std::vector<char> seen(states.size(), 0);

    auto closure = [&](std::vector<int>& set) {
        std::fill(seen.begin(), seen.end(), 0);
        for (int s : set) seen[s] = 1;
        std::size_t i = 0;
        while (i < set.size()) {
            int s = set[i++];
            for (int t : states[s].eps) {
                if (!seen[t]) { seen[t] = 1; set.push_back(t); }
            }
        }
    };

    cur.push_back(start);
    closure(cur);

    for (char c : input) {
        std::vector<int> next;
        for (int s : cur) {
            for (auto& [cs, t] : states[s].trans) {
                if (cs.match(c)) next.push_back(t);
            }
        }
        if (next.empty()) return false;
        closure(next);
        cur = std::move(next);
    }

    for (int s : cur) if (s == accept) return true;
    return false;
}

} // namespace fsm::nfa