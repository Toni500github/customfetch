#include "query.hpp"
#include "display.hpp"
#include "util.hpp"

int main (int argc, char *argv[]) {
#ifdef PARSER_TEST
    // test
    fmt::println("=== PARSER TEST! ===");

    std::string test_1 = "Hello, World!";
    std::string test_2 = "Hello, $(echo \"World\")!";
    std::string test_3 = "Hello, \\$(echo \"World\")!";
    std::string test_4 = "Hello, $\\(echo \"World\")!";

    fmt::print("Useless string (input: {}): ", test_1);
    parse(test_1);
    fmt::println("{}", test_1);
    fmt::print("Exec string (input: {}): ", test_2);
    parse(test_2);
    fmt::println("{}", test_2);
    fmt::print("Bypassed exec string #1 (input: {}): ", test_3);
    parse(test_3);
    fmt::println("{}", test_3);
    fmt::print("Bypassed exec string #2 (input: {}): ", test_4);
    parse(test_4);
    fmt::println("{}", test_4);
#endif

    Display::display(Display::render());
    return 0;
}
