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

inline std::string_view strip_dash(std::string_view v) {
    for(size_t i = 0; !v.empty() && i < 2; i++) {
        if(v.front() != '-')
            break;
        v = v.substr(1);
    }

    return v;
}

template<typename... Args>
[[noreturn]] inline void error_and_exit(Args&&... args);

[[noreturn]] inline void abort() {
    std::fputs("Unreachable code detected\n", stdout);
    std::exit(3);
}

template<typename... Args>
void print(Args&&... args) {
    std::array<std::string_view, sizeof...(Args)> arr{args...};

    for(std::string_view a : arr)
        std::fputs(a.data(), stdout);

    if(!arr.empty())
        std::fputs("\n", stdout);
}

template<typename... Args>
[[noreturn]] void print_and_exit(Args&&... args) {
    impl::print(std::forward<Args>(args)...);
    std::exit(2);
}

struct OptArg {
    OptArg(const char* n): val{n}, flag{true} {}; // NOLINT
    OptArg(std::string_view n, bool f): val{n}, flag{f} {}

    std::string_view val;
    bool flag;
};

struct Option {
    std::string_view shortname;
    std::string_view name;
    std::string_view description;
    bool flag;

    explicit Option(std::string_view s, const OptArg& n,
                    std::string_view d = {})
        : shortname{s}, name{n.val}, flag{n.flag}, description{d} {
        if(name.empty())
            impl::print_and_exit("Option name is empty");
    }

    explicit Option(const OptArg& n, std::string_view d = {})
        : name{n.val}, flag{n.flag}, description{d} {
        if(name.empty())
            impl::print_and_exit("Option name is empty");
    };

    [[nodiscard]] size_t columns() const {
        return shortname.size() + name.size() + 4;
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

struct Arg: public Base<Arg> {
    explicit Arg(std::string_view arg): Base<Arg>{}, val{arg} {}

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

struct Value {
    template<typename>
    static constexpr bool always_false_v = false;

    Value() = default;

    template<typename T>
    explicit Value(T arg): v{arg} {}

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

    [[nodiscard]] std::string_view to_string() const {
        return std::get<std::string_view>(v);
    }

    template<typename T>
    bool operator==(T rhs) const {
        using U = std::decay_t<T>;

        if constexpr(std::is_null_pointer_v<T>)
            return this->is_null();
        if constexpr(std::is_convertible_v<U, std::string_view>)
            return this->is_string() && this->to_string() == rhs;
        else if constexpr(std::is_same_v<U, bool>)
            return this->is_bool() && this->to_bool() == rhs;
        else if constexpr(std::is_convertible_v<U, int>)
            return this->is_int() && this->to_int() == rhs;
        else
            static_assert(Value::always_false_v<U>);
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
                    static_assert(Value::always_false_v<T>);
            },
            v);
    }

    explicit operator bool() const { return !this->is_null(); }

    std::variant<std::monostate, bool, int, std::string_view> v;
};

struct Info {
    static constexpr char PATH_SEPARATOR =
#if defined(_WIN32)
        '\\';
#else
        '/';
#endif

    std::string_view display;
    std::string_view description;
    std::string_view name{"program"};
    std::string_view version;

    std::string_view fullpath;
    std::string_view executable;
    std::string_view path;

    Info() = default;

    explicit Info(std::string_view displ, std::string_view descr,
                  std::string_view n, std::string_view v) {
        display = displ;
        description = descr;
        name = n;
        version = v;
    }

    [[nodiscard]] bool has_header() const {
        return !display.empty() || !version.empty() || !description.empty();
    }
};

inline Info info;

} // namespace impl

struct Options {
    Options(std::initializer_list<impl::Option> opts) {
        Options::maxlength = 0;
        Options::items = opts;
        Options::complete();
    }

    static bool empty() { return Options::items.empty(); }

