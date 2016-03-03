#include "Controller.h"

namespace OIC { namespace Service
{
    Controller::Controller() :
        m_discoverCallback(std::bind(&Controller::foundResourceCallback, this, std::placeholders::_1)),
        m_resourceList(),
        m_RDStarted(false)
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
        m_discoverCallback(std::bind(&Controller::foundResourceCallback, this, std::placeholders::_1)),
        m_resourceList(),
        m_RDStarted(false)
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

        this->stopRD();
	}

    /**
      * @brief Start the Controller
      *
      * @return The result of the startup. OC_STACK_OK on success
      */
    OCStackResult Controller::start()
    {
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

    /**
      * @brief Prints the data of an resource object
      *
      * @param resurce  Pointer holding the resource data
      *
      * @return OC_NO_RESOURCE if the resource doesn't exist.
      */
    OCStackResult Controller::printResourceData(OCResource::Ptr resource)
    {
        if(!resource)
            return OC_STACK_NO_RESOURCE;

        std::cout << "===================================================" << std::endl;
        std::cout << "\t Uri of the resources: " << resource->uri() << std::endl;
        std::cout << "\t Host address of the resources: " << resource->host() << std::endl;
        std::cout << "\t Types are: " << std::endl;

        for (auto type : resource->getResourceTypes())
        {
            std::cout << "\t\t type " << type << std::endl;
        }

        std::cout << "\t Interfaces are: " << std::endl;
        for (auto interface : resource->getResourceInterfaces())
        {
            std::cout << "\t\t interface " << interface << std::endl;
        }

        // DEBUG
        // Get the attibutes.
        /*if(Controller::isResourceLegit(resource))
        {
            resource->getRemoteAttributes(std::bind(&Controller::getAttributesCallback, this, std::placeholders::_1,
                                                    std::placeholders::_2));
        }*/
        return OC_STACK_OK;
    }


     /**
       * @brief Function callback for found resources
       *
       * @param resource     The discovered resource.
       */
     void Controller::foundResourceCallback(OCResource::Ptr resource)
     {
        std::lock_guard<std::mutex> lock(m_resourceMutex);

        if(this->isResourceLegit(resource))
        {
            this->printResourceData(resource);

            if(m_resourceList.insert({resource->uri() + resource->host(), resource}).second)
            {
                std::cout << "Added device: " << resource->uri() + resource->host() << std::endl;
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
   /* void Controller::getAttributesCallback(const RCSResourceAttributes& attr, int eCode)
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
    }*/

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
      * @brief Looks up the list of known resources type
      *
      * @param resource     Pointer to the resource object
      *
      * @return True if the type is found, false otherwise.
      */
    bool Controller::isResourceLegit(OCResource::Ptr resource)
    {
        // Filter platform and device resources
        std::string uri = resource->uri();
        std::vector<std::string> types = resource->getResourceTypes();

        if (uri == "/oic/p" || uri == "/oic/d")
            return false;
        else if (uri.compare(
                uri.size()-HOSTING_TAG_SIZE, HOSTING_TAG_SIZE, HOSTING_TAG) == 0)
        {
            std::cout << "Device: " << resource->uri() << " is not a legit device. Device is hosting" << std::endl;
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

} }
