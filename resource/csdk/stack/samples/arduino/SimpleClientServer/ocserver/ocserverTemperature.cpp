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

#include "OCBaseResource.h"
#include "ResourceTypes.h"
#include "/home/markpovlsen/Documents/iotivity/extlibs/arduino/arduino-1.5.8/libraries/TaskScheduler/src/TaskScheduler.h"

static const int DELAY_TIME_INPUT_THREAD = 1;      // ms
static const int INPUT_THREAD_TASK_INTERVAL = 10;  // ms

const char *getResult(OCStackResult result);

// Blinking LED
static const char LED_PIN = 13;
static const char TEST_LED_PIN = 5; // PWM Pin
static const char TEST_BUT_PIN = 2;

bool buttonPrevValue = false;

// Tasks
/*
void checkInputThread();
void aliveThread();

Scheduler runner;
Task inputTask(INPUT_THREAD_TASK_INTERVAL, TASK_FOREVER, &checkInputThread);
Task ledTask(100, TASK_FOREVER, &aliveThread);*/

#define TAG "ArduinoServer"

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

        // Check if it's being observed
        if(*underObservation && buttonPrevValue != readValue)
        {
            OIC_LOG(DEBUG, TAG, "BUTTON: Notifying observers");
            //OCNotifyAllObservers(handle, OC_NA_QOS);
        }

        buttonPrevValue = readValue;
    }
    else
    {
        OIC_LOG(ERROR, TAG, "BUTTON is read only!");
    }
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

                char* value = *((char**) current->value.data);
                OIC_LOG_V(DEBUG, TAG, "Value received is: %s", value);
                if(strcmp(value, "on"))
                {
                    digitalWrite(attribute->port->pin, LOW);
                }
                else if (strcmp(value, "off"))
                {
                    digitalWrite(attribute->port->pin, HIGH);
                }

                if(attribute)
                {
                    *((char**)attribute->value.data) = value;
                }
                /*uint8_t port = 5;
                if(value >= 255)
                {
                    analogWrite(port, 255);
                }
                else if(value < 0)
                {
                    analogWrite(port, 0);
                }
                else
                {
                    OIC_LOG_V(DEBUG, TAG, "Value analogWrite is: %i", value);
                    analogWrite(current->port->pin, value);
                }*/
                // TODO
                if(*underObservation)
                {
                    OIC_LOG(DEBUG, TAG, "LIGHT: Notifying observers");
                    OCNotifyAllObservers(handle, OC_NA_QOS);
                }
            }

            current = current->next;
        }
    }
   // OIC_LOG(DEBUG, TAG, "Leaving light handler");
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

    // DEBUG PIN
    pinMode(LED_PIN, OUTPUT);

    // Button resource
   /* OCBaseResourceT *buttonResource = createResource("/a/button", OIC_DEVICE_BUTTON, OC_RSRVD_INTERFACE_DEFAULT,
                                                  (OC_DISCOVERABLE | OC_OBSERVABLE), buttonIOHandler);

    buttonResource->name = "Marks Button";

    OCIOPort port;
    port.pin = TEST_BUT_PIN;
    port.type = IN;

    Value buttonValue = malloc(sizeof(bool));
    bool boolVal = false;
    *((bool*)buttonValue) = boolVal;
    addAttribute(&buttonResource->attribute, "state", buttonValue, BOOL, &port);

    printResourceData(buttonResource);*/

    OCBaseResourceT *resourceLight = createResource("/a/light", OIC_DEVICE_LIGHT, OC_RSRVD_INTERFACE_DEFAULT,
                                               (OC_DISCOVERABLE | OC_OBSERVABLE), lightIOHandler);
    resourceLight->name = "Mark's Light";

    addType(resourceLight, OIC_TYPE_BINARY_SWITCH);
    addType(resourceLight, OIC_TYPE_LIGHT_BRIGHTNESS);

    OCIOPort portLight;
    portLight.pin = TEST_LED_PIN; // LED_PIN for debug
    portLight.type = OUT;

    Value valueLight = malloc(sizeof(char*));
    char* charVal = "off";
    *((char**)valueLight) = charVal;
    addAttribute(&resourceLight->attribute, "power", valueLight, STRING, &portLight);

    valueLight = malloc(sizeof(int));
    *((int*)valueLight) = 0;
    addAttribute(&resourceLight->attribute, "brightness", valueLight, INT, &portLight);

    printResourceData(resourceLight);


    // Set tup the tasks
   /* runner.init();
    OIC_LOG(DEBUG, TAG, "Scheduler initialized");

    //runner.addTask(inputTask);
    OIC_LOG(DEBUG, TAG, "Added inputthread Task");

    runner.addTask(ledTask);

    inputTask.enable();
    //OIC_LOG(DEBUG, TAG, "Inputthread enabled");

    ledTask.enable();*/

}

// The loop function is called in an endless loop
void loop()
{
    // This artificial delay is kept here to avoid endless spinning
    // of Arduino microcontroller. Modify it as per specific application needs.
    delay(2000);

    // This call displays the amount of free SRAM available on Arduino
    //PrintArduinoMemoryStats();

    // Give CPU cycles to OCStack to perform send/recv and other OCStack stuff
    if (OCProcess() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, ("OCStack process error"));
        return;
    }

    //runner.execute();
}
