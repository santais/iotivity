#include "Controller.h"

namespace OIC { namespace Service { namespace RichServer
{
    Controller::Controller() :
        m_discoveryTask(nullptr),
        m_discoverCallback(std::bind(&Controller::foundResourceCallback, this, std::placeholders::_1)),
        m_resourceList()
	{
        std::cout << "Initializing default constructor " << std::endl;
        // Set default platform and device information
        Controller::setDeviceInfo();
        //Controller::setPlatformInfo();

        this->configurePlatform();
	}

    /**
      * @brief Default Constructor
      *
      * @param platformInfo Info regarding the platform
      * @param deviceInfo   Char* naming the device
      */
    Controller::Controller(OCPlatformInfo &platformInfo, OCDeviceInfo &deviceInfo) :
        m_discoveryTask(nullptr),
        m_discoverCallback(std::bind(&Controller::foundResourceCallback, this, std::placeholders::_1)),
        m_resourceList()
    {
        // Set the platform and device information
        Controller::setDeviceInfo(deviceInfo);
        Controller::setPlatformInfo(platformInfo);

        this->configurePlatform();
    }


    Controller::~Controller()
	{
        // Clear the resource list
        m_resourceList.clear();

        Controller::stopResourceHost();
	}

    /**
      * Starts the Rich Server process
      */
    void Controller::start()
    {


        // Start the Resource Host
       /* if (Controller::startResourceHost() != OC_STACK_OK)
        {
            std::cerr << "Unable to start Resource Host. " << std::endl;
        }*/

        // DEBUG. Discover RD Light and Button
        std::ostringstream ss;
        ss << OC_RSRVD_WELL_KNOWN_URI;// << "?rt=oic.r.light";
        OC::OCPlatform::findResource("", ss.str(), CT_DEFAULT, &this->foundResourceCBRD);

        /**std::ostringstream ss1;
        ss1 << OC_RSRVD_WELL_KNOWN_URI << "?rt=oic.r.button";
        OC::OCPlatform::findResource("", ss1.str(), CT_DEFAULT, &this->foundResourceCBRD);

        std::ostringstream ss2;
        ss2 << OC_RSRVD_WELL_KNOWN_URI << "?rt=oic.r.thermostat";
        OC::OCPlatform::findResource("", ss2.str(), CT_DEFAULT, &this->foundResourceCBRD);*/

        // Start the discovery module
       // m_discoveryTask = Controller::discoverResource(m_discoverCallback);

    }
    /**
     '* Stops the Rich Server process
      */
    void Controller::stop()
    {
        if(!m_discoveryTask->isCanceled())
        {
            m_discoveryTask->cancel();
        }

        Controller::stopResourceHost();

        // DEBUG. TODO: Remove
        std::cout << "Number of resources discovered by stop: " << m_resourceList.size() << std::endl;
    }

    /**
     * @brief configurePlatform Configures the platform
     */
    void Controller::configurePlatform()
    {
        PlatformConfig config
        {
            OC::ServiceType::InProc, ModeType::Both, "0.0.0.0", 0, OC::QualityOfService::NaQos
        };
        OCPlatform::Configure(config);
    }

    /**
      * @brief Prints the data of an resource object
      *
      * @param resurce  Pointer holding the resource data
      *
      * @return OC_NO_RESOURCE if the resource doesn't exist.
      */
    OCStackResult Controller::printResourceData(RCSRemoteResourceObject::Ptr resource)
    {
        std::cout << "===================================================" << std::endl;
        std::cout << "\t Uri of the resources: " << resource->getUri() << std::endl;
        std::cout << "\t Host address of the resources: " << resource->getAddress() << std::endl;
        std::cout << "\t Types are: " << std::endl;

        for (auto type : resource->getTypes())
        {
            std::cout << "\t\t type " << type << std::endl;
        }

        std::cout << "\t Interfaces are: " << std::endl;
        for (auto interface : resource->getInterfaces())
        {
            std::cout << "\t\t interface " << interface << std::endl;
        }

        // DEBUG
        // Get the attibutes.
        if(Controller::isResourceLegit(resource))
        {
            resource->getRemoteAttributes(std::bind(&Controller::getAttributesCallback, this, std::placeholders::_1,
                                                    std::placeholders::_2));
        }
    }

