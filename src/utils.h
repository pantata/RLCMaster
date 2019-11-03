/*
 * utils.h
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>

/**
* Milliseconds data type \n
* Data type				- Max time span			- Memory used \n
* unsigned char			- 255 milliseconds		- 1 byte \n
* unsigned int			- 65.54 seconds			- 2 bytes \n
* unsigned long			- 49.71 days			- 4 bytes \n
* unsigned long long	- 584.9 million years	- 8 bytes
*/
typedef unsigned long millis_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
* Initialise, must be called before anything else!
*
* @return (none)
*/
void millis_init(void);

/**
* Get milliseconds.
*
* @return Milliseconds.
*/
millis_t millis(void);



/**
* Map ()
*
*/
long map(long x, long in_min, long in_max, long out_min, long out_max);
uint16_t crc16_update(uint16_t crc, uint8_t a);


#ifdef __cplusplus
}
#endif

#endif /* UTILS_H_ */
