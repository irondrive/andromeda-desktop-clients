#ifndef LIBA2_SECUREBUFFER_H_
#define LIBA2_SECUREBUFFER_H_

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

    inline virtual ~SecureBuffer() noexcept { dealloc(mBuf); }

    inline SecureBuffer(const SecureBuffer& src) noexcept : // copy
        mSize(src.mSize), mBuf(alloc(mSize))
    {
        memcpy(mBuf, src.mBuf, mSize);
    }

    inline SecureBuffer(SecureBuffer&& old) noexcept : // move
        mSize(old.mSize), mBuf(old.mBuf)
    {
        old.mSize = 0;
        old.mBuf = nullptr;
    }

    SecureBuffer& operator=(const SecureBuffer& src) noexcept = delete; // copy

    SecureBuffer& operator=(SecureBuffer&& old) noexcept // move
    {
        mSize = old.mSize;
        mBuf = old.mBuf;
        old.mSize = 0;
        old.mBuf = nullptr;
        return *this;
    }

    /** Compare to another SecureBuffer */
    bool operator==(const SecureBuffer& rhs) const;

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
        T* newBuf = alloc(newSize);
        memcpy(newBuf, mBuf, (newSize < mSize) ? newSize : mSize); // min
        dealloc(mBuf);
        
        mSize = newSize;
        mBuf = newBuf;
    }

    /** Returns a new SecureBuffer whose content is size# bytes from offset */
    [[nodiscard]] inline SecureBuffer substr(size_t offset, size_t size) const noexcept
    {
        SecureBuffer ret(size);
        memcpy(ret.mBuf, mBuf+offset, size);
        return ret;
    }

    /** Construct from a nul-terminated c-string (insecure - unit test only!) */
    inline static SecureBuffer Insecure_FromCstr(const char* cstr) { return SecureBuffer(cstr); }

    /** Construct from bytes from a char buf (insecure - unit test only!) */
    inline static SecureBuffer Insecure_FromBuf(const char* buf, size_t size) { return SecureBuffer(buf, size); }

    /** Construct a std::string from the SecureBuffer (insecure - unit test only!) */
    [[nodiscard]] inline std::string Insecure_ToStr() const { return std::string(mBuf, mSize); }

    /** Compare to another c-string (insecure - unit test only!) */
    bool operator==(const char* cstr) const;

private:

    /** Construct from a nul-terminated c-string (insecure - unit test only!) */
    explicit inline SecureBuffer(const char* cstr) :
        SecureBuffer(cstr, strlen(cstr)) { }

    /** Construct from bytes from a char buf (insecure - unit test only!) */
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
