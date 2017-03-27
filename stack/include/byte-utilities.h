/**
 * @file byte-utilities.h
 * @brief Data store and fetch routines.
 * See @ref byte-utilities for documentation.
 *
 * <!--Copyright 2014 Silicon Laboratories, Inc.                         *80*-->
 */

/**
 * @addtogroup stack_info
 *
 * See byte-utilities.h for source code.
 * @{
 */

/** @brief This function copies an array of bytes and reverses the order
 *  before writing the data to the destination.
 *
 * @param dest A pointer to the location where the data will be copied to.
 * @param src A pointer to the location where the data will be copied from.
 * @param length The length (in bytes) of the data to be copied.
 */
void emberReverseMemCopy(int8u* dest, const int8u* src, int8u length);

/** @brief Returns the value built from the two \c int8u values
 *  \c contents[0] and \c contents[1]. \c contents[0] is the low byte.
 */
XAP2B_PAGEZERO_ON
int16u emberFetchLowHighInt16u(const int8u *contents);
XAP2B_PAGEZERO_OFF

/** @brief Returns the value built from the two \c int8u values
 *  \c contents[0] and \c contents[1]. \c contents[0] is the high byte.
 */
XAP2B_PAGEZERO_ON
int16u emberFetchHighLowInt16u(const int8u *contents);
XAP2B_PAGEZERO_OFF

/** @brief Stores \c value in \c contents[0] and \c contents[1]. \c
 *  contents[0] is the low byte.
 */
void emberStoreLowHighInt16u(int8u *contents, int16u value);

/** @brief Stores \c value in \c contents[0] and \c contents[1]. \c
 *  contents[0] is the high byte.
 */
void emberStoreHighLowInt16u(int8u *contents, int16u value);

#if !defined DOXYGEN_SHOULD_SKIP_THIS
int32u emFetchInt32u(boolean lowHigh, int8u* contents);
#endif

/** @brief Returns the value built from the four \c int8u values
 *  \c contents[0], \c contents[1], \c contents[2] and \c contents[3]. \c
 *  contents[0] is the low byte.
 */
#if defined DOXYGEN_SHOULD_SKIP_THIS
int32u emberFetchLowHighInt32u(int8u *contents);
#else
#define emberFetchLowHighInt32u(contents) \
  (emFetchInt32u(TRUE, contents))
#endif

/** @description Returns the value built from the four \c int8u values
 *  \c contents[0], \c contents[1], \c contents[2] and \c contents[3]. \c
 *  contents[3] is the low byte.
 */
#if defined DOXYGEN_SHOULD_SKIP_THIS
int32u emberFetchHighLowInt32u(int8u *contents);
#else
#define emberFetchHighLowInt32u(contents) \
  (emFetchInt32u(FALSE, contents))
#endif

#if !defined DOXYGEN_SHOULD_SKIP_THIS
void emStoreInt32u(boolean lowHigh, int8u* contents, int32u value);
#endif

/** @brief Stores \c value in \c contents[0], \c contents[1], \c
 *  contents[2] and \c contents[3]. \c contents[0] is the low byte.
 */
#if defined DOXYGEN_SHOULD_SKIP_THIS
void emberStoreLowHighInt32u(int8u *contents, int32u value);
#else
#define emberStoreLowHighInt32u(contents, value) \
  (emStoreInt32u(TRUE, contents, value))
#endif

/** @description Stores \c value in \c contents[0], \c contents[1], \c
 *  contents[2] and \c contents[3]. \c contents[3] is the low byte.
 */
#if defined DOXYGEN_SHOULD_SKIP_THIS
void emberStoreHighLowInt32u(int8u *contents, int32u value);
#else
#define emberStoreHighLowInt32u(contents, value) \
  (emStoreInt32u(FALSE, contents, value))
#endif

/** @} END addtogroup */
