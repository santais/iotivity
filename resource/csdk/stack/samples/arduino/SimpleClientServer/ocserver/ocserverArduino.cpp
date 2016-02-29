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

#include "ocserverArduino.h"


const int LED_PIN = 13;

//string length of "/a/light/" + std::numeric_limits<int>::digits10 + '\0'"
// 9 + 9 + 1 = 19
const int URI_MAXSIZE = 19;

static int gObserveNotifyType = 3;

int gQuitFlag = 0;
int gLightUnderObservation = 0;

static LightResource Light;
// This variable determines instance number of the Light resource.
// Used by POST method to create a new instance of Light resource.
static int gCurrLightInstance = 0;

static LightResource gLightInstance[SAMPLE_MAX_NUM_POST_INSTANCE];

Observers interestedObservers[SAMPLE_MAX_NUM_OBSERVATIONS];

char *gResourceUri = "/a/light";
const char *dateOfManufacture = "myDateOfManufacture";
const char *deviceName = "myDeviceName";
const char *deviceUUID = "myDeviceUUID";
const char *firmwareVersion = "myFirmwareVersion";
const char *manufacturerName = "myName";
const char *operatingSystemVersion = "myOS";
const char *hardwareVersion = "myHardwareVersion";
const char* platformID = "myPlatformID";
const char *manufacturerUrl = "myManufacturerUrl";
const char *modelNumber = "myModelNumber";
const char *platformVersion = "myPlatformVersion";
const char *supportUrl = "mySupportUrl";
const char *version = "myVersion";
const char *systemTime = "2015-05-15T11.04";

// Entity handler should check for resourceTypeName and ResourceInterface in order to GET
// the existence of a known resource
const char *resourceTypeName = "core.light";
const char *resourceInterface = OC_RSRVD_INTERFACE_DEFAULT;

