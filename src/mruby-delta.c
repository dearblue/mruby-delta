#include "delta.h"
#include "mruby-delta.h"
#include <mruby-aux/scanhash.h>

#include <string.h>
/* strings.h で定義されたり string.h で定義されたりバラバラなので、苦肉の策 */
int strcasecmp(const char *, const char *);

/*
 * 整数値やシンボル値を、型を表す列挙値に変換する。
 */
static int
type2enum(MRB, VALUE v)
{
    if (mrb_nil_p(v) || mrb_undef_p(v)) {
        return TYPE_INT8;
    } else if (mrb_string_p(v) || mrb_symbol_p(v)) {
        const char *type = mrbx_get_const_cstr(mrb, v);
        if (strcasecmp(type, "int8") == 0) {
            return TYPE_INT8;
        } else if (strcasecmp(type, "int16be") == 0) {
            return TYPE_INT16BE;
        } else if (strcasecmp(type, "int16le") == 0) {
            return TYPE_INT16LE;
        } else if (strcasecmp(type, "int24be") == 0) {
            return TYPE_INT24BE;
        } else if (strcasecmp(type, "int24le") == 0) {
            return TYPE_INT24LE;
        } else if (strcasecmp(type, "int32be") == 0) {
            return TYPE_INT32BE;
        } else if (strcasecmp(type, "int32le") == 0) {
            return TYPE_INT32LE;
        } else if (strcasecmp(type, "int48be") == 0) {
            return TYPE_INT48BE;
        } else if (strcasecmp(type, "int48le") == 0) {
            return TYPE_INT48LE;
        } else if (strcasecmp(type, "int64be") == 0) {
            return TYPE_INT64BE;
        } else if (strcasecmp(type, "int64le") == 0) {
            return TYPE_INT64LE;
        } else {
            mrb_raise(mrb, E_ARGUMENT_ERROR, "wrong type name");
        }
    } else {
        int n = mrb_int(mrb, v);
        if (n >= TYPE_MIN && n <= TYPE_MAX) {
            return n;
        }
        mrb_raise(mrb, E_ARGUMENT_ERROR, "wrong type number");
    }
}

static int
nil_or_undef_p(VALUE v)
{
    return (mrb_nil_p(v) || mrb_undef_p(v));
}

