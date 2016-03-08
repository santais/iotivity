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
#include <condition_variable>

#include "ResourceTypes.h"

#include "OCPlatform.h"
#include "OCApi.h"
#include "ExpiryTimer.h"
#include "rd_server.h"

#include "PrimitiveResource.h"
#include "RCSResourceObject.h"
#include "RCSRemoteResourceObject.h"
#include "RCSDiscoveryManager.h"

// Scene-manager
#include "SceneList.h"



namespace OIC { namespace Service
{

	using namespace OC;
	using namespace OIC;
	using namespace OIC::Service;

    const std::string HOSTING_TAG = "/hosting";
    const auto HOSTING_TAG_SIZE = HOSTING_TAG.size();


    /**
     * @brief The DiscoveryManagerInfo class
     */
    class DiscoveryManagerInfo
    {
    public:
        /**
         * @brief DiscoveryManagerInfo
         */
        DiscoveryManagerInfo();

        /**
         * @brief DiscoveryManagerInfo
         * @param host
         * @param uri
         * @param types
         * @param cb
         */
        DiscoveryManagerInfo(const std::string& host, const std::string& uri,
                             const std::vector<std::string>& types, FindCallback cb);


        /**
         * @brief discover
         */
        void discover() const;

    private:
        std::string m_host;
        std::string m_relativeUri;
        std::vector<std::string> m_resourceTypes;
        FindCallback m_discoveryCb;
    };

    /**
     * @brief The DiscoveryManager class
     *
     * Discovers resource on the network
     */
    class DiscoveryManager
    {
        typedef long long cbTimer;

    public:
        /**
         * @brief DiscoveryManager
         * @param time_ms
         */
        DiscoveryManager(cbTimer timeMs);
        DiscoveryManager()                                          = default;
        DiscoveryManager(const DiscoveryManager& dm)                = default;
        DiscoveryManager(DiscoveryManager&& dm)                     = default;
        DiscoveryManager& operator=(const DiscoveryManager& dm)     = default;
        DiscoveryManager& operator=(DiscoveryManager&& dm)          = default;

        ~DiscoveryManager();

        /**
         * @brief isSearching
         * @return
         */
        bool isSearching() const;

        /**
         * @brief cancel
         */
        void cancel();

        /**
         * @brief setTimer
         * @param time_ms
         */
        void setTimer(cbTimer time_ms);

        /**
         * @brief discoverResource
         * @param types
         * @param cb
         * @param host
         */
        void discoverResource(const std::string& uri, const std::vector<std::string>& types, FindCallback cb,
                              std::string host = "");

        /**
         * @brief discoverResource
         * @param type
         * @param cb
         * @param host
         */
        void discoverResource(const std::string& uri, const std::string& type, FindCallback cb,
                              std::string host = "");
    private:
        /**
         * @brief m_timer
         */
        ExpiryTimer m_timer;

        /**
         * @brief m_timerMs
         */
        cbTimer m_timerMs;

        /**
         * @brief m_isRunning
         */
        bool m_isRunning;

        /**
         * @brief m_discoveryInfo
         */
        DiscoveryManagerInfo m_discoveryInfo;

        /**
         * @brief m_cancelMutex
         */
        std::mutex m_discoveryMutex;

    private:

        /**
         * @brief timeOutCB
         * @param id
         */
        void timeOutCB();
    };

    class Controller
	{
    private:
		typedef std::string ResourceKey;

    public:
        typedef std::unique_ptr<Controller> Ptr;
        typedef std::unique_ptr<const Controller> ConstPtr;

	public:
        /**
          * @brief Default Constructor
          *
          * @param platformInfo Info regarding the platform
          * @param deviceInfo   Char* naming the device
          */
        //Controller(OCPlatformInfo &platformInfo, OCDeviceInfo &deviceInfo);

        /**
         * @brief getInstance
         * @return
         */
        static Controller* getInstance();

		/**
          * Destructor.
          *
          *	Clear all memory and stop all processes.
          */
        ~Controller();

        /**
          * Start the Controller
          */
        OCStackResult start();

        /**
         '* Stops the Controller
          */
        OCStackResult stop();

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
          * Mutex when writing and reading to/from a ResourceHosting
          */
        std::mutex m_resourceMutex;
        std::condition_variable m_cond;


        /**
          * DiscoveryTask used to cancel and observe the discovery process.
          */
        RCSDiscoveryManager::DiscoveryTask::Ptr m_discoveryTask;
        RCSDiscoveryManager::ResourceDiscoveredCallback m_discoverCallback;

        /**
          *
          */
        DiscoveryManager m_discoveryManager;

        /**
          * Timer used for monitoring
          */
         ExpiryTimer m_discoveryTimer;
         unsigned int m_discoveryTimerHandler;

         /**
           * Boolean to indicate if the RD is started already
           */
         bool m_RDStarted;

	private:
         /**
           *	Default Constructor.
           *
           *	Initialize platform and device info.
           *	Starts by discovering resource hosts and stores them in the resource list
           *	Discovers other resources afterwards.
           */
         Controller();

         /**
          * @brief configurePlatform Configures the platform
          */
         void configurePlatform();

        /**
          * @brief Function callback for found resources
          *
          * @param resource     The discovered resource.
          */
        void foundResourceCallback(std::shared_ptr<RCSRemoteResourceObject> resource);

		/**
          * Start the Resource Directory Server. Initiates resource discovery
		  * and stores the discovered resources.
		  *
		  * @return Result of the startup
		  */
        OCStackResult startRD();

		/**
          * Stop the Resource Directory Server. Clears all memory used by
		  * the resource host.
		  *
		  * @return Result of the shutdown
		  */
        OCStackResult stopRD();

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
         * @brief getRequest CB called when a get request has been answered
         *
         * @param options       Header options containing vendor specific information
         * @param rep           Attribute representation
         * @param eCode         Result of the get request
         */
        static void getRequestCb(const HeaderOptions&, const OCRepresentation& rep, const int eCode);

        /**
         * @brief putRequestCb CB called when a put request has been answered
         *
         * @param options       Header options containing vendor specific information
         * @param rep           Attribute representation
         * @param eCode         Result of the PUT request
         */
        static void putRequestCb(const HeaderOptions& options, const OCRepresentation& rep, const int eCode);


        /**
         * @brief postRequestCB CB called when a put request has been answered
         *
         * @param options       Header options containing vendor specific information
         * @param rep           Attribute representation
         * @param eCode         Result of the POST request
         */
        static void postRequestCb(const HeaderOptions& options, const OCRepresentation& rep, const int eCode);

        /**
         * @brief putRequestCb CB called when a put request has been answered
         *
         * @param options       Header options containing vendor specific information
         * @param rep           Attribute representation
         * @param eCode         Result of the PUT request
         * @param sequenceNum   The current number of notified calls. Used for synchronization.
         */
        static void onObserve(const HeaderOptions& options, const OCRepresentation& rep, const int& eCode,
                       const int& sequenceNumber);

	protected:

	};
} }

#endif /* _CONTROLLER_H_ */

