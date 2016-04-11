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
#include "easysetup.h"

static const int DELAY_TIME_INPUT_THREAD = 1000;      // ms

// Blinking LED
static const char LED_PIN = 13;
static const char TEST_LED_PIN = 5; // PWM Pin

/**
 * @var g_OnBoardingSucceeded
 * @brief This variable will be set if OnBoarding is successful
 */
static bool g_OnBoardingSucceeded = false;

/**
 * @var g_ProvisioningSucceeded
 * @brief This variable will be set if Provisioning is successful
 */
static bool g_ProvisioningSucceeded = false;


#define TAG "ArduinoServer"

// Functions
void EventCallbackInApp(ESResult esResult, ESEnrolleeState enrolleeState);
ESResult StartEasySetup();
void ESInitResources();
void createLightResource();

// Arduino WiFi Shield
// Note : Arduino WiFi Shield currently does NOT support multicast and therefore
// this server will NOT be listening on 224.0.1.187 multicast address.

/// WiFi network info and credentials
char g_ssid[] = "EasySetup123";
char g_pass[] = "EasySetup123";

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

void EventCallbackInApp(ESResult esResult, ESEnrolleeState enrolleeState)
{
    Serial.println("callback!!! in app");

    if(esResult == ES_OK)
    {
        if(!g_OnBoardingSucceeded){
            Serial.println("Device is successfully OnBoarded");
            g_OnBoardingSucceeded = true;
        }
        else if(g_OnBoardingSucceeded & enrolleeState == ES_ON_BOARDED_STATE){
            Serial.println("Device is successfully OnBoared with SoftAP");
            g_ProvisioningSucceeded = true;
        }

        if(enrolleeState == ES_PROVISIONED_STATE && esResult == ES_OK)
        {
            // Create the Light resource.
            createLightResource();
        }
    }
    else if (esResult == ES_ERROR)
    {
        if(g_OnBoardingSucceeded)
        {
            OIC_LOG_V(ERROR, TAG, "Failure in Provisioning. \
                                        Current Enrollee State: %d",enrolleeState);
            g_OnBoardingSucceeded = false;
        }
        else if(g_ProvisioningSucceeded)
        {
            OIC_LOG_V(ERROR, TAG, "Failure in connect to target network. \
                                        Current Enrollee State: %d",enrolleeState);
            g_ProvisioningSucceeded = false;
        }
    }
}

void startProvisioning()
{
    OIC_LOG(DEBUG, TAG, "ESInitResources is invoked...");

    // Initialize the OC Stack in Server mode
    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack init error!!");
        return;
    }

    if (ESInitProvisioning() == ES_ERROR)
    {
        OIC_LOG(ERROR, TAG, "Init Provisioning Failed!!");
        return;
    }
}

ESResult StartEasySetup()
{
    OIC_LOG(DEBUG, TAG, "OCServer is starting...");

    //ESInitEnrollee with sercurity mode disabled for arduino
    if(ESInitEnrollee(CT_ADAPTER_IP, g_ssid, g_pass, false, EventCallbackInApp) == ES_ERROR)
    {
        OIC_LOG(ERROR, TAG, "OnBoarding Failed");
        return ES_ERROR;
    }

    OIC_LOG_V(ERROR, TAG, "OnBoarding succeded. Successfully connected to ssid : %s",ssid);
    return ES_OK;
}

void createLightResource()
{
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


//The setup function is called once at startup of the sketch
void setup()
{
    // Add your initialization code here
    // Note : This will initialize Serial port on Arduino at 115200 bauds
   	OIC_LOG_INIT();
    OIC_LOG(DEBUG, TAG, ("OCServer is starting..."));

    // Start the onboarding process
    while(StartEasySetup() != ES_OK);

    // Start provisioning
    startProvisioning();
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
