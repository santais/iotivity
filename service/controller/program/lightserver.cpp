//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
// Copyright 2014 Samsung Electronics All Rights Reserved.
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

///
/// This sample provides steps to define an interface for a resource
/// (properties and methods) and host this resource on the server.
///

#include <functional>

#include <pthread.h>
#include <mutex>
#include <condition_variable>

#include "OCPlatform.h"
#include "OCApi.h"
#include "ResourceTypes.h"

using namespace OC;
using namespace std;
namespace PH = std::placeholders;

int gObservation = 0;
void * ChangeLightRepresentation(void *param);
void * handleSlowResponse(void *param, std::shared_ptr< OCResourceRequest > pRequest);
bool gUnderObservation = false;

// Specifies secure or non-secure
// false: non-secure resource
// true: secure resource
bool isSecure = false;

/// Specifies whether Entity handler is going to do slow response or not
bool isSlowResponse = false;

// Forward declaring the entityHandler

/// This class represents a single resource named 'lightResource'. This resource has
/// two simple properties named 'state' and 'power'

class LightResource
{

public:
    /// Access this property from a TB client
    bool m_power;
    std::string m_lightUri;
    OCResourceHandle m_resourceHandle;
    OCRepresentation m_lightRep;

public:
    /// Constructor
    LightResource() :
            m_power(false), m_lightUri("/a/light"), m_resourceHandle(0)
    {
        // Initialize representation
        m_lightRep.setUri(m_lightUri);

        m_lightRep.setValue("power", m_power);
    }

    /* Note that this does not need to be a member function: for classes you do not have
     access to, you can accomplish this with a free function: */

    /// This function internally calls registerResource API.
    void createResource()
    {
        std::string resourceURI = m_lightUri; //URI of the resource
        std::string resourceTypeName = "oic.d.light"; //resource type name. In this case, it is light
        std::string resourceInterface = DEFAULT_INTERFACE; // resource interface.

        EntityHandler cb = std::bind(&LightResource::entityHandler, this, PH::_1);

        // This will internally create and register the resource.
        OCStackResult result = OCPlatform::registerResource(m_resourceHandle, resourceURI,
                resourceTypeName, resourceInterface, cb, OC_DISCOVERABLE | OC_OBSERVABLE);

        result = OCPlatform::bindTypeToResource(m_resourceHandle, OIC_TYPE_BINARY_SWITCH);

        if (OC_STACK_OK != result)
        {
            cout << "Resource creation was unsuccessful\n";
        }
        else
        {
            cout << "Resource URI : " << resourceURI << endl;
            cout << "\tResource Type Name : " << resourceTypeName << endl;
            cout << "\tResource Interface : " << DEFAULT_INTERFACE << endl;
            cout << "\tResource creation is successful with resource handle : " << m_resourceHandle
                    << endl;
        }
    }

    OCResourceHandle getHandle()
    {
        return m_resourceHandle;
    }

    // Puts representation.
    // Gets values from the representation and
    // updates the internal state
    void put(OCRepresentation& rep)
    {
        std::cout << "Number of attributes: " << rep.numberOfAttributes() << std::endl;

        // Search through the attributes
        std::unique_ptr<OCRepPayload> payloadPtr(rep.getPayload());

        OCRepPayloadValue* values = payloadPtr->values;

        while(values != NULL)
        {
            std::cout << values->name << " value is: ";
            switch(values->type)
            {
                case OCREP_PROP_INT:
                    std::cout << "INT: " << values->i << std::endl;
                break;
                case OCREP_PROP_DOUBLE:
                    std::cout << "DOUBLE: " << values->d << std::endl;
                break;
                case OCREP_PROP_BOOL:
                    std::cout << "BOOL: " << std::boolalpha <<  values->b << std::endl;
                break;
                case OCREP_PROP_STRING:
                    std::cout << "STRING: " << values->str << std::endl;
                break;

            }
            values = values->next;
        }

        try
        {
            if (rep.getValue("power", m_power))
            {
                cout << "\t\t\t\t" << "power: " << std::boolalpha << m_power << endl;
            }
            else
            {
                cout << "\t\t\t\t" << "power not found in the representation" << endl;
            }

            OCPlatform::notifyAllObservers(m_resourceHandle);
        }
        catch (exception& e)
        {
            cout << e.what() << endl;
        }

    }

    // Post representation.
    // Post can create new resource or simply act like put.
    // Gets values from the representation and
    // updates the internal state
    OCRepresentation post(OCRepresentation& rep)
    {
        put(rep);
        return get();
    }

    // gets the updated representation.
    // Updates the representation with latest internal state before
    // sending out.
    OCRepresentation get()
    {
        m_lightRep.setValue("power", m_power);

        return m_lightRep;
    }

    void addType(const std::string& type) const
    {
        OCStackResult result = OCPlatform::bindTypeToResource(m_resourceHandle, type);
        if (OC_STACK_OK != result)
        {
            cout << "Binding TypeName to Resource was unsuccessful\n";
        }
    }