    /**
     * @brief foundResourceCBRD
     * @param resource
     */
    void Controller::foundResourceCBRD(std::shared_ptr< OC::OCResource > resource)
    {
        try
        {
            std::cout << "inside foundResourceCBRD" << std::endl;
            if(resource)
            {
                std::cout << "Found RD resource with uri: " << resource->uri() << std::endl;
            }
        }
        catch (std::exception &ex)
        {
            std::cout << "Exception :" << ex.what() << " in foundResource" << std::endl;
        }
    }


     /**
       * @brief Function callback for found resources
       *
       * @param resource     The discovered resource.
       */
     void Controller::foundResourceCallback(RCSRemoteResourceObject::Ptr resource)
     {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        if(this->isResourceLegit(resource))
        {
            this->printResourceData(resource);

            if(m_resourceList.insert({resource->getUri() + resource->getAddress(), resource}).second)
            {
                std::cout << "Added device: " << resource->getUri() + resource->getAddress() << std::endl;
                std::cout << "Device successfully added to the list" << std::endl;
            }
        }
     }


    /**
      * Start the Resource Host. Initiates resource  discovery
      * and stores the discovered resources.
      *
      * @return Result of the startup
      */
    OCStackResult Controller::startResourceHost()
    {
        OCStackResult result = OC_STACK_OK;

        try
        {
            ResourceHosting::getInstance()->startHosting();
        }catch(const RCSPlatformException &e)
        {
            OIC_HOSTING_LOG(DEBUG,
                    "[OICStartCoordinate] platformException, reason:%s", e.what());
            result = OC_STACK_ERROR;
        }catch(const RCSInvalidParameterException &e)
        {
            OIC_HOSTING_LOG(DEBUG,
                    "[OICStartCoordinate] InvalidParameterException, reason:%s", e.what());
            result = OC_STACK_ERROR;
        }catch(...)
        {
            OIC_HOSTING_LOG(DEBUG, "[OICStartCoordinate] Unknown Exception");
            result = OC_STACK_ERROR;
        }

        return result;
    }

    /**
      * Stop the Resource Host. Clears all memory used by
      * the resource host.
      *
      * @return Result of the shutdown
      */
    OCStackResult Controller::stopResourceHost()
    {
        OCStackResult result = OC_STACK_OK;

        ResourceHosting::getInstance()->stopHosting();

        return result;
    }

