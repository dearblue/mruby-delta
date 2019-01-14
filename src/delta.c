#include "delta.h"
#include "memldst.h"
#include <string.h>
#include <limits.h>
#include <math.h>

#define BITMASK(INT, BITS) ((sizeof(INT) * CHAR_BIT) < (BITS) ? ~((~(INT)0) << BITS) : ~(INT)0)

#ifndef DELTA_WEAK_INLINE
# if defined(DELTA_FORCE_INLINE_EXTRACT)
#  if defined(__GNUC__) || defined (__clang__)
#   define DELTA_WEAK_INLINE __attribute__((always_inline))
#  elif defined _MSC_VER
#   define DELTA_WEAK_INLINE __forceinline
#  else
#   define DELTA_WEAK_INLINE inline
#  endif
# endif
#endif

#ifndef DELTA_WEAK_INLINE
# define DELTA_WEAK_INLINE
#endif

#define IMPLEMENT_BY_TYPES(S)                                               \
    S( 8, int8_t,  uint8_t)                                                 \
    S(16, int16_t, uint16_t)                                                \
    S(24, int32_t, uint32_t)                                                \
    S(32, int32_t, uint32_t)                                                \
    S(48, int64_t, uint64_t)                                                \
    S(64, int64_t, uint64_t)                                                \

#define IMPLEMENT_BY_MODE(S)                                                \
    S(encode,  8,   , int8_t)                                               \
    S(encode, 16, be, int16_t)                                              \
    S(encode, 16, le, int16_t)                                              \
    S(encode, 24, be, int32_t)                                              \
    S(encode, 24, le, int32_t)                                              \
    S(encode, 32, be, int32_t)                                              \
    S(encode, 32, le, int32_t)                                              \
    S(encode, 48, be, int64_t)                                              \
    S(encode, 48, le, int64_t)                                              \
    S(encode, 64, be, int64_t)                                              \
    S(encode, 64, le, int64_t)                                              \
    S(decode,  8,   , int8_t)                                               \
    S(decode, 16, be, int16_t)                                              \
    S(decode, 16, le, int16_t)                                              \
    S(decode, 24, be, int32_t)                                              \
    S(decode, 24, le, int32_t)                                              \
    S(decode, 32, be, int32_t)                                              \
    S(decode, 32, le, int32_t)                                              \
    S(decode, 48, be, int64_t)                                              \
    S(decode, 48, le, int64_t)                                              \
    S(decode, 64, be, int64_t)                                              \
    S(decode, 64, le, int64_t)                                              \

enum {
    mode_encode = 0,
    mode_decode = 1,
};

// encode_difference8  / decode_difference8
// encode_difference16 / decode_difference16
// encode_difference24 / decode_difference24
// encode_difference32 / decode_difference32
// encode_difference48 / decode_difference48
// encode_difference64 / decode_difference64
#define SNNIPET_DEFFERENCE(BITS, INT, UINT)                                 \
    static inline INT                                                       \
    encode_difference ## BITS(INT a, INT b)                                 \
    {                                                                       \
        return (a - b) & BITMASK(INT, BITS);                                \
    }                                                                       \
                                                                            \
    static inline INT                                                       \
    decode_difference ## BITS(INT a, INT b)                                 \
    {                                                                       \
        return (a + b) & BITMASK(INT, BITS);                                \
    }                                                                       \

IMPLEMENT_BY_TYPES(SNNIPET_DEFFERENCE)

// encode_dirrerence8w  / decode_dirrerence8w
// encode_dirrerence16w / decode_dirrerence16w
// encode_dirrerence24w / decode_dirrerence24w
// encode_dirrerence32w / decode_dirrerence32w
// encode_dirrerence48w / decode_dirrerence48w
// encode_dirrerence64w / decode_dirrerence64w
#define SNNIPET_DIFFERENCEW(BITS, INT, UINT)                                \
    static inline INT                                                       \
    encode_difference ## BITS ## w(INT a, INT *b)                           \
    {                                                                       \
        INT w = (a - *b) & BITMASK(INT, BITS);                              \
        *b = a;                                                             \
        return w;                                                           \
    }                                                                       \
                                                                            \
    static inline INT                                                       \
    decode_difference ## BITS ## w(INT a, INT *b)                           \
    {                                                                       \
        INT w = (a + *b) & BITMASK(INT, BITS);                              \
        *b = w;                                                             \
        return w;                                                           \
    }                                                                       \

