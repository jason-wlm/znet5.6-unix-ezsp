// Copyright 2014 Silicon Laboratories, Inc.
#ifndef __KEY_MATRIX_DRIVER_H__
#define __KEY_MATRIX_DRIVER_H__

#include PLATFORM_HEADER

/** @brief Initializes the key-matrix driver. This function is automatically 
 * called by ::halInit().
 */
void halKeyMatrixInitialize(void);

/** @brief Returns true if any key is held down
 *
 * This function checks if any inputs are being pulled low. indicating that a
 * key is pressed
 *
 * Note: This function must be run after halKeyMatrixPrepareForSleep() and 
 * before halKeyMatrixRestoreFromSleep() as it requires all output pins to be 
 * set to output high to detect key presses. 
 *
 * @param none
 */
boolean halKeyMatrixAnyKeyDown(void);

/** @brief Scans the key-matrix matrix and stores result
 *
 * This function scans the actual state of the key-matrix. 
 *
 * @param none
 */
void halKeyMatrixScan(void);

/** @brief Scans one key and  stores result
 *
 * This function scans the actual state a key. 
 *
 * @param key = input*NUM_OUTPUTS + output%NUM_OUTPUTS
 */
void halKeyMatrixScanSingle(int8u key);

/** @brief Get key-matrix scan state for a particular key.
 *
 *
 * @param key = input*NUM_OUTPUTS + output%NUM_OUTPUTS
 *
 * @return  FALSE if the is key released and TRUE if key is pressed
 *
 */
boolean halKeyMatrixGetValue(int8u key);

/** @brief Config key-matrix GPIO pins before sleep, i.e. output pins set to
 * output driving to active level.
 *
 * @param none
 */
void halKeyMatrixPrepareForSleep(void);

/** @brief Config key-matrix GPIO pins after sleep, i.e. input pins set as
 * input with internal pull resistors pulling to inactive level
 *
 * @param none
 */
void halKeyMatrixRestoreFromSleep(void);

#endif // __KEY_MATRIX_DRIVER_H__

