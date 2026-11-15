/* fix32.h - 32-bit fixed-point library, tuned for Cortex-M0 (ARMv6-M).
 * Default format Q16.16 (signed). Override with -DFIX_FRAC_BITS=n (1..31).
 *
 * M0 notes: no 64-bit multiply (MULS is 32x32->32), no saturating or
 * conditional-execution instructions, soft-float only. So the runtime
 * functions below use no int64_t and no floating point (except the explicit
 * float conversions), avoiding libgcc calls like __aeabi_lmul.
 *
 * Add/sub/mul saturate. Mul rounds half away from zero.
 * Assumes arithmetic right shift on signed ints (true on GCC/Clang/ARM).
 */
#ifndef FIX32_H
#define FIX32_H

#include <stdint.h>

#ifndef FIX_FRAC_BITS
#define FIX_FRAC_BITS 16
#endif
#if FIX_FRAC_BITS < 1 || FIX_FRAC_BITS > 31
#error "FIX_FRAC_BITS must be in 1..31"
#endif

typedef int32_t fix32_t;

#define FIX_ONE ((int64_t)1 << FIX_FRAC_BITS) /* compile-time use only */

/* ---- Runtime (no 64-bit math) ------------------------------------------- */

static inline fix32_t fix_from_int(int32_t x) {
  if (x > (INT32_MAX >> FIX_FRAC_BITS))
    return INT32_MAX;
  if (x < (INT32_MIN >> FIX_FRAC_BITS))
    return INT32_MIN;
  return (fix32_t)((uint32_t)x << FIX_FRAC_BITS);
}

static inline int32_t fix_to_int(fix32_t x) /* floors toward -inf */
{
  return x >> FIX_FRAC_BITS;
}

/* Soft-float on M0 (slow, double especially): keep out of hot paths and
 * use FIX_FROM_FLOAT for constants. */
static inline fix32_t fix_from_float(double x) {
  double scaled = x * (double)FIX_ONE;
  if (scaled >= 2147483647.0)
    return INT32_MAX;
  if (scaled <= -2147483648.0)
    return INT32_MIN;
  return (fix32_t)(scaled + (scaled >= 0 ? 0.5 : -0.5));
}

static inline double fix_to_float(fix32_t x) {
  return (double)x / (double)FIX_ONE;
}

/* Saturating add via wraparound + sign-overflow check. */
static inline fix32_t fix_add(fix32_t a, fix32_t b) {
  fix32_t s = (fix32_t)((uint32_t)a + (uint32_t)b);
  if (((a ^ s) & (b ^ s)) < 0)
    s = (fix32_t)((uint32_t)INT32_MAX + ((uint32_t)a >> 31));
  return s;
}

static inline fix32_t fix_sub(fix32_t a, fix32_t b) {
  fix32_t d = (fix32_t)((uint32_t)a - (uint32_t)b);
  if (((a ^ b) & (a ^ d)) < 0)
    d = (fix32_t)((uint32_t)INT32_MAX + ((uint32_t)a >> 31));
  return d;
}

/* 32x32->64 unsigned multiply from four 16x16->32 products (4x MULS). */
static inline void fix_umul64(uint32_t a, uint32_t b, uint32_t *hi,
                              uint32_t *lo) {
  uint32_t al = a & 0xFFFFu, ah = a >> 16;
  uint32_t bl = b & 0xFFFFu, bh = b >> 16;
  uint32_t ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
  uint32_t mid = (ll >> 16) + (lh & 0xFFFFu) + (hl & 0xFFFFu);
  *lo = (ll & 0xFFFFu) | (mid << 16);
  *hi = hh + (lh >> 16) + (hl >> 16) + (mid >> 16);
}

static inline fix32_t fix_mul(fix32_t a, fix32_t b) {
  uint32_t neg = (uint32_t)(a ^ b) >> 31; /* signs differ */
  uint32_t ua = a < 0 ? 0u - (uint32_t)a : (uint32_t)a;
  uint32_t ub = b < 0 ? 0u - (uint32_t)b : (uint32_t)b;
  uint32_t hi, lo;
  fix_umul64(ua, ub, &hi, &lo);

  lo += 1u << (FIX_FRAC_BITS - 1);          /* round */
  hi += (lo < (1u << (FIX_FRAC_BITS - 1))); /* carry */

  uint32_t r = (lo >> FIX_FRAC_BITS) | (hi << (32 - FIX_FRAC_BITS));
  uint32_t rh = hi >> FIX_FRAC_BITS;

  if (rh || r > (uint32_t)INT32_MAX + neg) /* saturate */
    return neg ? INT32_MIN : INT32_MAX;
  return (fix32_t)(neg ? 0u - r : r);
}

