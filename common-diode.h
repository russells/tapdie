/**
 * @file Include this file if you need the definition of COMMON_ANODE or
 * COMMON_CATHODE.  It checks at compile time for one of those macros being
 * defined, but doesn't have any run time effect.
 */

#ifdef COMMON_ANODE
/* Nothing */
#else
#ifdef COMMON_CATHODE
/* Nothing */
#else
#error Must define either COMMON_ANODE or COMMON_CATHODE.
#endif
#endif
