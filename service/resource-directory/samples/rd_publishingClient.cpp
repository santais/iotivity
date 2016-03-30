//******************************************************************
//
// Copyright 2015 Samsung Electronics All Rights Reserved.
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
#include <iostream>
#include <functional>

#include <pthread.h>
#include <mutex>
#include <condition_variable>

#include "OCPlatform.h"
#include "OCApi.h"
#include "oic_string.h"

#include "rd_client.h"

using namespace OC;

OCResourceHandle g_curResource_t = NULL;
OCResourceHandle g_curResource_l = NULL;
OCResourceHandle g_curResource_b = NULL;
char rdAddress[MAX_ADDR_STR_SIZE];
uint16_t rdPort;

static bool initialized = false;

OCEntityHandlerResult entityHandlerCb(std::shared_ptr<OCResourceRequest> request)
{
    std::cout << "Inside eneityHandlerCb" << std::endl;

    OCEntityHandlerResult ehResult = OC_EH_ERROR;

    if(request)
    {
        std::cout << "Request for uri: " << request->getResourceUri() << std::endl;
        std::string requestType = request->getRequestType();
        int requestFlag = request->getRequestHandlerFlag();

        if(requestFlag & RequestHandlerFlag::RequestFlag)
        {
            std::cout << "\t\trequestFlag : Request\n";
            auto pResponse = std::make_shared<OC::OCResourceResponse>();
            pResponse->setRequestHandle(request->getRequestHandle());
            pResponse->setResourceHandle(request->getResourceHandle());

            // Check for query params (if any)
            QueryParamsMap queries = request->getQueryParameters();

            if (!queries.empty())
            {
                std::cout << "\nQuery processing upto entityHandler" << std::endl;
            }
            for (auto it : queries)
            {
                std::cout << "Query key: " << it.first << " value : " << it.second
                        << std:: endl;
            }

            OCRepresentation rep;
            std::string name("Mark's Light");
            rep.setValue("state", true);
            rep.setValue("power", (int)100);
            rep.setValue("name", name);


            // If the request type is GET
            if(requestType == "GET")
            {
                /*if(!initialized)
                {
                    OCStackResult res = OCPlatform::bindTypeToResource(request->getResourceHandle(),
                                                        "oic.r.light.dimming");

                    if(res != OC_STACK_OK)
                    {
                        std::cout << "Error binding type" << std::endl;
                    }
                }*/

                std::cout << "\t\t\trequestType : GET\n";
                pResponse->setErrorCode(200);
                pResponse->setResponseResult(OC_EH_OK);
                pResponse->setResourceRepresentation(rep);
                if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                {
                    ehResult = OC_EH_OK;
                }

            }
        }
    }
    return ehResult;
}

void registerLocalResources()
{
    std::string resourceURI_thermostat = "/a/thermostat";
    std::string resourceTypeName_thermostat = "oic.d.sensor";
    std::string resourceURI_light = "/a/light";
    std::string resourceTypeName_light = "oic.d.light";
    std::string resourceURI_button = "/a/button";
    std::string resourceTypeName_button = "oic.r.button";
    std::string resourceInterface = DEFAULT_INTERFACE;
    uint8_t resourceProperty = OC_DISCOVERABLE;

    EntityHandler cb = std::bind(&entityHandlerCb, std::placeholders::_1);

    /*OCStackResult result = OCPlatform::registerResource(g_curResource_t,
                           resourceURI_thermostat,
                           resourceTypeName_thermostat,
                           resourceInterface,
                           cb,
                           resourceProperty);

    //OCPlatform::bindTypeToResource(g_curResource_t, OC_RSRVD_RESOURCE_TYPE_RD);

    if (OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("Device Resource failed to start") + std::to_string(result));
    }*/

    OCStackResult result = OCPlatform::registerResource(g_curResource_l,
                                          resourceURI_light,
                                          resourceTypeName_light,
                                          resourceInterface,
                                          cb,
                                          resourceProperty);

    if (OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("Device Resource failed to start") + std::to_string(result));
    }

    /*result = OCPlatform::registerResource(g_curResource_b,
                                          resourceURI_button,
                                          resourceTypeName_button,
                                          resourceInterface,
                                          NULL,
                                          resourceProperty);

    if (OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("Device Resource failed to start") + std::to_string(result));
    }

    OCPlatform::bindTypeToResource(g_curResource_b, OC_RSRVD_RESOURCE_TYPE_RD);

    if (OC_STACK_OK != result)
    {
        throw std::runtime_error(
            std::string("Device Resource failed to start") + std::to_string(result));
    }*/
}

void printHelp()
{
    std::cout << std::endl;
    std::cout << "********************************************" << std::endl;
    std::cout << "*  method Type : 1 - Discover RD           *" << std::endl;
    std::cout << "*  method Type : 2 - Publish               *" << std::endl;
    std::cout << "*  method Type : 3 - Update                *" << std::endl;
    std::cout << "*  method Type : 4 - Delete                *" << std::endl;
    std::cout << "*  method Type : 5 - Status                *" << std::endl;
    std::cout << "********************************************" << std::endl;
    std::cout << std::endl;
}

int biasFactorCB(char addr[MAX_ADDR_STR_SIZE], uint16_t port)
{
    OICStrcpy(rdAddress, MAX_ADDR_STR_SIZE, addr);
    rdPort = port;
    std::cout << "RD Address is : " <<  addr << ":" << port << std::endl;
    initialized = true;
    return 0;
}

int main()
{
    int in;
    PlatformConfig cfg;

    OCPlatform::Configure(cfg);

    std::cout << "Created Platform..." << std::endl;

    try
    {
        registerLocalResources();
    }
    catch (std::runtime_error e)
    {
        std::cout << "Caught OCException [Code: " << e.what() << std::endl;
    }

    std::cout << "Entering while loop" << std::endl;

    //while (1)
    //{
        sleep(2);

        if (g_curResource_t == NULL || g_curResource_l == NULL)// || g_curResource_b == NULL)
        {
            std::cout << "Either of the handles are NULL" << std::endl;
        }
        /*printHelp();

        in = 0;
        std::cin >> in;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input type, please try again" << std::endl;
            continue;
        }*/
        try
        {
            std::cout << "Finding RD !" << std::endl;
            while(!initialized)
            {
                OCRDDiscover(biasFactorCB);
                sleep(3);
            }

            std::cout << "Found RD server" << std::endl;
            OCRDPublish(rdAddress, rdPort, 2, g_curResource_t, g_curResource_l);


            std::mutex blocker;
            std::condition_variable cv;
            std::unique_lock<std::mutex> lock(blocker);
            std::cout <<"Waiting" << std::endl;
            cv.wait(lock, []{return false;});
        }
        catch(OCException &e)
        {
            std::cout << "OCException in main: " << e.what() << std::endl;
        }

        /*try
        {
            switch ((int)in)
            {
                case 1:
                    OCRDDiscover(biasFactorCB);
                    break;
                case 2:
                    OCRDPublish(rdAddress, rdPort, 2, g_curResource_t, g_curResource_l);
                    break;
                case 3:
                    break;
                default:
                    std::cout << "Invalid input, please try again" << std::endl;
                    break;
            }
        }
        catch (OCException e)
        {
            std::cout << "Caught OCException [Code: " << e.code() << " Reason: " << e.reason() << std::endl;
        }
   // }
    //return 0;*/
}
