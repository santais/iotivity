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

#include <malloc.h>

#include "../include/OCBaseResource.h"
#include <Scheduler.h>

static const int DELAY_TIME_INPUT_THREAD = 100;      // ms

const char *getResult(OCStackResult result);

// Blinking LED
static const char LED_PIN = 13;
static const char TEST_LED_PIN = 0;
static const char TEST_BUT_PIN = 4;

extern char _end;
extern "C" char *sbrk(int i);
char *ramstart=(char *)0x20070000;
char *ramend=(char *)0x20088000;

#define TAG "ArduinoServer"

int gLightUnderObservation = 0;
void createLightResource();

#ifdef ARDUINOWIFI
// Arduino WiFi Shield
// Note : Arduino WiFi Shield currently does NOT support multicast and therefore
// this server will NOT be listening on 224.0.1.187 multicast address.

static const char ARDUINO_WIFI_SHIELD_UDP_FW_VER[] = "1.1.0";
	
/// WiFi Shield firmware with Intel patches
static const char INTEL_WIFI_SHIELD_FW_VER[] = "1.2.0";

/// WiFi network info and credentials
char ssid[] = "mDNSAP";
char pass[] = "letmein9";

int ConnectToNetwork()
{
    char *fwVersion;
    int status = WL_IDLE_STATUS;
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

void lightIOHandler(OCAttributeT *attribute, int IOType, OCResourceHandle handle,
                    bool *underObservation)
{
    if(IOType == OUTPUT)
    {
       // OIC_LOG(DEBUG, TAG, "LightIOHandler: OUTPUT");
        OCAttributeT *current = attribute;
        while(current != NULL)
        {
            //OIC_LOG(DEBUG, TAG, "Searching light");
            if(strcmp(current->name, "power") == 0)
            {
                digitalWrite(current->port->pin, *((int*)current->value.data));

                // TODO
              /*  if(underObservation)
                {
                    OIC_LOG(DEBUG, TAG, "LIGHT: Notifying observers");
                    OCNotifyAllObservers(handle, OC_NA_QOS);
                }*/
            }
            current = current->next;
        }
    }
   // OIC_LOG(DEBUG, TAG, "Leaving light handler");
}

void buttonIOHandler(OCAttributeT *attribute, int IOType, OCResourceHandle handle,
                     bool *underObservation)
{
    if(IOType == INPUT)
    {
       // OIC_LOG(DEBUG, TAG, "ButtonIOHandler: INPUT");
        bool readValue(false);
        if(digitalRead(attribute->port->pin))
        {
            readValue = true;
        }
        else
        {
            readValue = false;
        }

        if(attribute)
        {
            *((bool*)attribute->value.data) = readValue;
        }
    }
    else
    {
        OIC_LOG(ERROR, TAG, "BUTTON is read only!");
    }
}

void checkInputThread()
{
    //OIC_LOG(DEBUG, TAG, "Checking input thread");

    // Search through added resources
    OCBaseResourceT *current = getResourceList();

    while(current != NULL)
    {
        if(current->attribute->port->type == IN)
        {
            //OIC_LOG_V(DEBUG, TAG, "Found resource with name: %s", current->name);
            //OIC_LOG_V(DEBUG, TAG, "checkInputThread Observation: %s", current->underObservation ? "true" : "false");
            current->OCIOhandler(current->attribute, INPUT, current->handle, &current->underObservation);
        }
        current = current->next;
    }

    delay(DELAY_TIME_INPUT_THREAD);
}

void aliveThread()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}

//The setup function is called once at startup of the sketch
void setup()
{
    // Add your initialization code here
    // Note : This will initialize Serial port on Arduino at 115200 bauds
   	OIC_LOG_INIT();
    OIC_LOG(DEBUG, TAG, ("OCServer is starting..."));

    // mDNSAP
    pinMode(LED_PIN, OUTPUT);
    pinMode(TEST_LED_PIN, OUTPUT);
    pinMode(TEST_BUT_PIN, INPUT);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(TEST_LED_PIN, LOW);

    // Connect to Ethernet or WiFi network
    if (ConnectToNetwork() != 0)
    {
    	Serial.print("Unable to connect to Network");
        OIC_LOG(ERROR, TAG, ("Unable to connect to network"));
        return;
    }

    // Initialize the OC Stack in Server mode
    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, ("OCStack init error"));
        return;
    }


    OCBaseResourceT *newResource = createResource("/a/button", "oic.r.button", OC_RSRVD_INTERFACE_DEFAULT,
                                                  (OC_DISCOVERABLE | OC_OBSERVABLE), buttonIOHandler);

    newResource->name = "Marks Button";
/*
    OCIOPort port;
    port.pin = TEST_BUT_PIN;
    port.type = IN;

    Value value = malloc(sizeof(int));
    int intVal = 0;
    *((int*)value) = intVal;
    addAttribute(&newResource->attribute, "power", value, INT, &port);*/

    //printResourceData(newResource);

    OCBaseResourceT *resource = createResource("/a/light/hosting", "oic.r.light", OC_RSRVD_INTERFACE_DEFAULT,
                                               (OC_DISCOVERABLE | OC_OBSERVABLE), lightIOHandler);
    resource->name = "Mark's Light";

    addType(resource, "oic.r.switch.binary");
    addType(resource, "oic.r.light.brightness");
    addType(resource, "oic.r.resourcehosting");

    OCIOPort port;
    port.pin = TEST_LED_PIN;
    port.type = OUT;

    Value value = malloc(sizeof(bool));
    bool boolVal = false;
    *((bool*)value) = boolVal;
    addAttribute(&resource->attribute, "state", value, BOOL, &port);

    value = malloc(sizeof(int));
    int intVal = 0;
    *((int*)value) = intVal;
    addAttribute(&resource->attribute, "power", value, INT, &port);

    //printResourceData(resource);

 /*
    OCBaseResourceT *humidtyResource = createResource("/a/temperatureSensor", "oic.r.sensor", OC_RSRVD_INTERFACE_DEFAULT,
                                    (OC_DISCOVERABLE | OC_OBSERVABLE), NULL);
*/


    // Start the thread to take for change in the input of the resources
    Scheduler.startLoop(checkInputThread);

    // Alive LED
    Scheduler.startLoop(aliveThread);

   /* char *heapend=sbrk(0);
    register char * stack_ptr asm ("sp");
    struct mallinfo mi=mallinfo();
    OIC_LOG_V(DEBUG, TAG, "\nDynamic ram used: %d\n",mi.uordblks);
    OIC_LOG_V(DEBUG, TAG, "Program static ram used %d\n",&_end - ramstart);
    OIC_LOG_V(DEBUG, TAG, "Stack ram used %d\n\n",ramend - stack_ptr);
    OIC_LOG_V(DEBUG, TAG, "My guess at free mem: %d\n",stack_ptr - heapend + mi.fordblks);*/
}

// The loop function is called in an endless loop
void loop()
{
    // This artificial delay is kept here to avoid endless spinning
    // of Arduino microcontroller. Modify it as per specific application needs.
    delay(100);

    // This call displays the amount of free SRAM available on Arduino
    //PrintArduinoMemoryStats();

    // Give CPU cycles to OCStack to perform send/recv and other OCStack stuff
    if (OCProcess() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, ("OCStack process error"));
        return;
    }

    yield();
}
