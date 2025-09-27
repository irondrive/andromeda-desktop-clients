
#include <array>
#include <filesystem>
#include <fstream>

#include "catch2/catch_test_macros.hpp"

#include "BaseOptions.hpp"
#include "TempPath.hpp"

namespace Andromeda {
namespace { // anonymous

class TestOptions : public BaseOptions
{
public:
    bool AddFlag(const std::string& flag) override
    {
        if(flag == "INVALID") return false;
        flags.push_back(flag); return true;
    }

    bool AddOption(const std::string& option, const std::string& value) override
    {
        if (value == "INVALID") return false;
        options.emplace(option, value); return true;
    }

    void TryAddUrlFlag(const std::string& flag) override
    {
        flags.push_back(flag);
    }

    void TryAddUrlOption(const std::string& option, const std::string& value) override
    {
        options.emplace(option, value);
    }

    void Validate() const override { }
    void Reset() { flags.clear(); options.clear(); }

    using Flags = std::list<std::string>;
    using Options = std::multimap<std::string, std::string>;

    Flags flags;
    Options options;
};

/*****************************************************/
TEST_CASE("ParseArgs", "[BaseOptions]")
{
    {
        const std::array<const char*,0> args { };
        TestOptions options; options.ParseArgs(args.size(), args.data());
        REQUIRE(options.flags == TestOptions::Flags{}); // NOLINT(readability-container-size-empty)
        REQUIRE(options.options == TestOptions::Options{}); // NOLINT(readability-container-size-empty)
    }

    {
        const std::array<const char*,1> args { "-d" };
        TestOptions options; options.ParseArgs(args.size(), args.data());
        REQUIRE(options.flags == TestOptions::Flags{"d"});
        REQUIRE(options.options.empty());
    }

    {
        const std::array<const char*,11> args { "-a", "-b1", "-c", "2", "-x=5", "--y=6", "--test", "--test2", "val", "--test3", "" };
        TestOptions options; options.ParseArgs(args.size(), args.data());
        REQUIRE(options.flags == TestOptions::Flags{"a","test"});
        REQUIRE(options.options == TestOptions::Options{{"b","1"},{"c","2"},{"x","5"},{"y","6"},{"test2","val"},{"test3",""}});
    }

    {
        const std::array<const char*,3> args { "-a", "test1", "" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data()), BaseOptions::BadUsageException);
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data(), true), BaseOptions::BadUsageException);
    }

    {
        const std::array<const char*,2> args { "-a", "INVALID" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data()), BaseOptions::BadOptionException); options.Reset();

        REQUIRE(options.ParseArgs(args.size(), args.data(), true) == args.size()-1);
        REQUIRE(options.flags == TestOptions::Flags{"a"});
        REQUIRE(options.options.empty());
    }

    {
        const std::array<const char*,3> args { "-a", "test1", "INVALID" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data()), BaseOptions::BadUsageException); options.Reset();

        REQUIRE(options.ParseArgs(args.size(), args.data(), true) == args.size()-1);
        REQUIRE(options.flags.empty());
        REQUIRE(options.options == TestOptions::Options{{"a","test1"}});
    }
}

/*****************************************************/
void DoParseFile(TestOptions& options, const std::string& fileData)
{
    const TempPath tmppath("test_ParseFile"); 
    std::ofstream tmpfile;
    tmpfile.open(tmppath.Get()); 
    tmpfile << fileData; tmpfile.close();
    options.ParseFile(tmppath.Get());
}

TEST_CASE("ParseFile", "[BaseOptions]")
{
    {
        TestOptions options; DoParseFile(options, "");
        REQUIRE(options.flags.empty());
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; DoParseFile(options, "d");
        REQUIRE(options.flags == TestOptions::Flags{"d"});
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; DoParseFile(options, "a\n\n#test\nb=1\ntest=val\nccc\ntest2=\n");
        REQUIRE(options.flags == TestOptions::Flags{"a","ccc"});
        REQUIRE(options.options == TestOptions::Options{{"b","1"},{"test","val"},{"test2",""}});
    }
}

/*****************************************************/
TEST_CASE("ParseUrl", "[BaseOptions]")
{
    {
        TestOptions options; options.ParseUrl("");
        REQUIRE(options.flags.empty());
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; options.ParseUrl("myhost/path?test");
        REQUIRE(options.flags == TestOptions::Flags{"test"});
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; options.ParseUrl("https://test.com/path1/path2?test=&test2=a&b&c");
        REQUIRE(options.flags == TestOptions::Flags{"b","c"});
        REQUIRE(options.options == TestOptions::Options{{"test",""},{"test2","a"}});
    }
}

} // namespace
} // namespace Andromeda
