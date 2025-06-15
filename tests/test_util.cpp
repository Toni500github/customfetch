#include <string>
#include "util.hpp"

#include "catch2/catch_amalgamated.hpp"
std::string tmp;

TEST_CASE( "util.cpp test suitcase", "[Util]" ) {
    SECTION( "String asserts" ) {
        std::string strip_str{"   strip_\tthis_\nstring"};
        strip(strip_str);
        std::string replace_str_str{"replace foo and foo with bar"};
        replace_str(replace_str_str, "foo", "bar");
        std::string env = std::getenv("HOME");
        std::string path = "~/.config/rule34";

        REQUIRE(hasEnding("I want the end", "end"));
        REQUIRE(hasStart("And now the begin, then  I want the end", "And"));
        REQUIRE(expandVar(path) == env + "/.config/rule34");
        REQUIRE(read_shell_exec("echo hello") == "hello");
        REQUIRE(split("this;should;be;a;vector", ';') == std::vector<std::string>{"this", "should", "be", "a", "vector"});
        REQUIRE(strip_str == "strip_this_string");
        REQUIRE(replace_str_str == "replace bar and bar with bar");
        REQUIRE(str_tolower("ThIS SHouLD Be LOWER") == "this should be lower");
        REQUIRE(str_toupper("ThIS SHouLD Be UPPER") == "THIS SHOULD BE UPPER");
    }
}
