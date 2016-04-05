/******************************************************************
 *
 * Copyright 2016 MP All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Description:
 * Static list of legit resources types currently specified by OIC.
 * Specified as given in OIC Resource Type Specification 1.1.0
 ******************************************************************/

#ifndef RESOURCE_TYPES_H_
#define RESOURCE_TYPES_H_

using namespace std;

//
// Resource Devices
//
const static char* OIC_DEVICE_LIGHT                      = "oic.d.light";
const static char* OIC_DEVICE_SENSOR                     = "oic.d.sensor";
const static char* OIC_DEVICE_BUTTON                     = "oic.d.button";
const static char* OIC_DEVICE_TV                         = "oic.d.tv";
const static char* OIC_DEVICE_THERMOSTAT                 = "oic.d.theromstat";
const static char* OIC_DEVICE_FAN                        = "oic.d.fan";
// More toc ome...

//
// Light Resource Types
//

/**
 * @brief OIC_TYPE_LIGHT_BRIGHTNESS
 *
 * @param Brightness:       integer
 */
const static char* OIC_TYPE_LIGHT_BRIGHTNESS           = "oic.r.light.brightness";

/**
 * @brief OIC_TYPE_LIGHT_DIMMING
 *
 * @param dimmingSetting    integer
 * @param step              integer
 * @param range             char*
 */
const static char* OIC_TYPE_LIGHT_DIMMING              = "oic.r.light.dimming";

/**
 * @brief OIC_TYPE_LIGHT_RAMP_TIME
 *
 * @param rampTime          integer
 * @param range             char*
 */
const static char* OIC_TYPE_LIGHT_RAMP_TIME            = "oic.r.light.rampTime";

//
// Sensor Resource Types
//

/**
 * @brief OIC_TYPE_HUMIDTY
 *
 * @param humidity          integer
 * @param desiredHumidity   integer
 */
const static char* OIC_TYPE_HUMIDTY                    = "oic.r.humidty";

/**
 * @brief OIC_TYPE_TEMPERATURE
 *
 * @param temperature       number
 * @param units             enum ["C", "F", "K"]
 * @param range             char*
 */
const static char* OIC_TYPE_TEMPERATURE                = "oic.r.temperature";

//
// Generic Resource Types
//

/**
 * @brief OIC_TYPE_SENSOR_ACTITIVTY_COUNT
 *
 * @param count             integer
 */
const static char* OIC_TYPE_SENSOR_ACTITIVTY_COUNT     = "oic.r.sensor.activity.count";

/**
 * @brief OIC_TYPE_SENSOR_PRESSURE
 */
const static char* OIC_TYPE_SENSOR_PRESSURE            = "oic.r.sensor.atmosphericPressure";

/**
 * @brief OIC_TYPE_SENSOR_CONTACT
 *
 * @param value             boolean
 */

/**
 * @brief OIC_TYPE_SENSOR_CONTACT
 *
 * @param value             boolean
 */
const static char* OIC_TYPE_SENSOR_CONTACT             = "oic.r.sensor.contact";

/**
 * @brief OIC_TYPE_SENSOR_ILLUMINANCE
 *
 * @param illuminance       number
 */
const static char* OIC_TYPE_SENSOR_ILLUMINANCE         = "oic.r.sensor.illuminance";

/**
 * @brief OIC_TYPE_SENSOR_MOTION
 *
 * @para mvalue              boolean
 */
const static char* OIC_TYPE_SENSOR_MOTION              = "oic.r.sensor.motion";

/**
 * @brief OIC_TYPE_SENSOR_PRESENCE
 *
 * @param value             boolean
 */
const static char* OIC_TYPE_SENSOR_PRESENCE            = "oic.r.sensor.presence";

/**
 * @brief OIC_TYPE_SENSOR_TOUCH
 *
 * @param value             boolean
 */
const static char* OIC_TYPE_SENSOR_TOUCH               = "oic.r.sensor.touch";

/**
 * @brief OIC_TYPE_SENSOR_GLASSBREAK
 *
 * @param value             boolean
 */
const static char* OIC_TYPE_SENSOR_GLASSBREAK          = "oic.r.sensor.glassBreak";

/**
 * @brief OIC_TYPE_SENSOR_HEARTRATE
 *
 * @param heartRateZone     enum["Zone1", "Zone2", "Zone3", "Zone4", "Zone5"]
 */
const static char* OIC_TYPE_SENSOR_HEARTRATE           = "oic.r.sensor.heart.zone";

//
// Media Resource Types
//

/**
 * @brief OIC_TYPE_MEDIA
 *
 * @param url               char*
 * @param sdp               array char* (Array of SDP media or attribute line).
 */
const static char* OIC_TYPE_MEDIA                      = "oic.r.media";

/**
 * @brief OIC_TYPE_MEDIA_SOURCE
 *
 * @param sourceName        char*
 * @param sourceNumber      integer
 * @param sourceType        enum ["audioOnly", "videoOnly", "audioPlusVideo"]
 * @param status            boolean
 */
const static char* OIC_TYPE_MEDIA_SOURCE               = "oic.r.media.source";

/**
 * @brief OIC_TYPE_AUDIO
 *
 * @param volume            integer
 * @param mute              boolean
 */
const static char* OIC_TYPE_AUDIO                      = "oic.r.audio";


//
// Button Resource Types
//
/**
 * @brief OIC_TYPE_BINARY_SWITCH
 *
 * @param value             boolean
 */
const static char* OIC_TYPE_BINARY_SWITCH              = "oic.r.binary.switch";

//
// Service Resource Types
//
const static char* OIC_TYPE_RESOURCE_HOST              = "oic.r.resourcehosting";

//
// Color Resource Types
//

/**
 * @brief OIC_TYPE_COLOUR_CHROMA
 *
 * @param hue               integer
 * @param saturation        integer
 * @param colourspacevalue  char*
 */
const static char* OIC_TYPE_COLOUR_CHROMA              = "oic.r.colour.chroma";

/**
 * @brief OIC_TYPE_COLOUR_RGB
 *
 * @param rgbValue          char*
 * @param range             char*
 */
const static char* OIC_TYPE_COLOUR_RGB                 = "oic.r.colour.rgb";

/**
 * @brief OIC_TYPE_COLOUR_SATURATION
 *
 * @param colourSaturation  integer (min: 0, max: 100)
 */
const static char* OIC_TYPE_COLOUR_SATURATION          = "oic.r.colour.saturation";

/**
 * @brief OIC_TYPE_COLOR_AUTOWHITEBALANCE
 *
 * @param autoWhiteBalance  boolean
 */
const static char* OIC_TYPE_COLOR_AUTOWHITEBALANCE     = "oic.r.colour.autowhitebalance";

//
// Energi Resources Types
//
const static char* OIC_TYPE_ENERGY_OVERLOAD            = "oic.r.energy.overload";
const static char* OIC_TYPE_ENERGY_DRLC                = "oic.r.energy.drlc";
const static char* OIC_TYPE_ENERGY_USAGE               = "oic.r.energy.usage";
const static char* OIC_TYPE_ENERGY_CONSUMPTION         = "oic.r.energy.consumption";



#endif /* RESOURCE_TYPES_H_ */
