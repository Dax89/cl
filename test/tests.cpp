#include <catch2/catch_test_macros.hpp>
#include <cl/cl.h>

using namespace cl::string_literals;

void clear_cl() {
    cl::Options::items.clear();
    cl::Options::valid.clear();
    cl::Usage::items.clear();
}

template<typename... Args>
cl::Values parse_args(Args&&... args) {
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

    auto values = parse_args("command1", "one", "two");
    REQUIRE(!values.empty());
    REQUIRE(values["command1"] == true);
    REQUIRE(values["arg1_1"] == "one");
    REQUIRE(values["arg1_2"] == "two");
    REQUIRE(!values["arg1_3"]);

    values = parse_args("command2", "one", "two", "three");
    REQUIRE(!values.empty());
    REQUIRE(values["command2"] == true);
    REQUIRE(values["arg2_1"] == "one");
    REQUIRE(values["arg2_2"] == "two");
    REQUIRE(values["arg2_3"] == "three");

    values = parse_args("command3", "one", "two", "three");
    REQUIRE(!values.empty());
    REQUIRE(values["command3"] == true);
    REQUIRE(values["arg3_1"] == "one");
    REQUIRE(values["arg3_2"] == "two");
    REQUIRE(values["arg3_3"] == "three");
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

    auto values = parse_args("command1", "one", "two", "--option1");
    REQUIRE(!values.empty());
    REQUIRE(values["command1"] == true);
    REQUIRE(values["arg1_1"] == "one");
    REQUIRE(values["arg1_2"] == "two");
    REQUIRE(values["option1"] == true);

    values = parse_args("command2", "one", "two", "-o1");
    REQUIRE(!values.empty());
    REQUIRE(values["command2"] == true);
    REQUIRE(values["arg2_1"] == "one");
    REQUIRE(values["arg2_2"] == "two");
    REQUIRE(values["option1"] == true);
    REQUIRE(values["option2"].is_null());

    values = parse_args("command3", "-o2", "foo", "--option3=bar");
    REQUIRE(!values.empty());
    REQUIRE(values["command3"] == true);
    REQUIRE(values["arg3_1"].is_null());
    REQUIRE(values["arg3_2"].is_null());
    REQUIRE(values["option2"] == "foo");
    REQUIRE(values["option3"] == "bar");
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

    auto values = parse_args("command1", "one", "two", "foo", "--option1");
    REQUIRE(!values.empty());
    REQUIRE(values["command1"] == true);
    REQUIRE(values["arg1_1"] == "one");
    REQUIRE(values["arg1_2"] == "two");
    REQUIRE(values["foo"] == true);
    REQUIRE(values["bar"] == false);
    REQUIRE(values["option1"] == true);

    values = parse_args("command2", "one", "two", "-o1");
    REQUIRE(!values.empty());
    REQUIRE(values["command2"] == true);
    REQUIRE(values["arg2_1"] == "one");
    REQUIRE(values["arg2_2"] == "two");
    REQUIRE(values["foo"] == false);
    REQUIRE(values["bar"] == false);
    REQUIRE(values["option1"] == true);
    REQUIRE(values["option2"].is_null());

    values = parse_args("command3", "one", "two", "foo", "456", "-o2", "val", "--option3=bar");
    REQUIRE(!values.empty());
    REQUIRE(values["command3"] == true);
    REQUIRE(values["arg3_1"] == "one");
    REQUIRE(values["arg3_2"] == "two");
    REQUIRE(values["foo"] == true);
    REQUIRE(values["bar"] == false);
    REQUIRE(values["123"] == false);
    REQUIRE(values["456"] == true);
    REQUIRE(values["option2"] == "val");
    REQUIRE(values["option3"] == "bar");
}
