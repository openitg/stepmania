/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.com
 */
#include "tomcrypt.h"

/**
   @file dsa_free.c
   DSA implementation, free a DSA key, Tom St Denis
*/

#ifdef MDSA

/**
   Free a DSA key
   @param key   The key to free from memory
*/
void dsa_free(dsa_key *key)
{
   LTC_ARGCHKVD(key != NULL);
   mp_clear_multi(key->g, key->q, key->p, key->x, key->y, NULL);
}

#endif

/* $Source$ */
/* $Revision: 24838 $ */
/* $Date: 2007-01-24 00:16:57 -0500 (Wed, 24 Jan 2007) $ */
