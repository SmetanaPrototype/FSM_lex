module;

#include <string_view>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <queue>
#include <cstddef>
#include <cstdint>

export module fsm.dfa;

import fsm.nfa;

export namespace fsm::dfa {

struct DFA {
    std::vector<char>                 accepting;
    std::vector<std::array<int, 256>> trans;
    int start = 0;

    bool accepts(std::string_view input) const noexcept {
        int s = start;
        for (char c : input) {
            int t = trans[s][static_cast<unsigned char>(c)];
            if (t < 0) return false;
            s = t;
        }
        return accepting[s] != 0;
    }
};

DFA build(const nfa::NFA& n);

} // namespace fsm::dfa

module :private;

namespace fsm::dfa {

namespace {

using StateSet = std::set<int>;

StateSet closure(const nfa::NFA& nfa, const StateSet& s) {
    StateSet result = s;
    std::vector<int> stack(s.begin(), s.end());
    while (!stack.empty()) {
        int st = stack.back(); stack.pop_back();
        for (int t : nfa.states[st].eps) {
            if (result.insert(t).second) stack.push_back(t);
        }
    }
    return result;
}

StateSet move(const nfa::NFA& nfa, const StateSet& s, char c) {
    StateSet result;
    for (int st : s) {
        for (auto& [cs, t] : nfa.states[st].trans) {
            if (cs.match(c)) result.insert(t);
        }
    }
    return result;
}

} // anonymous

DFA build(const nfa::NFA& nfa) {
    DFA d;
    if (nfa.start < 0) return d;

    std::map<StateSet, int>  seen;
    std::queue<StateSet>     work;

    auto start_set = closure(nfa, {nfa.start});
    seen[start_set] = 0;
    work.push(start_set);

    std::array<int, 256> empty_trans;
    empty_trans.fill(-1);
    d.trans.push_back(empty_trans);
    d.accepting.push_back(0);

    while (!work.empty()) {
        auto s = work.front(); work.pop();
        int idx = seen[s];

        if (s.count(nfa.accept)) d.accepting[idx] = 1;

        for (int c = 0; c < 256; ++c) {
            auto ns = closure(nfa, move(nfa, s, static_cast<char>(c)));
            if (ns.empty()) continue;

            auto it = seen.find(ns);
            int nidx;
            if (it == seen.end()) {
                nidx = static_cast<int>(d.accepting.size());
                seen[ns] = nidx;
                work.push(ns);
                d.accepting.push_back(0);
                d.trans.push_back(empty_trans);
            } else {
                nidx = it->second;
            }
            d.trans[idx][c] = nidx;
        }
    }
    return d;
}

} // namespace fsm::dfa