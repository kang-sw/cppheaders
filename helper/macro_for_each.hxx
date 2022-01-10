#pragma once
#define INTERNAL_CPPH_EXPAND(x)                x
#define INTERNAL_CPPH_FOR_EACH_1(what, x, ...) what(x)
#define INTERNAL_CPPH_FOR_EACH_2(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_1(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_3(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_2(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_4(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_3(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_5(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_4(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_6(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_5(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_7(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_6(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_8(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_7(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_9(what, x, ...) \
    what(x);                                   \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_8(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_10(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_9(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_11(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_10(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_12(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_11(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_13(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_12(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_14(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_13(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_15(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_14(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_16(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_15(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_17(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_16(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_18(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_17(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_19(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_18(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_20(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_19(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_21(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_20(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_22(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_21(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_23(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_22(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_24(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_23(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_25(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_24(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_26(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_25(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_27(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_26(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_28(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_27(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_29(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_28(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_30(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_29(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_31(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_30(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_32(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_31(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_33(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_32(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_34(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_33(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_35(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_34(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_36(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_35(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_37(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_36(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_38(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_37(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_39(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_38(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_40(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_39(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_41(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_40(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_42(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_41(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_43(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_42(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_44(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_43(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_45(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_44(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_46(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_45(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_47(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_46(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_48(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_47(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_49(what, x, ...) \
    what(x);                                    \
    INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_48(what, __VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_NARG(...)                                                                                                                                                                                                                                                 INTERNAL_CPPH_FOR_EACH_NARG_(__VA_ARGS__, INTERNAL_CPPH_FOR_EACH_RSEQ_N())
#define INTERNAL_CPPH_FOR_EACH_NARG_(...)                                                                                                                                                                                                                                                INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_FOR_EACH_ARG_N(__VA_ARGS__))
#define INTERNAL_CPPH_FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, N, ...) N
#define INTERNAL_CPPH_FOR_EACH_RSEQ_N()                                                                                                                                                                                                                                                  49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define INTERNAL_CPPH_CONCATENATE(x, y)                                                                                                                                                                                                                                                  x##y
#define INTERNAL_CPPH_FOR_EACH_(N, what, ...)                                                                                                                                                                                                                                            INTERNAL_CPPH_EXPAND(INTERNAL_CPPH_CONCATENATE(INTERNAL_CPPH_FOR_EACH_, N)(what, __VA_ARGS__))
#define CPPH_FOR_EACH(what, ...)                                                                                                                                                                                                                                                         INTERNAL_CPPH_FOR_EACH_(INTERNAL_CPPH_FOR_EACH_NARG(__VA_ARGS__), what, __VA_ARGS__)