static void
delta_scan_args(MRB, int destructive, VALUE *src, VALUE *dest, int *type, int *stripe,
                int *round, int weight[MAX_WEIGHTS], int *nweight, int *gain)
{
    VALUE opts = Qnil;
    if (destructive) {
        mrb_get_args(mrb, "S|H", src, &opts);
        *dest = *src;
    } else {
        *dest = Qnil;
        switch (mrb_get_args(mrb, "S|oH", src, dest, &opts)) {
        case 1:
            *dest = Qnil;
            break;
        case 2:
            if (mrb_hash_p(*dest)) {
                opts = *dest;
                *dest = Qnil;
            } else {
                mrb_check_type(mrb, *dest, MRB_TT_STRING);
            }
            break;
        case 3:
            mrb_check_type(mrb, *dest, MRB_TT_STRING);
            break;
        default:
            mrb_bug(mrb, "%S:%S:%S", VALUE(__FILE__), VALUE((mrb_int)__LINE__), VALUE(__func__));
        }

        if (mrb_nil_p(*dest)) { *dest = mrb_str_new(mrb, NULL, RSTRING_LEN(*src)); }
    }

    if (mrb_nil_p(opts)) {
        *type = TYPE_INT8;
        *stripe = LEAST_STRIPE;
        *round = LEAST_ROUND;
        *nweight = 0;
        *gain = 0;
    } else {
        VALUE values[6];

        MRBX_SCANHASH(mrb, opts, Qnil,
                MRBX_SCANHASH_ARG("type", &values[0], Qnil),
                MRBX_SCANHASH_ARG("stripe", &values[1], Qnil),
                MRBX_SCANHASH_ARG("round", &values[2], Qnil),
                MRBX_SCANHASH_ARG("prediction", &values[3], Qnil),
                MRBX_SCANHASH_ARG("weight", &values[4], Qnil),
                MRBX_SCANHASH_ARG("gain", &values[5], Qnil));

        *type = type2enum(mrb, values[0]);

        *stripe = nil_or_undef_p(values[1]) ? 1 : mrb_int(mrb, values[1]);
        if (*stripe < LEAST_STRIPE || *stripe > MOST_STRIPE) {
            mrb_raisef(mrb, E_ARGUMENT_ERROR,
                       "wrong stripe (not %S..%S)",
                       mrb_fixnum_value(LEAST_STRIPE),
                       mrb_fixnum_value(MOST_STRIPE));
        }

        int check = (RTEST(values[2]) ? 0x01 : 0) |
                    (RTEST(values[3]) ? 0x02 : 0) |
                    (!mrb_nil_p(values[4]) || !mrb_nil_p(values[5]) ? 0x04 : 0);

        switch (check) {
        case 0x00:
            *round = LEAST_ROUND;
            *nweight = 0;

            return;
        case 0x01:
            *round = mrb_int(mrb, values[2]);
            if (*round < LEAST_ROUND || *round > MOST_ROUND) {
                mrb_raisef(mrb, E_ARGUMENT_ERROR,
                           "wrong round (not %S..%S)",
                           mrb_fixnum_value(LEAST_ROUND),
                           mrb_fixnum_value(MOST_ROUND));
            }

            return;
        case 0x02:
            *round = 0;
            *nweight = 2;
            weight[0] = weight[1] = 1;
            *gain = 2;

            return;
        case 0x04:
            *round = 0;

            if (mrb_nil_p(values[4])) {
                *nweight = 1;
                weight[0] = 1;
                *gain = 1;
            } else {
                mrb_check_type(mrb, values[4], MRB_TT_ARRAY);
                *nweight = RARRAY_LEN(values[4]);
                if (*nweight < MIN_WEIGHTS || *nweight > MAX_WEIGHTS) {
                    mrb_raisef(mrb, E_ARGUMENT_ERROR,
                               "``weight'' size is too small or too large (size is must %S..%S)",
                               mrb_fixnum_value(MIN_WEIGHTS),
                               mrb_fixnum_value(MAX_WEIGHTS));
                }
                int i;
                const VALUE *p = RARRAY_PTR(values[4]);
                *gain = 0;
                for (i = 0; i < *nweight; i ++) {
                    weight[i] = mrb_int(mrb, p[i]);
                    if (weight[i] < MIN_WEIGHT || weight[i] > MAX_WEIGHT) {
                        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                                   "``weight'' element is too small or too large (size is must %S..%S)",
                                   mrb_fixnum_value(MIN_WEIGHT),
                                   mrb_fixnum_value(MAX_WEIGHT));
                    }
                    *gain += weight[i];
                }
            }

            if (!mrb_nil_p(values[5])) {
                *gain = mrb_int(mrb, values[5]);
            }

            if (*gain == 0) {
                mrb_raise(mrb, E_ARGUMENT_ERROR,
                          "gain must not zero (given zero for ``gain'', or total sum is zero for ``weight'')");
            }

            return;
        }

        mrb_raise(mrb, E_ARGUMENT_ERROR, "given more than one ``round'', ``prediction'' or ``weight/gain``");
    }
}

static inline VALUE
delta_encode_common(MRB, int destructive)
{
    VALUE src, dest;
    int type, stripe, round, nweight, gain;
    int weight[MAX_WEIGHTS];
    delta_scan_args(mrb, destructive, &src, &dest, &type, &stripe,
                    &round, weight, &nweight, &gain);

    if (mrb_nil_p(dest)) { dest = src; }

    mrb_str_resize(mrb, dest, RSTRING_LEN(src));

    int err;

    if (round != 0) {
        err = delta_encode(RSTRING_PTR(dest), RSTRING_PTR(src), RSTRING_LEN(src), type, stripe, round);
    } else {
        err = prediction_encode(RSTRING_PTR(dest), RSTRING_PTR(src), RSTRING_LEN(src), type, stripe, nweight, weight, gain);
    }

    if (err) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
                   "wrong data length - divisible by zero (data length is %S)",
                   mrb_fixnum_value(RSTRING_LEN(src)));
    }

    return dest;
}


