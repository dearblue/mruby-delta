#ifndef MEMLDST_H
#define MEMLDST_H 1

#include <stdint.h>

static inline uint8_t load8(const void *ptr);
static inline void store8(void *ptr, uint8_t n);
static inline uint16_t load16be(const void *ptr);
static inline uint16_t load16le(const void *ptr);
static inline void store16be(void *ptr, uint16_t n);
static inline void store16le(void *ptr, uint16_t n);
static inline uint32_t load24be(const void *ptr);
static inline uint32_t load24le(const void *ptr);
static inline void store24be(void *ptr, uint32_t n);
static inline void store24le(void *ptr, uint32_t n);
static inline uint32_t load32be(const void *ptr);
static inline uint32_t load32le(const void *ptr);
static inline void store32be(void *ptr, uint32_t n);
static inline void store32le(void *ptr, uint32_t n);
static inline uint64_t load48be(const void *ptr);
static inline uint64_t load48le(const void *ptr);
static inline void store48be(void *ptr, uint64_t n);
static inline void store48le(void *ptr, uint64_t n);
static inline uint64_t load64be(const void *ptr);
static inline uint64_t load64le(const void *ptr);
static inline void store64be(void *ptr, uint64_t n);
static inline void store64le(void *ptr, uint64_t n);

#define LOAD_STORE_FOREACH(S)                                               \
    S(16, uint16_t)                                                         \
    S(24, uint32_t)                                                         \
    S(32, uint32_t)                                                         \
    S(48, uint64_t)                                                         \
    S(64, uint64_t)                                                         \

/* clang で命令一つにまとまる。gcc だとバイト単位となる。そのうち修正予定 */

#define LOAD_STORE_SNNIPET(BITS, INT)                                       \
    static inline INT                                                       \
    load ## BITS ## be(const void *ptr)                                     \
    {                                                                       \
        const uint8_t *p = (const uint8_t *)ptr;                            \
        INT n = 0;                                                          \
        int i;                                                              \
        for (i = 0; i < BITS; i += 8) {                                     \
            n = (n << 8) | *p ++;                                           \
        }                                                                   \
        return n;                                                           \
    }                                                                       \
                                                                            \
    static inline INT                                                       \
    load ## BITS ## le(const void *ptr)                                     \
    {                                                                       \
        const uint8_t *p = (const uint8_t *)ptr;                            \
        INT n = 0;                                                          \
        int i;                                                              \
        for (i = 0; i < BITS; i += 8) {                                     \
            n = n | ((INT)*p ++ << i);                                      \
        }                                                                   \
        return n;                                                           \
    }                                                                       \
                                                                            \
    static inline void                                                      \
    store ## BITS ## be(void *ptr, INT n)                                   \
    {                                                                       \
        uint8_t *p = (uint8_t *)ptr;                                        \
        int i;                                                              \
        for (i = 0; i < BITS; i += 8) {                                     \
            *p ++ = n >> (BITS - i - 8);                                    \
        }                                                                   \
    }                                                                       \
                                                                            \
    static inline void                                                      \
    store ## BITS ## le(void *ptr, INT n)                                   \
    {                                                                       \
        uint8_t *p = (uint8_t *)ptr;                                        \
        int i;                                                              \
        for (i = 0; i < BITS; i += 8) {                                     \
            *p ++ = n >> i;                                                 \
        }                                                                   \
    }                                                                       \

static inline uint8_t
load8(const void *ptr)
{
    return *(const uint8_t *)ptr;
}

static inline void
store8(void *ptr, uint8_t n)
{
    *(uint8_t *)ptr = n;
}

LOAD_STORE_FOREACH(LOAD_STORE_SNNIPET)

#endif /* MEMLDST_H */
