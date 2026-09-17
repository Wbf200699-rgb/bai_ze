/*! ----------------------------------------------------------------------------
 *  @file    algorithm.h
 *  @brief   Declarations of ranging/angle algorithms (PDOA, Kalman filter)
 *
 *  Shared by main.c and instance.c so prototypes stay consistent.
 */
#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <stdint.h>

double calc_aoa(uint8_t src, int16_t pdoa1, int16_t pdoa2);
void kf_twr_init(float dt);
float kf_twr_update(uint8_t src, float meas_d);

#endif /* ALGORITHM_H */
