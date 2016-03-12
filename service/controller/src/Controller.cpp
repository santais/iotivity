#include "Controller.h"
#include "RCSRemoteResourceObject.h"

namespace OIC { namespace Service
{

    constexpr unsigned int CONTROLLER_POLLING_DISCOVERY_MS = 5000; // in milliseconds

/********************************** DiscoveryMangerInfo *************************************/

    /**
     * @brief Controller::DiscoveryManagerInfo::DiscoveryManagerInfo
     */
    DiscoveryManagerInfo::DiscoveryManagerInfo()
    {
        ;
    }

    /**
     * @brief Controller::DiscoveryManagerInfo::DiscoveryManagerInfo
     * @param host
     * @param uri
     * @param types
     * @param cb
     */
    DiscoveryManagerInfo::DiscoveryManagerInfo(const string&host, const string& uri, const std::vector<std::string>& types, FindCallback cb)
        : m_host(host),
          m_relativeUri(uri),
          m_resourceTypes(std::move(types)),
          m_discoveryCb(std::move(cb)) {;}

    /**
     * @brief Controller::DiscoveryManagerInfo::discover
     */
    void DiscoveryManagerInfo::discover() const
    {
        for(auto& type : m_resourceTypes)
        {
            OC::OCPlatform::findResource(m_host, m_relativeUri + "?rt=" + type, CT_IP_USE_V4, m_discoveryCb, QualityOfService::NaQos);
        }
    }


/********************************** DsicoveryManager *************************************/

    /**
     * @brief Controller::DiscoveryManager::DiscoveryManager
     * @param time_ms
     */
    DiscoveryManager::DiscoveryManager(cbTimer time_ms) : m_timerMs(time_ms), m_isRunning(false) {}


    /**
     * @brief Controller::DiscoveryManager::~DiscoveryManager
     */
    DiscoveryManager::~DiscoveryManager()
    {

    }

    /**
     * @brief isSearching
     * @return
     */
    bool DiscoveryManager::isSearching() const
    {
        return m_isRunning;
    }

    /**
     * @brief cancel
     */
    void DiscoveryManager::cancel()
    {
        std::lock_guard<std::mutex> lock(m_discoveryMutex);
        if(m_isRunning)
        {
           m_isRunning = false;
        }
    }

    /**
     * @brief setTimer
     * @param time_ms
     */
    void DiscoveryManager::setTimer(const cbTimer time_ms)
    {
        m_timerMs = time_ms;
    }

    /**
     * @brief discoverResource
     * @param types
     * @param cb
     * @param host
     */
    void DiscoveryManager::discoverResource(const std::string& uri, const std::vector<std::string>& types, FindCallback cb,
                                std::string host )
    {
        std::lock_guard<std::mutex> lock(m_discoveryMutex);

        m_isRunning = true;

        DiscoveryManagerInfo discoveryInfo(host, uri.empty() ? OC_RSRVD_WELL_KNOWN_URI : uri, types,
                                           std::move(cb));

        m_discoveryInfo = std::move(discoveryInfo);

        m_discoveryInfo.discover();

        m_timer.post(m_timerMs, std::bind(&DiscoveryManager::timeOutCB, this));
    }

    /**
     * @brief discoverResource
     * @param type
     * @param cb
     * @param host
     */
    void DiscoveryManager::discoverResource(const std::string& uri, const std::string& type, FindCallback cb,
                                std::string host)
    {
        std::lock_guard<std::mutex> lock(m_discoveryMutex);

        m_isRunning = true;

        DiscoveryManagerInfo discoveryInfo(host, uri.empty() ? OC_RSRVD_WELL_KNOWN_URI : uri, std::vector<std::string> { type },
                                           std::move(cb));

        m_discoveryInfo = std::move(discoveryInfo);

        m_discoveryInfo.discover();

        // DEBUG
        std::cout << "Starting timer for DiscoveryManager with timer: " << m_timerMs << std::endl;
        m_timer.post(m_timerMs, std::bind(&DiscoveryManager::timeOutCB, this));
    }


    /**
     * @brief timeOutCB
     * @param id
     */
    void DiscoveryManager::timeOutCB()
    {
        // Check if the mutex is free
        std::lock_guard<std::mutex> lock(m_discoveryMutex);

        // Only restartt he callback timer if the process has not been stopped.
        if(m_isRunning)
        {
            m_discoveryInfo.discover();

            m_timer.post(m_timerMs, std::bind(&DiscoveryManager::timeOutCB, this));
        }
    }

/****************************************** Controller ************************************************/

    Controller::Controller() :
        m_discoverCallback(std::bind(&Controller::foundResourceCallback, this, std::placeholders::_1)),
        m_resourceList(),
        m_RDStarted(false)
	{
        // Set default platform and device information
        Controller::setDeviceInfo();
        //Controller::setPlatformInfo();

        this->configurePlatform();

        // Set up the scene and the collection
        SceneList::getInstance()->setName("Office");
        m_sceneCollection = SceneList::getInstance()->addNewSceneCollection();
        m_sceneCollection->setName("Meeting Room");

        m_sceneStart = m_sceneCollection->addNewScene("Start Conference");
        m_sceneStop = m_sceneCollection->addNewScene("Stop Conference");
    }

