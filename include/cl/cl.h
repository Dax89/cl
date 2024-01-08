/*
 *  _____  _
 * /  __ \| |      Easy command line parsing with EDSL
 * | /  \/| |      Version: 1.1.0
 * | |    | |
 * | \__/\| |____  https://github.com/Dax89/cl
 *  \____/\_____/
 *
 * License: MIT
 * https://github.com/Dax89/cl/blob/master/LICENSE
 */

#pragma once

#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace cl {

inline bool help_on_exit = true;

namespace impl {

constexpr std::string_view PROGRAM_DEFAULT = "program";

inline std::string_view strip_dash(std::string_view v) {
    for(size_t i = 0; !v.empty() && i < 2; i++) {
        if(v.front() != '-')
            break;
        v = v.substr(1);
    }

    return v;
}

template<typename... Ts>
[[noreturn]] inline void error_and_exit(Ts&&... args);

[[noreturn]] inline void abort() {
    std::fputs("Unreachable code detected\n", stdout);
    std::exit(3);
}

template<typename... Ts>
void print(Ts&&... args) {
    std::array<std::string_view, sizeof...(Ts)> arr{args...};

    for(std::string_view a : arr)
        std::fputs(a.data(), stdout);

    if(!arr.empty())
        std::fputs("\n", stdout);
}

template<typename... Ts>
[[noreturn]] void print_and_exit(Ts&&... args) {
    impl::print(std::forward<Ts>(args)...);
    std::exit(2);
}

struct OptParam {
    OptParam(const char* n): val{n}, flag{true} {}; // NOLINT
    OptParam(std::string_view n, bool f): val{n}, flag{f} {}

    std::string_view val;
    bool flag;
};

struct Opt {
    std::string_view shortname;
    std::string_view name;
    std::string_view description;
    bool flag;

    explicit Opt(std::string_view s, const OptParam& n, std::string_view d = {})
        : shortname{s}, name{n.val}, flag{n.flag}, description{d} {
        if(name.empty())
            impl::print_and_exit("Option name is empty");
    }

    explicit Opt(const OptParam& n, std::string_view d = {})
        : name{n.val}, flag{n.flag}, description{d} {
        if(name.empty())
            impl::print_and_exit("Option name is empty");
    };

    [[nodiscard]] std::string to_short_string() const {
        if(shortname.empty())
            return std::string{};

        return "-" + std::string{shortname};
    }

    [[nodiscard]] std::string to_string() const {
        std::string res{name};
        if(!flag)
            res += "=ARG";
        return "--" + res;
    }
};

template<typename Derived>
struct Base {
    bool required{true};
    bool option{false};

    Derived& operator*() {
        required = false;
        return *static_cast<Derived*>(this);
    }

    Derived& operator--() {
        option = true;
        return *static_cast<Derived*>(this);
    }
};

struct Param: public Base<Param> {
    explicit Param(std::string_view arg): Base<Param>{}, val{arg} {}

    std::string_view val;
};

struct One: public Base<One> {
    One(std::initializer_list<std::string_view> args)
        : Base<One>{}, items{args} {}

    One& operator--() = delete;

    [[nodiscard]] bool contains(std::string_view arg) const {
        for(std::string_view x : items) // NOLINT
            if(arg == x)
                return true;

        return false;
    }

    std::vector<std::string_view> items;
};

using ParamType = std::variant<Param, One>;

struct Info {
    static constexpr char PATH_SEPARATOR =
#if defined(_WIN32)
        '\\';
#else
        '/';
#endif

    std::string_view name;
    std::string_view description;
    std::string_view version;
    std::string_view program{PROGRAM_DEFAULT};

    [[nodiscard]] bool has_header() const {
        return !name.empty() || !version.empty() || !description.empty();
    }
};

inline Info info;

} // namespace impl

struct Arg {
    template<typename>
    static constexpr bool always_false_v = false;

    Arg() = default;

    template<typename T>
    explicit Arg(T t): v{t} {}

    [[nodiscard]] bool is_null() const {
        return std::holds_alternative<std::monostate>(v);
    }

    [[nodiscard]] bool is_bool() const {
        return std::holds_alternative<bool>(v);
    }

    [[nodiscard]] bool is_int() const { return std::holds_alternative<int>(v); }