OCPlatformInfo platformInfo;
OCDeviceInfo deviceInfo;

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
        OC_LOG(ERROR, TAG, ("WiFi shield not present"));
        return -1;
    }

    // Verify that WiFi Shield is running the firmware with all UDP fixes
    fwVersion = WiFi.firmwareVersion();
    OC_LOG_V(INFO, TAG, "WiFi Shield Firmware version %s", fwVersion);
    if ( strncmp(fwVersion, ARDUINO_WIFI_SHIELD_UDP_FW_VER, sizeof(ARDUINO_WIFI_SHIELD_UDP_FW_VER)) !=0 )
    {
        OC_LOG(DEBUG, TAG, ("!!!!! Upgrade WiFi Shield Firmware version !!!!!!"));
        return -1;
    }

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED)
    {
        OC_LOG_V(INFO, TAG, "Attempting to connect to SSID: %s", ssid);
        status = WiFi.begin(ssid,pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    OC_LOG(DEBUG, TAG, ("Connected to wifi"));

    IPAddress ip = WiFi.localIP();
    OC_LOG_V(INFO, TAG, "IP Address:  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
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
        OC_LOG_V(ERROR, TAG, "error is: %d", error);
        return -1;
    }

    IPAddress ip = Ethernet.localIP();
    OC_LOG_V(INFO, TAG, "IP Address:  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return 0;
}
#endif //ARDUINOWIFI

OCRepPayload* getPayload(const char* uri, int64_t power, bool state)
{
    OCRepPayload* payload = OCRepPayloadCreate();
    if(!payload)
    {
        OC_LOG(ERROR, TAG, "Failed to allocate Payload");
        return NULL;
    }

    OCRepPayloadSetUri(payload, uri);
    OCRepPayloadSetPropBool(payload, "state", state);
    OCRepPayloadSetPropInt(payload, "power", power);

    return payload;
}

//This function takes the request as an input and returns the response
OCRepPayload* constructResponse(OCEntityHandlerRequest *ehRequest)
{
    if(ehRequest->payload && ehRequest->payload->type != PAYLOAD_TYPE_REPRESENTATION)
    {
        OC_LOG(ERROR, TAG, "Incoming payload not a representation");
        return NULL;
    }

    OCRepPayload* input = (OCRepPayload*) (ehRequest->payload);

    LightResource *currLightResource = &Light;

    if (ehRequest->resource == gLightInstance[0].handle)
    {
        currLightResource = &gLightInstance[0];
        gResourceUri = (char *) "a/light/0";
    }
    else if (ehRequest->resource == gLightInstance[1].handle)
    {
        currLightResource = &gLightInstance[1];
        gResourceUri = (char *) "a/light/1";
    }

    if(OC_REST_PUT == ehRequest->method)
    {
        // Get pointer to query
        int64_t pow;
        if(OCRepPayloadGetPropInt(input, "power", &pow))
        {
            currLightResource->power =pow;
        }

        bool state;
        if(OCRepPayloadGetPropBool(input, "state", &state))
        {
            currLightResource->state = state;
        }
    }

    return getPayload(gResourceUri, currLightResource->power, currLightResource->state);
}


/*
 * Very simple example of query parsing.
 * The query may have multiple filters separated by ';'.
 * It is upto the entity handler to parse the query for the individual filters,
 * VALIDATE them and respond as it sees fit.

 * This function only returns false if the query is exactly "power<X" and
 * current power is greater than X. If X cannot be parsed for an int,
 * true is returned.
 */
bool checkIfQueryForPowerPassed(char * query)
{
    if (query && strncmp(query, "power<", strlen("power<")) == 0)
    {
        char * pointerToOperator = strstr(query, "<");

        if (pointerToOperator)
        {
            int powerRequested = atoi(pointerToOperator + 1);
            if (Light.power > powerRequested)
            {
                OC_LOG_V(INFO, TAG, "Current power: %d. Requested: <%d", Light.power
                            , powerRequested);
                return false;
            }
        }
    }
    return true;
}

/*
 * Application should validate and process these as desired.
 */
OCEntityHandlerResult ValidateQueryParams (OCEntityHandlerRequest *entityHandlerRequest)
{
    OC_LOG_V(INFO, TAG, "Received query %s", entityHandlerRequest->query);
    OC_LOG(INFO, TAG, "Not processing query");
    return OC_EH_OK;
}

OCEntityHandlerResult ProcessGetRequest (OCEntityHandlerRequest *ehRequest,
        OCRepPayload **payload)
{
    OCEntityHandlerResult ehResult;
    bool queryPassed = checkIfQueryForPowerPassed(ehRequest->query);

    // Empty payload if the query has no match.
    if (queryPassed)
    {
        OCRepPayload *getResp = constructResponse(ehRequest);
        if(!getResp)
        {
            OC_LOG(ERROR, TAG, "constructResponse failed");
            return OC_EH_ERROR;
        }

        *payload = getResp;
        ehResult = OC_EH_OK;
    }
    else
    {
        ehResult = OC_EH_OK;
    }

    return ehResult;
}

OCEntityHandlerResult ProcessPutRequest (OCEntityHandlerRequest *ehRequest,
        OCRepPayload** payload)
{
    OCEntityHandlerResult ehResult;
    OCRepPayload *putResp = constructResponse(ehRequest);

    if(!putResp)
    {
        OC_LOG(ERROR, TAG, "Failed to construct Json response");
        return OC_EH_ERROR;
    }

    *payload = putResp;
    ehResult = OC_EH_OK;

    return ehResult;
}

OCEntityHandlerResult ProcessPostRequest (OCEntityHandlerRequest *ehRequest,
        OCEntityHandlerResponse *response, OCRepPayload** payload)
{
    OCEntityHandlerResult ehResult = OC_EH_OK;
    OCRepPayload *respPLPost_light = NULL;

    /*
     * The entity handler determines how to process a POST request.
     * Per the REST paradigm, POST can also be used to update representation of existing
     * resource or create a new resource.
     * In the sample below, if the POST is for /a/light then a new instance of the Light
     * resource is created with default representation (if representation is included in
     * POST payload it can be used as initial values) as long as the instance is
     * lesser than max new instance count. Once max instance count is reached, POST on
     * /a/light updated the representation of /a/light (just like PUT)
     */

    if (ehRequest->resource == Light.handle)
    {
        if (gCurrLightInstance < SAMPLE_MAX_NUM_POST_INSTANCE)
        {
            // Create new Light instance
            char newLightUri[URI_MAXSIZE];
            snprintf(newLightUri, URI_MAXSIZE, "/a/light/%d", gCurrLightInstance);

            respPLPost_light = OCRepPayloadCreate();
            OCRepPayloadSetUri(respPLPost_light, gResourceUri);
            OCRepPayloadSetPropString(respPLPost_light, "createduri", newLightUri);

            if (0 == createLightResource (newLightUri, &gLightInstance[gCurrLightInstance]))
            {
                OC_LOG (INFO, TAG, "Created new Light instance\n");
                gLightInstance[gCurrLightInstance].state = 0;
                gLightInstance[gCurrLightInstance].power = 0;
                gCurrLightInstance++;
                strncpy ((char *)response->resourceUri, newLightUri, MAX_URI_LENGTH);
                ehResult = OC_EH_RESOURCE_CREATED;
            }
        }
        else
        {
            // Update repesentation of /a/light
            Light.state = true;
            Light.power = 11;
            respPLPost_light = constructResponse(ehRequest);
        }
    }
    else
    {
        for (int i = 0; i < SAMPLE_MAX_NUM_POST_INSTANCE; i++)
        {
            if (ehRequest->resource == gLightInstance[i].handle)
            {
                gLightInstance[i].state = true;
                gLightInstance[i].power = 22;
                if (i == 0)
                {
                    respPLPost_light = constructResponse(ehRequest);
                    break;
                }
                else if (i == 1)
                {
                    respPLPost_light = constructResponse(ehRequest);
                }
            }
        }
    }

    if ((respPLPost_light != NULL))
    {
        *payload = respPLPost_light;
    }
    else
    {
        OC_LOG(INFO, TAG, "Payload was NULL");
        ehResult = OC_EH_ERROR;
    }

    return ehResult;
}



OCEntityHandlerResult ProcessDeleteRequest (OCEntityHandlerRequest *ehRequest)
{
    if(ehRequest == NULL)
    {
        OC_LOG(INFO, TAG, "The ehRequest is NULL");
        return OC_EH_ERROR;
    }
    OCEntityHandlerResult ehResult = OC_EH_OK;

    OC_LOG_V(INFO, TAG, "\n\nExecuting %s for resource %d ", __func__, ehRequest->resource);

    /*
     * In the sample below, the application will:
     * 1a. pass the delete request to the c stack
     * 1b. internally, the c stack figures out what needs to be done and does it accordingly
     *    (e.g. send observers notification, remove observers...)
     * 1c. the c stack returns with the result whether the request is fullfilled.
     * 2. optionally, app removes observers out of its array 'interestedObservers'
     */

    if ((ehRequest != NULL) && (ehRequest->resource == Light.handle))
    {
        //Step 1: Ask stack to do the work.
        OCStackResult result = OCDeleteResource(ehRequest->resource);

        if (result == OC_STACK_OK)
        {
            OC_LOG (INFO, TAG, "\n\nDelete Resource operation succeeded.");
            ehResult = OC_EH_OK;

            //Step 2: clear observers who wanted to observe this resource at the app level.
            for (uint8_t i = 0; i < SAMPLE_MAX_NUM_OBSERVATIONS; i++)
            {
                if (interestedObservers[i].resourceHandle == ehRequest->resource)
                {
                    interestedObservers[i].valid = false;
                    interestedObservers[i].observationId = 0;
                    interestedObservers[i].resourceHandle = NULL;
                }
            }
        }
        else if (result == OC_STACK_NO_RESOURCE)
        {
            OC_LOG(INFO, TAG, "\n\nThe resource doesn't exist or it might have been deleted.");
            ehResult = OC_EH_RESOURCE_DELETED;
        }
        else
        {
            OC_LOG(INFO, TAG, "\n\nEncountered error from OCDeleteResource().");
            ehResult = OC_EH_ERROR;
        }
    }
    else if (ehRequest->resource != Light.handle)
    {
        //Let's this app not supporting DELETE on some resources so
        //consider the DELETE request is received for a non-support resource.
        OC_LOG(INFO, TAG, "\n\nThe request is received for a non-support resource.");
        ehResult = OC_EH_FORBIDDEN;
    }

    return ehResult;
}


OCEntityHandlerResult ProcessNonExistingResourceRequest(OCEntityHandlerRequest * /*ehRequest*/)
{
    OC_LOG_V(INFO, TAG, "\n\nExecuting %s ", __func__);

    return OC_EH_RESOURCE_NOT_FOUND;
}


OCEntityHandlerResult
OCDeviceEntityHandlerCb (OCEntityHandlerFlag flag,
                         OCEntityHandlerRequest *entityHandlerRequest,
                         char* uri,
                         void* /*callbackParam*/)
{
    OC_LOG_V (INFO, TAG, "Inside device default entity handler - flags: 0x%x, uri: %s", flag, uri);

    OCEntityHandlerResult ehResult = OC_EH_OK;
    OCEntityHandlerResponse response;

    // Validate pointer
    if (!entityHandlerRequest)
    {
        OC_LOG (ERROR, TAG, "Invalid request pointer");
        return OC_EH_ERROR;
    }
    // Initialize certain response fields
    response.numSendVendorSpecificHeaderOptions = 0;
    memset(response.sendVendorSpecificHeaderOptions, 0,
            sizeof response.sendVendorSpecificHeaderOptions);
    memset(response.resourceUri, 0, sizeof response.resourceUri);
    OCRepPayload* payload = NULL;


    if (flag & OC_REQUEST_FLAG)
    {
        OC_LOG (INFO, TAG, "Flag includes OC_REQUEST_FLAG");

        if (entityHandlerRequest->resource == NULL)
        {
            OC_LOG (INFO, TAG, "Received request from client to a non-existing resource");
            ehResult = ProcessNonExistingResourceRequest(entityHandlerRequest);
        }
        else if (OC_REST_GET == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_GET from client");
            ehResult = ProcessGetRequest (entityHandlerRequest, &payload);
        }
        else if (OC_REST_PUT == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_PUT from client");
            ehResult = ProcessPutRequest (entityHandlerRequest, &payload);
        }
        else if (OC_REST_DELETE == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_DELETE from client");
            ehResult = ProcessDeleteRequest (entityHandlerRequest);
        }
        else
        {
            OC_LOG_V (INFO, TAG, "Received unsupported method %d from client",
                      entityHandlerRequest->method);
            ehResult = OC_EH_ERROR;
        }
               // If the result isn't an error or forbidden, send response
        if (!((ehResult == OC_EH_ERROR) || (ehResult == OC_EH_FORBIDDEN)))
        {
            // Format the response.  Note this requires some info about the request
            response.requestHandle = entityHandlerRequest->requestHandle;
            response.resourceHandle = entityHandlerRequest->resource;
            response.ehResult = ehResult;
            response.payload = reinterpret_cast<OCPayload*>(payload);
            // Indicate that response is NOT in a persistent buffer
            response.persistentBufferFlag = 0;

            // Send the response
            if (OCDoResponse(&response) != OC_STACK_OK)
            {
                OC_LOG(ERROR, TAG, "Error sending response");
                ehResult = OC_EH_ERROR;
            }
        }
    }
    if (flag & OC_OBSERVE_FLAG)
    {
        OC_LOG(INFO, TAG, "Flag includes OC_OBSERVE_FLAG");
        if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action)
        {
            OC_LOG (INFO, TAG, "Received OC_OBSERVE_REGISTER from client");
        }
        else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action)
        {
            OC_LOG (INFO, TAG, "Received OC_OBSERVE_DEREGISTER from client");
        }
    }

    return ehResult;
}

