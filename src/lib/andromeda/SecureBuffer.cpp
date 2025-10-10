
#include <sodium.h>

#include "Crypto.hpp"
#include "Debug.hpp"
#include "SecureBuffer.hpp"

namespace Andromeda {

namespace { // anonymous
Debug sDebug("SecureMemory",nullptr); // NOLINT(cert-err58-cpp)
} // anonymous namespace

/*****************************************************/
void* SecureMemory::alloc(size_t num, size_t size) noexcept
{
    void* const retval = sodium_allocarray(num, size); // alloc, lock
    SDBG_INFO("(num:" << num << " size:" << size << ") -> " << retval);
    return retval;
}

/*****************************************************/
void SecureMemory::dealloc(void* ptr) noexcept
{
    SDBG_INFO("(ptr:" << ptr << ")")
    sodium_free(ptr); // unlock, zero, dealloc
}

/*****************************************************/
bool SecureBuffer::operator==(const SecureBuffer& rhs) const
{
    return mSize == rhs.mSize &&
        !sodium_memcmp(mBuf, rhs.mBuf, mSize);
}

/*****************************************************/
bool SecureBuffer::operator==(const char* cstr) const
{
    return mSize == strlen(cstr) &&
        !sodium_memcmp(mBuf, cstr, mSize);
}

} // namespace Andromeda