IMPLEMENT_BY_TYPES(SNNIPET_DIFFERENCEW)

// delta_encode8
// delta_encode16be / delta_encode16le
// delta_encode24be / delta_encode24le
// delta_encode32be / delta_encode32le
// delta_encode48be / delta_encode48le
// delta_encode64be / delta_encode64le
// delta_decode8
// delta_decode16be / delta_decode16le
// delta_decode24be / delta_decode24le
// delta_decode32be / delta_decode32le
// delta_decode48be / delta_decode48le
// delta_decode64be / delta_decode64le
#define IMP_DELTA(MODE, BITS, ENDIAN, TYPE)                                 \
    static DELTA_WEAK_INLINE int                                            \
    delta_ ## MODE ## BITS ## ENDIAN ## _body(                              \
            void *dest, const void *src, size_t len,                        \
                                     int stripe, int round)                 \
    {                                                                       \
        TYPE state[stripe][round];                                          \
        memset(state, 0, sizeof(state));                                    \
        const char *p = (const char *)src;                                  \
        const char *pp = p + len;                                           \
        char *q = (char *)dest;                                             \
        for (;;) {                                                          \
            for (int i = 0; i < stripe; i ++) {                             \
                if (p >= pp) { return 0; }                                  \
                TYPE t = load ## BITS ## ENDIAN(p);                         \
                for (int j = 0; j < round; j ++) {                          \
                    t = MODE ## _difference ## BITS ## w(t, &state[i][j]);  \
                }                                                           \
                store ## BITS ## ENDIAN(q, t);                              \
                p += (BITS / 8);                                            \
                q += (BITS / 8);                                            \
            }                                                               \
        }                                                                   \
    }                                                                       \
                                                                            \
    static DELTA_WEAK_INLINE int                                            \
    delta_ ## MODE ## BITS ## ENDIAN(                                       \
            void *dest, const void *src, size_t len,                        \
            int stripe, int round)                                          \
    {                                                                       \
        if (len % (BITS / 8) != 0) { return -1; }                           \
                                                                            \
        switch (round) {                                                    \
        case 1:                                                             \
            switch (stripe) {                                               \
            case 1:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 1, 1); \
            case 2:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 2, 1); \
            case 3:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 3, 1); \
            case 4:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 4, 1); \
            default: break;                                                 \
            }                                                               \
        case 2:                                                             \
            switch (stripe) {                                               \
            case 1:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 1, 2); \
            case 2:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 2, 2); \
            case 3:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 3, 2); \
            case 4:  return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 4, 2); \
            default: break;                                                 \
            }                                                               \
        default: break;                                                     \
        }                                                                   \
                                                                            \
        return delta_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, stripe, round); \
    }                                                                       \

IMPLEMENT_BY_MODE(IMP_DELTA)

int
delta_encode(void *dest, const void *src, size_t len, int type, int stripe, int round)
{
    switch (type) {
    case TYPE_INT8:    return delta_encode8   (dest, src, len, stripe, round);
    case TYPE_INT16BE: return delta_encode16be(dest, src, len, stripe, round);
    case TYPE_INT16LE: return delta_encode16le(dest, src, len, stripe, round);
    case TYPE_INT24BE: return delta_encode24be(dest, src, len, stripe, round);
    case TYPE_INT24LE: return delta_encode24le(dest, src, len, stripe, round);
    case TYPE_INT32BE: return delta_encode32be(dest, src, len, stripe, round);
    case TYPE_INT32LE: return delta_encode32le(dest, src, len, stripe, round);
    case TYPE_INT48BE: return delta_encode48be(dest, src, len, stripe, round);
    case TYPE_INT48LE: return delta_encode48le(dest, src, len, stripe, round);
    case TYPE_INT64BE: return delta_encode64be(dest, src, len, stripe, round);
    case TYPE_INT64LE: return delta_encode64le(dest, src, len, stripe, round);
    default:           return -1;
    }
}