OCEntityHandlerResult
OCNOPEntityHandlerCb (OCEntityHandlerFlag /*flag*/,
                      OCEntityHandlerRequest * /*entityHandlerRequest*/,
                      void* /*callbackParam*/)
{
    // This is callback is associated with the 2 presence notification
    // resources. They are non-operational.
    return OC_EH_OK;
}

OCEntityHandlerResult
OCEntityHandlerCb (OCEntityHandlerFlag flag,
        OCEntityHandlerRequest *entityHandlerRequest, void* /*callback*/)
{
    OC_LOG_V (INFO, TAG, "Inside entity handler - flags: 0x%x", flag);

    OCEntityHandlerResult ehResult = OC_EH_OK;
    OCEntityHandlerResponse response = { 0, 0, OC_EH_ERROR, 0, 0, { },{ 0 }, false };

    // Validate pointer
    if (!entityHandlerRequest)
    {
        OC_LOG (ERROR, TAG, "Invalid request pointer");
        return OC_EH_ERROR;
    }

    // Initialize certain response fields
    response.numSendVendorSpecificHeaderOptions = 0;
    memset(response.sendVendorSpecificHeaderOptions,
            0, sizeof response.sendVendorSpecificHeaderOptions);
    memset(response.resourceUri, 0, sizeof response.resourceUri);
    OCRepPayload* payload = NULL;

    if (flag & OC_REQUEST_FLAG)
    {
        OC_LOG (INFO, TAG, "Flag includes OC_REQUEST_FLAG");

        if (OC_REST_GET == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_GET from client");
            ehResult = ProcessGetRequest (entityHandlerRequest, &payload);
        }
        else if (OC_REST_PUT == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_PUT from client");
            ehResult = ProcessPutRequest (entityHandlerRequest, &payload);
        }
        else if (OC_REST_POST == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_POST from client");
            ehResult = ProcessPostRequest (entityHandlerRequest, &response, &payload);
        }
        else if (OC_REST_DELETE == entityHandlerRequest->method)
        {
            OC_LOG (INFO, TAG, "Received OC_REST_DELETE from client");
            ehResult = ProcessDeleteRequest (entityHandlerRequest);
        }
        else
        {
            OC_LOG_V (INFO, TAG, "Received unsupported method %d from client",
                      entityHandlerRequest->method);
            ehResult = OC_EH_ERROR;
        }
        // If the result isn't an error or forbidden, send response
        if (!((ehResult == OC_EH_ERROR) || (ehResult == OC_EH_FORBIDDEN)))
        {
            // Format the response.  Note this requires some info about the request
            response.requestHandle = entityHandlerRequest->requestHandle;
            response.resourceHandle = entityHandlerRequest->resource;
            response.ehResult = ehResult;
            response.payload = (OCPayload*) (payload);
            // Indicate that response is NOT in a persistent buffer
            response.persistentBufferFlag = 0;

            // Handle vendor specific options
            if(entityHandlerRequest->rcvdVendorSpecificHeaderOptions &&
                    entityHandlerRequest->numRcvdVendorSpecificHeaderOptions)
            {
                OC_LOG (INFO, TAG, "Received vendor specific options");
                uint8_t i = 0;
                OCHeaderOption * rcvdOptions =
                        entityHandlerRequest->rcvdVendorSpecificHeaderOptions;
                for( i = 0; i < entityHandlerRequest->numRcvdVendorSpecificHeaderOptions; i++)
                {
                    if(((OCHeaderOption)rcvdOptions[i]).protocolID == OC_COAP_ID)
                    {
                        OC_LOG_V(INFO, TAG, "Received option with OC_COAP_ID and ID %u with",
                                ((OCHeaderOption)rcvdOptions[i]).optionID );

                        OC_LOG_BUFFER(INFO, TAG, ((OCHeaderOption)rcvdOptions[i]).optionData,
                            MAX_HEADER_OPTION_DATA_LENGTH);
                    }
                }
                OCHeaderOption * sendOptions = response.sendVendorSpecificHeaderOptions;
                uint8_t option2[] = {21,22,23,24,25,26,27,28,29,30};
                uint8_t option3[] = {31,32,33,34,35,36,37,38,39,40};
                sendOptions[0].protocolID = OC_COAP_ID;
                sendOptions[0].optionID = 2248;
                memcpy(sendOptions[0].optionData, option2, sizeof(option2));
                sendOptions[0].optionLength = 10;
                sendOptions[1].protocolID = OC_COAP_ID;
                sendOptions[1].optionID = 2600;
                memcpy(sendOptions[1].optionData, option3, sizeof(option3));
                sendOptions[1].optionLength = 10;
                response.numSendVendorSpecificHeaderOptions = 2;
            }

            // Send the response
            if (OCDoResponse(&response) != OC_STACK_OK)
            {
                OC_LOG(ERROR, TAG, "Error sending response");
                ehResult = OC_EH_ERROR;
            }
        }
    }
    if (flag & OC_OBSERVE_FLAG)
    {
        OC_LOG(INFO, TAG, "Flag includes OC_OBSERVE_FLAG");

        if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action)
        {
            OC_LOG (INFO, TAG, "Received OC_OBSERVE_REGISTER from client");
          //  ProcessObserveRegister (entityHandlerRequest);
        }
        else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action)
        {
            OC_LOG (INFO, TAG, "Received OC_OBSERVE_DEREGISTER from client");
            //ProcessObserveDeregister (entityHandlerRequest);
        }
    }

    OCPayloadDestroy(response.payload);
    return ehResult;
}


