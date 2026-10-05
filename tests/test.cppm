module;
#include <cstdio>
#include <string_view>

export module fsm.test;

namespace fsm::test::detail {
    inline int& passed() { static int v = 0; return v; }
    inline int& failed() { static int v = 0; return v; }
}

export namespace fsm::test {

inline void check(bool cond, std::string_view expr)
{
    if (cond) { ++detail::passed(); return; }
    ++detail::failed();
    std::fprintf(stderr, "FAIL  %.*s\n",
                 static_cast<int>(expr.size()), expr.data());
}

inline int summary()
{
    std::fprintf(stderr, "%d passed, %d failed\n",
                 detail::passed(), detail::failed());
    return detail::failed() == 0 ? 0 : 1;
}

} // namespace fsm::test