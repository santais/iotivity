//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ocbaseresource.h"
#include "resource_types.h"

static const int DELAY_TIME_INPUT_THREAD = 100;      // ms

// Blinking LED
static const char LED_PIN = 13;
static const char TEST_LED_PIN = 5; // PWM Pin

#define TAG "ArduinoServer"

#ifdef ARDUINOWIFI
// Arduino WiFi Shield
// Note : Arduino WiFi Shield currently does NOT support multicast and therefore
// this server will NOT be listening on 224.0.1.187 multicast address.

static const char ARDUINO_WIFI_SHIELD_UDP_FW_VER[] = "1.1.0";
	
/// WiFi Shield firmware with Intel patches
static const char INTEL_WIFI_SHIELD_FW_VER[] = "1.2.0";

/// WiFi network info and credentials
char ssid[] = "PussyCat24";
char pass[] = "benterikpovls";

int ConnectToNetwork()
{
    char *fwVersion;
    int status = WL_IDLE_STATUS;
    pinMode(9, OUTPUT);      // set the LED pin mode
    delay(1000);
    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD)
    {
        OIC_LOG(ERROR, TAG, ("WiFi shield not present"));
        return -1;
    }

    // Verify that WiFi Shield is running the firmware with all UDP fixes
    fwVersion = WiFi.firmwareVersion();
    OIC_LOG_V(INFO, TAG, "WiFi Shield Firmware version %s", fwVersion);
    if ( strncmp(fwVersion, ARDUINO_WIFI_SHIELD_UDP_FW_VER, sizeof(ARDUINO_WIFI_SHIELD_UDP_FW_VER)) !=0 )
    {
        OIC_LOG(DEBUG, TAG, ("!!!!! Upgrade WiFi Shield Firmware version !!!!!!"));
        return -1;
    }

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED)
    {
        OIC_LOG_V(INFO, TAG, "Attempting to connect to SSID: %s", ssid);
        status = WiFi.begin(ssid,pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    OIC_LOG(DEBUG, TAG, ("Connected to wifi"));

    IPAddress ip = WiFi.localIP();
    OIC_LOG_V(INFO, TAG, "IP Address:  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return 0;
}
#else
// Arduino Ethernet Shield
int ConnectToNetwork()
{
    // Note: ****Update the MAC address here with your shield's MAC address****
    uint8_t ETHERNET_MAC[] = {0x90, 0xA2, 0xDA, 0x10, 0x29, 0xE2}; 
    uint8_t error = Ethernet.begin(ETHERNET_MAC);
    if (error  == 0)
    {
        OIC_LOG_V(ERROR, TAG, "error is: %d", error);
        return -1;
    }

    IPAddress ip = Ethernet.localIP();
    OIC_LOG_V(INFO, TAG, "IP Address:  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return 0;
}
#endif //ARDUINOWIFI

// On Arduino Atmel boards with Harvard memory architecture, the stack grows
// downwards from the top and the heap grows upwards. This method will print
// the distance(in terms of bytes) between those two.
// See here for more details :
// http://www.atmel.com/webdoc/AVRLibcReferenceManual/malloc_1malloc_intro.html
void PrintArduinoMemoryStats()
{
    //#ifdef ARDUINO_AVR_MEGA2560
    //This var is declared in avr-libc/stdlib/malloc.c
    //It keeps the largest address not allocated for heap
    extern char *__brkval;
    //address of tmp gives us the current stack boundry
    int tmp;
    OIC_LOG_V(INFO, TAG, "Stack: %u         Heap: %u", (unsigned int)&tmp, (unsigned int)__brkval);
    OIC_LOG_V(INFO, TAG, "Unallocated Memory between heap and stack: %u",
            ((unsigned int)&tmp - (unsigned int)__brkval));
   // #endif
}

void printAttribute(OCAttributeT *attributes)
{
    OIC_LOG(DEBUG, TAG, "Attributes :");
    OCAttributeT *current = attributes;
    while(current != NULL)
    {
        OIC_LOG_V(DEBUG, TAG, "Name: %s", current->name);
        switch(current->value.dataType)
        {
        case INT:
            OIC_LOG_V(DEBUG, TAG, "Value: %i", current->value.data.i);
            //OIC_LOG_V(DEBUG, TAG, "Value: %i", *((int*)current->value.data));
            break;
        case DOUBLE:
            OIC_LOG_V(DEBUG, TAG, "Value: %f", current->value.data.d);
            //OIC_LOG_V(DEBUG, TAG, "Value: %f", *((double*)current->value.data));
            break;
            break;
        case BOOL:
            OIC_LOG_V(DEBUG, TAG, "Value: %s", current->value.data.b ? "true" : "false");
           /* bool boolean = *((bool*) current->value.data);
            OIC_LOG_V(DEBUG, TAG, "Value: %s", boolean ? "true" : "false");*/
            break;
        case STRING:
            OIC_LOG_V(DEBUG, TAG, "Value: %s", current->value.data.str);
            //OIC_LOG_V(DEBUG, TAG, "Value: %s", *((char**)current->value.data));
            break;
        }
        current = current->next;
    }
    OIC_LOG(DEBUG, TAG, "Done printing attributes!");
}

void printResource(OCBaseResourceT *resource)
{
    OIC_LOG(DEBUG, TAG, "=============================");
    OIC_LOG_V(DEBUG, TAG, "Resource URI: %s", resource->uri);
    OIC_LOG_V(DEBUG, TAG, "Handle of the resource: %p", (void*) resource->handle);

    OIC_LOG(DEBUG, TAG, "Resource Types: ");
    OCResourceType *currentType = resource->type;
    while(currentType != NULL)
    {
        OIC_LOG_V(DEBUG, TAG, "\t%s", currentType->resourcetypename);
        currentType = currentType->next;
    }

    OIC_LOG(DEBUG, TAG, "Resource Interfaces: ");
    OCResourceInterface *currentInterface = resource->interface;
    while(currentInterface != NULL)
    {
        OIC_LOG_V(DEBUG, TAG, "\t%s", currentInterface->name);
        currentInterface = currentInterface->next;
    }

    printAttribute(resource->attribute);
    OIC_LOG(DEBUG, TAG, "=============================");
}


void lightIOHandler(OCAttributeT *attribute, int IOType, OCResourceHandle handle,
                    bool *underObservation)
{
    if(IOType == OUTPUT)
    {
        bool power(false);
        int brightness(0);
       // OIC_LOG(DEBUG, TAG, "LightIOHandler: OUTPUT");
        OCAttributeT *current = attribute;
        while(current != NULL)
        {
            //OIC_LOG_V(DEBUG, TAG, "Attribute name: %s", current->name);
            //OIC_LOG(DEBUG, TAG, "Searching light");
            if(strcmp(current->name, "power") == 0)
            {

                power = current->value.data.b;
                //OIC_LOG_V(DEBUG, TAG, "Power value received is: %s", power ? "true" : "false");

                if(attribute)
                {
                    attribute->value.data.b = power;
                }
            }
            else if (strcmp(current->name, "brightness") == 0)
            {
                brightness = current->value.data.i;
                //OIC_LOG_V(DEBUG, TAG, "Brightness value set to: %i", brightness);

                if(attribute)
                {
                    attribute->value.data.i = brightness;
                }
            }

            current = current->next;
        }
        if(power)
        {
            analogWrite(attribute->port->pin, brightness);
        }
        else
        {
            analogWrite(attribute->port->pin, 0);
        }

        if(*underObservation)
        {
            OIC_LOG(DEBUG, TAG, "LIGHT: Notifying observers");
            if(OCNotifyAllObservers(handle, OC_LOW_QOS) == OC_STACK_NO_OBSERVERS)
            {
                OIC_LOG(DEBUG, TAG, "No more observers!");
                *underObservation = false;
            }
        }
    }
}


//The setup function is called once at startup of the sketch
void setup()
{
    // Add your initialization code here
    // Note : This will initialize Serial port on Arduino at 115200 bauds
   	OIC_LOG_INIT();
    OIC_LOG(DEBUG, TAG, ("OCServer is starting..."));


    // Connect to Ethernet or WiFi network
    if (ConnectToNetwork() != 0)
    {
    	Serial.print("Unable to connect to Network");
        OIC_LOG(ERROR, TAG, ("Unable to connect to network"));
        return;
    }

    delay(1000);

    // Initialize the OC Stack in Server mode
    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, ("OCStack init error"));
        return;
    }

    // DEBUG PIN
    pinMode(LED_PIN, OUTPUT);

    OIC_LOG(DEBUG, TAG, "Creating resource");
    // Light resource
    OCBaseResourceT *resourceLight = createResource("/a/light", OIC_DEVICE_LIGHT, OC_RSRVD_INTERFACE_DEFAULT,
                                              (OC_DISCOVERABLE | OC_OBSERVABLE), lightIOHandler);
    resourceLight->name = "Mark's Light";

    addType(resourceLight, OIC_TYPE_BINARY_SWITCH);
    addType(resourceLight, OIC_TYPE_LIGHT_BRIGHTNESS);

    OCIOPort portLight;
    portLight.pin = TEST_LED_PIN; // LED_PIN for debug
    portLight.type = OUT;

    ResourceData power;
    power.b = true;
    addAttribute(&resourceLight->attribute, "power", power, BOOL, &portLight);

    ResourceData brightness;
    brightness.i = 255;
    addAttribute(&resourceLight->attribute, "brightness", brightness, INT, &portLight);

    printResource(resourceLight);

    OIC_LOG(DEBUG, TAG, "Finished setup");
}

// The loop function is caplled in an endless loop
void loop()
{
    // This artificial delay is kept here to avoid endless spinning
    // of Arduino microcontroller. Modify it as per specific application needs.
    delay(DELAY_TIME_INPUT_THREAD);
    //checkInputThread();

    // This call displays the amount of free SRAM available on Arduino
    //PrintArduinoMemoryStats();

    // Give CPU cycles to OCStack to perform send/recv and other OCStack stuff
    if (OCProcess() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, ("OCStack process error"));
        return;
    }

    //yield();
}