    /**
     * @brief Callback when getting the remote attributes
     *
     * @param attr          Attributes received from the server
     * @param eCode         Result code of the initiate request
     */
    void Controller::getAttributesCallback(const RCSResourceAttributes& attr, int eCode)
    {
        if (eCode == OC_STACK_OK)
        {
            if(attr.empty())
            {
                std::cout << "Attributes empty" << std::endl;
            }
            else
            {
                std::cout << "\t Attributes: " << std::endl;

                for (const auto& attribute : attr)
                {
                    std::cout << "\t\t Key: " << attribute.key() << std::endl;
                    std::cout << "\t\t Value: " << attribute.value().toString() << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "Get attributes request failed with code: " << eCode << std::endl;
        }
    }

    /**
      * Sets the device information
      *
      * @param deviceInfo 			Container with all platform info.
      */
    void Controller::setDeviceInfo(OCDeviceInfo &deviceInfo)
    {
        OC::OCPlatform::registerDeviceInfo(deviceInfo);
    }

    /**
      * Sets the device information. Uses default parameters.
      */
    void Controller::setDeviceInfo()
    {
        OCDeviceInfo deviceInfo;
        deviceInfo.deviceName = "OIC Controller";

        OC::OCPlatform::registerDeviceInfo(deviceInfo);
    }

    /**
      *	Sets the platform information.
      *
      * @param platformInfo 		Container with all platform info
      */
    void Controller::setPlatformInfo(OCPlatformInfo &platformInfo)
    {
        OC::OCPlatform::registerPlatformInfo(platformInfo);
    }

    /**
      *	Sets the platform information. Uses default parameters
      */
    void Controller::setPlatformInfo()
    {
        OCPlatformInfo platformInfo;

        platformInfo.dateOfManufacture = "01/03/16";
        platformInfo.firmwareVersion = "1.0";
        platformInfo.hardwareVersion = "1.0";
        platformInfo.manufacturerName = "Schneider Electric ECP Controller";
        platformInfo.manufacturerUrl = "controller";
        platformInfo.modelNumber = "1.0";
        platformInfo.operatingSystemVersion = "1.0";
        platformInfo.platformID = "1";
        platformInfo.platformVersion = "1.0";
        platformInfo.supportUrl = "controller";

        OC::OCPlatform::registerPlatformInfo(platformInfo);
    }

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
    RCSDiscoveryManager::DiscoveryTask::Ptr Controller::discoverResource(RCSDiscoveryManager::ResourceDiscoveredCallback cb,
        RCSAddress address, std::string uri, std::string type)

    {
        RCSDiscoveryManager::DiscoveryTask::Ptr discoveryTask;

        if (type.empty() && uri.empty())
        {
            discoveryTask = RCSDiscoveryManager::getInstance()->discoverResource(address, cb);
        }
        else if (type.empty() && !(uri.empty()))
        {
            discoveryTask = RCSDiscoveryManager::getInstance()->discoverResource(address, uri, cb);
        }
        else if (!(type.empty()) && uri.empty())
        {
            discoveryTask = RCSDiscoveryManager::getInstance()->discoverResourceByType(address, type, cb);
        }
        else
        {
            discoveryTask = OIC::Service::RCSDiscoveryManager::getInstance()->discoverResourceByType(address, uri, type, cb);
        }

        return discoveryTask;
    }

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
    RCSDiscoveryManager::DiscoveryTask::Ptr Controller::discoverResource(RCSDiscoveryManager::ResourceDiscoveredCallback cb,
        std::vector<std::string> &types, RCSAddress address, std::string uri)
    {
        RCSDiscoveryManager::DiscoveryTask::Ptr discoveryTask;

        if(uri.empty())
        {
            discoveryTask = RCSDiscoveryManager::getInstance()->discoverResourceByTypes(address, types, cb);
        }
        else
        {
            discoveryTask = RCSDiscoveryManager::getInstance()->discoverResourceByTypes(address, uri, types, cb);
        }

        return discoveryTask;
    }

    /**
      * @brief Looks up the list of known resources type
      *
      * @param resource     Pointer to the resource object
      *
      * @return True if the type is found, false otherwise.
      */
    bool Controller::isResourceLegit(RCSRemoteResourceObject::Ptr resource)
    {
        // Filter platform and device resources
        std::string uri = resource->getUri();
        std::vector<std::string> types = resource->getTypes();

        if (uri == "/oic/p" || uri == "/oic/d")
            return false;
        else if (uri.compare(
                uri.size()-HOSTING_TAG_SIZE, HOSTING_TAG_SIZE, HOSTING_TAG) == 0)
        {
            std::cout << "Device: " << resource->getUri() << " is not a legit device. Device is hosting" << std::endl;
            return false;
        }
        else if (std::find_if(types.begin(), types.end(), [](const std::string &type) {return type == OIC_TYPE_RESOURCE_HOST;}) != types.end())
        {
            std::cout << "Resource type is Hosting. Not adding an additional monitoring state" << std::endl;
            return true;
        }
        else
        {
            // Check if the resource is being monitored
            if(!(resource->isMonitoring()))
            {
                std::cout << "Starting monitoring of resource: " << resource->getUri() << std::endl;
                resource->startMonitoring(std::bind(&Controller::monitoringCallback, this,
                                                    std::placeholders::_1));
            }
            return true;
        }
    }

    /**
     * TODO: Remove devices where signal has been lost from the list
     * @brief Callback invoked when a resource changed state
     */
    void Controller::monitoringCallback(ResourceState state)
    {
        std::cout << "Inside Monitoring Callback" << std::endl;
        // Lock mutex to ensure no resource is added to the list while erasing
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        bool resetDiscoveryManager(false);

        for (auto iterator = m_resourceList.begin(); iterator != m_resourceList.end();)
        {
            ResourceState newState = iterator->second->getState();
            if (newState == ResourceState::LOST_SIGNAL || newState == ResourceState::DESTROYED)
            {
                std::cout << "Removing resource: " << iterator->second->getUri() << std::endl;
                m_resourceList.erase(iterator++);

                // Reset discovery manager.
                // ISSUE MP: #0001
                resetDiscoveryManager = true;
            }
            else
            {
                iterator++;
            }
        }

        // Reset discovery manager.
        // ISSUE MP: #0001
        /*if(resetDiscoveryManager)
        {
            m_discoveryTask->cancel();
            m_discoveryTask = Controller::discoverResource(m_discoverCallback);
        }*/

    }

} } }