int createLightResource (char *uri, LightResource *lightResource)
{
    if (!uri)
    {
        OC_LOG(ERROR, TAG, "Resource URI cannot be NULL");
        return -1;
    }

    OC_LOG_V(DEBUG, TAG, "Creating new light resource with ur %s", uri);
    uint8_t props = (OC_DISCOVERABLE | OC_OBSERVABLE);

    lightResource->state = false;
    lightResource->power= 0;

    Light.state = false;
    OCStackResult res = OCCreateResource(&(lightResource->handle),
            "core.light",
            OC_RSRVD_INTERFACE_DEFAULT,
            uri,
            OCEntityHandlerCb,
            NULL,
            OC_DISCOVERABLE|OC_OBSERVABLE);

    /*OCStackResult res = OCCreateResource(&(lightResource->handle),
            "core.light",
            OC_RSRVD_INTERFACE_DEFAULT,
            uri,
            NULL,
            NULL,
            props);

    OC_LOG_V(INFO, TAG, "Created Light resource with result: %s", getResult(res));
*/
    return 0;
}

/**
 * @brief Returns the result of a OCStackResult as a string
 *
 * @param OCStackResult The result to be converted to a string
 *
 * @return A string with the result
*/
const char * getResults(OCStackResult result)
{
   switch (result) {
    case OC_STACK_OK:
        return "OC_STACK_OK";
    case OC_STACK_INVALID_URI:
        return "OC_STACK_INVALID_URI";
    case OC_STACK_INVALID_QUERY:
        return "OC_STACK_INVALID_QUERY";
    case OC_STACK_INVALID_IP:
        return "OC_STACK_INVALID_IP";
    case OC_STACK_INVALID_PORT:
        return "OC_STACK_INVALID_PORT";
    case OC_STACK_INVALID_CALLBACK:
        return "OC_STACK_INVALID_CALLBACK";
    case OC_STACK_INVALID_METHOD:
        return "OC_STACK_INVALID_METHOD";
    case OC_STACK_NO_MEMORY:
        return "OC_STACK_NO_MEMORY";
    case OC_STACK_COMM_ERROR:
        return "OC_STACK_COMM_ERROR";
    case OC_STACK_INVALID_PARAM:
        return "OC_STACK_INVALID_PARAM";
    case OC_STACK_NOTIMPL:
        return "OC_STACK_NOTIMPL";
    case OC_STACK_NO_RESOURCE:
        return "OC_STACK_NO_RESOURCE";
    case OC_STACK_RESOURCE_ERROR:
        return "OC_STACK_RESOURCE_ERROR";
    case OC_STACK_SLOW_RESOURCE:
        return "OC_STACK_SLOW_RESOURCE";
    case OC_STACK_NO_OBSERVERS:
        return "OC_STACK_NO_OBSERVERS";
    case OC_STACK_ERROR:
        return "OC_STACK_ERROR";
    default:
        return "UNKNOWN";
    }
}


void setup()
{
    // Add your initialization code here
    // Note : This will initialize Serial port on Arduino at 115200 bauds
   	OC_LOG_INIT();
    OC_LOG(DEBUG, TAG, ("OCServer is starting..."));

    // MP
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // Connect to Ethernet or WiFi network
    if (ConnectToNetwork() != 0)
    {
        OC_LOG(ERROR, TAG, ("Unable to connect to network"));
        return;
    }

    OC_LOG(DEBUG, TAG, "Network initialized");

    // Initialize the OC Stack in Server mode
    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OC_LOG(ERROR, TAG, ("OCStack init error"));
        return;
    }

    //OCSetDefaultDeviceEntityHandler(OCDeviceEntityHandlerCb, NULL);
    /*
     * Declare and create the example resource: Light
     */
    createLightResource(gResourceUri, &Light);

    OC_LOG(INFO, TAG, "Entering ocserver main loop...");
}

void loop()
{
	delay(100);

    // Give CPU cycles to OCStack to perform send/recv and other OCStack stuff
    if (OCProcess() != OC_STACK_OK)
    {
        OC_LOG(ERROR, TAG, ("OCStack process error"));
        return;
    }
}