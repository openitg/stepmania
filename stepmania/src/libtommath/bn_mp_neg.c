#include <tommath.h>
#ifdef BN_MP_NEG_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis
 *
 * LibTomMath is a library that provides multiple-precision
 * integer arithmetic as well as number theoretic functionality.
 *
 * The library was designed directly after the MPI library by
 * Michael Fromberger but has been written from scratch with
 * additional optimizations in place.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://math.libtomcrypt.com
 */

/* b = -a */
int mp_neg (mp_int * a, mp_int * b)
{
  int     res;
  if (a != b) {
     if ((res = mp_copy (a, b)) != MP_OKAY) {
        return res;
     }
  }

  if (mp_iszero(b) != MP_YES) {
     b->sign = (a->sign == MP_ZPOS) ? MP_NEG : MP_ZPOS;
  } else {
     b->sign = MP_ZPOS;
  }

  return MP_OKAY;
}
#endif

/* $Source$ */
/* $Revision: 24838 $ */
/* $Date: 2007-01-24 00:16:57 -0500 (Wed, 24 Jan 2007) $ */
