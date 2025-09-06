/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

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
