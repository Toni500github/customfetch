#include <string>
#include "util.hpp"

#include "catch2/catch_amalgamated.hpp"
std::string tmp;

TEST_CASE( "util.cpp test suitcase", "[Util]" ) {
    SECTION( "String asserts" ) {
        std::string env = std::getenv("HOME");
        std::string path = "~/.config/rule34";
        REQUIRE(hasEnding("I want the end", "end"));
        REQUIRE(hasStart("And now the begin, then  I want the end", "And"));
        REQUIRE(expandVar(path) == env + "/.config/rule34");
        REQUIRE(read_shell_exec("echo hello") == "hello");
    }
}
