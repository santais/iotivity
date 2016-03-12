/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** <OCBaseResource>.h
** <Base resource structure holding all the information required to create
** and manage a resource>
**
** Author: <Mark Povlsen>
** Date: 06/01/16
** -------------------------------------------------------------------------*/

#ifndef _OCBASERESOURCE_H
#define _OCBASERESOURCE_H


// include directives

// Do not remove the include below

#ifdef ARDUINO
  #define TAG "ArduinoServer"
  #include "Arduino.h"
  #ifdef ARDUINOWIFI
  // Arduino WiFi Shield
  #include <SPI.h>
  #include <WiFi.h>
  #include <WiFiUdp.h>
  #else
  // Arduino Ethernet Shield
  #include <Ethernet2.h>
  #include <EthernetServer.h>
  #include <EthernetClient.h>
  #include <Dhcp.h>
  #include <Dns.h>
  #include <EthernetUdp2.h>
  #include <Twitter.h>
  #include <util.h>
  #endif
#else
  #define TAG "ocbaseresource";
#endif

#include "logger.h"
#include "ocstack.h"
#include "ocpayload.h"
#include "ocresource.h"
//#include <vector>


//typedef std::vector<OCObservationId> ObservationIds;

/**
 * Handle to an OC Resource
 */
typedef void * OCResourceTypeT;

typedef void * Value;

static OCEntityHandlerResult OCEntityHandlerCbNew(OCEntityHandlerFlag flag, OCEntityHandlerRequest * entityHandlerRequest,
                            void *callbackParam);

/**
  * Data type structure to assign a type
  * to an attribute
  */
typedef enum DataType
{
    INT     = 0,
    DOUBLE  = (1 << 0),
    BOOL    = (2 << 0),
    STRING  = (3 << 0)
} DataType;

/**
  * 8 bit variable declaring a port type
  */
typedef enum IOPortType
{
    IN      = 0,
    OUT     = (1 << 0),
    INOUT   = (2 << 0)
} IOPortType;

/**
  * Structure defining which port to assign a value.
  */
typedef struct OCIOPort
{
    IOPortType type;

    uint8_t pin;
} OCIOPort;

typedef union ResourceData
{
    double d;
    int i;
    char* str;
    bool b;
} ResourceData;

/**
  * Structure to hold the attributes for a resource. Multiple attributes
  * are allowed, linked together using a linked list
  */
typedef struct OCAttributeT
{
    OCAttributeT* next;

    char* name;

    struct ValueType
    {
        DataType dataType;

        ResourceData data;

    } value;

    OCIOPort* port;
} OCAttributeT;


/**
  * The application calls this callback, when a PUT request has been initiated. The user
  * has to manually set what how the vlaues are sent to the ports (PWM, PPM etc.):
  */
typedef void (*OCIOHandler)
(OCAttributeT *attributes, int IOType, OCResourceHandle handle, bool *underObservation);


/**
  * BaseResource containing the necessary parameters for a resource.
  * Somehow similar to OCResource internal struct with minor changes.
  */
typedef struct OCBaseResourceT
{
    /** Points to the next resource. Used to forma  list of created resources */
    OCBaseResourceT *next;

    /** Handle to handle the resource current data to the connectivity layer */
    OCResourceHandle handle;

    /** Relative path on the device. */
    char *uri;

    /** Human friendly name */
    char* name;

    /** Resource type(s). Linked list */
    OCResourceType *type;

    /** Resource interface(s). Linked list */
    OCResourceInterface *interface;

    /** Resource attributes. Linked list */
    OCAttributeT *attribute;

    /** Resource Properties */
    uint8_t resourceProperties;

    /** Bool indicating if it is being observed */
    bool underObservation;

    /** Callback called when a PUT method has been called */
    OCIOHandler OCIOhandler;

} OCBaseResourceT;

/*********************** REVISED VERSION 1.1 **************************/

/**
  * @brief Initializes and creates a resource
  *
  * @param uri          Path and name of the resource
  * @param interface    Resource interfaces
  * @param type         Resource types
  * @param properties   Byte of allowed properties of the resources
  * @param outputHandler Callback called when a PUT request is received
  *
  * @return Pointer to the created resource
  */
OCBaseResourceT * createResource(char* uri, OCResourceType* type, OCResourceInterface* interface,
                         uint8_t properties, OCIOHandler outputHandler);


/**
  * @brief createResource
  *
  * @param uri           Path and name of the resource
  * @param type          Resource interface
  * @param interface     Resource type
  * @param properties    Allowed properties of the resource
  * @param outputHandler Callback called when a PUT request is received
  *
  * @return
  */
