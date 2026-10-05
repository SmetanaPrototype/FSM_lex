module;

#include <string>
#include <string_view>
#include <vector>
#include <thread>
#include <algorithm>
#include <cstddef>

export module fsm.parallel;

import fsm.dfa;

export namespace fsm::parallel {

std::vector<char> accepts_many(const dfa::DFA& d,
                                const std::vector<std::string>& inputs,
                                std::size_t num_threads = 0);

} // namespace fsm::parallel

module :private;

namespace fsm::parallel {

std::vector<char> accepts_many(const dfa::DFA& d,
                                const std::vector<std::string>& inputs,
                                std::size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 1;
    }

    std::vector<char> result(inputs.size(), 0);
    if (inputs.empty()) return result;

    std::size_t chunk = (inputs.size() + num_threads - 1) / num_threads;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t t = 0; t < num_threads; ++t) {
        std::size_t lo = t * chunk;
        std::size_t hi = std::min(lo + chunk, inputs.size());
        if (lo >= hi) break;

        threads.emplace_back([&d, &inputs, &result, lo, hi]() {
            for (std::size_t i = lo; i < hi; ++i)
                result[i] = d.accepts(inputs[i]) ? 1 : 0;
        });
    }
    for (auto& t : threads) t.join();
    return result;
}

} // namespace fsm::parallel