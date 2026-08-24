/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2022 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#undef ADDTYPE
#define ADDTYPE(a) CONCAT2X(a, CPL_TYPE_NAME)

#ifdef CPL_TYPE_IS_NUM

static int ADDTYPE(compar_ascn)(const void *, const void *) CPL_ATTR_NONNULL;

static int ADDTYPE(compar_desc)(const void *, const void *) CPL_ATTR_NONNULL;

inline static void ADDTYPE(cpl_tools_get_median_3)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_4)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_5)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_6)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_7_sorted)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_7)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_9_sorted)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_8)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_9)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_10)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_11)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_12)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_13)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_14)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_15)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_16)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_17)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_18)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_19)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_20)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_21)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_22)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_23)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_24)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_25)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void ADDTYPE(cpl_tools_get_median_26)(CPL_TYPE *)
    CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void
    ADDTYPE(cpl_tools_get_0th)(CPL_TYPE *,
                               size_t) CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;

inline static void
    ADDTYPE(cpl_tools_get_nth)(CPL_TYPE *,
                               size_t) CPL_ATTR_NONNULL CPL_ATTR_ALWAYSINLINE;


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Swap two entries in the array self
  @param  a   The first  index of element to be swapped
  @param  b   The second index of element to be swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP(a, b)             \
    do {                                     \
        register const CPL_TYPE t = self[a]; \
        self[a] = self[b];                   \
        self[b] = t;                         \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Cycle three entries in the array self: a,b,c -> c,a,b
  @param  a   The first  index of element to be cycled
  @param  b   The second index of element to be cycled
  @param  c   The third  index of element to be cycled
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_CYCLE(a, b, c)         \
    do {                                     \
        register const CPL_TYPE t = self[a]; \
        self[a] = self[c];                   \
        self[c] = self[b];                   \
        self[b] = t;                         \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Sort two entries in the array self
  @param  a   The first  index of element to be sorted
  @param  b   The second index of element to be sorted
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT(a, b)      \
    do {                              \
        if (self[a] > self[b])        \
            CPL_TYPE_SELF_SWAP(a, b); \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Swap two pairs of entries in the array self iff a1 > b1
  @param  a1   The first index of element to be swapped - along with a2
  @param  a2   The other index of element to be swapped - along with a1
  @param  b1   The second index of element to be swapped - along with b2
  @param  b2   The second index of element to be swapped - along with b1
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_PAIR(a1, a2, b1, b2)    \
    do {                                           \
        if (self[a1] > self[b1]) {                 \
            register const CPL_TYPE t1 = self[a1]; \
            register const CPL_TYPE t2 = self[a2]; \
            self[a1] = self[b1];                   \
            self[a2] = self[b2];                   \
            self[b1] = t1;                         \
            self[b2] = t2;                         \
        }                                          \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among three swap index in self w. minimum into first place
  @param  a   The first  index of element to be minimum swapped
  @param  b   The second index of element to be minimum swapped
  @param  c   The third  index of element to be minimum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MIN_3(a, b, c) \
    do {                                  \
        if (self[b] < self[c]) {          \
            CPL_TYPE_SELF_SORT(a, b);     \
        }                                 \
        else {                            \
            CPL_TYPE_SELF_SORT(a, c);     \
        }                                 \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among three swap index in self w. maximum into last place
  @param  a   The first  index of element to be maximum swapped
  @param  b   The second index of element to be maximum swapped
  @param  c   The third  index of element to be maximum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

*/
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MAX_3(a, b, c) \
    do {                                  \
        if (self[b] < self[a]) {          \
            CPL_TYPE_SELF_SORT(a, c);     \
        }                                 \
        else {                            \
            CPL_TYPE_SELF_SORT(b, c);     \
        }                                 \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among four swap index in self w. minimum into first place
  @param  a   The first  index of element to be minimum swapped
  @param  b   The second index of element to be minimum swapped
  @param  c   The third  index of element to be minimum swapped
  @param  d   The fourth index of element to be minimum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MIN_4(a, b, c, d)   \
    do {                                       \
        if (self[b] < self[c]) {               \
            CPL_TYPE_SELF_SWAP_MIN_3(a, b, d); \
        }                                      \
        else {                                 \
            CPL_TYPE_SELF_SWAP_MIN_3(a, c, d); \
        }                                      \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among four swap index in self w. maximum into last place
  @param  a   The first  index of element to be maximum swapped
  @param  b   The second index of element to be maximum swapped
  @param  c   The third  index of element to be maximum swapped
  @param  d   The fourth index of element to be maximum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MAX_4(a, b, c, d)   \
    do {                                       \
        if (self[b] < self[a]) {               \
            CPL_TYPE_SELF_SWAP_MAX_3(a, c, d); \
        }                                      \
        else {                                 \
            CPL_TYPE_SELF_SWAP_MAX_3(b, c, d); \
        }                                      \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among five swap index in self w. minimum into first place
  @param  a   The first  index of element to be minimum swapped
  @param  b   The second index of element to be minimum swapped
  @param  c   The third  index of element to be minimum swapped
  @param  d   The fourth index of element to be minimum swapped
  @param  e   The fifth  index of element to be minimum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MIN_5(a, b, c, d, e)   \
    do {                                          \
        if (self[b] < self[c]) {                  \
            CPL_TYPE_SELF_SWAP_MIN_4(a, b, d, e); \
        }                                         \
        else {                                    \
            CPL_TYPE_SELF_SWAP_MIN_4(a, c, d, e); \
        }                                         \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among five swap index in self w. maximum into last place
  @param  a   The first  index of element to be maximum swapped
  @param  b   The second index of element to be maximum swapped
  @param  c   The third  index of element to be maximum swapped
  @param  d   The fourth index of element to be maximum swapped
  @param  e   The fifth  index of element to be maximum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MAX_5(a, b, c, d, e)   \
    do {                                          \
        if (self[b] < self[a]) {                  \
            CPL_TYPE_SELF_SWAP_MAX_4(a, c, d, e); \
        }                                         \
        else {                                    \
            CPL_TYPE_SELF_SWAP_MAX_4(b, c, d, e); \
        }                                         \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among six swap index in self w. minimum into first place
  @param  a   The first  index of element to be minimum swapped
  @param  b   The second index of element to be minimum swapped
  @param  c   The third  index of element to be minimum swapped
  @param  d   The fourth index of element to be minimum swapped
  @param  e   The fifth  index of element to be minimum swapped
  @param  f   The sixth  index of element to be minimum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MIN_6(a, b, c, d, e, f)   \
    do {                                             \
        if (self[b] < self[c]) {                     \
            CPL_TYPE_SELF_SWAP_MIN_5(a, b, d, e, f); \
        }                                            \
        else {                                       \
            CPL_TYPE_SELF_SWAP_MIN_5(a, c, d, e, f); \
        }                                            \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among six swap index in self w. maximum into last place
  @param  a   The first  index of element to be maximum swapped
  @param  b   The second index of element to be maximum swapped
  @param  c   The third  index of element to be maximum swapped
  @param  d   The fourth index of element to be maximum swapped
  @param  e   The fifth  index of element to be maximum swapped
  @param  f   The sixth  index of element to be maximum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MAX_6(a, b, c, d, e, f)   \
    do {                                             \
        if (self[b] < self[a]) {                     \
            CPL_TYPE_SELF_SWAP_MAX_5(a, c, d, e, f); \
        }                                            \
        else {                                       \
            CPL_TYPE_SELF_SWAP_MAX_5(b, c, d, e, f); \
        }                                            \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among seven swap index in self w. minimum into first place
  @param  a   The first  index of element to be minimum swapped
  @param  b   The second index of element to be minimum swapped
  @param  c   The third  index of element to be minimum swapped
  @param  d   The fourth index of element to be minimum swapped
  @param  e   The fifth  index of element to be minimum swapped
  @param  f   The sixth  index of element to be minimum swapped
  @param  g   The 7th    index of element to be minimum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MIN_7(a, b, c, d, e, f, g)   \
    do {                                                \
        if (self[b] < self[c]) {                        \
            CPL_TYPE_SELF_SWAP_MIN_6(a, b, d, e, f, g); \
        }                                               \
        else {                                          \
            CPL_TYPE_SELF_SWAP_MIN_6(a, c, d, e, f, g); \
        }                                               \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among seven swap index in self w. maximum into last place
  @param  a   The first  index of element to be maximum swapped
  @param  b   The second index of element to be maximum swapped
  @param  c   The third  index of element to be maximum swapped
  @param  d   The fourth index of element to be maximum swapped
  @param  e   The fifth  index of element to be maximum swapped
  @param  f   The sixth  index of element to be maximum swapped
  @param  g   The 7th    index of element to be maximum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MAX_7(a, b, c, d, e, f, g)   \
    do {                                                \
        if (self[b] < self[a]) {                        \
            CPL_TYPE_SELF_SWAP_MAX_6(a, c, d, e, f, g); \
        }                                               \
        else {                                          \
            CPL_TYPE_SELF_SWAP_MAX_6(b, c, d, e, f, g); \
        }                                               \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among eight swap index in self w. minimum into first place
  @param  a   The first  index of element to be minimum swapped
  @param  b   The second index of element to be minimum swapped
  @param  c   The third  index of element to be minimum swapped
  @param  d   The fourth index of element to be minimum swapped
  @param  e   The fifth  index of element to be minimum swapped
  @param  f   The sixth  index of element to be minimum swapped
  @param  g   The 7th    index of element to be minimum swapped
  @param  h   The 8th    index of element to be minimum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

  This and similar macros reduce the number of swaps needed to put the median
  into its place. In principle, similar macros with more parameters could be
  created. This is not done, to avoid the creation of unwieldy macros and
  because of the diminishing returns from using such larger macros to reduce
  the number of swaps.

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MIN_8(a, b, c, d, e, f, g, h)   \
    do {                                                   \
        if (self[b] < self[c]) {                           \
            CPL_TYPE_SELF_SWAP_MIN_7(a, b, d, e, f, g, h); \
        }                                                  \
        else {                                             \
            CPL_TYPE_SELF_SWAP_MIN_7(a, c, d, e, f, g, h); \
        }                                                  \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Among eight swap index in self w. maximum into last place
  @param  a   The first  index of element to be maximum swapped
  @param  b   The second index of element to be maximum swapped
  @param  c   The third  index of element to be maximum swapped
  @param  d   The fourth index of element to be maximum swapped
  @param  e   The fifth  index of element to be maximum swapped
  @param  f   The sixth  index of element to be maximum swapped
  @param  g   The 7th    index of element to be maximum swapped
  @param  h   The 8th    index of element to be maximum swapped
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SWAP_MAX_8(a, b, c, d, e, f, g, h)   \
    do {                                                   \
        if (self[b] < self[a]) {                           \
            CPL_TYPE_SELF_SWAP_MAX_7(a, c, d, e, f, g, h); \
        }                                                  \
        else {                                             \
            CPL_TYPE_SELF_SWAP_MAX_7(b, c, d, e, f, g, h); \
        }                                                  \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Sort three entries in the array self, first two already sorted (a < b)
  @param  a   The index to hold the smallest element
  @param  b   The index to hold the middle element
  @param  c   The index to hold the largest element
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_A(a, b, c) \
    do {                                \
        if (self[b] > self[c]) {        \
            CPL_TYPE_SELF_SWAP(b, c);   \
            CPL_TYPE_SELF_SORT(a, b);   \
        }                               \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Sort three entries in the array self, last two already sorted (b < c)
  @param  a   The index to hold the smallest element
  @param  b   The index to hold the middle element
  @param  c   The index to hold the largest element
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_C(a, b, c) \
    do {                                \
        if (self[a] > self[b]) {        \
            CPL_TYPE_SELF_SWAP(a, b);   \
            CPL_TYPE_SELF_SORT(b, c);   \
        }                               \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Completely sort three entries in the array self
  @param  a   The index to hold the smallest element
  @param  b   The index to hold the middle element
  @param  c   The index to hold the largest element
  @return void
  @note Side-effects from parameter evaluation are not supported

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3(a, b, c)    \
    do {                                 \
        CPL_TYPE_SELF_SORT(a, b);        \
        CPL_TYPE_SELF_SORT_3_A(a, b, c); \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Interleaved sort of two separate triplets of entries in the array self
  @param  a1   The index to hold the smallest element
  @param  b1   The index to hold the middle element
  @param  c1   The index to hold the largest element
  @param  a2   The index to hold the smallest element
  @param  b2   The index to hold the middle element
  @param  c2   The index to hold the largest element
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see CPL_TYPE_SELF_SORT_3()

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_2(a1, b1, c1, a2, b2, c2) \
    do {                                               \
        CPL_TYPE_SELF_SORT(a1, b1);                    \
        CPL_TYPE_SELF_SORT(a2, b2);                    \
        CPL_TYPE_SELF_SORT_3_A(a1, b1, c1);            \
        CPL_TYPE_SELF_SORT_3_A(a2, b2, c2);            \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Interleaved sort of 3 separate triplets of entries in the array self
  @param  a1   The index to hold the smallest element, 1st triplet
  @param  b1   The index to hold the middle element, 1st triplet
  @param  c1   The index to hold the largest element, 1st triplet
  @param  a2   The index to hold the smallest element, 2nd triplet
  @param  b2   The index to hold the middle element, 2nd triplet
  @param  c2   The index to hold the largest element, 2nd triplet
  @param  a3   The index to hold the smallest element, 3rd triplet
  @param  b3   The index to hold the middle element, 3rd triplet
  @param  c3   The index to hold the largest element, 3rd triplet
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see CPL_TYPE_SELF_SORT_3_2()

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_3(a1, b1, c1, a2, b2, c2, a3, b3, c3) \
    do {                                                           \
        CPL_TYPE_SELF_SORT(a1, b1);                                \
        CPL_TYPE_SELF_SORT(a2, b2);                                \
        CPL_TYPE_SELF_SORT(a3, b3);                                \
        CPL_TYPE_SELF_SORT_3_A(a1, b1, c1);                        \
        CPL_TYPE_SELF_SORT_3_A(a2, b2, c2);                        \
        CPL_TYPE_SELF_SORT_3_A(a3, b3, c3);                        \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Interleaved sort of 4 separate triplets of entries in the array self
  @param  a1   The index to hold the smallest element, 1st triplet
  @param  b1   The index to hold the middle element, 1st triplet
  @param  c1   The index to hold the largest element, 1st triplet
  @param  a2   The index to hold the smallest element, 2nd triplet
  @param  b2   The index to hold the middle element, 2nd triplet
  @param  c2   The index to hold the largest element, 2nd triplet
  @param  a3   The index to hold the smallest element, 3rd triplet
  @param  b3   The index to hold the middle element, 3rd triplet
  @param  c3   The index to hold the largest element, 3rd triplet
  @param  a4   The index to hold the smallest element, 4th triplet
  @param  b4   The index to hold the middle element, 4th triplet
  @param  c4   The index to hold the largest element, 4th triplet
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see CPL_TYPE_SELF_SORT_3_3()

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_4(a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4) \
    do {                                                                       \
        CPL_TYPE_SELF_SORT(a1, b1);                                            \
        CPL_TYPE_SELF_SORT(a2, b2);                                            \
        CPL_TYPE_SELF_SORT(a3, b3);                                            \
        CPL_TYPE_SELF_SORT(a4, b4);                                            \
        CPL_TYPE_SELF_SORT_3_A(a1, b1, c1);                                    \
        CPL_TYPE_SELF_SORT_3_A(a2, b2, c2);                                    \
        CPL_TYPE_SELF_SORT_3_A(a3, b3, c3);                                    \
        CPL_TYPE_SELF_SORT_3_A(a4, b4, c4);                                    \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Interleaved sort of 5 separate triplets of entries in the array self
  @param  a1   The index to hold the smallest element, 1st triplet
  @param  b1   The index to hold the middle element, 1st triplet
  @param  c1   The index to hold the largest element, 1st triplet
  @param  a2   The index to hold the smallest element, 2nd triplet
  @param  b2   The index to hold the middle element, 2nd triplet
  @param  c2   The index to hold the largest element, 2nd triplet
  @param  a3   The index to hold the smallest element, 3rd triplet
  @param  b3   The index to hold the middle element, 3rd triplet
  @param  c3   The index to hold the largest element, 3rd triplet
  @param  a4   The index to hold the smallest element, 4th triplet
  @param  b4   The index to hold the middle element, 4th triplet
  @param  c4   The index to hold the largest element, 4th triplet
  @param  a5   The index to hold the smallest element, 5th triplet
  @param  b5   The index to hold the middle element, 5th triplet
  @param  c5   The index to hold the largest element, 5th triplet
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see CPL_TYPE_SELF_SORT_3_4()

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_5(a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4, \
                               a5, b5, c5)                                     \
    do {                                                                       \
        CPL_TYPE_SELF_SORT(a1, b1);                                            \
        CPL_TYPE_SELF_SORT(a2, b2);                                            \
        CPL_TYPE_SELF_SORT(a3, b3);                                            \
        CPL_TYPE_SELF_SORT(a4, b4);                                            \
        CPL_TYPE_SELF_SORT(a5, b5);                                            \
        CPL_TYPE_SELF_SORT_3_A(a1, b1, c1);                                    \
        CPL_TYPE_SELF_SORT_3_A(a2, b2, c2);                                    \
        CPL_TYPE_SELF_SORT_3_A(a3, b3, c3);                                    \
        CPL_TYPE_SELF_SORT_3_A(a4, b4, c4);                                    \
        CPL_TYPE_SELF_SORT_3_A(a5, b5, c5);                                    \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Interleaved sort of 6 separate triplets of entries in the array self
  @param  a1   The index to hold the smallest element, 1st triplet
  @param  b1   The index to hold the middle element, 1st triplet
  @param  c1   The index to hold the largest element, 1st triplet
  @param  a2   The index to hold the smallest element, 2nd triplet
  @param  b2   The index to hold the middle element, 2nd triplet
  @param  c2   The index to hold the largest element, 2nd triplet
  @param  a3   The index to hold the smallest element, 3rd triplet
  @param  b3   The index to hold the middle element, 3rd triplet
  @param  c3   The index to hold the largest element, 3rd triplet
  @param  a4   The index to hold the smallest element, 4th triplet
  @param  b4   The index to hold the middle element, 4th triplet
  @param  c4   The index to hold the largest element, 4th triplet
  @param  a5   The index to hold the smallest element, 5th triplet
  @param  b5   The index to hold the middle element, 5th triplet
  @param  c5   The index to hold the largest element, 5th triplet
  @param  a6   The index to hold the smallest element, 6th triplet
  @param  b6   The index to hold the middle element, 6th triplet
  @param  c6   The index to hold the largest element, 6th triplet
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see CPL_TYPE_SELF_SORT_3_5()

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_6(a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4, \
                               a5, b5, c5, a6, b6, c6)                         \
    do {                                                                       \
        CPL_TYPE_SELF_SORT(a1, b1);                                            \
        CPL_TYPE_SELF_SORT(a2, b2);                                            \
        CPL_TYPE_SELF_SORT(a3, b3);                                            \
        CPL_TYPE_SELF_SORT(a4, b4);                                            \
        CPL_TYPE_SELF_SORT(a5, b5);                                            \
        CPL_TYPE_SELF_SORT(a6, b6);                                            \
        CPL_TYPE_SELF_SORT_3_A(a1, b1, c1);                                    \
        CPL_TYPE_SELF_SORT_3_A(a2, b2, c2);                                    \
        CPL_TYPE_SELF_SORT_3_A(a3, b3, c3);                                    \
        CPL_TYPE_SELF_SORT_3_A(a4, b4, c4);                                    \
        CPL_TYPE_SELF_SORT_3_A(a5, b5, c5);                                    \
        CPL_TYPE_SELF_SORT_3_A(a6, b6, c6);                                    \
    } while (0)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Interleaved sort of 7 separate triplets of entries in the array self
  @param  a1   The index to hold the smallest element, 1st triplet
  @param  b1   The index to hold the middle element, 1st triplet
  @param  c1   The index to hold the largest element, 1st triplet
  @param  a2   The index to hold the smallest element, 2nd triplet
  @param  b2   The index to hold the middle element, 2nd triplet
  @param  c2   The index to hold the largest element, 2nd triplet
  @param  a3   The index to hold the smallest element, 3rd triplet
  @param  b3   The index to hold the middle element, 3rd triplet
  @param  c3   The index to hold the largest element, 3rd triplet
  @param  a4   The index to hold the smallest element, 4th triplet
  @param  b4   The index to hold the middle element, 4th triplet
  @param  c4   The index to hold the largest element, 4th triplet
  @param  a5   The index to hold the smallest element, 5th triplet
  @param  b5   The index to hold the middle element, 5th triplet
  @param  c5   The index to hold the largest element, 5th triplet
  @param  a6   The index to hold the smallest element, 6th triplet
  @param  b6   The index to hold the middle element, 6th triplet
  @param  c6   The index to hold the largest element, 6th triplet
  @param  a7   The index to hold the smallest element, 7th triplet
  @param  b7   The index to hold the middle element, 7th triplet
  @param  c7   The index to hold the largest element, 7th triplet
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see CPL_TYPE_SELF_SORT_3_6()

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_SORT_3_7(a1, b1, c1, a2, b2, c2, a3, b3, c3, a4, b4, c4, \
                               a5, b5, c5, a6, b6, c6, a7, b7, c7)             \
    do {                                                                       \
        CPL_TYPE_SELF_SORT(a1, b1);                                            \
        CPL_TYPE_SELF_SORT(a2, b2);                                            \
        CPL_TYPE_SELF_SORT(a3, b3);                                            \
        CPL_TYPE_SELF_SORT(a4, b4);                                            \
        CPL_TYPE_SELF_SORT(a5, b5);                                            \
        CPL_TYPE_SELF_SORT(a6, b6);                                            \
        CPL_TYPE_SELF_SORT(a7, b7);                                            \
        CPL_TYPE_SELF_SORT_3_A(a1, b1, c1);                                    \
        CPL_TYPE_SELF_SORT_3_A(a2, b2, c2);                                    \
        CPL_TYPE_SELF_SORT_3_A(a3, b3, c3);                                    \
        CPL_TYPE_SELF_SORT_3_A(a4, b4, c4);                                    \
        CPL_TYPE_SELF_SORT_3_A(a5, b5, c5);                                    \
        CPL_TYPE_SELF_SORT_3_A(a6, b6, c6);                                    \
        CPL_TYPE_SELF_SORT_3_A(a7, b7, c7);                                    \
    } while (0)


#ifdef CPL_TYPE_IS_TYPE_PIXEL
/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Flip (reverse the order of) the elements of the array
  @param  self The array to flip in place
  @param  n    The array size, nop unless greater than 1
  @return void
  @note Since the function is not exported its error checking is disabled

 */
/*----------------------------------------------------------------------------*/
void ADDTYPE(cpl_tools_flip)(CPL_TYPE *self, size_t n)
{
    for (size_t i = 0; i < n / 2; i++) {
        CPL_TYPE_SELF_SWAP(i, n - 1 - i);
    }
}
#endif


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the median of a numerical array
  @param  self The array to permute and request from
  @param  n   the number of elements, at least 1
  @return The median
  @note Since the function is not exported its error checking is disabled

  For a finite population or sample, the median is the middle value of an odd
  number of values (arranged in ascending order) or any value between the two
  middle values of an even number of values.
  For an even number of elements in the array, the mean of the two central
  values is returned. Note that in this case, the median might not be a value
  of the input array. Also, note that in the case of integer data types, 
  the division by 2 is performed with integer arithmetic.
  Consider casting your integer array to float if that is not the desired 
  behavior.  

  After a successful call, self is permuted so elements less than the median
  have lower indices, while elements greater than the median have higher
  indices.

  Example: the median of (1 2 3) is 2 and the median of (1. 2. 3. 4.) is 2.5.
  The median of the integers (1 2 3 4) is 2 (due to the integer arithmetic).

  The function is optimized for 1 through 26 elements. In the literature, the
  optimization metric is the number of guarded swaps for a given sample size.
  In addition to that it turns out that two subsequent guards should ideally
  not involve the same numbers, since this will prevent pipelining of the
  evaluations. The effect of this can be reduced by interleaving guarded swaps
  from different evaluations. Some of the larger functions are faster than the
  general method only in some (worst) cases.

  Given an (odd) 2P-1 sample median, an (even) 2P sample median can be
  implemented by first computing the 2P-1 median, and then sorting the
  extra element with the 2P-1 median value. The other, either upper or lower
  median value can then be determined from an extra P comparisons within the
  relevant half-set. For example, from the 9-sample median (P=5) that requires
  19 guarded swaps a 10-sample median can be made in 19 + 5 = 24 guarded swaps.

  Caveat maintainer: An array with N distinct values can be permuted in N! ways.
  While for a 16-sample median the 20922789888000 input combinations can all be
  unit-tested (in about 24 hours on a 24-core machine), the unit-test coverage
  of the 25! > 10^25 possible combinations of the input for 25-median function
  is minuscule (on the order of 10^-12). For any changes to this function
  involving sample sizes greater than 16 the randomized unit-test in
  cpl_median-test.c needs to run for a significant amount of time.

 */
/*----------------------------------------------------------------------------*/
CPL_TYPE
ADDTYPE(cpl_tools_get_median)(CPL_TYPE *self, cpl_size n)
{
    switch (n) {
        case 0:
            assert(n > 0);     /* Cannot meaningfully proceed */
            CPL_ATTR_FALLTRHU; /* fall through */

        case 1:
            break;

        case 2:
            CPL_TYPE_SELF_SORT(0, 1);
            break;

        case 3:
            ADDTYPE(cpl_tools_get_median_3)(self);
            break;

        case 4:
            ADDTYPE(cpl_tools_get_median_4)(self);
            break;

        case 5:
            ADDTYPE(cpl_tools_get_median_5)(self);
            break;

        case 6:
            ADDTYPE(cpl_tools_get_median_6)(self);
            break;

        case 7:
            ADDTYPE(cpl_tools_get_median_7)(self);
            break;

        case 8:
            ADDTYPE(cpl_tools_get_median_8)(self);
            break;

        case 9:
            ADDTYPE(cpl_tools_get_median_9)(self);
            break;

        case 10:
            ADDTYPE(cpl_tools_get_median_10)(self);
            break;

        case 11:
            ADDTYPE(cpl_tools_get_median_11)(self);
            break;

        case 12:
            ADDTYPE(cpl_tools_get_median_12)(self);
            break;

        case 13:
            ADDTYPE(cpl_tools_get_median_13)(self);
            break;

        case 14:
            ADDTYPE(cpl_tools_get_median_14)(self);
            break;

        case 15:
            ADDTYPE(cpl_tools_get_median_15)(self);
            break;

        case 16:
            ADDTYPE(cpl_tools_get_median_16)(self);
            break;

        case 17:
            ADDTYPE(cpl_tools_get_median_17)(self);
            break;

        case 18:
            ADDTYPE(cpl_tools_get_median_18)(self);
            break;

        case 19:
            ADDTYPE(cpl_tools_get_median_19)(self);
            break;

        case 20:
            ADDTYPE(cpl_tools_get_median_20)(self);
            break;

        case 21:
            ADDTYPE(cpl_tools_get_median_21)(self);
            break;

        case 22:
            ADDTYPE(cpl_tools_get_median_22)(self);
            break;

        case 23:
            ADDTYPE(cpl_tools_get_median_23)(self);
            break;

        case 24:
            ADDTYPE(cpl_tools_get_median_24)(self);
            break;

        case 25:
            ADDTYPE(cpl_tools_get_median_25)(self);
            break;

        case 26:
            ADDTYPE(cpl_tools_get_median_26)(self);
            break;

        default:
            if (n & 1) { /* Odd size */
                (void)ADDTYPE(cpl_tools_quickselection)(self, n, n / 2);
            }
            else { /* Even size */
                /* The median is computed as the mean of the actual, lower and
                   upper median values. One approach to finding the lower and
                   upper median values is to first use the standard method to
                   find e.g. the lower median, after which the upper median
                   value is the minimum among the upper half-set of values.
                   Alternatively, and since a minimum (or maximum) among half
                   the values has to be found in any case, one might as well
                   first compute the median of the n - 1 samples and then
                   determine the other median value via a minimum or maximum
                   search */
                /* Compute odd size median, then bubble in last element */
                (void)ADDTYPE(cpl_tools_quickselection)(self, n - 1, n / 2 - 1);
                if (self[n - 1] < self[n / 2 - 1]) {
                    /* Last element belongs to lower half-set
                       - move odd-size median to upper median position
                       - and find lower half-set maximum  */
                    CPL_TYPE_SELF_CYCLE(n / 2 - 1, n / 2, n - 1);
                    ADDTYPE(cpl_tools_get_nth)(self, n / 2);
                }
                else {
                    /* Last element belongs to upper half-set
                       - find upper half-set minimum  */
                    ADDTYPE(cpl_tools_get_0th)(self + n / 2, n / 2);
                }
            }
    }

    return (n & 1) ? self[n / 2] /* Odd size */
                   : self[n / 2 - 1] +
                         (self[n / 2] - self[n / 2 - 1]) / 2; /* Even size */
}


CPL_DIAG_PRAGMA_PUSH_IGN(-Wattributes)

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the smallest value in a numerical array
  @param  self The array to permute and request from
  @param  n    The number of elements in the array, must be at least 1
  @return void
  @see cpl_tools_get_kth
  @note Since the function is static its error checking is disabled
  @note cpl_tools_get_0th(self, n) is the same as
  (void)cpl_tools_get_kth(self, n, 0)

 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_0th)(CPL_TYPE *self, size_t n)
{
    CPL_TYPE min = self[0];
    size_t amin = 0;

    for (size_t i = 1; i < n; i++) {
        if (self[i] < min) {
            amin = i;
            min = self[i];
        }
    }

    CPL_TYPE_SELF_SWAP(0, amin);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the largest value in a numerical array
  @param  self The array to permute and request from
  @param  n    The number of elements in the array, must be at least 1
  @return void
  @see cpl_tools_get_kth
  @note Since the function is static its error checking is disabled
  @note cpl_tools_get_nth(self, n) is the same as
  (void)cpl_tools_get_kth(self, n, n-1)

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_nth)(CPL_TYPE *self, size_t n)
{
    CPL_TYPE max = self[n - 1];
    size_t amax = n - 1;

    for (size_t i = n - 1; i > 0; --i) {
        if (self[i - 1] > max) {
            amax = i - 1;
            max = self[i - 1];
        }
    }

    CPL_TYPE_SELF_SWAP(n - 1, amax);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 3 elements (via 3 guarded swaps)
  @param  self  Array to sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  An already sorted array incurs 2 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_3)(CPL_TYPE *self)
{
    CPL_TYPE_SELF_SORT_3(0, 1, 2); /* Who's on first? */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 4 elements (via 6 guarded swaps)
  @param  self  Array to sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_3()

  Since both central values must be correctly placed the whole array must be
  completely sorted. With bubble sort this can be done with 6 guarded swaps.

  Completely sorting the first triplet and then bubble-sorting the last
  element into place has a 50% chance of eliminating two guarded swaps and
  a 25% chance of eliminating one guarded swap.

  An already sorted array incurs 3 comparisons (and zero swaps).

  A variant where the 2nd last element is done last requires only 5 guarded
  swaps, but is on average slower.

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_4)(CPL_TYPE *self)
{
    CPL_TYPE_SELF_SORT_3(0, 1, 2);
    /* Now bubble index 3 into place */
    if (self[2] > self[3]) {
        CPL_TYPE_SELF_SWAP(2, 3);
        CPL_TYPE_SELF_SORT_3_A(0, 1, 2);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 5 elements (via 6 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled

  In any 4-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This method, where two of the six guarded swaps involve two pairs of elements,
  is due to Donald Knuth. If multiple, simultaneous 5-medians are needed then
  an interleaved implementation of the 7 guarded swaps method is likely faster.

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_5)(CPL_TYPE *self)
{
#ifdef CPL_MEDIAN_NO_PAIR
    /* No paired swaps incurs 1 extra guard */
    CPL_TYPE_SELF_SORT(0, 1);

    CPL_TYPE_SELF_SORT(3, 4);

    CPL_TYPE_SELF_SORT(1, 4); /* As 4-tuple maximum cut 4 */

    CPL_TYPE_SELF_SORT(0, 3); /* As 4-tuple minimum cut 0 */

    CPL_TYPE_SELF_SORT_3(1, 2, 3);
#else
    CPL_TYPE_SELF_SORT(0, 1);
    CPL_TYPE_SELF_SORT(2, 3);

    /* Cut 3 as 4-tuple maximum */
    CPL_TYPE_SELF_SORT_PAIR(1, 0, 3, 2);

    CPL_TYPE_SELF_SORT(2, 4);

    /* Cut 4 as 4-tuple maximum */
    CPL_TYPE_SELF_SORT_PAIR(1, 0, 4, 2);

    CPL_TYPE_SELF_SORT(1, 2);
#endif
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 6 elements (via 9 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_5()

  In any 5-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 5-median, then bubble the 6th element into place

  An already sorted array incurs 9 comparisons (and zero swaps).

  A different implementation from Christoph_John@gmx.de
  based on a selection network which was proposed in
  "FAST, EFFICIENT MEDIAN FILTERS WITH EVEN LENGTH WINDOWS"
  J.P. HAVLICEK, K.A. SAKADY, G.R.KATZ
  - however this paper proposes selection networks which for 2N samples require
  N^2 + 2N - 3 guarded swaps, more than is used here.

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_6)(CPL_TYPE *self)
{
    /* First compute 5-median, on first 5 elements */
    ADDTYPE(cpl_tools_get_median_5)(self);

    /* Now bubble last element into place */
    if (self[5] < self[2]) {
        CPL_TYPE_SELF_CYCLE(2, 3, 5);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_3(0, 1, 2);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_3(3, 4, 5);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Find the median among a 7-sample w. 2 outer values ready to be cut
  @param  self Pointer to the 7-element array
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see cpl_tools_get_median_7_int()

  Incurs 2 * 2 + 2 + 2 = 8 guarded swaps.

  An already sorted array incurs 6 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_7_sorted)(CPL_TYPE *self)
{
    /* Sort (truncated) triplet of central samples */
    CPL_TYPE_SELF_SORT(0, 3);

    CPL_TYPE_SELF_SORT(2, 5); /* Cut minimum with triplet minimum */
    CPL_TYPE_SELF_SORT_3_A(0, 3, 6);
    CPL_TYPE_SELF_SORT(1, 4); /* Cut maximum with triplet maximum */

    /* Final sort of central triplet - upper part first */
    CPL_TYPE_SELF_SORT(3, 5);
    CPL_TYPE_SELF_SORT_3_C(1, 3, 5);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 7 elements (via 13 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_9_int()

  In any 5-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  Implemented by removing the first and last elements from the 9-sample code
  (and decrementing all indices).

  An already sorted array incurs 10 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_7)(CPL_TYPE *self)
{

    /* Sort each of the three triplets (two outer ones truncated) */
    CPL_TYPE_SELF_SORT(0, 1);
    CPL_TYPE_SELF_SORT_3(2, 3, 4);
    CPL_TYPE_SELF_SORT(5, 6);

    /* 7 samples remaining */
    ADDTYPE(cpl_tools_get_median_7_sorted)(self);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 8 elements (via 17 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_7_int()

  First compute the 7-median, then bubble the 8th element into place

  An already sorted array incurs 14 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_8)(CPL_TYPE *self)
{
    /* Compute 7-median, on first 7 elements */
    ADDTYPE(cpl_tools_get_median_7)(self);

    /* Now bubble last element into place */
    if (self[7] < self[3]) {
        CPL_TYPE_SELF_CYCLE(3, 4, 7);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_4(0, 1, 2, 3);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_4(4, 5, 6, 7);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Find the median among a 9-sample w. 2 outer values ready to be cut
  @param  self Pointer to the 9-element array
  @return void
  @note Side-effects from parameter evaluation are not supported
  @see cpl_tools_get_median_9_int()

  Incurs 2 * 3 + 2 + 2 = 10 guarded swaps.

  An already sorted array incurs 8 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_9_sorted)(CPL_TYPE *self)
{
    /* Cut minimum of first+center triplet minima */
    CPL_TYPE_SELF_SORT(0, 3);
    /* Cut maximum of center+last triplet maxima */
    CPL_TYPE_SELF_SORT(5, 8);

    /* 7 samples remaining */
    ADDTYPE(cpl_tools_get_median_7_sorted)(self + 1);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 9 elements (via 19 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  In any 6-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  Each node in the selection network is a guarded swap, the tree has 19 nodes,
  longest path has 9 nodes.

  The idea is to first form three completely sorted 3-tuples and then to
  discard their combined minimum or maximum. The sorted 3-tuples are then
  reformed and their combined minimum or maximum discarded.
  Finally, completely sorted 3-tuples are formed to determine the median.

  An already sorted array incurs 14 comparisons (and zero swaps).

  This selection network is minimal according to:
  XILINX XCELL magazine, vol. 23 by John L. Smith
  - the 1996 Q4 issue.

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_9)(CPL_TYPE *self)
{
    /* Sort each of the three triplets */
    CPL_TYPE_SELF_SORT_3_3(0, 1, 2, 3, 4, 5, 6, 7, 8);

    /* 9 samples remaining */
    ADDTYPE(cpl_tools_get_median_9_sorted)(self);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 10 elements (via 24 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_9_int()

  In any 7-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 9-median, then bubble the 10th element into place

  An already sorted array incurs 19 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_10)(CPL_TYPE *self)
{
    /* First compute 9-median, on first 9 elements */
    ADDTYPE(cpl_tools_get_median_9)(self);

    /* Now bubble last element into place */
    if (self[9] < self[4]) {
        CPL_TYPE_SELF_CYCLE(4, 5, 9);
        /* Put maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_5(0, 1, 2, 3, 4);
    }
    else {
        /* Put minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_5(5, 6, 7, 8, 9);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 11 elements (via 28 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_9_int()

  In any 7-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done via two completely sorted 3-tuples and two additional samples.

  Then from the 9 remaining samples (and reusing some state from the 3-tuples)
  three sorted 3-tuples are formed and processed similarly to the 9-sample
  median.

  Incurs 5 * 3 complete triplet sorts, 2 * 2 sorts of partially sorted
  triplets and 9 guarded swaps, for at total of 28 guarded swaps.

  An already sorted array incurs 21 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_11)(CPL_TYPE *self)
{
    /* Sort a 1st and last triplet (just inside outermost samples) */
    CPL_TYPE_SELF_SORT_3_2(1, 2, 3, 7, 8, 9);

    /* Minimum of first+center triplet minima */
    CPL_TYPE_SELF_SORT(1, 7);

    /* Maximum of center+last triplet maxima */
    CPL_TYPE_SELF_SORT(3, 9);

    /* 11 samples remaining */
    CPL_TYPE_SELF_SORT(0, 1);  /* Cut minimum of 7-tuple */
    CPL_TYPE_SELF_SORT(9, 10); /* Cut maximum of 7-tuple */

    /* Sort triplet pair's middle elements */
    CPL_TYPE_SELF_SORT(2, 8);

    CPL_TYPE_SELF_SORT(4, 5);
    CPL_TYPE_SELF_SORT_3_C(1, 2, 3);
    CPL_TYPE_SELF_SORT_3_A(4, 5, 6);
    CPL_TYPE_SELF_SORT_3_A(7, 8, 9);

    /* 9 samples remaining */
    ADDTYPE(cpl_tools_get_median_9_sorted)(self + 1);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 12 elements (via 34 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_11_int()

  In any 8-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 11-median, then bubble the 12th element into place

  Incurs 6 + 28 = 34 guarded swaps (28 from the 11-median).

  An already sorted array incurs 6 + 21 = 27 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_12)(CPL_TYPE *self)
{
    /* First compute 11-median, on first 11 elements */
    ADDTYPE(cpl_tools_get_median_11)(self);

    /* Now bubble last element into place - assuming mostly sorted elements */
    if (self[11] < self[5]) {
        CPL_TYPE_SELF_CYCLE(5, 6, 11);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_6(0, 1, 2, 3, 4, 5);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_6(6, 7, 8, 9, 10, 11);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Partition 4 samples into two lower and two upper values
  @param  a   The first  index of element to be partioned
  @param  b   The second index of element to be partioned
  @param  c   The third  index of element to be partioned
  @param  d   The fourth index of element to be partioned
  @return void
  @note Side-effects from parameter evaluation are not supported

  Incurs 4 (or 3) guards on 2 swaps.

 */
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_SELF_PART_4(a, b, c, d)           \
    do {                                           \
        if (self[d] < self[c]) {                   \
            if (self[d] < self[a]) {               \
                CPL_TYPE_SELF_SWAP(a, d);          \
                CPL_TYPE_SELF_SWAP_MIN_3(b, c, d); \
            }                                      \
            else {                                 \
                CPL_TYPE_SELF_SORT(b, d);          \
            }                                      \
        }                                          \
        else {                                     \
            if (self[c] < self[a]) {               \
                CPL_TYPE_SELF_SWAP(a, c);          \
                CPL_TYPE_SELF_SWAP_MIN_3(b, c, d); \
            }                                      \
            else {                                 \
                CPL_TYPE_SELF_SORT(b, c);          \
            }                                      \
        }                                          \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 13 elements (via 37 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_11_int()

  In any 8-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via 3 completely sorted 3-tuples and one sorted pair,
  from which two minima and two maxima in 8-tuples are discarded into
  their respective lower and upper halves. On the remaining 9 samples, the
  9-sample method is used.

  Incurs 3 * 3 + 1 + 4 * 2 + 19 = 37 guarded swaps (19 from the 9-median).

  An already sorted array incurs 15 + 14 = 29 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_13)(CPL_TYPE *self)
{
    /* Sort three (outer) triplets
       - such that 2 pairs of outer entries become tuple minima + maxima
       - and with 1 triplet truncated, since only 8-tuples are needed */
    CPL_TYPE_SELF_SORT_3_3(0, 4, 9, 1, 5, 10, 3, 8, 12);
    CPL_TYPE_SELF_SORT(2, 11);

    /* Cut 0/1 as 8-tuple minima
       Cut 11/12 as 8-tuple maxima  */
    CPL_TYPE_SELF_PART_4(0, 1, 2, 3);
    CPL_TYPE_SELF_PART_4(9, 10, 11, 12);

    /* 9 samples remaining */
    ADDTYPE(cpl_tools_get_median_9)(self + 2);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 14 elements (via 44 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_15_int()

  In any 9-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via four completely sorted 3-tuples, from which two
  minima and three maxima in 9-tuples are discarded into their respective lower
  and upper halves. On the remaining 10 samples, the 10-sample method is used.

  Incurs 4 * 3 + 4 * 2 + 24 = 44 guarded swaps (24 from the 10-sample).

  An already sorted array incurs 16 + 19 = 35 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_14)(CPL_TYPE *self)
{
    /* Sort four (outer) triplets
       - such that 2 pairs of outer entries become tuple minima + maxima */
    CPL_TYPE_SELF_SORT_3_4(0, 4, 10, 1, 5, 11, 2, 8, 12, 3, 9, 13);

    /* Cut 0/1 as 9-tuple minima
       Cut 12/13 as 9-tuple maxima  */
    CPL_TYPE_SELF_PART_4(0, 1, 2, 3);
    CPL_TYPE_SELF_PART_4(10, 11, 12, 13);

    /* 10 samples remaining */
    ADDTYPE(cpl_tools_get_median_10)(self + 2);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 15 elements (via 46 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_9_int()

  In any 9-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via five completely sorted 3-tuples, from which three
  minima and three maxima in 9-tuples are discarded into their respective lower
  and upper halves. On the remaining 9 samples, the 9-sample method is used.

  Incurs 5 * 3 + 4 * 3 + 19 = 46 guards on 38 swaps (19 from the 9-sample).

  An already sorted array incurs 22 + 14 = 36 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_15)(CPL_TYPE *self)
{
    /* Sort all of the five step-5 triplets
       - such that 3 pairs of outer entries become tuple minima + maxima */
    CPL_TYPE_SELF_SORT_3_5(0, 5, 10, 1, 6, 11, 2, 7, 12, 3, 8, 13, 4, 9, 14);

    /* Cut 0/1/2 as 9-tuple minima (leaving 3/4 as maxima)
       Cut 12/13/14 as 9-tuple maxima (leaving 10/11 as minima) */
    CPL_TYPE_SELF_PART_4(1, 2, 3, 4);
    CPL_TYPE_SELF_PART_4(10, 11, 12, 13);
    CPL_TYPE_SELF_SWAP_MIN_3(0, 3, 4);
    CPL_TYPE_SELF_SWAP_MAX_3(10, 11, 14);

    /* 9 samples remaining */
    ADDTYPE(cpl_tools_get_median_9)(self + 3);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 16 elements (via 54 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_15_int()

  In any 10-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 15-median, then bubble the 16th element into place

  Incurs 8 + 46 = 54 guarded swaps (46 from the 15-sample).

  An already sorted array incurs 8 + 30 = 38 comparisons (and zero swaps).

  A modification of the 17-sample method also incurs 54 guarded swaps but
  it slower.

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_16)(CPL_TYPE *self)
{
    /* First compute 15-median, on first 15 elements */
    ADDTYPE(cpl_tools_get_median_15)(self);

    /* Now bubble last element into place - assuming mostly sorted elements */
    if (self[15] < self[7]) {
        CPL_TYPE_SELF_CYCLE(7, 8, 15);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_8(0, 1, 2, 3, 4, 5, 6, 7);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_8(8, 9, 10, 11, 12, 13, 14, 15);
    }
}

/*----------------------------------------------------------------------------*/
/**
   @internal
   @brief  Twice partition 6-tuple in self so entries a & b & c <= d & e & f
   @param  a1   The index to hold one element not greater than d & e & f
   @param  b1   The index to hold 2nd element not greater than d & e & f
   @param  c1   The index to hold 3rd element not greater than d & e & f
   @param  d1   The index to hold one element not smaller than a & b & c
   @param  e1   The index to hold 2nd element not smaller than a & b & c
   @param  f1   The index to hold 3rd element not smaller than a & b & c
   @param  a2   The index to hold one element not greater than d & e & f
   @param  b2   The index to hold 2nd element not greater than d & e & f
   @param  c2   The index to hold 3rd element not greater than d & e & f
   @param  d2   The index to hold one element not smaller than a & b & c
   @param  e2   The index to hold 2nd element not smaller than a & b & c
   @param  f2   The index to hold 3rd element not smaller than a & b & c
   @return void
   @note Side-effects from parameter evaluation are not supported

   Each 6-tuple partitioning is done by first partly using the 7-guard
   5-median method to cut one pair of minimum+maximum values from a 4-tuple.
   The remaining 4 samples are then partitioned using a 3-sample
   minimum/maximum twice.
   This is interleaved so subsequent guards do not block each other.

   Incurs 16 guarded swaps.

*/
/*----------------------------------------------------------------------------*/
#define CPL_TYPE_PART_6_6(a1, b1, c1, d1, e1, f1, a2, b2, c2, d2, e2, f2) \
    do {                                                                  \
        CPL_TYPE_SELF_SORT(a1, b1);                                       \
        CPL_TYPE_SELF_SORT(b2, a2);                                       \
                                                                          \
        CPL_TYPE_SELF_SORT(d1, f1);                                       \
        CPL_TYPE_SELF_SORT(d2, e2);                                       \
                                                                          \
        CPL_TYPE_SELF_SORT(b1, f1); /* As 4-tuple maximum cut f1 */       \
        CPL_TYPE_SELF_SORT(b2, d2); /* As 4-tuple minimum cut b2 */       \
                                                                          \
        CPL_TYPE_SELF_SORT(a1, d1); /* As 4-tuple minimum cut a1 */       \
        CPL_TYPE_SELF_SORT(a2, e2); /* As 4-tuple maximum cut e2 */       \
                                                                          \
        /* 4-tuple lower half cut b1+c1 */                                \
        /* 4-tuple upper half cut d2+f2 */                                \
        CPL_TYPE_SELF_PART_4(b1, c1, d1, e1);                             \
        CPL_TYPE_SELF_PART_4(a2, c2, d2, f2);                             \
    } while (0)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 17 elements (via 58 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_15_int()

  In any 10-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done via four completely sorted 3-tuples and two additional pairs.

  Incurs 4 * 3 + 2 + 16 + 28 = 58 guards on 54 swaps (28 from the 11-sample).

  An already sorted array incurs 26 + 21 = 47 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_17)(CPL_TYPE *self)
{
    /* Sort six (outer) triplets - ensuring that 3 pairs of outer entries
       become tuple minima + maxima
       - and with 2 triplets truncated, since only 10-tuples are needed */
    CPL_TYPE_SELF_SORT_3_4(0, 6, 11, 1, 7, 12, 4, 9, 15, 5, 10, 16);
    CPL_TYPE_SELF_SORT(2, 13);
    CPL_TYPE_SELF_SORT(3, 14);

    /* Cut 0/1/2 as 10-tuple minima, i.e. cut lower 3 from a 6-tuple
       Cut 14/15/16 as 10-tuple maxima, i.e. cut upper 3 from a 6-tuple */
    CPL_TYPE_PART_6_6(0, 1, 2, 3, 4, 5, 11, 12, 13, 14, 15, 16);

    /* 11 samples remaining */
    ADDTYPE(cpl_tools_get_median_11)(self + 3); /* FIXME: Reuse state ? */
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 18 elements (via 66 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_17_int()

  In any 11-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via five completely sorted 3-tuples and one sorted pair,
  from which three minima and three maxima in 11-tuples are discarded into
  their respective lower and upper halves. On the remaining 13 samples, the
  13-sample method is used.

  Incurs 5 * 3 + 1 + 16 + 34 = 66 guarded swaps (34 from the 13-median).

  An already sorted array incurs 27 + 27 = 54 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_18)(CPL_TYPE *self)
{
    /* Sort six (outer) triplets - ensuring that 3 pairs of outer entries
       become tuple minima + maxima
       - and with 1 triplet truncated, since only 11-tuples are needed */
    CPL_TYPE_SELF_SORT_3_5(0, 6, 12, 1, 7, 13, 2, 8, 14, 4, 10, 16, 5, 11, 17);
    CPL_TYPE_SELF_SORT(3, 15);

    /* Cut 0/1/2 as 11-tuple minima, i.e. cut lower 3 from a 6-tuple
       Cut 15/16/17 as 11-tuple maxima, i.e. cut upper 3 from a 6-tuple */
    CPL_TYPE_PART_6_6(0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17);

    /* 12 samples remaining */
    ADDTYPE(cpl_tools_get_median_12)(self + 3); /* FIXME: Reuse state ? */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 19 elements (via 69 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_17_int()

  In any 11-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via five completely sorted 3-tuples and one sorted pair,
  from which three minima and three maxima in 11-tuples are discarded into
  their respective lower and upper halves. On the remaining 13 samples, the
  13-sample method is used.

  Incurs 5 * 3 + 1 + 16 + 37 = 69 guarded swaps (37 from the 13-median).

  An already sorted array incurs 27 + 29 = 56 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_19)(CPL_TYPE *self)
{
    /* Sort six (outer) triplets - ensuring that 3 pairs of outer entries
       become tuple minima + maxima
       - and with 1 triplet truncated, since only 11-tuples are needed */
    CPL_TYPE_SELF_SORT_3_5(0, 6, 13, 1, 7, 14, 2, 8, 15, 4, 11, 17, 5, 12, 18);
    CPL_TYPE_SELF_SORT(3, 16);

    /* Cut 0/1/2 as 11-tuple minima, i.e. cut lower 3 from a 6-tuple
       Cut 16/17/18 as 11-tuple maxima, i.e. cut upper 3 from a 6-tuple */
    CPL_TYPE_PART_6_6(0, 1, 2, 3, 4, 5, 13, 14, 15, 16, 17, 18);

    /* 13 samples remaining */
    ADDTYPE(cpl_tools_get_median_13)(self + 3); /* FIXME: Reuse state ? */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 20 elements (via 78 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_19_int()

  In any 12-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via five completely sorted 3-tuples and one sorted pair,
  from which three minima and three maxima in 11-tuples are discarded into
  their respective lower and upper halves. On the remaining 14 samples, the
  14-sample method is used.

  Incurs 6 * 3 + 16 + 44 = 78 guarded swaps (44 from the 14-median).

  An already sorted array incurs 27 + 35 = 62 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_20)(CPL_TYPE *self)
{
    /* Sort six (outer) step-6/7 triplets - ensuring that 3 pairs of outer
       entries become tuple minima + maxima
       - and with 1 triplet truncated, since only 11-tuples are needed */
    CPL_TYPE_SELF_SORT_3_6(0, 6, 14, 1, 7, 15, 2, 8, 16, 3, 11, 17, 4, 12, 18,
                           5, 13, 19);

    /* Cut 0/1/2 as 12-tuple minima, i.e. cut lower 3 from a 6-tuple
       Cut 17/18/19 as 12-tuple maxima, i.e. cut upper 3 from a 6-tuple */
    CPL_TYPE_PART_6_6(0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 18, 19);

    /* 14 samples remaining */
    ADDTYPE(cpl_tools_get_median_14)(self + 3); /* FIXME: Reuse state ? */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 21 elements (via 78 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_19_int()

  In any 12-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  This is done first via seven completely sorted 3-tuples, from which four
  minima and four maxima in 12-tuples are discarded into their respective
  lower and upper halves. On the remaining 13 samples, the 13-sample method
  is used.

  Incurs 7 * 3 + 20 + 37 = 78 guarded swaps (37 from the 13-median).

  An already sorted array incurs 28 + 38 = 56 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_21)(CPL_TYPE *self)
{
    /* All seven step-7 triplets */
    CPL_TYPE_SELF_SORT_3_7(0, 7, 14, 1, 8, 15, 2, 9, 16, 3, 10, 17, 4, 11, 18,
                           5, 12, 19, 6, 13, 20);

    /* Cut 4 smallest among 7: 0, 1, 2, 3, 4, 5, 6 */
    /* Cut 4 largest among 7: 14, 15, 16, 17, 18, 19, 20 */

    /* Interleaved: First find inner 5-median, then place outer samples */
    CPL_TYPE_SELF_SORT(1, 2);
    CPL_TYPE_SELF_SORT(15, 16);

    CPL_TYPE_SELF_SORT(3, 4);
    CPL_TYPE_SELF_SORT(17, 18);

    /* Cut 4 as 4-tuple maximum */
    CPL_TYPE_SELF_SORT_PAIR(2, 1, 4, 3);
    /* Cut 18 as 4-tuple maximum */
    CPL_TYPE_SELF_SORT_PAIR(16, 15, 18, 17);

    CPL_TYPE_SELF_SORT(3, 5);
    CPL_TYPE_SELF_SORT(17, 19);

    /* Cut 5 as 4-tuple maximum */
    CPL_TYPE_SELF_SORT_PAIR(2, 1, 5, 3);
    /* Cut 19 as 4-tuple maximum */
    CPL_TYPE_SELF_SORT_PAIR(16, 15, 19, 17);

    /* 3 and 17 are 5-sample medians */
    CPL_TYPE_SELF_SORT(2, 3);
    CPL_TYPE_SELF_SORT(16, 17);

    if (self[0] > self[3]) {
        /* Replace lower sample by minimum among upper 3 */
        CPL_TYPE_SELF_SWAP_MIN_4(0, 4, 5, 6);
    }
    else {
        /* Lower sample already correctly placed, sort in upper */
        CPL_TYPE_SELF_SORT(3, 6);
    }
    if (self[17] > self[20]) {
        /* Replace upper sample by maximum among lower 3 */
        CPL_TYPE_SELF_SWAP_MAX_4(14, 15, 16, 20);
    }
    else {
        /* Upper sample already correctly placed, sort in lower */
        CPL_TYPE_SELF_SORT(14, 17);
    }

    /* 13 samples remaining */
    ADDTYPE(cpl_tools_get_median_13)(self + 4); /* FIXME: Reuse state ? */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 22 elements (via 89 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_21_int()

  In any 13-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 21-median, then bubble the 22th element into place

  Incurs 11 + 78 = 89 guarded swaps (78 from the 21-sample).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_22)(CPL_TYPE *self)
{
    /* First compute 21-median, on first 21 elements */
    ADDTYPE(cpl_tools_get_median_21)(self);

    /* Now bubble last element into place - assuming mostly sorted elements */
    if (self[21] < self[10]) {
        CPL_TYPE_SELF_CYCLE(10, 11, 21);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_6(0, 1, 2, 3, 4, 10);
        CPL_TYPE_SELF_SWAP_MAX_6(5, 6, 7, 8, 9, 10);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_6(11, 12, 13, 14, 15, 21);
        CPL_TYPE_SELF_SWAP_MIN_6(11, 16, 17, 18, 19, 20);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 23 elements (via 93 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_25_int()

  In any 13-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  Implemented by removing the first and last elements from the 25-sample code
  (and decrementing all indices).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_23)(CPL_TYPE *self)
{
    /* All step-1 triplets */
    CPL_TYPE_SELF_SORT_3_7(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                           16, 17, 18, 19, 20, 21);

    /* All step-3 triplets (4 truncated) */
    CPL_TYPE_SELF_SORT(1, 4);
    CPL_TYPE_SELF_SORT(2, 5);
    CPL_TYPE_SELF_SORT_3_5(0, 3, 6, 7, 10, 13, 8, 11, 14, 9, 12, 15, 16, 19,
                           22);
    CPL_TYPE_SELF_SORT(17, 20);
    CPL_TYPE_SELF_SORT(18, 21);

    /* All step-9 triplets (4 truncated) */
    CPL_TYPE_SELF_SORT(7, 16);
    CPL_TYPE_SELF_SORT(8, 17);
    CPL_TYPE_SELF_SORT_3_5(0, 9, 18, 1, 10, 19, 2, 11, 20, 3, 12, 21, 4, 13,
                           22);
    CPL_TYPE_SELF_SORT(5, 14);
    CPL_TYPE_SELF_SORT(6, 15);

    CPL_TYPE_SELF_SORT(6, 18);
    CPL_TYPE_SELF_SORT(12, 20);
    CPL_TYPE_SELF_SORT(14, 22);
    CPL_TYPE_SELF_SORT(6, 12);
    CPL_TYPE_SELF_SORT(6, 14);
    CPL_TYPE_SELF_SORT(0, 8);
    CPL_TYPE_SELF_SORT(2, 10);
    CPL_TYPE_SELF_SORT(4, 16);
    CPL_TYPE_SELF_SORT(10, 16);
    CPL_TYPE_SELF_SORT(8, 16);
    CPL_TYPE_SELF_SORT(3, 9);
    CPL_TYPE_SELF_SORT(5, 11);
    CPL_TYPE_SELF_SORT(6, 13);
    CPL_TYPE_SELF_SORT(3, 5);
    CPL_TYPE_SELF_SORT(3, 6);
    CPL_TYPE_SELF_SORT(11, 13);
    CPL_TYPE_SELF_SORT(9, 13);
    CPL_TYPE_SELF_SORT(5, 6);
    CPL_TYPE_SELF_SORT(9, 11);
    CPL_TYPE_SELF_SORT(5, 9);
    CPL_TYPE_SELF_SORT(5, 16);
    CPL_TYPE_SELF_SORT(11, 16);
    CPL_TYPE_SELF_SORT(6, 16);
    CPL_TYPE_SELF_SORT(6, 9);
    CPL_TYPE_SELF_SORT(11, 17);
    CPL_TYPE_SELF_SORT(6, 11);
    CPL_TYPE_SELF_SORT(9, 17);

    CPL_TYPE_SELF_SORT_3(9, 11, 19);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 24 elements (via 105 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_23_int()

  In any 14-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 23-median, then bubble the 24th element into place

  Incurs 12 + 93 = 105 guarded swaps (93 from the 23-sample).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_24)(CPL_TYPE *self)
{
    /* First compute 23-median, on first 23 elements */
    ADDTYPE(cpl_tools_get_median_23)(self);

    /* Now bubble last element into place - assuming mostly sorted elements */
    if (self[23] < self[11]) {
        CPL_TYPE_SELF_CYCLE(11, 12, 23);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_7(0, 1, 2, 3, 4, 5, 11);
        CPL_TYPE_SELF_SWAP_MAX_6(6, 7, 8, 9, 10, 11);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_7(12, 13, 14, 15, 16, 17, 23);
        CPL_TYPE_SELF_SWAP_MIN_6(12, 18, 19, 20, 21, 22);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 25 elements (via 99 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  In any 14-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  http://ndevilla.free.fr/median/median/

  In theory, cannot go faster without assumptions on the signal.
  Inspired by code from Graphic Gems.

  Of the 99 guarded swaps 3 * 22 are complete triplet sorts.

  An already sorted array incurs 77 comparisons (and zero swaps).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_25)(CPL_TYPE *self)
{
    /* All (9) step-1 triplets (2 truncated) */
    CPL_TYPE_SELF_SORT(0, 1);
    CPL_TYPE_SELF_SORT_3_7(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                           17, 18, 19, 20, 21, 22);
    CPL_TYPE_SELF_SORT(23, 24);

    /* All (9) step-3 triplets (2 truncated) */
    CPL_TYPE_SELF_SORT(2, 5);          /*  2 is 6-tuple (2..7) minimum */
    CPL_TYPE_SELF_SORT_3_7(0, 3, 6,    /*  0 is 6-tuple (0,1,3,4,6,7) minimum */
                           1, 4, 7,    /*  7 is 2+3+3-tuple (0- 7) maximum */
                           8, 11, 14,  /*  8 is 9-tuple (8-16) minimum */
                           9, 12, 15,  /*  9 is 6-tuple  min */
                           10, 13, 16, /* 16 is 9-tuple ( 8-16) maximum */
                           17, 20, 23, /* 17 is 3+3+2-tuple (17-24) minimum */
                           18, 21, 24); /* 18 is 2+2+1-tuple min */
    CPL_TYPE_SELF_SORT(19, 22);         /* 22 is 6-tuple (17-22) maximum */

    /* All (8) step-9 triplets (2 truncated) */
    CPL_TYPE_SELF_SORT(8, 17);         /* Cut  8 as 17-tuple (8-24) minimum */
    CPL_TYPE_SELF_SORT_3_4(0, 9, 18,   /* Cut  0 as 17-tuple minimum */
                           2, 11, 20,  /* Cut  2 as 17-tuple minimum */
                           4, 13, 22,  /* Cut 22 as 17-tuple maximum */
                           6, 15, 24); /* Cut 24 as 17-tuple maximum */
    CPL_TYPE_SELF_SORT(7, 16);         /* Cut 16 as 17-tuple (0-16) maximum */

    /* 19 samples remaining */
    CPL_TYPE_SELF_SORT_3_3(1, 10, 19,  /*  1 is 8-tuple minimum */
                           3, 12, 21,  /* 21 is 3+3+3-tuple maximum */
                           5, 14, 23); /* 23 is 8-tuple maximum */

    CPL_TYPE_SELF_SORT(7, 19);  /* Cut 19 as 8+3-tuple maximum */
    CPL_TYPE_SELF_SORT(15, 23); /* Cut 23 as 3+8-tuple maximum */

    /* 17 samples remaining */
    CPL_TYPE_SELF_SORT(13, 21); /* Cut 21 as 6+9-tuple maximum */
    CPL_TYPE_SELF_SORT(7, 13);  /* Cut 13 as 8+9-tuple maximum */

    /* 15 samples remaining */
    CPL_TYPE_SELF_SORT(7, 15); /* Cut 15 as 8+8-tuple maximum */
    CPL_TYPE_SELF_SORT(1, 9);  /* Cut 1 as 8+1-tuple minimum */
    CPL_TYPE_SELF_SORT(3, 11); /* Cut 3 as minimum */
    CPL_TYPE_SELF_SORT(5, 17); /* Cut 5 as minimum */

    CPL_TYPE_SELF_SORT(11, 17); /* Cut 11 as minimum */
    CPL_TYPE_SELF_SORT(9, 17);  /* Cut 9 as minimum */

    /* 9 samples remaining */
    CPL_TYPE_SELF_SORT(4, 10);
    CPL_TYPE_SELF_SORT(6, 12);
    CPL_TYPE_SELF_SORT(7, 14);
    CPL_TYPE_SELF_SORT(4, 6);
    CPL_TYPE_SELF_SORT(4, 7); /* Cut 4 as minimum */
    CPL_TYPE_SELF_SORT(12, 14);
    CPL_TYPE_SELF_SORT(10, 14); /* Cut 14 as maximum */
    /* 7 samples left */
    CPL_TYPE_SELF_SORT(6, 7);
    CPL_TYPE_SELF_SORT(10, 12);
    CPL_TYPE_SELF_SORT(6, 10);
    CPL_TYPE_SELF_SORT(6, 17); /* Cut 6 as minimum */
    CPL_TYPE_SELF_SORT(12, 17);
    CPL_TYPE_SELF_SORT(7, 17); /* Cut 17 as maximum */
    /* 5 samples left */
    CPL_TYPE_SELF_SORT(7, 10);
    CPL_TYPE_SELF_SORT(12, 18);
    CPL_TYPE_SELF_SORT(7, 12);  /* Cut 7 as minimum */
    CPL_TYPE_SELF_SORT(10, 18); /* Cut 18 as maximum */

    CPL_TYPE_SELF_SORT_3(10, 12, 20);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 26 elements (via 112 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_25_int()

  In any 15-tuple both the minimum and the maximum can be discarded into their
  respective, unsorted, lower and uppper half-sets.

  First compute the 25-median, then bubble the 26th element into place

  Incurs 13 + 99 = 112 guarded swaps (99 from the 25-sample).

 */
/*----------------------------------------------------------------------------*/
inline static void ADDTYPE(cpl_tools_get_median_26)(CPL_TYPE *self)
{
    /* First compute 25-median, on first 25 elements */
    ADDTYPE(cpl_tools_get_median_25)(self);

    /* Now bubble last element into place - assuming mostly sorted elements */
    if (self[25] < self[12]) {
        CPL_TYPE_SELF_CYCLE(12, 13, 25);
        /* Swap lower half-set's maximum element into lower median position */
        CPL_TYPE_SELF_SWAP_MAX_7(0, 1, 2, 3, 4, 5, 12);
        CPL_TYPE_SELF_SWAP_MAX_7(6, 7, 8, 9, 10, 11, 12);
    }
    else {
        /* Swap upper half-set's minimum element into upper median position */
        CPL_TYPE_SELF_SWAP_MIN_7(13, 14, 15, 16, 17, 18, 25);
        CPL_TYPE_SELF_SWAP_MIN_7(13, 19, 20, 21, 22, 23, 24);
    }
}

CPL_DIAG_PRAGMA_PUSH_IGN(-Wattributes)


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the arithmetic mean of an array
  @param    a     The array
  @param    n     The (positive) array size
  @return   The mean, S(n) = (1/n) sum(a_i) (i=1 -> n), or 0 on NULL input

  Compute the arithmetic mean of a dataset using the recurrence relation
     mean_(n) = mean(n-1) + (v[n] - mean(n-1))/(n+1)
     - this has a measurable impact on the output of
       cpl_polynomial_fit_{1,2}d_create()

 */
/*----------------------------------------------------------------------------*/
double ADDTYPE(cpl_tools_get_mean)(const CPL_TYPE *a, cpl_size nn)
{
    double mean = 0.0;
    const size_t n = (size_t)nn;
    size_t i;


    cpl_ensure(a != NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(nn > 0, CPL_ERROR_ILLEGAL_INPUT, 0.0);
    /* Ensure that the cast was OK */
    cpl_ensure((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE, 0.0);

    for (i = 0; i < n; i++)
        mean += ((double)a[i] - mean) / (double)(i + 1);

    cpl_tools_add_flops(3 * n);

    return mean;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the summed sample variance of an array
  @param    a     The array
  @param    n     The (non-negative) array size
  @param    pmean Iff non-NULL, *pmean is the mean (at no extra cost)
  @return   The summed sample variance, S(n) = sum((a_i-mean)^2) (i=1 -> n)
  @note Even with rounding errors the returned result is always non-negative

Math explanation for ticket DFS05126, written by Lander de Bilbao.


$$\sigma2 = \frac{\sum_{i=1}^N (x_i - \overline{x})2}{N-1}$$


We concentrate on how to compute

$$\sum_{i=1}^N (x_i - \overline{x})2$$

developed as follows

$$\sum_{i=1}^N (x_i - \overline{x})2 = \sum_{i=1}^N x_i2 - 2 \, \overline{x} \, \sum_{i=1}^N x_i + N \, \overline{x}2$$

as 

$$ \overline{x} = \frac{\sum_{i=1}^N x_i}{N}$$

then we have 

$$\sum_{i=1}^N (x_i - \overline{x})2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}2$$


Now we look and see if it possible to, after doing this computation for N samples, add the contribution of a new sample

We call the new sample $x_{n+1}$, and the mean taken into account this new sample, $\overline{x}_{n+1}$. For clarity, we rewrite the previous equation with $\overline{x}$ renamed as $\overline{x}_n$

$$\sum_{i=1}^N (x_i - \overline{x}_n)2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}_n2$$

We want to compute now

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2$$

Developed

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^{N+1} x_i2 - (N+1) \, \overline{x}_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - (N+1) \, \overline{x}_{n+1}2$$

as

$$\overline{x}_{n+1} = \frac{\sum_{i=1}^N x_i + x_{n+1}}{N+1} $$

$$\overline{x}_{n+1} = \frac{\overline{x}_n \, N + x_{n+1}}{N+1} $$

then we have

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - (N+1) \, ( \, \frac{\overline{x}_n \, N + x_{n+1}}{N+1} ) \, ^2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{( \, \overline{x}_n \, N + x_{n+1} ) \, ^2}{N+1}$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{( \, N2 \, \overline{x}_n2 + 2 \, N \, \overline{x}_n \, x_{n+1} + x_{n+1}2 \, )}{N+1}$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{N2}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} - \frac1{N+1} \, x_{n+1}2$$

we add in both parts of the equation

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{N2}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} - \frac1{N+1} \, x_{n+1}2 + \frac1{N+1} \, \overline{x}_n2$$

we can now group some terms

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 - \frac{N2 - 1}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 - (N-1) \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}_n2 + \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}_n2 + \frac{N}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 = \sum_{i=1}^N (x_i - \overline{x}_n)2 + \frac{N}{N+1} \,( \, \overline{x}_n2 - 2 \, \overline{x}_n \, x_{n+1} + x_{n+1}2 )$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 = \sum_{i=1}^N (x_i - \overline{x}_n)2 + \frac{N}{N+1} \,( \, \overline{x}_n - x_{n+1} )2 $$

 */
/*----------------------------------------------------------------------------*/
double ADDTYPE(cpl_tools_get_variancesum)(const CPL_TYPE *a,
                                          cpl_size nn,
                                          double *pmean)
{
    double varsum = 0.0;
    double mean = 0.0;
    const size_t n = (size_t)nn;
    size_t i;

    cpl_ensure(a != NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(nn >= 0, CPL_ERROR_ILLEGAL_INPUT, 0.0);
    /* Ensure that the cast was OK */
    cpl_ensure((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE, 0.0);

    for (i = 0; i < n; i++) {
        const double delta = (double)a[i] - mean;

        varsum += (double)i * delta * (delta / (double)(i + 1));
        mean += delta / (double)(i + 1);
    }

    cpl_tools_add_flops(1 + 6 * n); /* Assume expression reuse */

    if (pmean != NULL)
        *pmean = mean;

    return varsum;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the sample variance of an array
  @param    a     The array
  @param    n     The (positive) array size
  @param    pmean Iff non-NULL, *pmean is the mean (at no extra cost)
  @return   The sample variance, S(n) = (1/n) sum((a_i-mean)^2) (i=1 -> n)
  @see cpl_tools_get_variancesum_double()
 */
/*----------------------------------------------------------------------------*/
double ADDTYPE(cpl_tools_get_variance)(const CPL_TYPE *a,
                                       cpl_size n,
                                       double *pmean)
{
    const double varsum = ADDTYPE(cpl_tools_get_variancesum)(a, n, pmean);

    cpl_ensure(n > 0, CPL_ERROR_ILLEGAL_INPUT, 0.0);

    return varsum / (double)n;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Compare two numerical values for qsort()
  @param  p1 Pointer to the 1st value
  @param  p2 Pointer to the 2nd value
  @return 1, 0, -1 depending on whether *p1 is smaller than, equal to or
          greater than *p2.
  @see qsort()
  @note Since the function is not exported its error checking is disabled

 */
/*----------------------------------------------------------------------------*/
static int ADDTYPE(compar_ascn)(const void *p1, const void *p2)
{
    const CPL_TYPE a1 = *(const CPL_TYPE *)p1;
    const CPL_TYPE a2 = *(const CPL_TYPE *)p2;

    return a1 < a2 ? -1 : (a1 > a2 ? 1 : 0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Compare two numerical values for qsort()
  @param  p1 Pointer to the 1st value
  @param  p2 Pointer to the 2nd value
  @return -1, 0, 1 depending on whether *p1 is smaller than, equal to or
          greater than *p2.
  @see qsort()
  @note Since the function is not exported its error checking is disabled

 */
/*----------------------------------------------------------------------------*/
static int ADDTYPE(compar_desc)(const void *p1, const void *p2)
{
    const CPL_TYPE a1 = *(const CPL_TYPE *)p1;
    const CPL_TYPE a2 = *(const CPL_TYPE *)p2;

    return a1 < a2 ? 1 : (a1 > a2 ? -1 : 0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the median of a numerical array
  @param  self The array to permute and request from
  @param  nn    The number of elements in the array
  @param  kk    The requested value position in the sorted array, zero for 1st
  @return The median of the partially sorted array.
  @see cpl_tools_get_median_int
  @note Since the function is not exported its error checking is disabled
  @note Benchmarking up to 10M randomized elements on a Xeon E5345 show no
        significant advantage over cpl_tools_get_kth. However, for an almost
        sorted array, Quickselect can be a lot faster.

    The Quickselect algorithm is derived from the Quicksort algorithm, see e.g.

    https://en.wikipedia.org/wiki/Quickselect

    Comments on the below implementation and its performance can be found at
  
    http://ndevilla.free.fr/median/median/

 */
/*----------------------------------------------------------------------------*/
CPL_TYPE
ADDTYPE(cpl_tools_quickselection)(CPL_TYPE *self, cpl_size nn, cpl_size kk)
{
    const size_t n = (size_t)nn;
    const size_t k = (size_t)kk;
    size_t low = 0;
    size_t high = n - 1;

#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(nn > 0, CPL_ERROR_ILLEGAL_INPUT, (CPL_TYPE)0);
    cpl_ensure(nn > kk - 1, CPL_ERROR_ILLEGAL_INPUT, (CPL_TYPE)0);
    /* Ensure that the cast was OK */
    cpl_ensure((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE, 0.0);
    cpl_ensure((cpl_size)k == kk, CPL_ERROR_UNSUPPORTED_MODE, 0.0);
#endif

    /* Control flow has been changed to a single return at the end */

    for (; low + 1 < high;) {
        /* Find median of low, middle and high items; swap into position low */
        const size_t middle = low + (high - low) / 2;
        size_t ll = low + 1;
        size_t hh = high;

        CPL_TYPE_SELF_SORT_3(middle, low, high);

        /* Swap low item (now in position middle) into position (low+1) */
        CPL_TYPE_SELF_SWAP(middle, low + 1);

        /* Nibble from each end towards middle, swapping items when stuck */
        for (;;) {
            do
                ll++;
            while (self[low] > self[ll]);
            do
                hh--;
            while (self[hh] > self[low]);

            if (hh < ll)
                break;

            CPL_TYPE_SELF_SWAP(ll, hh);
        }

        /* Swap middle item (in position low) back into correct position */
        CPL_TYPE_SELF_SWAP(low, hh);

        /* Re-set active partition */
        if (hh <= k)
            low = ll;
        if (hh >= k)
            high = hh - 1;
    }

    if (high == low + 1) { /* Two elements only */
        CPL_TYPE_SELF_SORT(low, high);
    }

    return self[k];
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the kth smallest value in a numerical array
  @param  self The array to permute and request from
  @param  n    The number of elements in the array
  @param  k    The requested value position in the sorted array, zero for 1st
  @return The kth smallest value in the partially sorted array.
  @note Since the function is not exported its error checking is disabled
  @see cpl_tools_get_median_int

  After a successful call, self is permuted so elements less than the kth have
  lower indices, while elements greater than the kth have higher indices.

  Reference:

  Author: Wirth, Niklaus 
  Title: Algorithms + data structures = programs 
  Publisher: Englewood Cliffs: Prentice-Hall, 1976 
  Physical description: 366 p. 
  Series: Prentice-Hall Series in Automatic Computation 

  See also: http://ndevilla.free.fr/median/median/

 */
/*----------------------------------------------------------------------------*/
CPL_TYPE
ADDTYPE(cpl_tools_get_kth)(CPL_TYPE *self, cpl_size n, cpl_size k)
{
    register cpl_size l = 0;
    register cpl_size m = n - 1;
    register cpl_size i = l;
    register cpl_size j = m;

#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(k >= 0, CPL_ERROR_ILLEGAL_INPUT, (CPL_TYPE)0);
    cpl_ensure(k < n, CPL_ERROR_ACCESS_OUT_OF_RANGE, (CPL_TYPE)0);
#endif

    while (l < m) {
        register const CPL_TYPE x = self[k];

        do {
            while (self[i] < x)
                i++;
            while (x < self[j])
                j--;
            if (i <= j) {
                CPL_TYPE_SELF_SWAP(i, j);
                i++;
                j--;
            }
        } while (i <= j);

        /* assert( j < i ); */

        /* The original implementation has two index comparisons and
           two, three or four index assignments. This has been reduced
           to one or two index comparisons and two index assignments.
        */

        if (k <= j) {
            /* assert( k < i ); */
            m = j;
            i = l;
        }
        else {
            if (k < i) {
                m = j;
            }
            else {
                j = m;
            }
            l = i;
        }
    }
    return self[k];
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array into ascending order using qsort
  @param    self The array to sort
  @param    n    The number of array elements
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note Since the function is not exported its NULL-pointer checking is disabled

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
void ADDTYPE(cpl_tools_sort_ascn)(CPL_TYPE *self, int n)
{
    qsort(self, n, sizeof(*self), ADDTYPE(compar_ascn));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array into ascending order using qsort
  @param    self The array to sort
  @param    n    The number of array elements
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note Since the function is not exported its NULL-pointer checking is disabled

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
void ADDTYPE(cpl_tools_sort_desc)(CPL_TYPE *self, int n)
{
    qsort(self, (size_t)n, sizeof(*self), ADDTYPE(compar_desc));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array
  @param    self     The array to sort
  @param    n        The number of array elements
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note Since the function is not exported its NULL-pointer checking is disabled

  On a nearly sorted array, this function is a LOT slower than qsort()
  - but on "normal" random data, it can be several times faster.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
cpl_error_code ADDTYPE(cpl_tools_sort)(CPL_TYPE *self, cpl_size nn)
{
    const size_t n = (size_t)nn;
    size_t i, ir, j, k, l;
    size_t i_stack[CPL_PIX_STACK_SIZE];
    size_t j_stack;
    CPL_TYPE a;

    /* Ensure that the cast was OK */
    cpl_ensure_code((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE);

    ir = n;
    l = 1;
    j_stack = 0;
    for (;;) {
        if (ir - l < 7) {
            for (j = l + 1; j <= ir; j++) {
                a = self[j - 1];
                for (i = j - 1; i >= 1; i--) {
                    if (self[i - 1] <= a)
                        break;
                    self[i] = self[i - 1];
                }
                self[i] = a;
            }
            if (j_stack == 0)
                break;
            ir = i_stack[j_stack-- - 1];
            l = i_stack[j_stack-- - 1];
        }
        else {
            k = (l + ir) >> 1;
            CPL_TYPE_SELF_SWAP(k - 1, l);
            CPL_TYPE_SELF_SORT_3(l, l - 1, ir - 1);
            i = l + 1;
            j = ir;
            a = self[l - 1];
            for (;;) {
                do
                    i++;
                while (self[i - 1] < a);
                do
                    j--;
                while (self[j - 1] > a);
                if (j < i)
                    break;
                CPL_TYPE_SELF_SWAP(i - 1, j - 1);
            }
            self[l - 1] = self[j - 1];
            self[j - 1] = a;
            j_stack += 2;
            cpl_ensure_code(j_stack <= CPL_PIX_STACK_SIZE,
                            CPL_ERROR_ILLEGAL_INPUT);

            if (ir - i + 1 >= j - l) {
                i_stack[j_stack - 1] = ir;
                i_stack[j_stack - 2] = i;
                ir = j - 1;
            }
            else {
                i_stack[j_stack - 1] = j - 1;
                i_stack[j_stack - 2] = l;
                l = i;
            }
        }
    }
    return CPL_ERROR_NONE;
}

#undef CPL_TYPE_SELF_SWAP
#undef CPL_TYPE_SELF_SORT
#undef CPL_TYPE_SELF_SORT_3
#undef CPL_TYPE_SELF_SORT_3_A
#undef CPL_TYPE_SELF_SORT_3_C

/* End of CPL_TYPE_IS_NUM */

#endif


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array
  @param    self   The numerical array to sort
  @param    n      The number of array elemenst
  @param    reverse      flag indicating whether to sort ascending (zero) or
                         descending (non-zero)
  @param    stable       flag to indicate whether to guarantee stability at
                         the cost of execution time (if cpl_tools_sort_int is
                         O(n log n), this function would still be O(n log n))
  @param    sort_pattern resulting sort pattern
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  The heap sort algorithm used here
  - has bad cache performance, but
  - is worst case O(n log n)
  - is stable (meaning that the order of equal elements is conserved 
               indepent on the value of the reverse flag)
  - is in place

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
cpl_error_code ADDTYPE(cpl_tools_sort_stable_pattern)(CPL_TYPE const *self,
                                                      cpl_size n,
                                                      int reverse,
                                                      int stable,
                                                      cpl_size *sort_pattern)
{
    cpl_size i;

    /* Check entries */
    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(sort_pattern, CPL_ERROR_NULL_INPUT);

    if (n == 0)
        return CPL_ERROR_NONE;

    for (i = 0; i < n; i++) {
        sort_pattern[i] = i;
    }

    /* 
     * Heap sort
     */
    for (i = n / 2 - 1; i >= 0; i--) {
        int done = 0;
        cpl_size root = i;
        cpl_size bottom = n - 1;

        while ((root * 2 + 1 <= bottom) && (!done)) {
            cpl_size child = root * 2 + 1;

            if (child + 1 <= bottom) {
                if ((!reverse &&
                     CPL_TOOLS_SORT_LT(self[sort_pattern[child]],
                                       self[sort_pattern[child + 1]])) ||
                    (reverse && CPL_TOOLS_SORT_LT(self[sort_pattern[child + 1]],
                                                  self[sort_pattern[child]]))) {
                    child += 1;
                }
            }

            if ((!reverse && CPL_TOOLS_SORT_LT(self[sort_pattern[root]],
                                               self[sort_pattern[child]])) ||
                (reverse && CPL_TOOLS_SORT_LT(self[sort_pattern[child]],
                                              self[sort_pattern[root]]))) {
                CPL_INT_SWAP(sort_pattern[root], sort_pattern[child]);
                root = child;
            }
            else {
                done = 1;
            }
        }
    }

    for (i = n - 1; i >= 1; i--) {
        int done = 0;
        cpl_size root = 0;
        cpl_size bottom = i - 1;
        CPL_INT_SWAP(sort_pattern[0], sort_pattern[i]);

        while ((root * 2 + 1 <= bottom) && (!done)) {
            cpl_size child = root * 2 + 1;

            if (child + 1 <= bottom) {
                if ((!reverse &&
                     CPL_TOOLS_SORT_LT(self[sort_pattern[child]],
                                       self[sort_pattern[child + 1]])) ||
                    (reverse && CPL_TOOLS_SORT_LT(self[sort_pattern[child + 1]],
                                                  self[sort_pattern[child]]))) {
                    child += 1;
                }
            }
            if ((!reverse && CPL_TOOLS_SORT_LT(self[sort_pattern[root]],
                                               self[sort_pattern[child]])) ||
                (reverse && CPL_TOOLS_SORT_LT(self[sort_pattern[child]],
                                              self[sort_pattern[root]]))) {
                CPL_INT_SWAP(sort_pattern[root], sort_pattern[child]);
                root = child;
            }
            else {
                done = 1;
            }
        }
    }

    /* 
     * Enforce stability
     */
    if (stable) {
        for (i = 0; i < n; i++) {
            cpl_size j;
            j = i + 1;
            while (j < n &&
                   !CPL_TOOLS_SORT_LT(self[sort_pattern[i]],
                                      self[sort_pattern[j]]) &&
                   !CPL_TOOLS_SORT_LT(self[sort_pattern[j]],
                                      self[sort_pattern[i]])) {
                j++;
            }
            if (j - i > 1) {
                cpl_tools_sort_cplsize(sort_pattern + i, j - i);
            }
            i = j - 1;
        }
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Apply a permutation to one or two arrays
  @param   self   The permutation array, this is destroyed by the call
  @param   n      The (positive) number of elements to permute
  @param   awrite The array to hold the permuted A-array
  @param   aread  The array to hold the input A-array, may equal awrite
  @param   bwrite The array to hold the permuted B-array, or NULL
  @param   bread  The array to hold the input B-array, may equal awrite or NULL
  @return  CPL_ERROR_NONE on success or the relevant #_cpl_error_code_ on error
  @note self is destroyed by the call. bread and bwrite must both be either
        NULL or non-NULL

  In-place permutation is done in O(n) time with O(1) extra storage
  using the fact that any permutation can be decomposed into a sequence
  of cyclic permutations.

  Uni-cycles are processed as well, to support in-place permuting of
  only one of the two arrays. This means that the below code also
  works for out-of-place permuting.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT         self, aread or awrite is NULL
  - CPL_ERROR_ILLEGAL_INPUT      size is not positive
  - CPL_ERROR_INCOMPATIBLE_INPUT only one of bread/bwrite is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code ADDTYPE(cpl_tools_permute)(cpl_size *self,
                                          cpl_size n,
                                          CPL_TYPE *awrite,
                                          CPL_TYPE const *aread,
                                          CPL_TYPE *bwrite,
                                          CPL_TYPE const *bread)
{
    cpl_size ido = 0; /* First element in cycle to process */
    const cpl_boolean dob = bwrite != NULL;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(n > 0, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(awrite != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(aread != NULL, CPL_ERROR_NULL_INPUT);

    if (dob) {
        cpl_ensure_code(bread != NULL, CPL_ERROR_INCOMPATIBLE_INPUT);
    }
    else {
        cpl_ensure_code(bread == NULL, CPL_ERROR_INCOMPATIBLE_INPUT);
    }

    do {
        /* Save last pair in cycle */
        CPL_TYPE const aread0 = aread[ido];
        CPL_TYPE const bread0 = dob ? bread[ido] : 0;
        cpl_size ifrom = ido;

        while (self[ifrom] != ido) { /* Test here to avoid uni-cycles */
            /* Copy the pair first to support cross-wrapped bivectors */
            CPL_TYPE const areadj = aread[self[ifrom]];
            CPL_TYPE const breadj = dob ? bread[self[ifrom]] : 0;
            const cpl_size j = ifrom;

            ifrom = self[ifrom]; /* Point to next in cycle */

            assert(ifrom != -1); /* Can fail only on non-perm array */

            awrite[j] = areadj;
            if (dob)
                bwrite[j] = breadj;
            self[j] = -1; /* This position now has the right value */
        }
        /* Cycle is finished, copy and flag last pair */
        awrite[ifrom] = aread0;
        if (dob)
            bwrite[ifrom] = bread0;
        self[ifrom] = -1;

        /* Find start of next cycle */
        while (++ido < n && self[ido] < 0)
            ;
    } while (ido < n);

    return CPL_ERROR_NONE;
}
