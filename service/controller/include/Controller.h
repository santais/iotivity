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

#include "OCPlatform.h"
#include "OCApi.h"
#include "ExpiryTimer.h"
#include "rd_server.h"


namespace OIC { namespace Service
{

	using namespace OC;
	using namespace OIC;
	using namespace OIC::Service;

    const std::string HOSTING_TAG = "/hosting";
    const auto HOSTING_TAG_SIZE = HOSTING_TAG.size();

    class Controller
	{
    private:
		typedef std::string ResourceKey;

    public:
        typedef std::unique_ptr<Controller> Ptr;
        typedef std::unique_ptr<const Controller> ConstPtr;

     /*   typedef std::shared_ptr<OCResource> Ptr;
        typedef std::shared_ptr<const OCResource> ConstPtr;

        typedef std::shared_ptr<OCResourceRequest> Ptr;*/

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
        OCStackResult start();

        /**
         '* Stops the Rich Server process
          */
        OCStackResult stop();

        /**
          * @brief Prints the data of an resource object
          *
          * @param resurce      Pointer holding the resource data
          *
          * @return OC_NO_RESOURCE if the resource doesn't exist.
          */
        OCStackResult printResourceData(OCResource::Ptr resource);

    private:
		/**
		  * Map containing all discovered resources. 
		  */
        std::unordered_map<ResourceKey, OCResource::Ptr> m_resourceList;

        /**
          * Mutex when writing and reading to/from a ResourceHosting
          */
        std::mutex m_resourceMutex;

        /**
         * Discovery callback called when a resource is discovered
         */
        FindCallback m_discoverCallback;

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
          * @brief configurePlatform Configures the platform
          */
         void configurePlatform();

        /**
          * @brief Function callback for found resources
          *
          * @param resource     The discovered resource.
          */
        void foundResourceCallback(OCResource::Ptr resource);

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
        //void getAttributesCB(const RCSResourceAttributes& attr, int eCode);

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
          * @brief Looks up the list of known resources type
          *
          * @param resource     Pointer to the resource object
          *
          * @return True if the type is found, false otherwise.
          */
        bool isResourceLegit(OCResource::Ptr resource);


	protected:

	};
} }

#endif /* _CONTROLLER_H_ */

