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
 * Rich OIC Server applicable of discovery, device management and
 * complies all the functionalities given by the OC Core.
 ******************************************************************/

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_


#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

#include "ResourceTypes.h"

#include "PrimitiveResource.h"
#include "RCSResourceObject.h"
#include "RCSRemoteResourceObject.h"
#include "RCSDiscoveryManager.h"
#include "HostingObject.h"
#include "ResourceHosting.h"
#include "OCPlatform.h"
#include "OCApi.h"
#include "ExpiryTimer.h"

 /*
	The rich server is to automatically discover new devices in the network.

	It has an implemented Resource Host to control constrained devices. The RH 
	automatically disocvers new hosting devices so this is not a requirement.

	When a resource is discovered by the RH, the hosted resource should no longer
	be discoverable.

	It has to automatically onboard and provision for new devices wishing
	to join the network.

	Create a file of genuine resource types.

    Common independent resources are discovered through resource type "oic.if.baseline".
    This type is not present when discovering hosted resources.

    The Discovery process is to be 

		
 */


/* Variables included in the class
	std::map of the registered resources (avoid duplication and lookup).

	RCSResourceObject::Ptr m_serverResource;

	// Object controlling and managing all resources discovered by the Resource Host. 
	// TODO: Add getResources to the class
	ResourceHosting resourceHost

	// Map in HostingObject ẃhich has all the objects.
	std::unordered_map<HostingObjectKey, HostingObject::Ptr> m_hostingObjects;

	// List of stored resources
	// key: Uri + Host, Value: RCSResourceObject::Ptr
	std::unordered_map<key, value> m_resourceList;

	// Discovery Manager.
	RCSDiscoveryManager m_manager;




*/

/* Functions to be implemented 
	setRequestHandler

	// Copies a discovered remote resource to a resource object
	RCSResourceObject::Ptr copyRemoteRcToObjectRc(RCSRemoteResourceObject::Ptr remoteResourceObject)

	void setPlatformInfo();	// OCPlatform::registerPlatformInfo

	void setDeviceInfo();	// OCPlatform::registerDeviceInfo	

	void discoverResourceHost();

	void discoverResources();

	// Start the resource host
	OCStackResult startResourceHost;

	// Stop the resource host
	OCStackResult stopResourceHost;

*/

/* TODO LIST:
 *
 * 2. Make new header file with list of available resource types.
 *  - Resource types are declared by oic.r.<resource type>
 *      - i.e. oic.r.brightness. oic.r.light
 * 5. Add DEBUG function to get the list of available resources.
 * 6. Write the test program to select desired resource and its attribute.
 */

/* TODO:
 * MOVE INTO A HEADER FILES
 */

namespace OIC { namespace Service { namespace RichServer
{

	using namespace OC;
	using namespace OIC;
	using namespace OIC::Service;

    constexpr unsigned int CONTROLLER_MONITORING_SECOND = 5l;
    constexpr unsigned int CONTROLLER_MONITORING_MILLISECOND = CONTROLLER_MONITORING_SECOND * 1000;

    const std::string HOSTING_TAG = "/hosting";
    const auto HOSTING_TAG_SIZE = HOSTING_TAG.size();

    class Controller
	{
    private:
		typedef std::string ResourceKey;

    public:
        typedef std::unique_ptr<Controller> Ptr;
        typedef std::unique_ptr<const Controller> ConstPtr;

	public:
		/** 
          *	Default Constructor.
          *
          *	Initialize platform and device info.
          *	Starts by discovering resource hosts and stores them in the resource list
          *	Discovers other resources afterwards.
          */
        Controller();

        /**
          * @brief Default Constructor
          *
          * @param platformInfo Info regarding the platform
          * @param deviceInfo   Char* naming the device
          */
        Controller(OCPlatformInfo &platformInfo, OCDeviceInfo &deviceInfo);

		/**
          * Destructor.
          *
          *	Clear all memory and stop all processes.
          */
        ~Controller();

        /**
          * Starts the Rich Server process
          */
        void start();

        /**
         '* Stops the Rich Server process
          */
        void stop();

        /**
          * @brief Prints the data of an resource object
          *
          * @param resurce      Pointer holding the resource data
          *
          * @return OC_NO_RESOURCE if the resource doesn't exist.
          */
        OCStackResult printResourceData(RCSRemoteResourceObject::Ptr resource);

