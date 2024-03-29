
#include <chrono>
#include <list>
#include <mutex>
#include <thread>

#include "catch2/catch_test_macros.hpp"

#include "Semaphor.hpp"

namespace Andromeda {
namespace { // anonymous

// Yes obviously these tests are full of race conditions and timing assumptions
// ... don't run these regularly unless doing development on this class

using Results = std::list<std::string>;

void wait(const size_t mstime) 
{ 
    std::this_thread::sleep_for(
        std::chrono::milliseconds(mstime)); 
}

void RunLock(Semaphor& sem, Results& res, 
    std::mutex& resMutex, const std::string& name)
{
    sem.lock();

    { // lock scope
        const std::lock_guard<std::mutex> resLock(resMutex);
        res.push_back(name+"_lock"); 
    }
}

void RunUnlock(Semaphor& sem, Results& res, 
    std::mutex& resMutex, const std::string& name)
{
    { // lock scope
        const std::lock_guard<std::mutex> resLock(resMutex);
        res.push_back(name+"_unlock"); 
    }

    sem.unlock();
}

void RunTimed(Semaphor& sem, Results& res, std::mutex& resMutex, 
    const char* const name, const size_t mstime)
    // name is not a string ref because it would go out of scope (thread)
{
    RunLock(sem, res, resMutex, name);
    wait(mstime);
    RunUnlock(sem, res, resMutex, name);
}

#define RunThread(tn, name, mstime) \
    std::thread tn(RunTimed, std::ref(sem), std::ref(res), std::ref(resMutex), name, mstime);

/*****************************************************/
TEST_CASE("Test2.1", "[Semaphor]") 
{
    Semaphor sem; Results res; std::mutex resMutex;

    RunLock(sem, res, resMutex, "1");
    RunThread(t2, "2", 10); wait(100);

    RunUnlock(sem, res, resMutex, "1");
    t2.join();

    REQUIRE(res == Results{"1_lock", "1_unlock", "2_lock", "2_unlock"});
}

/*****************************************************/
TEST_CASE("Test3.2", "[Semaphor]") 
{
    Semaphor sem(2); Results res; std::mutex resMutex;

    RunLock(sem, res, resMutex, "1");
    RunLock(sem, res, resMutex, "2");
    RunThread(t3, "3", 10); wait(100);

    RunUnlock(sem, res, resMutex, "2");
    t3.join();
    RunUnlock(sem, res, resMutex, "1");

    REQUIRE(res == Results{"1_lock", "2_lock", "2_unlock", "3_lock", "3_unlock", "1_unlock"});
}

/*****************************************************/
TEST_CASE("Test8.3", "[Semaphor]") 
{
    Semaphor sem(3); Results res; std::mutex resMutex;

    RunLock(sem, res, resMutex, "1");
    RunLock(sem, res, resMutex, "2");
    RunLock(sem, res, resMutex, "3");
    RunThread(t4, "4", 100); wait(30);
    RunThread(t5, "5", 100); wait(30);
    RunThread(t6, "6", 100); wait(30);
    RunThread(t7, "7", 100); wait(30);
    RunThread(t8, "8", 100); wait(30);
    // this will check the FIFO order

    RunUnlock(sem, res, resMutex, "2"); // run 4
    t4.join(); wait(30); // unlock 4, run 5

    RunUnlock(sem, res, resMutex, "1"); wait(30); // run 6
    RunUnlock(sem, res, resMutex, "3"); // run 7

    // 5 finish, run 8, 6 finish
    t7.join(); // 7 finish
    t8.join(); // 8 finish
    t5.join();
    t6.join();

    REQUIRE(res == Results{
        "1_lock", "2_lock", "3_lock", 
        "2_unlock", "4_lock", 
        "4_unlock", "5_lock", 
        "1_unlock", "6_lock", 
        "3_unlock", "7_lock", 
        "5_unlock", "8_lock", 
        "6_unlock", "7_unlock", "8_unlock"});
}

/*****************************************************/
/*TEST_CASE("SetMax", "[Semaphor]")
{
    Semaphor sem(2); Results res; std::mutex resMutex;

    RunThread(t1, "1", 1000); wait(100);
    RunThread(t2, "2", 1000); wait(100);
    RunThread(t3, "3", 1000); wait(100);
    RunThread(t4, "4", 2000); wait(100);

    res.push_back("before set_max(4)");
    sem.set_max(4); wait(100); // should allow t3 and t4 to go
    res.push_back("after set_max(4)");

    sem.set_max(1); // should block until 3 are unlocked
    res.push_back("after set_max(1)");

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    REQUIRE(res == Results{
        "1_lock", "2_lock", 
        "before set_max(4)",
        "3_lock", "4_lock",
        "after set_max(4)",
        "1_unlock", "2_unlock", "3_unlock", 
        "after set_max(1)", "4_unlock" });
}*/ // TODO FUTURE implement set_max again if needed

/*****************************************************/
TEST_CASE("TryLock", "[Semaphor]")
{
    Semaphor sem(2);

    sem.lock();
    sem.lock();
    REQUIRE(sem.try_lock() == false);
    sem.unlock();
    REQUIRE(sem.try_lock() == true);
    REQUIRE(sem.try_lock() == false);
    sem.unlock();
    sem.unlock();

    /*sem.set_max(0);
    REQUIRE(sem.try_lock() == false);
    sem.set_max(1);
    REQUIRE(sem.try_lock() == true);
    REQUIRE(sem.try_lock() == false);
    sem.unlock();*/
}

} // namespace
} // namespace Andromeda
