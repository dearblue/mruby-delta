#ifndef DELTA_H_31415926535
#define DELTA_H_31415926535 1

#include <stddef.h>
#include <stdint.h>

enum {
    TYPE_INT8    =  1,
    TYPE_INT16BE =  2,
    TYPE_INT16LE =  3,
    TYPE_INT24BE =  4,
    TYPE_INT24LE =  5,
    TYPE_INT32BE =  6,
    TYPE_INT32LE =  7,
    TYPE_INT48BE =  8,
    TYPE_INT48LE =  9,
    TYPE_INT64BE = 10,
    TYPE_INT64LE = 11,

    TYPE_MIN = TYPE_INT8,
    TYPE_MAX = TYPE_INT64LE,

    MIN_WEIGHTS = 1,
    MAX_WEIGHTS = 16,

    MAX_WEIGHT = 255,
    MIN_WEIGHT = -MAX_WEIGHT,

    MAX_GAIN = (1 << 24) - 1,
    MIN_GAIN = -MAX_GAIN,

    LEAST_STRIPE = 1,
    MOST_STRIPE = 16,

    LEAST_ROUND = 1,
    MOST_ROUND = 16,
};

int delta_encode(void *dest, const void *src, size_t len, int type, int stripe, int round);
int delta_decode(void *dest, const void *src, size_t len, int type, int stripe, int round);
int prediction_encode(void *dest, const void *src, size_t len, int type, int stripe, int nweight, int weight[], int gain);
int prediction_decode(void *dest, const void *src, size_t len, int type, int stripe, int nweight, int weight[], int gain);
double delta_entoropy(const void *buf, size_t size);

#endif /* DELTA_H_31415926535 */
