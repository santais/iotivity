#ifndef _RESOURCE_OBJECT_HPP
#define _RESOURCE_OBJECT_HPP

#include "RCSRemoteResourceObject.h"
#include "RCSResourceAttributes.h"
#include "resource_types.h"
#include <mutex>

using namespace OIC;
using namespace OC;
using namespace OIC::Service;

enum class ResourceDeviceType
{
    OIC_BUTTON,
    OIC_LIGHT,
    OIC_FAN,
    OIC_SPEAKER,
    OIC_TV,
    OIC_SENSOR,
    UNKNOWN_DEVICE
};


enum class ResourceObjectState
{
    CACHE_CHANGED,
    PRESENCE_CHANGD
};


class ResourceObject
{
public:
    typedef std::shared_ptr<ResourceObject> Ptr;
    typedef std::shared_ptr<const ResourceObject> ConstPtr;

    /**
     * Callback to be invoked when a resource changes it state.
     */
    typedef std::function< void(const RCSResourceAttributes &attrs, const ResourceObjectState &state, const ResourceDeviceType &type) > ResourceObjectCallback;

public:

    ResourceObject(RCSRemoteResourceObject::Ptr remoteResource);

    /**
     * @brief ~ResourceObject     Destructor. Currently not used.
     */
    ~ResourceObject() {}

    /**
     * @brief getAttributes
     * @return The latest cached resources.
     */
    RCSResourceAttributes getAttributes();

    /**
     * @brief setAttributes Set the resource attributes.
     * @param attrs
     */
    void setAttributes(RCSResourceAttributes attrs);

    /**
     * @brief getRemoteResourceObject
     * @return The remote resource
     */
    RCSRemoteResourceObject::Ptr getRemoteResourceObject();

    /**
     * @brief getResourceDeviceType
     * @return The Resource Device Type
     */
    ResourceDeviceType getResourceDeviceType();

    /**
     * @brief convertResourceDeviceTypeToString
     * @return a string representing the resource device type
     */
    static std::string convertResourceDeviceTypeToString(const ResourceDeviceType &deviceType);

    /**
     * @brief startCaching Starts caching the device
     */
    void startCaching();

    /**
     * @brief stopCaching Stop caching the deivce
     */
    void stopCaching();

private:

    /**
     * @brief m_resourceObject Pointer to the hold object
     */
    RCSRemoteResourceObject::Ptr m_resourceObject;

    /**
     * @brief attrs Attributes of the resource
     */
    RCSResourceAttributes m_attrs;

    /**
     * @brief m_attrsMutex Mutext o ensure m_attrs is not accessed simultaneuously.
     */
    std::mutex m_attrsMutex;

    /**
     * @brief m_resourceDeviceType The type of the remote device.
     */
    ResourceDeviceType m_resourceDeviceType;

    /**
     * @brief m_reosurceObjectCallback Callback to invoke the controller when a changed in the resource occurs
     */
    ResourceObjectCallback m_resourceObjectCallback;

private:

    /**
     * @brief ResourceObject      Default constructor. Curerntly not used.
     */
    ResourceObject() {}

    /**
     * @brief cacheUpdateCallback Cache callback called when the remote attributes
     *                            have changed.
     *
     * @param attrs               The new set of attributes
     */
    void cacheUpdateCallback(const RCSResourceAttributes attrs);

    /**
     * @brief remoteAttributesGetcallback Attributes of the called resource
     *
     * @param attrs               The resource current attributes state
     */
    void remoteAttributesGetCallback(const RCSResourceAttributes attrs);

    /**
     * @brief remoteAttributesSetCallback Set the attributes of the resource
     *
     * @param attrs               The attributes to set at the endpoint device.
     */
    void remoteAttributesSetCallback(const RCSResourceAttributes attrs);


    /**
     * @brief printAttributes     Prints the attributes.
     */
    void printAttributes(RCSResourceAttributes attrs);

    /**
     * @brief setResourceDeviceType Find the resource device type and sets it.
     * @param types
     */
    void setResourceDeviceType(const std::vector<std::string> &types, ResourceDeviceType &type);


};

#endif /* RESOURCE_OBJECT_HPP */