    /**
      * @brief Default Constructor
      *
      * @param platformInfo Info regarding the platform
      * @param deviceInfo   Char* naming the device
      */
    /*Controller::Controller(OCPlatformInfo &platformInfo, OCDeviceInfo &deviceInfo) :
        m_discoverCallback(std::bind(&Controller::foundResourceCallback, this, std::placeholders::_1)),
        m_resourceList(),
        m_RDStarted(false)
    {
        // Set the platform and device information
        Controller::setDeviceInfo(deviceInfo);
        Controller::setPlatformInfo(platformInfo);

        this->configurePlatform();
    }*/

    /**
     * @brief getInstance
     * @return
     */
    Controller* Controller::getInstance()
    {
        static Controller* instance(new Controller);
        return instance;
    }


    Controller::~Controller()
	{
        m_resourceList.clear();

        this->stop();
	}

    /**
      * @brief Start the Controller
      *
      * @return The result of the startup. OC_STACK_OK on success
      */
    OCStackResult Controller::start()
    {
        // Start the discoveryManager
        const std::vector<std::string> types{OIC_DEVICE_LIGHT, OIC_DEVICE_BUTTON, OIC_DEVICE_SENSOR};

        m_discoveryTask = Controller::discoverResource(m_discoverCallback);//, types);

        // Start the discovery manager
        return(this->startRD());
    }
    /**
      * @brief Stop the Controller
      *
      * @param OC_STACK_OK on success
      */
    OCStackResult Controller::stop()
    {
        OCStackResult result = this->stopRD();

        if(!m_discoveryTask->isCanceled())
        {
            m_discoveryTask->cancel();
        }

        /*for (auto iterator = m_resourceList.begin(); iterator != m_resourceList.end();)
        {
            if(iterator->second->isCaching())
                iterator->second->stopCaching();
            if(iterator->second->isMonitoring())
                iterator->second->stopMonitoring();
        }*/

        // DEBUG. TODO: Remove
        std::cout << "Number of resources instance discovered by stop() call: " << m_resourceList.size() << std::endl;

        return result;
    }

    /**
     * @brief configurePlatform Configures the platform
     */
    void Controller::configurePlatform()
    {
        // Create PlatformConfig object
        PlatformConfig cfg {
            OC::ServiceType::InProc,
            OC::ModeType::Both,
            "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
            0,         // Uses randomly available port
            OC::QualityOfService::LowQos
        };

        OCPlatform::Configure(cfg);

        /*OCStackResult result = OCInit(NULL, 0, OC_CLIENT_SERVER);
        if(result != OC_STACK_OK)
        {
            std::cerr << "Failed to initialize OIC server" << std::endl;
        }*/
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
        /*if(this->isResourceLegit(resource))
        {
            resource->getRemoteAttributes(std::bind(&Controller::getAttributesCallback, this, std::placeholders::_1,
                                                    std::placeholders::_2));
        }*/
    }


     /**
       * @brief Function callback for found resources
       *
       * @param resource     The discovered resource.
       */
     void Controller::foundResourceCallback(RCSRemoteResourceObject::Ptr resource)
     {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        std::cout << __func__ << std::endl;

        if(this->isResourceLegit(resource))
        {
            if(m_resourceList.insert({resource->getUri() + resource->getAddress(), resource}).second)
            {
                this->printResourceData(resource);
                this->addResourceToScene(resource);

                // Start caching the resource
                if(!resource->isCaching())
                {
                    resource->startCaching(std::bind(&Controller::cacheUpdateCallback, this, std::placeholders::_1));
                }

                if(!resource->isMonitoring())
                {
                    resource->startMonitoring(std::bind(&Controller::stateChangeCallback, this, std::placeholders::_1));
                }

                std::cout << "\tAdded device: " << resource->getUri() + resource->getAddress() << std::endl;
                std::cout << "\tDevice successfully added to the list" << std::endl;
            }
        }
     }


    /**
      * Start the Resource Host. Initiates resource  discovery
      * and stores the discovered resources.
      *
      * @return Result of the startup
      */
    OCStackResult Controller::startRD()
    {
        std::cout << "Inside startRD" << std::endl;
        if(!m_RDStarted)
        {
            std::cout << "Starting OCResource Directory" << std::endl;
            if (OCRDStart() != OC_STACK_OK)
            {
                std::cerr << "Failed to start RD Server" << std::endl;
                return OC_STACK_ERROR;
            }

            std::cout << "RD Server started successfully" << std::endl;
            m_RDStarted = true;
        }

        return OC_STACK_OK;
    }

    /**
      * Stop the Resource Host. Clears all memory used by
      * the resource host.
      *
      * @return Result of the shutdown
      */
    OCStackResult Controller::stopRD()
    {
        if(m_RDStarted)
        {
            if(OCRDStop() != OC_STACK_OK)
            {
                std::cout << "Failed to stop the RD Server" << std::endl;
                return OC_STACK_ERROR;
            }
        }

        return OC_STACK_OK;
    }