/*
 * call-seq:
 *  Delta.encode(string, type: Delta::INT8, stripe: 1) -> new string
 *  Delta.encode(string, type: Delta::INT8, stripe: 1, round: 1) -> new string
 *  Delta.encode(string, type: Delta::INT8, stripe: 1, prediction: true) -> new string
 *  Delta.encode(string, type: Delta::INT8, stripe: 1, weight: [1, 1], gain: nil) -> new string
 *
 * string 引数で渡されるバイナリデータから整数値としての差分値を求める。
 *
 * round、prediction、weight/gain は同時に与えることは出来ない。
 *
 * round、prediction、weight/gain のいずれも与えられなかった場合、<code>round: 1</code>が与えられたものとみなされる。
 *
 * [RETURN]
 *  差分処理を行ったStringインスタンス。
 *
 * [type]
 *  要素の型を表す正の整数値。Delta::Constants::INT*を使うと記述性が向上する。
 *
 * [stripe]
 *  数値列の数。数値列が二つ飛ばし、三つ飛ばしなどの場合に与える。
 *
 *  データが PCM ステレオの場合は2、RGB 画像であれば3、RGBA 画像であれば4を指定すると良好な結果を得られる。
 *
 * [round]
 *  折り返し数。
 *
 * [prediction]
 *  線形予測差分値を計算する場合に true を与える。
 *
 *  <code>weight: [1, 1], gain: 2</code> を与えたものと等価である。
 *
 * [weight]
 *  線形予測差分値を計算する場合に、2から8個の整数値要素を持つ配列を与える。
 *
 *  線形予測に使われる前方向要素数は、weightで与えた整数値要素数と等価であり、それぞれに対して重み付けが行われる。
 *
 * [gain]
 *  線形予測差分値を計算する場合に整数値を与える。
 *
 *  正負は問わないが、0を与えることは出来ない (冷害が発生する)。
 *
 *  指定しないか nil を与えた場合、weight の総和が用いられる。この場合の総和が0になった場合は例外が発生する。
 *
 *
 * - 256色BMP画像の場合: type = Delta::INT8, stripe = 1, round = 1
 * - 24ビットBMP画像の場合: type = Delta::INT8, stripe = 3, round = 1
 * - 32ビットBMP画像の場合: type = Delta::INT8, stripe = 4, round = 1
 * - RGBA各16ビット無圧縮PNG画像の場合: type = Delta::INT16BE, stripe = 4, round = 1
 * - 8ビットモノラルwavの場合: type = Delta::INT8, stripe = 1, prediction = true
 * - 8ビットステレオwavの場合: type = Delta::INT8, stripe = 2, prediction = true
 * - 16ビットステレオwavの場合: type = Delta::INT16LE, stripe = 2, prediction = true
 * - 24ビットステレオwavの場合: type = Delta::INT24LE, stripe = 2, prediction = true
 *
 * 一般的に round の値は、画像は『1』が、音声などのなだらかな曲線を描く波形などは『2』が好ましい。
 */
static VALUE
delta_encode_m(MRB, VALUE mod)
{
    return delta_encode_common(mrb, FALSE);
}

/*
 * Delta.encode と同じ処理を行いますが、引数で与えた string を上書きして結果を返します。
 */
static VALUE
delta_encode_ban(MRB, VALUE mod)
{
    return delta_encode_common(mrb, TRUE);
}



static inline VALUE
delta_decode_common(MRB, int destructive)
{
    VALUE src, dest;
    int type, stripe, round, nweight, gain;
    int weight[MAX_WEIGHTS];
    delta_scan_args(mrb, destructive, &src, &dest, &type, &stripe,
                    &round, weight, &nweight, &gain);

    if (mrb_nil_p(dest)) { dest = src; }

    mrb_str_resize(mrb, dest, RSTRING_LEN(src));

    int err;

    if (round != 0) {
        err = delta_decode(RSTRING_PTR(dest), RSTRING_PTR(src), RSTRING_LEN(src), type, stripe, round);
    } else {
        err = prediction_decode(RSTRING_PTR(dest), RSTRING_PTR(src), RSTRING_LEN(src), type, stripe, nweight, weight, gain);
    }

    if (err) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
                   "wrong data length - divisible by zero (data length is %S)",
                   mrb_fixnum_value(RSTRING_LEN(src)));
    }

    return dest;
}


static VALUE
delta_decode_m(MRB, VALUE mod)
{
    return delta_decode_common(mrb, FALSE);
}

