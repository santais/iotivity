#include "../include/Controller.h"
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

        /*m_discoveryManager.setTimer(CONTROLLER_POLLING_DISCOVERY_MS);
        m_discoveryManager.discoverResource(OC_RSRVD_WELL_KNOWN_URI, OIC_DEVICE_LIGHT, m_discoverCallback);*/
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
        // Clear the resource list
        m_resourceList.clear();

        this->stopRD();
	}

    /**
      * @brief Start the Controller
      *
      * @return The result of the startup. OC_STACK_OK on success
      */
    OCStackResult Controller::start()
    {
        // Start the discoveryManager
        std::vector<std::string> types{OIC_DEVICE_LIGHT, OIC_DEVICE_BUTTON, "oic.d.fan"};
        m_discoveryTask = Controller::discoverResource(m_discoverCallback, types);

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

        // DEBUG. TODO: Remove
        std::cout << "Number of resources instance discovered by stop() call: " << m_resourceList.size() << std::endl;

        return result;
    }

    /**
     * @brief configurePlatform Configures the platform
     */
    void Controller::configurePlatform()
    {
        /*PlatformConfig config
        {
            OC::ServiceType::InProc, ModeType::Both, "0.0.0.0", 0, OC::QualityOfService::NaQos
        };
        OCPlatform::Configure(config);*/

        OCStackResult result = OCInit(NULL, 0, OC_CLIENT_SERVER);
        if(result != OC_STACK_OK)
        {
            std::cerr << "Failed to initialize OIC server" << std::endl;
        }
    }

    void getRequest(const HeaderOptions&, const OCRepresentation& rep, const int eCode)
    {

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
       * @brief Function callback for found resources
       *
       * @param resource     The discovered resource.
       */
     void Controller::foundResourceCallback(RCSRemoteResourceObject::Ptr resource)
     {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        if(this->isResourceLegit(resource))
        {
            if(m_resourceList.insert({resource->getUri() + resource->getAddress(), resource}).second)
            {
                this->printResourceData(resource);

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

        /*if (uri == "/oic/p" || uri == "/oic/d")
            return false;
        else*/ if (uri.compare(
                uri.size()-HOSTING_TAG_SIZE, HOSTING_TAG_SIZE, HOSTING_TAG) == 0)
        {
            std::cout << "Device: " << uri << " is not a legit device. Device is hosting" << std::endl;
            return false;
        }
        else if (std::find_if(types.begin(), types.end(), [](const std::string &type) {return type == OIC_TYPE_RESOURCE_HOST;}) != types.end())
        {
            std::cout << "Resource type is Hosting. Not adding an additional monitoring state" << std::endl;
            return true;
        }
        else
        {
            return true;
        }
    }

    /**
     * @brief getRequest CB called when a get request has been answered
     *
     * @param options       Header options containing vendor specific information
     * @param rep           Attribute representation
     * @param eCode         Result of the get request
     */
    void Controller::getRequestCb(const HeaderOptions&, const OCRepresentation& rep, const int eCode)
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
                    std::cout << std::boolalpha <<  values->b << std::endl;
                break;
                case OCREP_PROP_STRING:
                    std::cout << values->str << std::endl;
                break;

            }
            values = values->next;
        }
    }

    /**
     * @brief putRequestCb CB called when a put request has been answered
     *
     * @param options       Header options containing vendor specific information
     * @param rep           Attribute representation
     * @param eCode         Result of the PUT request
     */
    void Controller::putRequestCb(const HeaderOptions& options, const OCRepresentation& rep, const int eCode)
    {

    }

    /**
     * @brief postRequestCB CB called when a put request has been answered
     *
     * @param options       Header options containing vendor specific information
     * @param rep           Attribute representation
     * @param eCode         Result of the POST request
     */
    void Controller::postRequestCb(const HeaderOptions& options, const OCRepresentation& rep, const int eCode)
    {

    }

    /**
     * @brief putRequestCb CB called when a put request has been answered
     *
     * @param options       Header options containing vendor specific information
     * @param rep           Attribute representation
     * @param eCode         Result of the PUT request
     * @param sequenceNum   The current number of notified calls. Used for synchronization.
     */
    void Controller::onObserve(const HeaderOptions& options, const OCRepresentation& rep, const int& eCode,
                   const int& sequenceNumber)
    {

    }

} }