int
delta_decode(void *dest, const void *src, size_t len, int type, int stripe, int round)
{
    switch (type) {
    case TYPE_INT8:    return delta_decode8   (dest, src, len, stripe, round);
    case TYPE_INT16BE: return delta_decode16be(dest, src, len, stripe, round);
    case TYPE_INT16LE: return delta_decode16le(dest, src, len, stripe, round);
    case TYPE_INT24BE: return delta_decode24be(dest, src, len, stripe, round);
    case TYPE_INT24LE: return delta_decode24le(dest, src, len, stripe, round);
    case TYPE_INT32BE: return delta_decode32be(dest, src, len, stripe, round);
    case TYPE_INT32LE: return delta_decode32le(dest, src, len, stripe, round);
    case TYPE_INT48BE: return delta_decode48be(dest, src, len, stripe, round);
    case TYPE_INT48LE: return delta_decode48le(dest, src, len, stripe, round);
    case TYPE_INT64BE: return delta_decode64be(dest, src, len, stripe, round);
    case TYPE_INT64LE: return delta_decode64le(dest, src, len, stripe, round);
    default:           return -1;
    }
}

// predict8
// predict16
// predict24
// predict32
// predict48
// predict64
#define IMP_PREDICT(BITS, TYPE, UINT)                                       \
    static inline TYPE                                                      \
    predict ## BITS(TYPE state[], int nweight, int weight[], int gain)      \
    {                                                                       \
        TYPE n = 0;                                                         \
        TYPE g = gain;                                                      \
        TYPE *p = state;                                                    \
        for (int i = 0; i < nweight; i ++) {                                \
            TYPE *q = p + 1;                                                \
            n += (*p - *q) / g * weight[i];                                 \
            p = q;                                                          \
        }                                                                   \
        return (TYPE)(state[0] + n);                                        \
    }                                                                       \

IMPLEMENT_BY_TYPES(IMP_PREDICT)

// slide_state_8
// slide_state_16
// slide_state_24
// slide_state_32
// slide_state_48
// slide_state_64
#define IMP_SLIDESTATE(BITS, TYPE, UINT)                                    \
    static inline void                                                      \
    slide_state_ ## BITS(TYPE *state, int samples, TYPE first)              \
    {                                                                       \
        for (int i = 1; i < samples; i ++) {                                \
            state[i] = state[i - 1];                                        \
        }                                                                   \
        state[0] = first;                                                   \
    }                                                                       \


IMPLEMENT_BY_TYPES(IMP_SLIDESTATE)

