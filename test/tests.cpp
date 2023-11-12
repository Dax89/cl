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
cl::Args parse_args(Ts&&... args) {
    std::initializer_list<const char*> arr = {"", args...};
    return cl::parse(arr.size(), const_cast<char**>(arr.begin()));
}

TEST_CASE("Positionals", "[positional]") {
    clear_cl();
    cl::help_on_exit = false;

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", *"arg1_3"__),
        cl::cmd("command2", "arg2_1", *"arg2_2"__, *"arg2_3"__),
        cl::cmd("command3", "arg3_1"__, "arg3_2"__, *"arg3_3"__),
    };

    cl::Args args = parse_args("command1", "one", "two");
    REQUIRE(!args.empty());
    REQUIRE(args["command1"] == true);
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE(!args["arg1_3"]);

    args = parse_args("command2", "one", "two", "three");
    REQUIRE(!args.empty());
    REQUIRE(args["command2"] == true);
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["arg2_3"] == "three");

    args = parse_args("command3", "one", "two", "three");
    REQUIRE(!args.empty());
    REQUIRE(args["command3"] == true);
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
        cl::opt("o2", "option2"_arg, "Option 2"),
        cl::opt("o3", "option3"_arg, "Option 3"),
    };

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", *--"option1"__),
        cl::cmd("command2", "arg2_1", *"arg2_2"__, --"o1"__, *--"option2"__),
        cl::cmd("command3", *"arg3_1"__, *"arg3_2"__, --"option2"__, --"option3"__),
    };
    // clang-format oon

    cl::Args args = parse_args("command1", "one", "two", "--option1");
    REQUIRE(!args.empty());
    REQUIRE(args["command1"] == true);
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE(args["option1"] == true);

    args = parse_args("command2", "one", "two", "-o1");
    REQUIRE(!args.empty());
    REQUIRE(args["command2"] == true);
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["option1"] == true);
    REQUIRE(args["option2"].is_null());

    args = parse_args("command3", "-o2", "foo", "--option3=bar");
    REQUIRE(!args.empty());
    REQUIRE(args["command3"] == true);
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
        cl::opt("o2", "option2"_arg, "Option 2"),
        cl::opt("o3", "option3"_arg, "Option 3"),
    };

    cl::Usage{
        cl::cmd("command1", "arg1_1", "arg1_2", cl::one("foo", "bar"), *--"option1"__),
        cl::cmd("command2", "arg2_1", *"arg2_2"__, *cl::one("foo", "bar"), --"o1"__, *--"option2"__),
        cl::cmd("command3", "arg3_1"__, "arg3_2"__, cl::one("foo", "bar"), cl::one("123", "456"), --"option2"__, --"option3"__),
    };
    // clang-format oon

    cl::Args args = parse_args("command1", "one", "two", "foo", "--option1");
    REQUIRE(!args.empty());
    REQUIRE(args["command1"] == true);
    REQUIRE(args["arg1_1"] == "one");
    REQUIRE(args["arg1_2"] == "two");
    REQUIRE(args["foo"] == true);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["option1"] == true);

    args = parse_args("command2", "one", "two", "-o1");
    REQUIRE(!args.empty());
    REQUIRE(args["command2"] == true);
    REQUIRE(args["arg2_1"] == "one");
    REQUIRE(args["arg2_2"] == "two");
    REQUIRE(args["foo"] == false);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["option1"] == true);
    REQUIRE(args["option2"].is_null());

    args = parse_args("command3", "one", "two", "foo", "456", "-o2", "val", "--option3=bar");
    REQUIRE(!args.empty());
    REQUIRE(args["command3"] == true);
    REQUIRE(args["arg3_1"] == "one");
    REQUIRE(args["arg3_2"] == "two");
    REQUIRE(args["foo"] == true);
    REQUIRE(args["bar"] == false);
    REQUIRE(args["123"] == false);
    REQUIRE(args["456"] == true);
    REQUIRE(args["option2"] == "val");
    REQUIRE(args["option3"] == "bar");
}

TEST_CASE("Foo", "[foo]") {
    clear_cl();
    cl::help_on_exit = false;

    // Needed later for __ operator
    using namespace cl::string_literals;

    // Set some application info (optional)
    cl::set_program("cl_app");
    cl::set_name("CL App");
    cl::set_description("App Description");
    cl::set_version("1.0");

    // Define options (needed if you use options later)
    cl::Options{
        cl::opt("o1", "opt1", "I'm the option 1"),
        cl::opt("o2", "opt2", "I'm the option 2"),
        cl::opt("o3", "opt3"_arg, "I'm the option 3"),
    };

    // Create parsing rules (for options, the short name can also be used)
    cl::Usage{
      cl::cmd("command1", "pos1", *"pos2"__, --"opt1"__, *--"opt2"__),
      cl::cmd("command2", "pos1", "pos2"__, --"opt1"__, --"opt2"__),
      cl::cmd("command3", "pos1", cl::one("val1", "val2", "val3")),
      cl::cmd("command4", "pos1", *cl::one("val1", "val2", "val3"), *--"opt3"__),
    };

    cl::help();
}
