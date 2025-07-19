
#include "catch2/catch_test_macros.hpp"

#include "base64.hpp"

namespace Andromeda {
namespace { // anonymous

/*****************************************************/
TEST_CASE("Encode", "base64")
{
    REQUIRE(StringUtil::base64_encode("").empty());

    REQUIRE(StringUtil::base64_encode("a") == "YQ=="); // 2==
    REQUIRE(StringUtil::base64_encode("ab") == "YWI="); // 1==
    REQUIRE(StringUtil::base64_encode("abc") == "YWJj"); // 0==

    REQUIRE(StringUtil::base64_encode("What's in a name? That which we call a rose By any other word would smell as sweet.")
        == "V2hhdCdzIGluIGEgbmFtZT8gVGhhdCB3aGljaCB3ZSBjYWxsIGEgcm9zZSBCeSBhbnkgb3RoZXIgd29yZCB3b3VsZCBzbWVsbCBhcyBzd2VldC4=");

    const std::string str("\x10\x00\x21\xD0\x9C\x61\xFF\x46",8);
    REQUIRE(StringUtil::base64_encode(str) == "EAAh0Jxh/0Y=");
}

/*****************************************************/
TEST_CASE("Decode", "base64")
{
    REQUIRE(StringUtil::base64_decode("")->empty());

    REQUIRE(StringUtil::base64_decode("YQ==") == "a"); // 2==
    REQUIRE(StringUtil::base64_decode("YWI=") == "ab"); // 1==
    REQUIRE(StringUtil::base64_decode("YWJj") == "abc"); // 0==

    REQUIRE(StringUtil::base64_decode("V2hhdCdzIGluIGEgbmFtZT8gVGhhdCB3aGljaCB3ZSBjYWxsIGEgcm9zZSBCeSBhbnkgb3RoZXIgd29yZCB3b3VsZCBzbWVsbCBhcyBzd2VldC4=")
        == "What's in a name? That which we call a rose By any other word would smell as sweet.");

    const std::string str("\x10\x00\x21\xD0\x9C\x61\xFF\x46",8);
    REQUIRE(StringUtil::base64_decode("EAAh0Jxh/0Y=") == str);

    REQUIRE(StringUtil::base64_decode(" ") == std::nullopt);
    REQUIRE(StringUtil::base64_decode(std::string("\0",1)) == std::nullopt);
    REQUIRE(StringUtil::base64_decode("not valid") == std::nullopt); // spaces
    REQUIRE(StringUtil::base64_decode("123456 ") == std::nullopt); // spaces
    REQUIRE(StringUtil::base64_decode(" 123456") == std::nullopt); // spaces
    REQUIRE(StringUtil::base64_decode("YWI") == std::nullopt); // missing padding
    REQUIRE(StringUtil::base64_decode("YWIax") == std::nullopt); // missing padding
    REQUIRE(StringUtil::base64_decode("YWIaxy") == std::nullopt); // missing padding
}

} // namespace
} // namespace Andromeda
