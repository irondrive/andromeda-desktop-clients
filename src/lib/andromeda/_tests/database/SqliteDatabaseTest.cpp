
#include <array>
#include <string>
#include <vector>

#include "catch2/catch_test_macros.hpp"

#include "andromeda/TempPath.hpp"
#include "andromeda/database/DatabaseException.hpp"
#include "andromeda/database/MixedValue.hpp"
#include "andromeda/database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {
namespace { // anonymous

using RowVector = std::vector<SqliteDatabase::Row*>; // can't be &
void ToVector(SqliteDatabase::RowList& in, RowVector& out)
{
    out.resize(in.size()); size_t i { 0 };
    for (SqliteDatabase::RowList::value_type& val : in)
        out[i++] = &val;
}

/*****************************************************/
TEST_CASE("Query", "[SqliteDatabase]")
{
    const TempPath tmppath("test_sqlite_query.s3db"); 
    SqliteDatabase database(tmppath.Get());

    database.query("CREATE TABLE `mytest` (`id` INTEGER, `name` TEXT);",{});

    REQUIRE(database.query("INSERT INTO `mytest` VALUES (:d0,:d1)", {{":d0",5},{":d1","test1"}}) == 1);
    REQUIRE(database.query("INSERT INTO `mytest` VALUES (:d0,:d1)", {{":d0",7},{":d1","test2"}}) == 1);

    SqliteDatabase::RowList rows; RowVector rowsV;

    database.query("SELECT * FROM `mytest`",{},rows);
    REQUIRE(rows.size() == 2); ToVector(rows,rowsV);
    REQUIRE(rowsV[0]->at("id") == 5);
    REQUIRE(rowsV[0]->at("name") == "test1");
    REQUIRE(rowsV[1]->at("id") == 7);
    REQUIRE(rowsV[1]->at("name").is_null() == false);
    REQUIRE(rowsV[1]->at("name") == "test2");

    // note that only 1 column is modified, but 2 are matched, so retval is 2
    REQUIRE(database.query("UPDATE `mytest` SET `name`=:d0", {{":d0","test2"}}) == 2);
    REQUIRE(database.query("INSERT INTO `mytest` VALUES (:d0,:d1)", {{":d0",9},{":d1",nullptr}}) == 1);

    rows.clear();
    database.query("SELECT * FROM `mytest` WHERE `name`=:d0", {{":d0","test2"}}, rows);
    REQUIRE(rows.size() == 2); ToVector(rows,rowsV);
    REQUIRE(rowsV[0]->at("id") == 5);
    REQUIRE(rowsV[0]->at("name") == "test2"); // updated

    REQUIRE(database.query("DELETE FROM `mytest` WHERE `id`=:d0", {{":d0",7}}) == 1);

    rows.clear();
    database.query("SELECT * FROM `mytest`",{},rows);
    REQUIRE(rows.size() == 2); ToVector(rows,rowsV);
    REQUIRE(rowsV[0]->at("id") == 5);
    REQUIRE(rowsV[0]->at("name") == "test2");
    REQUIRE(rowsV[1]->at("id") == 9);
    REQUIRE(rowsV[1]->at("name").is_null());
    REQUIRE(rowsV[1]->at("name") == nullptr);
}

/*****************************************************/
TEST_CASE("MixedTypes", "[SqliteDatabase]")
{
    const TempPath tmppath("test_sqlite_types.s3db"); 
    SqliteDatabase database(tmppath.Get());

    database.query("CREATE TABLE `mytest` (`int` INTEGER, `int64` INTEGER, `string` VARCHAR(32), `blob` BLOB, `float` REAL, `null` TEXT);",{});

    const int myint { -3874 };
    const int64_t myint64 { static_cast<int64_t>(1024)*1024*1024*1024 }; // 1T
    const char* const mystr { "mytest123" };
    const std::string myblob("\x10\x00\x21\xD0\x9C\x61\xFF\x46",8);
    const double myfloat { 3.1415926 };

    database.query("INSERT INTO `mytest` VALUES(:d0,:d1,:d2,:d3,:d4,:d5)", 
        {{":d0",myint},{":d1",myint64},{":d2",mystr},{":d3",myblob},{":d4",myfloat},{":d5",nullptr}});

    SqliteDatabase::RowList rows; 
    database.query("SELECT * from `mytest`",{},rows);
    REQUIRE(rows.size() == 1); SqliteDatabase::Row& row { rows.front() };

    // test the various interfaces for getting data
    REQUIRE(row.at("int") == myint);
    REQUIRE(row.at("int").get<int>() == myint);
    { int out { 0 }; row.at("int").get_to(out); REQUIRE(out == myint); }

    // test the various data types came out right
    REQUIRE(row.at("string") == mystr); // operator== strcmp
    REQUIRE(std::string(row.at("string").get<const char*>()) == mystr); // char* cast
    REQUIRE(row.at("blob") == myblob); // std::string cast

    REQUIRE(row.at("int64") == myint64);
    REQUIRE(row.at("float") == myfloat);
    REQUIRE(row.at("null") == nullptr);
}

/*****************************************************/
TEST_CASE("CommitRollback", "[SqliteDatabase]")
{
    const TempPath tmppath("test_sqlite_query.s3db"); 
    SqliteDatabase database(tmppath.Get());

    database.beginTransaction();
    database.query("CREATE TABLE `mytest` (`id` INTEGER);",{});
    database.commit();

    database.beginTransaction();
    database.query("INSERT INTO `mytest` VALUES(:d0)",{{":d0",5}});

    SqliteDatabase::RowList rows; 
    database.query("SELECT * from `mytest`",{},rows);
    REQUIRE(rows.size() == 1); REQUIRE(rows.front().at("id") == 5);
    database.rollback();

    rows.clear();
    database.query("SELECT * from `mytest`",{},rows);
    REQUIRE(rows.empty());

    database.beginTransaction();
    database.query("INSERT INTO `mytest` VALUES(:d0)",{{":d0",5}});
    database.commit();

    database.beginTransaction();
    database.rollback(); // no effect
    
    rows.clear();
    database.query("SELECT * from `mytest`",{},rows);
    REQUIRE(rows.size() == 1); REQUIRE(rows.front().at("id") == 5);
}

/*****************************************************/
TEST_CASE("AutoTransaction", "[SqliteDatabase]")
{
    const TempPath tmppath("test_sqlite_query.s3db"); 
    SqliteDatabase database(tmppath.Get());

    database.query("CREATE TABLE `mytest` (`id` INTEGER);",{});

    bool inserted { false };
    REQUIRE_THROWS_AS(database.transaction([&]()
    {
        database.query("INSERT INTO `mytest` VALUES(:d0)",{{":d0",5}});
        inserted = true;
        throw DatabaseException("should rollback");
    }), DatabaseException);
    REQUIRE(inserted);

    SqliteDatabase::RowList rows;
    database.query("SELECT * from `mytest`",{},rows);
    REQUIRE(rows.empty());

    database.transaction([&]()
    {
        database.query("INSERT INTO `mytest` VALUES(:d0)",{{":d0",5}});
    });

    database.beginTransaction();
    database.rollback(); // no effect

    rows.clear();
    database.query("SELECT * from `mytest`",{},rows);
    REQUIRE(rows.size() == 1); REQUIRE(rows.front().at("id") == 5);
}

} // namespace
} // namespace Database
} // namespace Andromeda