    [[nodiscard]] bool is_string() const {
        return std::holds_alternative<std::string_view>(v);
    }

    [[nodiscard]] bool to_bool() const { return std::get<bool>(v); }

    [[nodiscard]] int to_int() const { return std::get<int>(v); }

    [[nodiscard]] std::string_view to_stringview() const {
        return std::get<std::string_view>(v);
    }

    [[nodiscard]] std::string to_string() const {
        return std::string{std::get<std::string_view>(v)};
    }

    template<typename T>
    bool operator==(T rhs) const {
        using U = std::decay_t<T>;

        if constexpr(std::is_null_pointer_v<T>)
            return this->is_null();
        if constexpr(std::is_convertible_v<U, std::string_view>)
            return this->is_string() && this->to_stringview() == rhs;
        else if constexpr(std::is_same_v<U, bool>)
            return this->is_bool() && this->to_bool() == rhs;
        else if constexpr(std::is_convertible_v<U, int>)
            return this->is_int() && this->to_int() == rhs;
        else
            static_assert(Arg::always_false_v<U>);
    }

    template<typename T>
    bool operator!=(T rhs) const {
        return !this->operator==(rhs);
    }

    [[nodiscard]] std::string dump() const {
        return std::visit(
            [](auto& x) -> std::string {
                using T = std::decay_t<decltype(x)>;

                if constexpr(std::is_same_v<T, bool>)
                    return x ? "true" : "false";
                else if constexpr(std::is_same_v<T, int>)
                    return std::to_string(x);
                else if constexpr(std::is_same_v<T, std::string_view>)
                    return "\"" + std::string{x} + "\"";
                else if constexpr(std::is_same_v<T, std::monostate>)
                    return "null";
                else
                    static_assert(Arg::always_false_v<T>);
            },
            v);
    }

    explicit operator bool() const { return !this->is_null(); }

    std::variant<std::monostate, bool, int, std::string_view> v;
};

struct Options {
    Options(std::initializer_list<impl::Opt> opts) {
        Options::maxshortlength = 0;
        Options::maxlength = 0;
        Options::items = opts;
        Options::complete();
    }

    static bool empty() { return Options::items.empty(); }

    static void complete() {
        Options::items.insert(Options::items.begin(),
                              impl::Opt{"v", "version", "Show version"});

        Options::items.insert(Options::items.begin(),
                              impl::Opt{"h", "help", "Show this screen"});

        for(const impl::Opt& o : Options::items) {
            if(Options::valid.count(o.name))
                impl::print_and_exit("Duplicate Option '", o.name, "'");
            else if(Options::valid.count(o.shortname))
                impl::print_and_exit("Duplicate Short Option '", o.shortname,
                                     "'");

            int len = o.shortname.size();
            if(len > Options::maxshortlength)
                Options::maxshortlength = len;

            len = o.shortname.size();
            if(len > Options::maxlength)
                Options::maxlength = len;

            Options::valid.insert(o.name);
            Options::valid.insert(o.shortname);
        }

        Options::maxshortlength += 1;
        Options::maxlength += 6;
    }

    static std::pair<std::string_view, std::string_view>
    parse(std::string_view arg) {
        std::pair<std::string_view, std::string_view> res{impl::strip_dash(arg),
                                                          {}};

        for(size_t i = 0; i < arg.size(); i++) {
            if(arg[i] != '=')
                continue;

            res.first = impl::strip_dash(arg.substr(0, i));
            if(i + 1 < arg.size())
                res.second = arg.substr(i + 1);
            break;
        }

        return res;
    }

    static std::optional<impl::Opt> get_option(std::string_view arg) {
        if(arg.size() < 2)
            return std::nullopt;

        for(const impl::Opt& o : Options::items) {
            if(arg == o.shortname)
                return o;
            if(arg == o.name)
                return o;
        }

        return std::nullopt;
    }

    static bool is_short(std::string_view n) {
        bool isshort = false;

        for(size_t i = 0; i < 2; i++) {
            if(!n.empty() && n.front() == '-') {
                isshort = !i;
                n = n.substr(1);
            }
        }

        return isshort;
    }

    static void check(const impl::Param& opt) {
        if(!Options::valid.count(opt.val))
            impl::print_and_exit("Unknown option '", opt.val, "'");
    }

