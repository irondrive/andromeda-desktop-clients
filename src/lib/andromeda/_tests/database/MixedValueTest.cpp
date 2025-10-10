
#include <string>

#include "catch2/catch_test_macros.hpp"

#include "andromeda/database/MixedValue.hpp"

namespace Andromeda {
namespace Database {
namespace { // anonymous

/*****************************************************/
TEST_CASE("MixedValue", "[MixedValue]")
{
    const MixedValue a { 5 };
    const MixedValue b { a }; // copy // NOLINT(*-unnecessary-copy-initialization)
    const MixedValue c { "5" };
    const MixedValue d { nullptr };
    const MixedValue e { 6 };
    const MixedValue s { SecureBuffer::Insecure_FromCstr("test") };

    REQUIRE(a == a); REQUIRE(a == 5);
    REQUIRE(b == a); REQUIRE(b == 5);
    REQUIRE(c != a); REQUIRE(c == "5");
    REQUIRE(d != a); REQUIRE(d == nullptr);
    REQUIRE(e != a);

    REQUIRE(s == SecureBuffer::Insecure_FromStr("test"));

    REQUIRE(a.Debug_ToString() == "5");
    REQUIRE(c.Debug_ToString() == "5");
    REQUIRE(d.Debug_ToString() == "NULL");
    REQUIRE(s.Debug_ToString() == "test");

    const std::string sa { "test" };
    const MixedValue f { sa };
    const std::string sb { "test" };
    const MixedValue g { sb };

    REQUIRE(g != s); // can't compare SecureBuffer

    // same string, different pointers
    REQUIRE(f == g);

    const MixedParams ma {{"a",5},{"b","test"}};
    const MixedParams mb = ma; // copy // NOLINT(*-unnecessary-copy-initialization)
    const MixedParams mc {{"a",6},{"b","test"}};
    const MixedParams md {{"a",5},{"b","test2"}};

    REQUIRE(ma == ma);
    REQUIRE(ma == mb);
    REQUIRE(ma != mc);
    REQUIRE(ma != md);
}

} // namespace
} // namespace Database
} // namespace Andromeda
