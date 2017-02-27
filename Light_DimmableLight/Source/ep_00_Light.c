#include <jendefs.h>
#include "zps_gen.h"
#include "AppHardwareApi.h"
#include "App_Light_DimmableLight.h"
#include "app_zcl_light_task.h"
#include "app_common.h"
#include "dbg.h"
#include <string.h>
#include "os.h"
#include "app_light_interpolation.h"
#include "DriverBulb_Shim.h"
#include "ep_Light.h"


static tsZLL_DimmableLightDevice sLight;
static tsIdentifyWhite sIdEffect;

PUBLIC void ep_00_Register(tfpZCL_ZCLCallBackFunction fptr){

	eZLL_RegisterDimmableLightEndPoint(LIGHT_DIMMABLELIGHT_LIGHT_00_ENDPOINT, fptr, &sLight);

	/* Initialise the strings in Basic */
	memcpy(sLight.sBasicServerCluster.au8ManufacturerName, "PeeVeeOne", CLD_BAS_MANUF_NAME_SIZE);
	memcpy(sLight.sBasicServerCluster.au8ModelIdentifier, "PeeVeeOne", CLD_BAS_MODEL_ID_SIZE);
	memcpy(sLight.sBasicServerCluster.au8DateCode, "201601106", CLD_BAS_DATE_SIZE);
	memcpy(sLight.sBasicServerCluster.au8SWBuildID, "1000-9999", CLD_BAS_SW_BUILD_SIZE);
}


PUBLIC void ep_00_Initialise(){

	sLight.sLevelControlServerCluster.u8CurrentLevel = 0xFE;
	sLight.sOnOffServerCluster.bOnOff = TRUE;
	sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
	sIdEffect.u8Tick = 0;
}

PUBLIC void ep_00_SetCurentBulbState(){

	ep_00_SetBulbState(sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel);
}

PUBLIC void ep_00_SetBulbState(bool bOn, uint8 u8Level){

	if (bOn)
	{
		vLI_Start(u8Level);
	}
	else
	{
		vLI_Stop();
	}
	vBULB_SetOnOff(bOn);

}

PUBLIC void ep_00_SetIdentifyTime(uint16 u16Time){

	sLight.sIdentifyServerCluster.u16IdentifyTime = u16Time;
}

PUBLIC bool ep_00_IsIdentifying(){

	return sLight.sIdentifyServerCluster.u16IdentifyTime != 0;
}

PUBLIC void ep_00_HandleIdentify( ){

	uint16 u16Time = sLight.sIdentifyServerCluster.u16IdentifyTime;

	static bool bActive = FALSE;

	if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT) {
		/* do nothing */
		//DBG_vPrintf(TRACE_LIGHT_TASK, "Efect do nothing\n");
	}
	else if (u16Time == 0)
	{
		ep_00_SetBulbState( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel);
		bActive = FALSE;
	}
	else
	{
		/* Set the Identify levels */
		if (!bActive) {
			bActive = TRUE;
			sIdEffect.u8Level = 250;
			sIdEffect.u8Count = 5;
			ep_00_SetBulbState( TRUE, CLD_LEVELCONTROL_MAX_LEVEL );
		}
	}

}