    static std::vector<impl::Opt> items;
    static std::unordered_set<std::string_view> valid;
    static int maxshortlength;
    static int maxlength;
};

inline std::vector<impl::Opt> Options::items;
inline std::unordered_set<std::string_view> Options::valid;
inline int Options::maxshortlength;
inline int Options::maxlength;

using Args = std::unordered_map<std::string_view, Arg>;
using Entry = std::function<void(const Args&)>;

namespace impl {

struct ParamPrinter {
    static std::string dump(const Param& p) {
        if(p.option) {
            auto opt = Options::get_option(p.val);
            if(!opt)
                impl::abort();

            return opt->to_string();
        }

        return std::string{p.val};
    }

    static std::string dump(const One& p) {
        std::string res;

        for(size_t i = 0; i < p.items.size(); i++) {
            if(i)
                res += "|";
            res += p.items[i];
        }

        return "(" + res + ")";
    }

    static std::string dump(ParamType& p) {
        return std::visit([](auto&& x) { return ParamPrinter::dump(x); }, p);
    }

    void operator()(One& p) {
        if(!p.required)
            std::fputs("[", stdout);
        std::fputs(ParamPrinter::dump(p).c_str(), stdout);
        if(!p.required)
            std::fputs("]", stdout);
    }

    void operator()(Param& p) {
        if(!p.required)
            std::fputs("[", stdout);
        else if(!p.option)
            std::fputs("<", stdout);
        std::fputs(ParamPrinter::dump(p).c_str(), stdout);
        if(!p.required)
            std::fputs("]", stdout);
        else if(!p.option)
            std::fputs(">", stdout);
    }
};

struct Cmd {
    Cmd& operator,(std::string_view rhs) { return this->operator,(Param{rhs}); }

    Cmd& operator,(const Param& rhs) {
        if(rhs.option) {
            Options::check(rhs);
            options.emplace_back(rhs);
        }
        else {
            this->check_required(rhs);
            args.emplace_back(rhs);
        }

        return *this;
    }

    Cmd& operator,(const One& rhs) {
        this->check_required(rhs);
        args.emplace_back(rhs);
        return *this;
    }

    Cmd& operator>>(Entry&& rhs) {
        entry = std::move(rhs);
        return *this;
    }

    template<typename T>
    void check_required(const T& p) {
        if(p.required) {
            if(!this->is_prev_required()) {
                impl::error_and_exit(
                    "Positional '", impl::ParamPrinter::dump(p),
                    "' cannot be required because '",
                    impl::ParamPrinter::dump(args.back()),
                    "' is optional for command '", this->name, "'");
            }

            ++mincount;
        }
    }

    [[nodiscard]] bool is_prev_required() const {
        return args.empty() ||
               std::visit([](auto&& x) -> bool { return x.required; },
                          args.back());
    }

    explicit Cmd(std::string_view n): name{n} {}
    explicit Cmd(std::string_view n, bool a): name{n}, any{a} {}

    std::string_view name;
    bool any{false};
    std::vector<ParamType> args{};
    std::vector<Param> options{};
    Entry entry{nullptr};
    size_t mincount{0};
};

} // namespace impl

struct Usage {
    Usage(std::initializer_list<impl::Cmd> cmds) {
        for(const impl::Cmd& c : cmds) {
            if(Usage::commands.count(c.name))
                impl::print_and_exit("Duplicate command '", c.name, "'");

            Usage::items.push_back(c);
            Usage::commands.insert(c.name);
        }
    }

    static std::vector<impl::Cmd> items;
    static std::unordered_set<std::string_view> commands;
};

inline std::vector<impl::Cmd> Usage::items;
inline std::unordered_set<std::string_view> Usage::commands;

