#ifndef LIBA2_SECUREBUFFER_H_
#define LIBA2_SECUREBUFFER_H_

#include <cassert>
#include <cstring>

namespace Andromeda {

/** 
 * Secure memory allocation functions
 * THREAD SAFE (internal locks)
 */
struct SecureMemory
{
    SecureMemory() = delete; // static only

    /** allocate num number of elements with given size, aligned to size, NOT initialized! */
    [[nodiscard]] static void* alloc(size_t num, size_t size) noexcept;

    /** free a pointer returned by alloc */
    static void dealloc(void* ptr) noexcept;

    /** allocate num number of T elements */
    template<typename T>
    [[nodiscard]] inline static T* allocT(size_t num) noexcept { 
        return static_cast<T*>(alloc(num, sizeof(T))); }

    /** free a T pointer returned by allocT */
    template<typename T>
    inline static void freeT(T* ptr) noexcept {
        dealloc(static_cast<void*>(ptr)); }
};

/** Secure memory allocator as a std C++ allocator */
template<typename T>
struct SecureAllocator
{
    using value_type = T;
    inline T* allocate(std::size_t n){ return SecureMemory::allocT<T>(n); }
    inline void deallocate(T* p, std::size_t n){ return SecureMemory::freeT<T>(p); }
};

/** 
 * Holds a buffer allocated with SecureMemory
 * NOT THREAD SAFE (protect externally)
 */
class SecureBuffer
{
public:
    using T = char;

    SecureBuffer() = default; // empty

    /** Construct with the given buffer size */
    inline explicit SecureBuffer(size_t size) : 
        mSize(size), mBuf(alloc(mSize)) { }

    inline virtual ~SecureBuffer() noexcept { if (mBuf) dealloc(mBuf); }

    inline SecureBuffer(const SecureBuffer& src) noexcept : // copy
        mSize(src.mSize), mBuf(src.mBuf ? alloc(src.mSize) : nullptr)
    {
        if (src.mBuf) memcpy(mBuf, src.mBuf, mSize);
    }

    inline SecureBuffer& operator=(const SecureBuffer& src) noexcept // copy
    {
        if (&src == this) return *this;
        if (mBuf) dealloc(mBuf);

        mSize = src.mSize;
        mBuf = src.mBuf ? alloc(src.mSize) : nullptr;
        if (src.mBuf) memcpy(mBuf, src.mBuf, mSize);
        return *this;
    }

    inline SecureBuffer(SecureBuffer&& old) noexcept : // move
        mSize(old.mSize), mBuf(old.mBuf)
    {
        old.mSize = 0;
        old.mBuf = nullptr;
    }

    inline SecureBuffer& operator=(SecureBuffer&& old) noexcept // move
    {
        mSize = old.mSize;
        mBuf = old.mBuf;
        old.mSize = 0;
        old.mBuf = nullptr;
        return *this;
    }

    inline SecureBuffer& operator+=(const SecureBuffer& rhs) noexcept
    {
        const size_t size0 { mSize };
        resize(size0 + rhs.mSize);
        memcpy(mBuf+size0, rhs.mBuf, rhs.mSize);
        return *this;
    }

    inline SecureBuffer operator+(const SecureBuffer& rhs) noexcept
    {
        SecureBuffer retval(mSize + rhs.mSize);
        memcpy(retval.mBuf, mBuf, mSize);
        memcpy(retval.mBuf+mSize, rhs.mBuf, rhs.mSize);
        return retval;
    }

    /** Compare to another SecureBuffer */
    bool operator==(const SecureBuffer& rhs) const;
    inline bool operator!=(const SecureBuffer& rhs) const { return !(*this == rhs); }

    /** Compare to another c-string (insecure if not constants) */
    bool operator==(const char* cstr) const;
    inline bool operator!=(const char* cstr) const { return !(*this == cstr); }

    /** Returns a pointer to the secure buffer */
    [[nodiscard]] inline T* data() noexcept { return mBuf; }
    /** Returns a pointer to the secure buffer */
    [[nodiscard]] inline const T* data() const noexcept { return mBuf; }
    /** Returns the size of the secure buffer */
    [[nodiscard]] inline size_t size() const noexcept { return mSize; }
    /** Returns true if the buffer is empty */
    [[nodiscard]] inline bool empty() const noexcept { return mSize == 0; }

    /** Reallocates the buffer to the given size, copies data */
    inline void resize(size_t newSize) noexcept
    {
        if (mBuf && newSize == mSize) return;

        T* newBuf = alloc(newSize);
        if (mBuf != nullptr)
        {
            memcpy(newBuf, mBuf, (newSize < mSize) ? newSize : mSize); // min
            dealloc(mBuf);
        }
        
        mSize = newSize;
        mBuf = newBuf;
    }

    /** 
     * Returns a new SecureBuffer whose content is size# bytes from offset 
     * NOTE - undefined behavior if this reads past the end of the buffer
     */
    [[nodiscard]] inline SecureBuffer substr(const size_t offset, 
        size_t size = static_cast<size_t>(-1)) const noexcept
    {
        if (size == static_cast<size_t>(-1)) 
            size = mSize-offset;
        
        SecureBuffer ret(size);
        memcpy(ret.mBuf, mBuf+offset, size);
        return ret;
    }

    /** Construct from a nul-terminated c-string (insecure if not constants) */
    inline static SecureBuffer Insecure_FromCstr(const char* cstr) { return SecureBuffer(cstr); }

    /** Construct from a std::string (insecure) */
    inline static SecureBuffer Insecure_FromStr(const std::string& str) { return SecureBuffer(str.data(), str.size()); }

    /** Construct from bytes from a char buf (insecure) */
    inline static SecureBuffer Insecure_FromBuf(const char* buf, size_t size) { return SecureBuffer(buf, size); }

    /** Construct a std::string from the SecureBuffer (insecure) */
    [[nodiscard]] inline std::string Insecure_ToStr() const { return std::string(mBuf, mSize); }

private:

    /** Construct from a nul-terminated c-string (insecure) */
    explicit inline SecureBuffer(const char* cstr) :
        SecureBuffer(cstr, strlen(cstr)) { }

    /** Construct from bytes from a char buf (insecure) */
    explicit inline SecureBuffer(const char* buf, size_t size) :
        mSize(size), mBuf(alloc(mSize))
    {
        memcpy(mBuf, buf, mSize);
    }

    [[nodiscard]] inline static T* alloc(size_t size) noexcept { 
        return SecureMemory::allocT<T>(size); }

    inline static void dealloc(T* ptr) noexcept { 
        SecureMemory::freeT<T>(ptr); }

    size_t mSize { 0 };
    T* mBuf { nullptr };
};

} // namespace Andromeda

#endif // LIBA2_SECUREBUFFER_H_