// prediction_encode8
// prediction_encode16be / prediction_encode16le
// prediction_encode24be / prediction_encode24le
// prediction_encode32be / prediction_encode32le
// prediction_encode48be / prediction_encode48le
// prediction_encode64be / prediction_encode64le
// prediction_decode8
// prediction_decode16be / prediction_decode16le
// prediction_decode24be / prediction_decode24le
// prediction_decode32be / prediction_decode32le
// prediction_decode48be / prediction_decode48le
// prediction_decode64be / prediction_decode64le
#define IMP_PRED(MODE, BITS, ENDIAN, TYPE)                                  \
    static DELTA_WEAK_INLINE int                                            \
    prediction_ ## MODE ## BITS ## ENDIAN ## _body(                         \
            void *dest, const void *src, size_t len,                        \
            int stripe, int nweight,                                        \
            int weight[], int gain)                                         \
    {                                                                       \
        TYPE state[stripe][nweight + 1];                                    \
        memset(state, 0, sizeof(state));                                    \
        const char *p = (const char *)src;                                  \
        const char *pp = p + len;                                           \
        char *q = (char *)dest;                                             \
        for (;;) {                                                          \
            for (int i = 0; i < stripe; i ++) {                             \
                if (p >= pp) { return 0; }                                  \
                TYPE t = load ## BITS ## ENDIAN(p);                         \
                TYPE u = predict ## BITS(state[i], nweight, weight, gain);  \
                TYPE d = MODE ## _difference ## BITS(t, u);                 \
                slide_state_ ## BITS(state[i], nweight + 1,                 \
                                     (mode_ ## MODE == mode_encode) ? t : d); \
                                                                            \
                store ## BITS ## ENDIAN(q, d);                              \
                p += (BITS / 8);                                            \
                q += (BITS / 8);                                            \
            }                                                               \
        }                                                                   \
    }                                                                       \
                                                                            \
    static DELTA_WEAK_INLINE int                                            \
    prediction_ ## MODE ## BITS ## ENDIAN(                                  \
            void *dest, const void *src, size_t len,                        \
            int stripe, int nweight,                                        \
            int weight[], int gain)                                         \
    {                                                                       \
        if (len % (BITS / 8) != 0) { return -1; }                           \
                                                                            \
        switch (nweight) {                                                  \
        case 1:                                                             \
            switch (stripe) {                                               \
            case 1:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 1, 1, weight, gain); \
            case 2:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 2, 1, weight, gain); \
            case 3:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 3, 1, weight, gain); \
            case 4:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 4, 1, weight, gain); \
            default: break;                                                 \
            }                                                               \
        case 2:                                                             \
            switch (stripe) {                                               \
            case 1:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 1, 2, weight, gain); \
            case 2:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 2, 2, weight, gain); \
            case 3:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 3, 2, weight, gain); \
            case 4:  return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, 4, 2, weight, gain); \
            default: break;                                                 \
            }                                                               \
        default: break;                                                     \
        }                                                                   \
                                                                            \
        return prediction_ ## MODE ## BITS ## ENDIAN ## _body(dest, src, len, stripe, nweight, weight, gain); \
    }                                                                       \

IMPLEMENT_BY_MODE(IMP_PRED)

int
prediction_encode(void *dest, const void *src, size_t len, int type, int stripe, int nweight, int weight[], int gain)
{
    switch (type) {
    case TYPE_INT8:    return prediction_encode8   (dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT16BE: return prediction_encode16be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT16LE: return prediction_encode16le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT24BE: return prediction_encode24be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT24LE: return prediction_encode24le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT32BE: return prediction_encode32be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT32LE: return prediction_encode32le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT48BE: return prediction_encode48be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT48LE: return prediction_encode48le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT64BE: return prediction_encode64be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT64LE: return prediction_encode64le(dest, src, len, stripe, nweight, weight, gain);
    default:           return -1;
    }
}

int
prediction_decode(void *dest, const void *src, size_t len, int type, int stripe, int nweight, int weight[], int gain)
{
    switch (type) {
    case TYPE_INT8:    return prediction_decode8   (dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT16BE: return prediction_decode16be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT16LE: return prediction_decode16le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT24BE: return prediction_decode24be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT24LE: return prediction_decode24le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT32BE: return prediction_decode32be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT32LE: return prediction_decode32le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT48BE: return prediction_decode48be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT48LE: return prediction_decode48le(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT64BE: return prediction_decode64be(dest, src, len, stripe, nweight, weight, gain);
    case TYPE_INT64LE: return prediction_decode64le(dest, src, len, stripe, nweight, weight, gain);
    default:           return -1;
    }
}

double
delta_entoropy(const void *buf, size_t size)
{
    enum { table_length = UCHAR_MAX + 1, };

    if (size < 1) { return NAN; }

    size_t table[table_length] = { 0 };

    {
        const uint8_t *p = (const uint8_t *)buf;
        const uint8_t *pp = (const uint8_t *)p + size;
        for (; p < pp; p ++) {
            table[*p] ++;
        }
    }

    double e = 0;

    {
        const size_t *p = table;
        const size_t *pp = p + table_length;
        for (; p < pp; p ++) {
            if (*p == 0) { continue; }
            double x = (double)*p / (double)size;
            e += - x * log2(x);
        }
    }

    return e;
}