namespace impl {
inline void version() {
    if(!info.name.empty()) {
        std::fputs(info.name.data(), stdout);
        std::fputs(" ", stdout);
    }

    if(!info.version.empty())
        std::fputs(info.version.data(), stdout);

    if(!info.description.empty()) {
        std::fputs("\n", stdout);
        std::fputs(info.description.data(), stdout);
    }

    if(info.has_header())
        std::fputs("\n", stdout);
}

inline void help() {
    if(Options::empty())
        Options::complete();

    impl::version();

    if(info.has_header())
        std::fputs("\n", stdout);

    std::fputs("Usage:\n", stdout);

    for(impl::Cmd& c : Usage::items) {
        std::fputs("  ", stdout);
        std::fputs(info.program.data(), stdout);
        std::fputs(" ", stdout);

        if(c.any)
            std::fputs("{", stdout);

        std::fputs(c.name.data(), stdout);

        if(c.any)
            std::fputs("}", stdout);

        for(auto& a : c.args) {
            std::fputs(" ", stdout);
            std::visit(ParamPrinter{}, a);
        }

        for(auto& a : c.options) {
            std::fputs(" ", stdout);
            ParamPrinter{}(a);
        }

        std::fputs("\n", stdout);
    }

    std::fputs("  ", stdout);
    std::fputs(info.program.data(), stdout);
    std::fputs(" ", stdout);
    std::fputs("--version\n", stdout);

    std::fputs("  ", stdout);
    std::fputs(info.program.data(), stdout);
    std::fputs(" ", stdout);
    std::fputs("--help\n", stdout);

    std::fputs("\nOptions:\n", stdout);

    for(const Opt& o : Options::items) {
        std::fputs("  ", stdout);
        std::string s = o.to_short_string();
        std::fputs(s.data(), stdout);

        int n = Options::maxshortlength - s.size();

        for(int i = 0; i < n; i++)
            std::fputs(" ", stdout);

        std::fputs(" ", stdout);
        s = o.to_string();
        std::fputs(s.data(), stdout);

        n = Options::maxlength - s.size() + 6;

        for(int i = 0; i < n; i++)
            std::fputs(" ", stdout);

        std::fputs(" ", stdout);
        std::fputs(o.description.data(), stdout);
        std::fputs("\n", stdout);
    }
}

template<typename... Ts>
[[noreturn]] inline void error_and_exit(Ts&&... args) {
    std::fputs("ERROR: ", stdout);
    impl::print(std::forward<Ts>(args)...);
    std::fputs("\n", stdout);

    if(cl::help_on_exit)
        impl::help();
    std::exit(2);
}

[[noreturn]] inline void help_and_exit() {
    if(cl::help_on_exit)
        impl::help();
    std::exit(1);
}

[[noreturn]] inline void version_and_exit() {
    impl::version();
    std::exit(1);
}

inline Args init_value() {
    Args values;

    for(impl::Cmd& c : Usage::items) {
        values.try_emplace(c.name, false);

        for(ParamType& arg : c.args) {
            std::visit(
                [&](auto& x) {
                    using T = std::decay_t<decltype(x)>;

                    if constexpr(std::is_same_v<T, Param>) {
                        if(!x.option)
                            values.try_emplace(x.val, Arg{});
                    }
                    else if constexpr(std::is_same_v<T, One>) {
                        for(std::string_view item : x.items)
                            values.try_emplace(item, false);
                    }
                    else
                        static_assert(Arg::always_false_v<T>);
                },
                arg);
        }
    }

    for(impl::Opt& o : Options::items) {
        if(o.flag)
            values.try_emplace(o.name, Arg{false});
        else
            values.try_emplace(o.name, Arg{});
    }

    return values;
}

} // namespace impl

template<typename... Ts>
impl::One one(Ts&&... args) {
    return impl::One{std::forward<Ts>(args)...};
}

template<typename... Ts>
impl::Cmd cmd(impl::Cmd&& c, Ts&&... args) {
    return (c, ..., args);
}

template<typename... Ts>
impl::Cmd cmd(std::string_view c, Ts&&... args) {
    return (impl::Cmd{c}, ..., args);
}

inline impl::Opt opt(std::string_view s, impl::OptParam l,
                     std::string_view d = {}) {
    return impl::Opt{s, l, d};
}

inline impl::Opt opt(impl::OptParam l, std::string_view d = {}) {
    return impl::Opt{{}, l, d};
}

inline void set_name(std::string_view n) {
    impl::info.name = n.empty() ? impl::PROGRAM_DEFAULT : n;
}

inline void set_version(std::string_view v) { impl::info.version = v; }
inline void set_description(std::string_view v) { impl::info.description = v; }
inline void set_program(std::string_view n) { impl::info.program = n; }