OCBaseResourceT * createResource(char* uri, const char* type, const char* interface, uint8_t properties,
                                 OCIOHandler outputHandler);

/**
 * @brief Initializes and creates a resource
 *
 * @param resource      Resource to be initialized
 */
void createResource(OCBaseResourceT *resource);


/**
 * @brief addType Adds and bind a type to a resource
 *
 * @param handle        The handle of the resource
 * @param type          Type to be bound and added
 *
 * @return              OC_STACK_OK if successfully bound
 */
OCStackResult addType(OCBaseResourceT *resource, OCResourceType *type);
OCStackResult addType(OCBaseResourceT *resource, const char *typeName);

/**
 * @brief addInterface Adds and bind a interface to a resource
 *
 * @param handle        The handle of the resource
 * @param interface     Type to be bound and added
 *
 * @return              OC_STACK_OK if successfully bound
 */
OCStackResult addInterface(OCBaseResourceT *resource, OCResourceInterface *interface);
OCStackResult addInterface(OCBaseResourceT *resource, char* interfaceName);

/**
 * @brief addAttribute  Adds an attribute to the resource
 *
 * @param resource      The resource to add an attribute to
 * @param attribute     The attribute to be added
 */
void addAttribute(OCAttributeT **head, OCAttributeT *attribute, OCIOPort *port);
void addAttribute(OCAttributeT **head, char *name, ResourceData value, DataType type,
                  OCIOPort *port);

/**
 * @brief getResourceList Returns the list of registered devices
 *
 * @return The list of registered devices
 */
OCBaseResourceT * getResourceList();


/**
 * @brief printResourceData
 *
 * @param resource
 */
void printResourceData(OCBaseResourceT *resource);

/**
 * @brief printAttributes Print a list of the attributes
 *
 * @param attributes    Linked list of attributes
 */
void printAttributes(OCAttributeT *attributes);

/**
 * @brief Create and loads the payload
 *
 * @return the created payload
 */
OCRepPayload *getPayload(OCEntityHandlerRequest *ehRequest, OCBaseResourceT *resource);

 /**
  * @brief handles the response to the entity handler
  *
  * @param response   The entityhandler response
  * @param EntityHandlerRequest
  * @param resource   The base resource attributes
  *
  * @parma result of the entityhandler;
  */
OCEntityHandlerResult responseHandler(OCEntityHandlerResponse *response, OCEntityHandlerRequest *entityHandlerRequest, OCRepPayload *payload, OCEntityHandlerResult ehResult);

 /**
  * @brief Handles what request was instantiated and the corrensponding action
  *
  * @param handler  The EntityHandler
  * @param resource   Base resource
  *
  * @return the result of the request
  */
OCEntityHandlerResult requestHandler(OCEntityHandlerRequest *ehRequest,
                                      OCBaseResourceT *resource, OCRepPayload **payload);

/**
 * @brief observerHandler
 *
 * @param ehRequest     Request information from the client
 * @param resource      Pointer to the request resource
 */
OCEntityHandlerResult observerHandler(OCEntityHandlerRequest *ehRequest, OCBaseResourceT *resource);

/**
 * @brief Called when a REST GET is request
 *
 * @param OCBaseResource base resource attributes
 *
 * @return result of the entityHandler
 */
 OCEntityHandlerResult getRequest(OCBaseResourceT *resource, OCRepPayload *payload);

 /**
 * @brief Called when a REST PUT is request
 *
 * @param OCBaseResource base resource attributes
 *
 * @return result of the entityHandler
 */
 OCEntityHandlerResult putRequest(OCEntityHandlerRequest *ehRequest, OCRepPayload* payload, OCBaseResourceT *resource);


 /**
  * @brief postRequest Called when a RESTful POST request is called
  *
  * @param ehRequest    Request parameters
  * @param payload      Payload from the client
  * @param resource     Resource the call was refered to
  * @return
  */
 OCEntityHandlerResult postRequest(OCEntityHandlerRequest *ehRequest, OCRepPayload* payload, OCBaseResourceT *resource);


 /**
  * @brief Returns the result of a OCStackResult as a string
  *
  * @param OCStackResult The result to be converted to a string
  *
  * @return A string with the result
 */
 const char * getOCStackResult(OCStackResult result);

 /**
  * @brief Returns a string corrensponding to the request
  *
  * @param The entity handler request
  *
  * @return the string of the request
  */
 const char * getEntityHandlerRequestResult(OCEntityHandlerRequest *entityHandler);

#endif /* _OCBASERESOURCE_H */

