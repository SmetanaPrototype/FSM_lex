#include <print>
#include <string>
#include <vector>

import fsm.regex;
import fsm.nfa;
import fsm.dfa;
import fsm.parallel;

int main() {
    // 1. -> AST
    auto ast = fsm::regex::parse("(a|b)*abb");
    if (!ast) {
        std::println("bad regex");
        return 1;
    }
    std::println("AST:      {}", ast->toString());

    // 2. AST -> NFA
    auto nfa = fsm::nfa::build(*ast);
    std::println("NFA size: {} states", nfa.states.size());

    // 3. NFA -> DFA
    auto dfa = fsm::dfa::build(nfa);
    std::println("DFA size: {} states", dfa.trans.size());

    std::println("");
    for (auto s : {"abb", "aabb", "ababb", "ab", "abba", ""})
        std::println("  {:<7} -> {}", s, dfa.accepts(s) ? "accept" : "reject");

    std::vector<std::string> inputs;
    for (int i = 0; i < 1000; ++i)
        inputs.push_back(i % 2 ? "aabb" : "ab");

    auto results = fsm::parallel::accepts_many(dfa, inputs, 4);

    std::size_t accepted = 0;
    for (char r : results) accepted += (r != 0);
    std::println("\nParallel: {}/{} accepted", accepted, results.size());

    return 0;
}