inline void help() { impl::help(); }

inline Args parse(int argc, char** argv) {
    if(argc <= 1) {
        if(!Usage::items.empty())
            impl::help_and_exit();
        return {};
    }

    if(Options::empty())
        Options::complete();

    std::string_view c = argv[1];

    if(argc == 2) {
        if(c == "-h" || c == "--help")
            impl::help_and_exit();
        else if(c == "-v" || c == "--version")
            impl::version_and_exit();
    }

    std::unordered_map<std::string_view, std::string_view> mopts;
    std::vector<std::string_view> margs;

    for(int i = 2; i < argc;) {
        std::string_view arg{argv[i]};

        if(arg.front() == '-') {
            auto [name, val] = Options::parse(arg);
            auto opt = Options::get_option(name);

            if(opt) {
                bool isshort = Options::is_short(arg);

                if(!opt->flag) {
                    if(isshort) {
                        if(++i >= argc)
                            impl::error_and_exit("Invalid short option format");
                        arg = argv[i];
                    }
                    else {
                        if(val.empty())
                            impl::error_and_exit("Invalid option format '", arg,
                                                 "'");
                        arg = val;
                    }
                }

                ++i;
                mopts[opt->name] = arg;
            }
            else
                impl::error_and_exit("Invalid option '", arg, "'");

            continue;
        }

        margs.push_back(arg);
        i++;
    }

    // Keep an original copy for rollback
    std::vector<std::string_view> basemargs = margs;
    Args v = impl::init_value();

    for(impl::Cmd& cmd : Usage::items) {
        if(!cmd.any && cmd.name != c)
            continue;

        if(margs.size() > cmd.args.size())
            impl::help_and_exit();

        v[cmd.name] = Arg{c};

        bool skip = false;

        for(size_t i = margs.size(); !skip && i < cmd.args.size(); i++) {
            std::visit(
                [&](auto& x) {
                    if(x.required) {
                        if(cmd.any)
                            skip = true;
                        else
                            impl::error_and_exit("Missing required argument '",
                                                 impl::ParamPrinter::dump(x),
                                                 "'");
                    }
                },
                cmd.args[i]);

            margs.emplace_back(); // Fill remaining slots
        }

        /*
         * Maybe an any-type command with invalid arguments has been found,
         * rollback changes and keep finding for a better-fit one.
         */
        if(skip) {
            margs = basemargs;
            v = impl::init_value();
            continue;
        }

        for(size_t i = 0; i < cmd.args.size(); i++) {
            std::visit(
                [&](auto& x) {
                    using T = std::decay_t<decltype(x)>;

                    if constexpr(std::is_same_v<T, impl::One>) {
                        if(!margs[i].empty() && !x.contains(margs[i])) {
                            impl::error_and_exit("Invalid argument '", margs[i],
                                                 "'");
                        }

                        for(std::string_view one : x.items)
                            v[one] = Arg{margs[i] == one};
                    }
                    else if constexpr(std::is_same_v<T, impl::Param>) {
                        if(!margs[i].empty())
                            v[x.val] = Arg{margs[i]};
                    }
                    else
                        static_assert(Arg::always_false_v<T>);
                },
                cmd.args[i]);
        }

        for(const impl::Param& arg : cmd.options) {
            auto o = Options::get_option(arg.val);
            if(!o)
                impl::abort();

            if(mopts.count(o->name)) {
                if(o->flag)
                    v[o->name] = Arg{true};
                else
                    v[o->name] = Arg{mopts[arg.val]};
            }
            else if(arg.required)
                impl::error_and_exit("Missing required option '", o->name, "'");
        }

        if(cmd.entry)
            cmd.entry(v);
        return v;
    }

    impl::print_and_exit("Unknown command '", c, "'");
}

namespace string_literals {

inline impl::Cmd operator""_a(const char* arg, std::size_t len) {
    return impl::Cmd{std::string_view{arg, len}, true};
}

inline impl::Param operator""_p(const char* arg, std::size_t len) {
    return impl::Param{std::string_view{arg, len}};
}

inline impl::OptParam operator""_o(const char* arg, std::size_t len) {
    return impl::OptParam{std::string_view{arg, len}, false};
}

} // namespace string_literals

} // namespace cl