static VALUE
delta_decode_ban(MRB, VALUE mod)
{
    return delta_decode_common(mrb, TRUE);
}

/*
 * call-seq:
 *  entoropy(string) -> entoropy
 *
 * 入力文字列をバイト列と見立てた場合の情報エントロピーを計算します。
 */
static VALUE
delta_s_entoropy(MRB, VALUE self)
{
    VALUE src;
    mrb_get_args(mrb, "S", &src);

    return mrb_float_value(mrb, delta_entoropy(RSTRING_PTR(src), RSTRING_LEN(src)));
}

void
mrb_mruby_delta_gem_init(MRB)
{
    struct RClass *delta_module = mrb_define_module(mrb, "Delta");
    mrb_define_class_method(mrb, delta_module, "encode", delta_encode_m, MRB_ARGS_ANY());
    mrb_define_class_method(mrb, delta_module, "decode", delta_decode_m, MRB_ARGS_ANY());
    mrb_define_class_method(mrb, delta_module, "encode!", delta_encode_ban, MRB_ARGS_ANY());
    mrb_define_class_method(mrb, delta_module, "decode!", delta_decode_ban, MRB_ARGS_ANY());
    mrb_define_const(mrb, delta_module, "INT8",         mrb_fixnum_value(TYPE_INT8));
    mrb_define_const(mrb, delta_module, "INT16LE",      mrb_fixnum_value(TYPE_INT16LE));
    mrb_define_const(mrb, delta_module, "INT16BE",      mrb_fixnum_value(TYPE_INT16BE));
    mrb_define_const(mrb, delta_module, "INT24LE",      mrb_fixnum_value(TYPE_INT24LE));
    mrb_define_const(mrb, delta_module, "INT24BE",      mrb_fixnum_value(TYPE_INT24BE));
    mrb_define_const(mrb, delta_module, "INT32LE",      mrb_fixnum_value(TYPE_INT32LE));
    mrb_define_const(mrb, delta_module, "INT32BE",      mrb_fixnum_value(TYPE_INT32BE));
    mrb_define_const(mrb, delta_module, "INT48LE",      mrb_fixnum_value(TYPE_INT48LE));
    mrb_define_const(mrb, delta_module, "INT48BE",      mrb_fixnum_value(TYPE_INT48BE));
    mrb_define_const(mrb, delta_module, "INT64LE",      mrb_fixnum_value(TYPE_INT64LE));
    mrb_define_const(mrb, delta_module, "INT64BE",      mrb_fixnum_value(TYPE_INT64BE));

    struct RClass *const_module = mrb_define_module_under(mrb, delta_module, "Constants");
    mrb_include_module(mrb, delta_module, const_module);
    mrb_define_const(mrb, const_module, "TYPE_INT8",    mrb_fixnum_value(TYPE_INT8));
    mrb_define_const(mrb, const_module, "TYPE_INT16LE", mrb_fixnum_value(TYPE_INT16LE));
    mrb_define_const(mrb, const_module, "TYPE_INT16BE", mrb_fixnum_value(TYPE_INT16BE));
    mrb_define_const(mrb, const_module, "TYPE_INT24LE", mrb_fixnum_value(TYPE_INT24LE));
    mrb_define_const(mrb, const_module, "TYPE_INT24BE", mrb_fixnum_value(TYPE_INT24BE));
    mrb_define_const(mrb, const_module, "TYPE_INT32LE", mrb_fixnum_value(TYPE_INT32LE));
    mrb_define_const(mrb, const_module, "TYPE_INT32BE", mrb_fixnum_value(TYPE_INT32BE));
    mrb_define_const(mrb, const_module, "TYPE_INT48LE", mrb_fixnum_value(TYPE_INT48LE));
    mrb_define_const(mrb, const_module, "TYPE_INT48BE", mrb_fixnum_value(TYPE_INT48BE));
    mrb_define_const(mrb, const_module, "TYPE_INT64LE", mrb_fixnum_value(TYPE_INT64LE));
    mrb_define_const(mrb, const_module, "TYPE_INT64BE", mrb_fixnum_value(TYPE_INT64BE));

    mrb_define_class_method(mrb, delta_module, "entoropy", delta_s_entoropy, MRB_ARGS_REQ(1));
}

void
mrb_mruby_delta_gem_final(MRB)
{
}
