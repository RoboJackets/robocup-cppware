/* fix32.h - minimal 32-bit fixed-point library (header-only).
 * Default format Q16.16 (signed): range [-32768, 32767.99998], resolution
 * 2^-16. Override with -DFIX_FRAC_BITS=n (1..31) for other Qm.n splits. Add/mul
 * saturate on overflow; mul rounds to nearest. Assumes arithmetic right shift
 * on signed ints (true on all common compilers).
 */
#ifndef FIX32_H
#define FIX32_H

#include <stdint.h>

#ifndef FIX_FRAC_BITS
#define FIX_FRAC_BITS 16
#endif

typedef int32_t fix32_t;

#define FIX_ONE ((int64_t)1 << FIX_FRAC_BITS)

static inline fix32_t fix_sat(int64_t x) {
  if (x > INT32_MAX)
    return INT32_MAX;
  if (x < INT32_MIN)
    return INT32_MIN;
  return (fix32_t)x;
}

/* Conversions */
static inline fix32_t fix_from_int(int32_t x) {
  return fix_sat((int64_t)x * FIX_ONE);
}

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

static inline int32_t fix_to_int(fix32_t x) /* floors toward -inf */
{
  return x >> FIX_FRAC_BITS;
}

/* Arithmetic */
static inline fix32_t fix_add(fix32_t a, fix32_t b) {
  return fix_sat((int64_t)a + b);
}

static inline fix32_t fix_sub(fix32_t a, fix32_t b) {
  return fix_sat((int64_t)a - b);
}

static inline fix32_t fix_mul(fix32_t a, fix32_t b) {
  int64_t p = (int64_t)a * b;             /* Q(2*FRAC) in 64 bits */
  p += (int64_t)1 << (FIX_FRAC_BITS - 1); /* round to nearest */
  return fix_sat(p >> FIX_FRAC_BITS);
}

/* ---- Compile-time versions ----------------------------------------------
 * Macro equivalents usable in static/global initializers and _Static_assert
 * (integer ones are true constant expressions). Arguments are evaluated
 * multiple times, so pass constants only; use the inline functions at runtime.
 * Nesting deep expressions grows the expansion quickly (SAT repeats its arg).
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

#endif /* FIX32_H */