    /**
     * @brief Callback when getting the remote attributes
     *
     * @param attr          Attributes received from the server
     * @param eCode         Result code of the initiate request
     */
    void Controller::getAttributesCallback(const RCSResourceAttributes& attr, int eCode)
    {
        std::cout << __func__ << std::endl;

        if (eCode == OC_STACK_OK)
        {
            this->printAttributes(attr);
        }
        else
        {
            std::cerr << "Get attributes request failed with code: " << eCode << std::endl;
        }
    }

    /**
     * @brief printAttributes Prints the attributes of a resource
     *
     * @param attr          Attributes to be printed
     */
    void Controller::printAttributes(const RCSResourceAttributes& attr)
    {
        if(attr.empty())
        {
            std::cout << "\tAttributes empty" << std::endl;
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
        RCSAddress address, const std::string& uri, const std::string& type)

    {
        RCSDiscoveryManager::DiscoveryTask::Ptr discoveryTask;

        try
        {
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
        }
        catch(const RCSPlatformException& e)
        {
             std::cout << e.what() << std::endl;
        }
        catch(const RCSException& e)
        {
            std::cout << e.what() << std::endl;
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
       const std::vector<std::string> &types, RCSAddress address, const std::string& uri)
    {
        RCSDiscoveryManager::DiscoveryTask::Ptr discoveryTask;

        try
            {
            if(uri.empty())
            {
                discoveryTask = RCSDiscoveryManager::getInstance()->discoverResourceByTypes(address, types, cb);
            }
            else
            {
                discoveryTask = RCSDiscoveryManager::getInstance()->discoverResourceByTypes(address, uri, types, cb);
            }
        }
        catch(const RCSPlatformException& e)
        {
             std::cout << e.what() << std::endl;
        }
        catch(const RCSException& e)
        {
            std::cout << e.what() << std::endl;
        }

        return discoveryTask;
    }

    /**
     * @brief getCacheUpdateCallback Callback invoked when a changed of the paramters
     * of the resource occurs.
     *
     * @param attr The new attributes of the resource
     */
    void Controller::cacheUpdateCallback(const RCSResourceAttributes& attr)
    {
        std::cout << __func__ << std::endl;

        this->printAttributes(attr);

        // Check if the attribute is "state" and is "on"
        for(auto const &attribute : attr)
        {
            const std::string key = attribute.key();
            const RCSResourceAttributes::Value value = attribute.value();
            if(key == "state" && value.toString() == "true")
            {
                std::cout << "\tButton state is ON. Setting scene on" << std::endl;
                m_sceneStart->execute(std::bind(&Controller::executeSceneCallback, this, std::placeholders::_1));
            }
            else if(key == "state" && value.toString() == "false")
            {
                std::cout << "\tButton state is OFF. Setting scene off" << std::endl;
                m_sceneStop->execute(std::bind(&Controller::executeSceneCallback, this, std::placeholders::_1));
            }
        }
    }

    /**
     * @brief stateChangeCallback Callback invoked when a change in the resources'
     * state is encountered
     *
     * @param state         New state of the resource
     */
    void Controller::stateChangeCallback(ResourceState state)
    {
        std::cout << __func__ << std::endl;

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
            }
            else
            {
                iterator++;
            }
        }
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
        {
            return false;
        }
        else if(uri.size() > HOSTING_TAG_SIZE)
        {
            if (uri.compare(
                    uri.size()-HOSTING_TAG_SIZE, HOSTING_TAG_SIZE, HOSTING_TAG) == 0)
            {
                std::cout << "Device: " << uri << " is not a legit device. Device is hosting" << std::endl;
                return false;
            }
            return true;
        }
        else if (std::find_if(types.begin(), types.end(), [](const std::string &type) {return type == OIC_TYPE_RESOURCE_HOST;}) != types.end())
        {
            std::cout << "Resource type is Hosting. Not adding an additional monitoring state" << std::endl;
            return false;
        }
        else
        {
            return true;
        }
    }


    /**
     * @brief addResourceToScene Adds a resource to the two scenes
     *
     * @param resource THe resource to be added
     */
    void Controller::addResourceToScene(RCSRemoteResourceObject::Ptr resource)
    {
        // Search through the resource types
        for(const auto& type : resource->getTypes())
        {
            if(type.compare(OIC_DEVICE_LIGHT) == 0)
            {
                m_sceneStart->addNewSceneAction(resource, "power", "on");
                m_sceneStop->addNewSceneAction(resource, "power", "off");
            }
        }
    }

    /**
     * @brief executeSceneCallback Cb invoked when a scene is executed
     *
     * @param eCode Result of the scene execution.
     */
    void Controller::executeSceneCallback(int eCode)
    {
        std::cout << __func__ << std::endl;

        std::cout << "Result of eCode: " << eCode << std::endl;
    }

} }