    static void complete() {
        Options::items.insert(Options::items.begin(),
                              impl::Option{"-v", "--version", "Show version"});

        Options::items.insert(Options::items.begin(),
                              impl::Option{"-h", "--help", "Show this screen"});

        for(const impl::Option& o : Options::items) {
            if(Options::valid.count(o.name))
                impl::print_and_exit("Duplicate Option '", o.name, "'");
            else if(Options::valid.count(o.shortname))
                impl::print_and_exit("Duplicate Short Option '", o.shortname,
                                     "'");

            int len = o.columns();
            if(len > Options::maxlength)
                Options::maxlength = len + 2;

            Options::valid.insert(o.name);
            Options::valid.insert(o.shortname);
        }
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

    static std::optional<impl::Option> get_option(std::string_view arg) {
        if(arg.size() < 2)
            return std::nullopt;

        for(const impl::Option& o : Options::items) {
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

    static void check(const impl::Arg& opt) {
        if(!Options::valid.count(opt.val))
            impl::print_and_exit("Unknown option '", opt.val, "'");
    }

    static std::vector<impl::Option> items;
    static std::unordered_set<std::string_view> valid;
    static int maxlength;
};

inline std::vector<impl::Option> Options::items;
inline std::unordered_set<std::string_view> Options::valid;
inline int Options::maxlength;

using Values = std::unordered_map<std::string_view, impl::Value>;
using Entry = std::function<void(const Values&)>;

namespace impl {

struct Cmd {
    Cmd& operator,(std::string_view rhs) { return this->operator,(Arg{rhs}); }

    Cmd& operator,(const Arg& rhs) {
        if(rhs.option) {
            Options::check(rhs);
            options.emplace_back(rhs);
        }
        else {
            args.emplace_back(rhs);
            if(rhs.required)
                ++mincount;
        }
        return *this;
    }

    Cmd& operator,(const One& rhs) {
        args.emplace_back(rhs);
        if(rhs.required)
            ++mincount;
        return *this;
    }

    Cmd& operator>>(Entry&& rhs) {
        entry = std::move(rhs);
        return *this;
    }

    std::string_view name;
    std::vector<std::variant<Arg, One>> args{};
    std::vector<Arg> options{};
    Entry entry{nullptr};
    size_t mincount{0};
};

} // namespace impl

namespace string_literals {

inline impl::Arg operator""__(const char* arg, std::size_t len) {
    return impl::Arg{std::string_view{arg, len}};
}

inline impl::OptArg operator""_arg(const char* arg, std::size_t len) {
    return impl::OptArg{std::string_view{arg, len}, false};
}

} // namespace string_literals

struct Usage {
    Usage(std::initializer_list<impl::Cmd> cmds) { Usage::items = cmds; }
    static std::vector<impl::Cmd> items;
};

inline std::vector<impl::Cmd> Usage::items;

namespace impl {

struct ArgPrinter {
    static std::string dump(const Arg& arg) {
        if(arg.option) {
            auto opt = Options::get_option(arg.val);
            if(!opt)
                impl::abort();

            std::string res{opt->name};
            if(!opt->flag)
                res += "=ARG";
            return res;
        }

        return std::string{arg.val};
    }

    static std::string dump(const One& arg) {
        std::string res;

        for(size_t i = 0; i < arg.items.size(); i++) {
            if(i)
                res += "|";
            res += arg.items[i];
        }

        return "(" + res + ")";
    }

    void operator()(One& arg) {
        if(!arg.required)
            std::fputs("[", stdout);
        std::fputs(ArgPrinter::dump(arg).c_str(), stdout);
        if(!arg.required)
            std::fputs("]", stdout);
    }

    void operator()(Arg& arg) {
        if(!arg.required)
            std::fputs("[", stdout);
        else if(!arg.option)
            std::fputs("<", stdout);
        std::fputs(ArgPrinter::dump(arg).c_str(), stdout);
        if(!arg.required)
            std::fputs("]", stdout);
        else if(!arg.option)
            std::fputs(">", stdout);
    }
};

inline void version() {
    if(!info.display.empty()) {
        std::fputs(info.display.data(), stdout);
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

    if(!Usage::items.empty()) {
        std::fputs("Usage:\n", stdout);

        for(impl::Cmd& c : Usage::items) {
            std::fputs("  ", stdout);

            if(!info.name.empty()) {
                std::fputs(info.name.data(), stdout);
                std::fputs(" ", stdout);
            }

            std::fputs(c.name.data(), stdout);

            for(auto& a : c.args) {
                std::fputs(" ", stdout);
                std::visit(ArgPrinter{}, a);
            }

            for(auto& a : c.options) {
                std::fputs(" ", stdout);
                ArgPrinter{}(a);
            }

            std::fputs("\n", stdout);
        }
    }

    std::fputs("\nOptions:\n", stdout);

    for(const Option& o : Options::items) {
        std::fputs("  ", stdout);
        std::fputs(o.shortname.data(), stdout);
        std::fputs(" ", stdout);
        std::fputs(o.name.data(), stdout);

        int n = Options::maxlength - o.columns();

        for(int i = 0; i < n; i++)
            std::fputs(" ", stdout);

        std::fputs(o.description.data(), stdout);
        std::fputs("\n", stdout);
    }
}

template<typename... Args>
[[noreturn]] inline void error_and_exit(Args&&... args) {
    std::fputs("ERROR: ", stdout);
    impl::print(std::forward<Args>(args)...);
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

} // namespace impl

template<typename... Args>
impl::One one(Args&&... args) {
    return impl::One{std::forward<Args>(args)...};
}

template<typename... Args>
impl::Cmd cmd(std::string_view c, Args&&... args) {
    return (impl::Cmd{c}, ..., args);
}

inline impl::Option opt(std::string_view s, impl::OptArg l,
                        std::string_view d = {}) {
    return impl::Option{s, l, d};
}

inline impl::Option opt(impl::OptArg l, std::string_view d = {}) {
    return impl::Option{{}, l, d};
}

inline void set_info(impl::Info i) { impl::info = i; }
inline void help() { impl::help(); }

inline Values parse(int argc, char** argv) {
    if(argc <= 1) {
        if(!Usage::items.empty())
            impl::help_and_exit();
        return {};
    }

    impl::info.fullpath = argv[0];

    if(!impl::info.fullpath.empty()) {
        size_t idx = impl::info.fullpath.rfind(impl::Info::PATH_SEPARATOR);

        if(idx != std::string_view::npos) {
            impl::info.path = impl::info.fullpath.substr(0, idx);
            impl::info.executable = impl::info.fullpath.substr(idx + 1);
        }
        else
            impl::error_and_exit("Cannot parse executable path");
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

    Values v;

    for(impl::Cmd& cmd : Usage::items) {
        if(cmd.name != c)
            continue;
        if(margs.size() > cmd.args.size())
            impl::help_and_exit();

        for(size_t i = margs.size(); i < cmd.args.size(); i++) {
            std::visit(
                [](auto& x) {
                    if(x.required) {
                        impl::error_and_exit("Missing required argument '",
                                             impl::ArgPrinter::dump(x), "'");
                    }
                },
                cmd.args[i]);

            margs.emplace_back(); // Fill remaining slots
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
                            v[one] = impl::Value{margs[i] == one};
                    }
                    else if constexpr(std::is_same_v<T, impl::Arg>) {
                        v[x.val] = margs[i].empty() ? impl::Value{}
                                                    : impl::Value{margs[i]};
                    }
                    else
                        static_assert(impl::Value::always_false_v<T>);
                },
                cmd.args[i]);
        }

        for(const impl::Arg& arg : cmd.options) {
            auto o = Options::get_option(arg.val);
            if(!o)
                impl::abort();

            if(mopts.count(o->name)) {
                if(o->flag)
                    v[o->name] = impl::Value{true};
                else
                    v[o->name] = impl::Value{mopts[arg.val]};
            }
            else if(arg.required)
                impl::error_and_exit("Missing required option '", o->name, "'");
            else
                v[o->name] = o->flag ? impl::Value{false} : impl::Value{};
        }

        if(cmd.entry)
            cmd.entry(v);
        return v;
    }

    impl::print_and_exit("Unknown command '", c, "'");
}

inline std::string_view full_path() { return impl::info.fullpath; }
inline std::string_view path() { return impl::info.path; }
inline std::string_view executable() { return impl::info.executable; }

} // namespace cl
