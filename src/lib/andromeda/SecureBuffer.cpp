
#include <sodium.h>

#include "Crypto.hpp"
#include "SecureBuffer.hpp"

namespace Andromeda {

/*****************************************************/
void* SecureMemory::alloc(size_t num, size_t size) noexcept
{
    Crypto::SodiumInit(); // abort on except

    return sodium_allocarray(num, size); // alloc, lock
}

/*****************************************************/
void SecureMemory::free(void* ptr) noexcept
{
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