PUBLIC void ep_00_IdEffectTick(){

	if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
	{
		if (sIdEffect.u8Tick > 0)
		{
			//DBG_vPrintf(TRACE_PATH, "\nPath 5");

			sIdEffect.u8Tick--;
			/* Set the light parameters */

			ep_00_SetBulbState(TRUE, sIdEffect.u8Level);

			/* Now adjust parameters ready for for next round */
			switch (sIdEffect.u8Effect) {
			case E_CLD_IDENTIFY_EFFECT_BLINK:
				break;

			case E_CLD_IDENTIFY_EFFECT_BREATHE:
				if (sIdEffect.bDirection) {
					if (sIdEffect.u8Level >= 250) {
						sIdEffect.u8Level -= 50;
						sIdEffect.bDirection = 0;
					} else {
						sIdEffect.u8Level += 50;
					}
				} else {
					if (sIdEffect.u8Level == 0) {
						// go back up, check for stop
						sIdEffect.u8Count--;
						if ((sIdEffect.u8Count) && ( !sIdEffect.bFinish)) {
							sIdEffect.u8Level += 50;
							sIdEffect.bDirection = 1;
						} else {
							//DBG_vPrintf(TRACE_LIGHT_TASK, "\n>>set tick 0<<");
							/* lpsw2773 - stop the effect on the next tick */
							sIdEffect.u8Tick = 0;
						}
					} else {
						sIdEffect.u8Level -= 50;
					}
				}
				break;
			case E_CLD_IDENTIFY_EFFECT_OKAY:
				if ((sIdEffect.u8Tick == 10) || (sIdEffect.u8Tick == 5)) {
					sIdEffect.u8Level ^= 254;
					if (sIdEffect.bFinish) {
						sIdEffect.u8Tick = 0;
					}
				}
				break;
			case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
				if ( sIdEffect.u8Tick == 74) {
					sIdEffect.u8Level = 1;
					if (sIdEffect.bFinish) {
						sIdEffect.u8Tick = 0;
					}
				}
				break;
			default:
				if ( sIdEffect.bFinish ) {
					sIdEffect.u8Tick = 0;
				}
			}
		} else {
			/*
			 * Effect finished, restore the light
			 */

			sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
			sIdEffect.bDirection = FALSE;

			ep_00_SetIdentifyTime(0);

			ep_00_SetBulbState( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel);
		}
	} else {
		if (sLight.sIdentifyServerCluster.u16IdentifyTime) {
			sIdEffect.u8Count--;
			if (0 == sIdEffect.u8Count) {
				sIdEffect.u8Count = 5;
				if (sIdEffect.u8Level) {
					sIdEffect.u8Level = 0;
					ep_00_SetBulbState( FALSE, 0);
				}
				else
				{
					sIdEffect.u8Level = 250;

					ep_00_SetBulbState( TRUE, CLD_LEVELCONTROL_MAX_LEVEL);
				}
			}
		}
	}
}

PUBLIC void ep_00_StartEffect(uint8 u8Effect){

	switch (u8Effect) {
	case E_CLD_IDENTIFY_EFFECT_BLINK:
		sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BLINK;
		if (sLight.sOnOffServerCluster.bOnOff) {
			sIdEffect.u8Level = 0;
		} else {
			sIdEffect.u8Level = 250;
		}
		sIdEffect.bFinish = FALSE;
		sLight.sIdentifyServerCluster.u16IdentifyTime = 2;
		sIdEffect.u8Tick = 10;
		break;
	case E_CLD_IDENTIFY_EFFECT_BREATHE:
		sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BREATHE;
		sIdEffect.bDirection = 1;
		sIdEffect.bFinish = FALSE;
		sIdEffect.u8Level = 0;
		sIdEffect.u8Count = 15;
		sLight.sIdentifyServerCluster.u16IdentifyTime = 17;

		sIdEffect.u8Tick = 200;
		break;
	case E_CLD_IDENTIFY_EFFECT_OKAY:
		sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_OKAY;
		sIdEffect.bFinish = FALSE;
		if (sLight.sOnOffServerCluster.bOnOff) {
			sIdEffect.u8Level = 0;
		} else {
			sIdEffect.u8Level = 254;
		}
		sLight.sIdentifyServerCluster.u16IdentifyTime = 3;
		sIdEffect.u8Tick = 15;
		break;
	case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
		sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE;
		sIdEffect.u8Level = 254;
		sIdEffect.bFinish = FALSE;
		sLight.sIdentifyServerCluster.u16IdentifyTime = 9;
		sIdEffect.u8Tick = 80;
		break;

	case E_CLD_IDENTIFY_EFFECT_FINISH_EFFECT:
		if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
		{
			sIdEffect.bFinish = TRUE;
		}
		break;
	case E_CLD_IDENTIFY_EFFECT_STOP_EFFECT:
		sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
		sLight.sIdentifyServerCluster.u16IdentifyTime = 1;

		break;
	}
}