    void addInterface(const std::string& interface) const
    {
        OCStackResult result = OCPlatform::bindInterfaceToResource(m_resourceHandle, interface);
        if (OC_STACK_OK != result)
        {
            cout << "Binding TypeName to Resource was unsuccessful\n";
        }
    }

private:
// This is just a sample implementation of entity handler.
// Entity handler can be implemented in several ways by the manufacturer
    OCEntityHandlerResult entityHandler(std::shared_ptr< OCResourceRequest > request)
    {
        //cout << "\tIn Server CPP entity handler:\n";
        OCEntityHandlerResult ehResult = OC_EH_ERROR;
        if (request)
        {
            // Get the request type and request flag
            std::string requestType = request->getRequestType();
            int requestFlag = request->getRequestHandlerFlag();

            if (requestFlag & RequestHandlerFlag::RequestFlag)
            {
                //cout << "\t\trequestFlag : Request\n";
                auto pResponse = std::make_shared< OC::OCResourceResponse >();
                pResponse->setRequestHandle(request->getRequestHandle());
                pResponse->setResourceHandle(request->getResourceHandle());

                // If the request type is GET
                if (requestType == "GET")
                {
                    //cout << "\t\t\trequestType : GET\n";
                    if (isSlowResponse) // Slow response case
                    {
                        static int startedThread = 0;
                        if (!startedThread)
                        {
                            std::thread t(handleSlowResponse, (void *) this, request);
                            startedThread = 1;
                            t.detach();
                        }
                        ehResult = OC_EH_SLOW;
                    }
                    else // normal response case.
                    {
                        pResponse->setErrorCode(200);
                        pResponse->setResponseResult(OC_EH_OK);
                        pResponse->setResourceRepresentation(get());
                        if (OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                        {
                            ehResult = OC_EH_OK;
                        }
                    }
                }
                else if (requestType == "PUT")
                {
                    //cout << "\t\t\trequestType : PUT\n";
                    OCRepresentation rep = request->getResourceRepresentation();

                    // Do related operations related to PUT request
                    // Update the lightResource
                    put(rep);
                    pResponse->setErrorCode(200);
                    pResponse->setResponseResult(OC_EH_OK);
                    pResponse->setResourceRepresentation(get());
                    if (OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                    {
                        ehResult = OC_EH_OK;
                    }
                }
                else if (requestType == "POST")
                {
                    //cout << "\t\t\trequestType : POST\n";

                    OCRepresentation rep = request->getResourceRepresentation();

                    // Do related operations related to POST request
                    OCRepresentation rep_post = post(rep);
                    pResponse->setResourceRepresentation(rep_post);
                    pResponse->setErrorCode(200);
                    if (rep_post.hasAttribute("createduri"))
                    {
                        pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
                        pResponse->setNewResourceUri(
                                rep_post.getValue< std::string >("createduri"));
                    }

                    if (OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                    {
                        ehResult = OC_EH_OK;
                    }
                }
                else if (requestType == "DELETE")
                {
                    // DELETE request operations
                }
                if (requestFlag & RequestHandlerFlag::ObserverFlag)
                {
                    ObservationInfo observationInfo = request->getObservationInfo();
                    if(ObserveAction::ObserveRegister == observationInfo.action)
                    {
                        cout << "\t\trequestFlag : Register Observer\n";
                        gUnderObservation = true;
                    }
                    else if(ObserveAction::ObserveUnregister == observationInfo.action)
                    {
                        gUnderObservation = false;
                        cout << "\t\trequestFlag : Degister Observer\n";
                    }
                }
            }
        }
        else
        {
            std::cout << "Request invalid" << std::endl;
        }

        return ehResult;
    }
};

void * handleSlowResponse(void *param, std::shared_ptr< OCResourceRequest > pRequest)
{
    // This function handles slow response case
    LightResource* lightPtr = (LightResource*) param;
    // Induce a case for slow response by using sleep
    std::cout << "SLOW response" << std::endl;
    sleep(10);

    auto pResponse = std::make_shared< OC::OCResourceResponse >();
    pResponse->setRequestHandle(pRequest->getRequestHandle());
    pResponse->setResourceHandle(pRequest->getResourceHandle());
    pResponse->setResourceRepresentation(lightPtr->get());
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);

    // Set the slow response flag back to false
    isSlowResponse = false;
    OCPlatform::sendResponse(pResponse);
    return NULL;
}

int main()
{
    // Create PlatformConfig object
    PlatformConfig cfg
    { OC::ServiceType::InProc, OC::ModeType::Server, "0.0.0.0",
    // By setting to "0.0.0.0", it binds to all available interfaces
            0,// Uses randomly available port
            OC::QualityOfService::LowQos };

    OCPlatform::Configure(cfg);
    try
    {
        // Create the instance of the resource class
        // (in this case instance of class 'LightResource').
        LightResource myLight;

        // Invoke createResource function of class light.
        myLight.createResource();

        while(OCProcess() == OC_STACK_OK)
        {
            sleep(0.5);
        }
    }
    catch (OCException e)
    {
        //log(e.what());
    }

    // No explicit call to stop the platform.
    // When OCPlatform::destructor is invoked, internally we do platform cleanup

    return 0;
}
