#include <print>
import fsm.token;

int main() {
    fsm::Token t{ fsm::Tok::End, {}, 1, 1 };
    std::println("kind = {}", std::to_underlying(t.kind));
    std::print("Привет, {}!\n", "мир");
    return 0;
}