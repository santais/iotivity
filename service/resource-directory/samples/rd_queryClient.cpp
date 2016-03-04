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
#include "OCApi.h"
#include "OCPlatform.h"
#include "ResourceTypes.h"

using namespace OC;

bool g_foundResource = true;

void onGet(const HeaderOptions& option, const OCRepresentation& rep, const int eCode)
{
    // Search through the attributes
    std::unique_ptr<OCRepPayload> payloadPtr(rep.getPayload());

    OCRepPayloadValue* values = payloadPtr->values;

    while(values != NULL)
    {
        std::cout << values->name << " value is: ";
        switch(values->type)
        {
            case OCREP_PROP_INT:
                std::cout << values->i << std::endl;
            break;
            case OCREP_PROP_DOUBLE:
                std::cout << values->d << std::endl;
            break;
            case OCREP_PROP_BOOL:
                std::cout << values->b << std::endl;
            break;
            case OCREP_PROP_STRING:
                std::cout << values->str << std::endl;
            break;

        }
        values = values->next;
    }
}

void foundResource(std::shared_ptr< OC::OCResource > resource)
{
    try
    {
        std::cout << "Found resource response." << std::endl;
        if (resource)
        {
            if (resource->uri() == "/a/light")
            {
                std::cout << "Found Resource at @ URI: " << resource->uri() << "\tHost Address: " <<
                          resource->host() << std::endl;
                QueryParamsMap map;
                resource->get(map, &onGet);
            }

            std::cout << "Resource uri is: " << resource->uri() << std::endl;
        }
        else
        {
            std::cout << "Resource is invalwid " << resource->uri() << std::endl;
        }
        /*g_foundResource = false;
        exit(0);*/
    }
    catch (std::exception &ex)
    {
        std::cout << "Exception: " << ex.what() << " in foundResource" << std::endl;
        exit(1);
    }
}

int main()
{
    OC::PlatformConfig cfg;
    OC::OCPlatform::Configure(cfg);
    bool sendRequest = true;

    std::cout << "Created Platform..." << std::endl;

    while (g_foundResource)
    {
        try
        {
            if (sendRequest)
            {
                sendRequest = false;
                std::ostringstream ss;
                ss << OC_RSRVD_WELL_KNOWN_URI << "?rt=" << OIC_DEVICE_LIGHT;
                std::cout << "Finding Resource light" << std::endl; //"/oic/res?rt=core.light"
                OC::OCPlatform::findResource("", ss.str(), CT_IP_USE_V4 , &foundResource);
            }
        }
        catch (OC::OCException &ex)
        {
            sendRequest = true;
            std::cout << "Exception finding resources : " << ex.reason() << std::endl;
        }
    }
}
