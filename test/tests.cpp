#include <catch2/catch_test_macros.hpp>
#include <cl/cl.h>
#include <iostream>

using namespace cl::string_literals;

void clear_cl() {
    cl::Options::items.clear();
    cl::Options::valid.clear();
    cl::Usage::items.clear();
    cl::Usage::commands.clear();
}

template<typename... Ts>
cl::Args parse_os(Ts&&... args) {
    std::initializer_list<const char*> arr = {"", args...};
    return cl::parse(arr.size(), const_cast<char**>(arr.begin()));
}

TEST_CASE("Positionals", "[positional]") {
    clear_cl();
    cl::help_on_exit = false;

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", *"arg1_3"_p),
        cl::cmd("command2", "arg2_1", *"arg2_2"_p, *"arg2_3"_p),
        cl::cmd("command3", "arg3_1"_p, "arg3_2"_p, *"arg3_3"_p),
    };

    cl::Args args = parse_os("command1", "one", "two");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command1"] == "command1");
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE_FALSE(args["arg1_3"]);

    args = parse_os("command2", "one", "two", "three");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command2"] == "command2");
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["arg2_3"] == "three");

    args = parse_os("command3", "one", "two", "three");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command3"] == "command3");
    REQUIRE(args["arg3_1"] == "one");
    REQUIRE(args["arg3_2"] == "two");
    REQUIRE(args["arg3_3"] == "three");
}

TEST_CASE("Options", "[options]") {
    clear_cl();
    cl::help_on_exit = false;

    // clang-format off
    cl::Options{
        cl::opt("o1", "option1", "Option 1"),
        cl::opt("o2", "option2"_o, "Option 2"),
        cl::opt("o3", "option3"_o, "Option 3"),
    };

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", *--"option1"_p),
        cl::cmd("command2", "arg2_1", *"arg2_2"_p, --"o1"_p, *--"option2"_p),
        cl::cmd("command3", *"arg3_1"_p, *"arg3_2"_p, --"option2"_p, --"option3"_p),
    };
    // clang-format oon

    cl::Args args = parse_os("command1", "one", "two", "--option1");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command1"] == "command1");
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE(args["option1"] == true);

    args = parse_os("command2", "one", "two", "-o1");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command2"] == "command2");
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["option1"] == true);
    REQUIRE(args["option2"].is_null());

    args = parse_os("command3", "-o2", "foo", "--option3=bar");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command3"] == "command3");
    REQUIRE(args["arg3_1"].is_null());
    REQUIRE(args["arg3_2"].is_null());
    REQUIRE(args["option2"] == "foo");
    REQUIRE(args["option3"] == "bar");
}

TEST_CASE("Ones", "[ones]") {
    clear_cl();
    cl::help_on_exit = false;

    // clang-format off
    cl::Options{
        cl::opt("o1", "option1", "Option 1"),
        cl::opt("o2", "option2"_o, "Option 2"),
        cl::opt("o3", "option3"_o, "Option 3"),
    };

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", cl::one("foo", "bar"), *--"option1"_p),
        cl::cmd("command2", "arg2_1", *"arg2_2"_p, *cl::one("foo", "bar"), --"o1"_p, *--"option2"_p),
        cl::cmd("command3", "arg3_1"_p, "arg3_2"_p, cl::one("foo", "bar"), cl::one("123", "456"), --"option2"_p, --"option3"_p),
    };
    // clang-format oon

    cl::Args args = parse_os("command1", "one", "two", "foo", "--option1");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command1"] == "command1");
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE(args["foo"] == true);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["option1"] == true);

    args = parse_os("command2", "one", "two", "-o1");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command2"] == "command2");
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["foo"] == false);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["option1"] == true);
    REQUIRE(args["option2"].is_null());

    args = parse_os("command3", "one", "two", "foo", "456", "-o2", "val", "--option3=bar");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command3"] == "command3");
    REQUIRE(args["arg3_1"] == "one");
    REQUIRE(args["arg3_2"] == "two");
    REQUIRE(args["foo"] == true);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["123"] == false);
    REQUIRE(args["456"] == true);
    REQUIRE(args["option2"] == "val");
    REQUIRE(args["option3"] == "bar");
}

TEST_CASE("Any", "[any]") {
    clear_cl();
    cl::help_on_exit = false;

    // clang-format off
    cl::Options{
        cl::opt("o1", "option1", "Option 1"),
        cl::opt("o2", "option2"_o, "Option 2"),
        cl::opt("o3", "option3"_o, "Option 3"),
    };

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", cl::one("foo", "bar"), *--"option1"_p),
        cl::cmd("command2", "arg2_1", *"arg2_2"_p, *cl::one("foo", "bar"), --"o1"_p, *--"option2"_p),
        cl::cmd("command3", "arg3_1"_p, "arg3_2"_p, cl::one("foo", "bar"), cl::one("123", "456"), --"option2"_p, --"option3"_p),
        cl::cmd("command4"_a, "arg4_1"),
        cl::cmd("command5"_a)
    };
    // clang-format oon

    cl::Args args = parse_os("command1", "one", "two", "foo", "--option1");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command1"] == "command1");
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE(args["foo"] == true);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["option1"] == true);

    args = parse_os("command2", "one", "two", "-o1");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command2"] == "command2");
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["foo"] == false);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["option1"] == true);
    REQUIRE(args["option2"].is_null());

    args = parse_os("command3", "one", "two", "foo", "456", "-o2", "val", "--option3=bar");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command3"] == "command3");
    REQUIRE(args["arg3_1"] == "one");
    REQUIRE(args["arg3_2"] == "two");
    REQUIRE(args["foo"] == true);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["123"] == false);
    REQUIRE(args["456"] == true);
    REQUIRE(args["option2"] == "val");
    REQUIRE(args["option3"] == "bar");

    args = parse_os("custom1", "myarg");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command5"].is_bool());
    REQUIRE_FALSE(args["command5"].to_bool());
    REQUIRE(args["command4"] == "custom1");
    REQUIRE(args["arg4_1"] == "myarg");

    args = parse_os("custom2");
    REQUIRE_FALSE(args.empty());
    REQUIRE(args["command4"].is_bool());
    REQUIRE_FALSE(args["command4"].to_bool());
    REQUIRE(args["command5"] == "custom2");
    REQUIRE_FALSE(args["arg4_1"]);
}