    private:
		/**
		  * Map containing all discovered resources. 
		  */
        std::unordered_map<ResourceKey, RCSRemoteResourceObject::Ptr> m_resourceList;

		 /**
		   * DiscoveryTask used to cancel and observe the discovery process.
		   */
        RCSDiscoveryManager::DiscoveryTask::Ptr m_discoveryTask;
        RCSDiscoveryManager::ResourceDiscoveredCallback m_discoverCallback;

        /**
          * Mutex when writing and reading to/from a ResourceHosting
          */
        std::mutex m_resourceMutex;

        /**
          * Timer used for monitoring
          */
         ExpiryTimer m_monitoringTimer;
         unsigned int m_monitoringTimerHandler;

	private:

         /**
          * @brief configurePlatform Configures the platform
          */
         void configurePlatform();

         /**
          * @brief foundResourceCBRD
          * @param resource
          */
         static void foundResourceCBRD(std::shared_ptr< OC::OCResource > resource);

        /**
          * @brief Function callback for found resources
          *
          * @param resource     The discovered resource.
          */
        void foundResourceCallback(std::shared_ptr<RCSRemoteResourceObject> resource);

        /**
         * @brief createResourceObject  Create a mirrored resource to be inserted into the list
         *                              of found device
         *
         * @param resource              The resource to be mirrored
         *
         * @return A pointer to the new mirrored object
         */
        auto createResourceObject(const RCSRemoteResourceObject::Ptr &resource) -> Ptr;

		/**
		  * Start the Resource Host. Initiates resource discovery
		  * and stores the discovered resources.
		  *
		  * @return Result of the startup
		  */
		OCStackResult startResourceHost();

		/**
		  * Stop the Resource Host. Clears all memory used by 
		  * the resource host.
		  *
		  * @return Result of the shutdown
		  */
		OCStackResult stopResourceHost();

        /**
         * @brief Callback when getting the remote attributes
         *
         * @param attr          Attributes received from the server
         * @param eCode         Result code of the initiate request
         */
        void getAttributesCallback(const RCSResourceAttributes& attr, int eCode);

		/**
		  * Sets the device information
		  *
          * @param deviceInfo   Container with all platform info.
		  */
        void setDeviceInfo(OCDeviceInfo &deviceInfo);

		/**
          * Sets the device information. Uses default parameters.
		  */
		void setDeviceInfo();

		/**
		  *	Sets the platform information.
          *
          * @param platformInfo Container with all platform info
		  */
        void setPlatformInfo(OCPlatformInfo &platformInfo);

		/**
		  *	Sets the platform information. Uses default parameters
		  */
		void setPlatformInfo();


		/**
          *  @brief Disovery of resources
		  *
		  *  @param address 	mutlicast or unicast address using RCSAddress class
		  *  @param cb 			Callback to which discovered resources are notified
		  *  @param uri 		Uri to discover. If null, do not include uri in discovery
          *  @param type        Resource type used as discovery filter
		  *
		  *  @return Pointer to the discovery task.
		  */
        RCSDiscoveryManager::DiscoveryTask::Ptr discoverResource(RCSDiscoveryManager::ResourceDiscoveredCallback cb,
            RCSAddress address = RCSAddress::multicast(), std::string uri = std::string(""),
            std::string type = std::string(""));

        /**
          *  @brief Disovery of resources
          *
          *  @param address 	mutlicast or unicast address using RCSAddress class
          *  @param cb 			Callback to which discovered resources are notified
          *  @param uri 		Uri to discover. If null, do not include uri in discovery
          *  @param types       Resources types used as discovery filter
          *
          *  @return Pointer to the discovery task.
          */
        RCSDiscoveryManager::DiscoveryTask::Ptr discoverResource(RCSDiscoveryManager::ResourceDiscoveredCallback cb,
            std::vector<std::string> &types, RCSAddress address = RCSAddress::multicast(), std::string uri = std::string(""));


        /**
          * @brief Looks up the list of known resources type
          *
          * @param resource     Pointer to the resource object
          *
          * @return True if the type is found, false otherwise.
          */
        bool isResourceLegit(RCSRemoteResourceObject::Ptr resource);

        /**
         * @brief Callback invoked when a resource changed state
         *
         * @param resourceState Current state of the resource
         */
        void monitoringCallback(ResourceState state);

	protected:

	};
} } }

#endif /* _CONTROLLER_H_ */

