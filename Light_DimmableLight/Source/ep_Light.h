/*
 * EP_Light.h
 *
 *  Created on: Feb 14, 2017
 *      Author: Peter Visser
 */

#ifndef EP_LIGHT_H_
#define EP_LIGHT_H_

#include "dimmable_light.h"
#include "commission_endpoint.h"
#include <jendefs.h>

PUBLIC void ep_00_SetCurentBulbState();
PUBLIC void ep_00_SetBulbState(bool bOn, uint8 u8Level);
PUBLIC void ep_00_SetIdentifyTime(uint16 u16Time);
PUBLIC void ep_00_Init(tfpZCL_ZCLCallBackFunction fptr);
PUBLIC bool ep_00_IsIdentifying();
PUBLIC void ep_00_HandleIdentify( );
PUBLIC void ep_00_IdEffectTick() ;
PUBLIC void ep_00_StartEffect(uint8 u8Effect);



#endif /* EP_LIGHT_H_ */
