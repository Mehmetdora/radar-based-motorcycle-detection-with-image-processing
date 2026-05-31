/*
 * IIR_DSP.h
 *
 *  Created on: May 8, 2026
 *      Author: mehmet_dora
 */

#ifndef MODULES_IIR_DSP_H_
#define MODULES_IIR_DSP_H_

#include "stdint.h"

typedef struct {
	float alpha;
	float out;
} FirstOrderIIR;


void FirstOrderIIR_Init (FirstOrderIIR *filt, float alpha) ;
float FirstOrderIIR_Update (FirstOrderIIR *filt, uint16_t in) ;

#endif /* MODULES_IIR_DSP_H_ */