/* Clamp x to [lo, hi]; if lo > hi, hi wins. */
static inline fix32_t fix_clamp(fix32_t x, fix32_t lo, fix32_t hi) {
  if (x < lo)
    x = lo;
  if (x > hi)
    x = hi;
  return x;
}

/* Absolute value, saturating: abs(INT32_MIN) returns INT32_MAX. */
static inline fix32_t fix_abs(fix32_t x) {
  if (x >= 0)
    return x;
  if (x == INT32_MIN)
    return INT32_MAX;
  return -x;
}

/* Reciprocal of an integer x in 1..32000 (any x >= 1 works), returned in the
 * library's format and rounded to nearest. One 32/32 divide, no 64-bit math.
 * x <= 0 is out of domain and returns INT32_MAX. x = 1 saturates only if
 * FIX_FRAC_BITS == 31. Mind the precision: 1/x is only 2^F / x LSBs
 * (e.g. 1/32000 is ~2 LSBs at F=16), so use a large FIX_FRAC_BITS. */
static inline fix32_t fix_recip_int(int32_t x) {
  uint32_t q;
  if (x <= 0)
    return INT32_MAX;
  q = ((1u << FIX_FRAC_BITS) + ((uint32_t)x >> 1)) / (uint32_t)x;
  return q > (uint32_t)INT32_MAX ? (fix32_t)INT32_MAX : (fix32_t)q;
}

/* ---- Compile-time versions ----------------------------------------------
 * Macro equivalents usable in static/global initializers and _Static_assert
 * (integer ones are true constant expressions). Folded by the compiler, so
 * the int64/double use here costs nothing at runtime. Arguments are
 * evaluated multiple times, so pass constants only.
 */
#define FIX_SAT(x)                                                             \
  ((x) > INT32_MAX   ? (fix32_t)INT32_MAX                                      \
   : (x) < INT32_MIN ? (fix32_t)INT32_MIN                                      \
                     : (fix32_t)(x))

#define FIX_FROM_INT(x) FIX_SAT((int64_t)(x) * FIX_ONE)

#define FIX_FROM_FLOAT(x)                                                      \
  ((double)(x) * (double)FIX_ONE >= 2147483647.0 ? (fix32_t)INT32_MAX          \
   : (double)(x) * (double)FIX_ONE <= -2147483648.0                            \
       ? (fix32_t)INT32_MIN                                                    \
       : (fix32_t)((double)(x) * (double)FIX_ONE + ((x) >= 0 ? 0.5 : -0.5)))

#define FIX_ADD(a, b) FIX_SAT((int64_t)(a) + (int64_t)(b))
#define FIX_SUB(a, b) FIX_SAT((int64_t)(a) - (int64_t)(b))
#define FIX_MUL(a, b)                                                          \
  FIX_SAT(                                                                     \
      (((int64_t)(a) * (int64_t)(b)) + ((int64_t)1 << (FIX_FRAC_BITS - 1))) >> \
      FIX_FRAC_BITS)
#define FIX_TO_INT(x) ((int32_t)((x) >> FIX_FRAC_BITS))
#define FIX_RECIP_INT(x) FIX_SAT((FIX_ONE + (int64_t)(x) / 2) / (int64_t)(x))

/* Same semantics as fix_clamp: if lo > hi, hi wins. */
#define FIX_CLAMP(x, lo, hi)                                                   \
  ((fix32_t)(((x) < (lo) ? (lo) : (x)) > (hi) ? (hi)                           \
                                              : ((x) < (lo) ? (lo) : (x))))

/* Saturating absolute value, as fix_abs. */
#define FIX_ABS(x)                                                             \
  ((x) >= 0           ? (fix32_t)(x)                                           \
   : (x) == INT32_MIN ? (fix32_t)INT32_MAX                                     \
                      : (fix32_t) - (x))

#endif /* FIX32_H */