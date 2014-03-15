/*
Copyright (c) 2013, Intel Corporation

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.

David Navarro <david.navarro@intel.com>
Domenico D'Ambrosio <dome.dambrosio@gmail.com>

*/


/*
 * This object and provides data related to a LWM2M Server.
 *
 *
 *                 Multiple
 *  Object        |  ID  | Instances | Mandatory |
 *  LWM2M Server  |  1   |    Yes    | Mandatory    |
 *
 *  Resources:
 *                       Supported    Multiple
 *      Name     | ID  | Operations | Instances | Mandatory |  Type   | Range | Units |
 *  Short Server |  0  |     R      |    No     |    Yes    | Integer | 0-255 |       |
 *     ID        |     |            |           |           |         |       |       |
 *  Lifetime     |  1  |    R/W     |    No     |    Yes    | Integer |       |   s   |
 *  Def. Min     |     |            |           |           |         |       |       |
 *   Period      |  2  |    R/W     |    No     |    No     | Integer |       |   s   |
 *  Def. Max     |     |            |           |           |         |       |       |
 *   Period      |  3  |    R/W     |    No     |    No     | Integer |       |   s   |
 *  Disable      |  4  |     E      |    No     |           |         |       |       |
 *  Disable      |  5  |    R/W     |    No     |    No     | Integer |       |   s   |
 *   Timout      |     |            |           |           |         |       |       |
 *  Notification |  6  |    R/W     |    No     |    Yes    | Boolean |       |       |
 *   Storing     |     |            |           |           |         |       |       |
 *  Binding      |  7  |    R/W     |    No     |    Yes    | String  |  (?)  |       |
 *  Reg Update   |  8  |     E      |    No     |    Yes    |         |       |       |
 *  Trigger      |     |            |           |           |         |       |       |
 *
 *
 *	(?) = see Specification
 */

#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PRV_TLV_BUFFER_SIZE 128

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_
{
	//TODO: check if join this object with the respective _lwm2m_server_ instance
	//(like done with lwm2m_list_t::next & id)

    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;               // matches lwm2m_list_t::id

    lwm2m_security_t  security;     // Matches lwm2m_server_t::security \ private
    void *            sessionH;     // Matches lwm2m_server_t::sessionH \ private
    lwm2m_status_t    status;       // Matches lwm2m_server_t::status   \ private
    char *            location;     // Matches lwm2m_server_t::location \ private
    uint16_t          mid;          // Matches lwm2m_server_t::mid      \ private
    //TODO: Implement the sending of Lifetime, Binding Mode, SMS Number (bitmask?)
    bool updateAvailable; //used for server Update
    //Public Object Info
    uint32_t lifetime;
    uint32_t defMinPeriod;
    uint32_t defMaxPeriod;
    uint32_t disableTimeout;
    bool notifyStoring;
    lwm2m_binding_t binding;

} prv_instance_t;


static void prv_output_buffer(uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        fprintf(stderr, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(stderr, "%02X ", array[j]);
        }
        while (j < 16)
        {
            fprintf(stderr, "   ");
            j++;
        }
        fprintf(stderr, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(stderr, "%c ", array[j]);
            else
                fprintf(stderr, ". ");
        }
        fprintf(stderr, "\n");

        i += 16;
    }
}

static int prv_get_object_tlv(char ** bufferP, prv_instance_t* obj_server_inst)
{
    int length = 0;
    int result;
    char temp_buffer[16];
    int temp_length;

    *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);

    if (NULL == *bufferP) return 0;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                                obj_server_inst->shortID, 0,
                                *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;
    result = lwm2m_intToTLV(TLV_RESSOURCE,
    						    (int64_t) obj_server_inst->lifetime, 1,
                                *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
        if (0 == result) goto error;
        length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
    		                    (int64_t) obj_server_inst->defMinPeriod,2,
                                *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                               obj_server_inst->defMaxPeriod, 3,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                               obj_server_inst->disableTimeout, 5,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_boolToTLV(TLV_RESSOURCE,
                               obj_server_inst->notifyStoring, 6,
                               *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
    if (0 == result) goto error;
    length += result;

    result = lwm2m_intToTLV(TLV_RESSOURCE,
                                   obj_server_inst->binding, 7,
                                   *bufferP + length, PRV_TLV_BUFFER_SIZE - length);
	if (0 == result) goto error;
	length += result;

    fprintf(stderr, "TLV (%d bytes):\r\n", length);
    prv_output_buffer(*bufferP, length);
    return length;

error:
    fprintf(stderr, "TLV generation failed:\r\n");
    free(*bufferP);
    *bufferP = NULL;
    return 0;
}

static uint8_t prv_read(lwm2m_uri_t * uriP,
                        char ** bufferP,
                        int * lengthP,
                        lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;

    *bufferP = NULL;
    *lengthP = 0;
    int len =0;

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
    	//reading of all object Instances
        *bufferP = (uint8_t *)malloc(PRV_TLV_BUFFER_SIZE);
        if (NULL == *bufferP) return COAP_500_INTERNAL_SERVER_ERROR;

        // TLV
        for (targetP = (prv_instance_t *)(objectP->instanceList); targetP != NULL ; targetP = targetP->next)
        {
            char* temp_buffer[PRV_TLV_BUFFER_SIZE];
            int temp_length = 0;
            int result;

            result = prv_get_object_tlv(temp_buffer, targetP);

            prv_output_buffer(*temp_buffer, targetP);

            if (0 == result)
            {
                *lengthP = 0;
                break;
            }
            temp_length += result;

            result = lwm2m_opaqueToTLV(TLV_OBJECT_INSTANCE, *temp_buffer, temp_length, targetP->shortID, *bufferP + *lengthP, PRV_TLV_BUFFER_SIZE - *lengthP);
            if (0 == result)
            {
                *lengthP = 0;
                break;
            }
            *lengthP += result;
        }
        if (*lengthP == 0)
        {
            *bufferP = NULL;
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        fprintf(stderr,"\n read print\n");
        fprintf(stderr, "TLV (%d bytes):\r\n", *lengthP);
        prv_output_buffer(*bufferP, *lengthP);
        return COAP_205_CONTENT;
    }
    else
    {
        targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->instanceId);
        if (NULL == targetP) return COAP_404_NOT_FOUND;

        if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
        {

        	// TLV
            *lengthP = prv_get_object_tlv(bufferP, targetP);
            if (0 != *lengthP)
                return COAP_205_CONTENT;
			else
                return COAP_500_INTERNAL_SERVER_ERROR;

        }
        //Single resource
        switch (uriP->resourceId)
        {
        case 0:
            // we use int16 because value is unsigned
            *lengthP = lwm2m_int16ToPlainText(targetP->shortID, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;
        case 1:
            *lengthP = lwm2m_int32ToPlainText(targetP->lifetime, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;
        case 2:
            *lengthP = lwm2m_int32ToPlainText(targetP->defMinPeriod, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;
        case 3:
            *lengthP = lwm2m_int32ToPlainText(targetP->defMaxPeriod, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;
        case 4:
            return COAP_405_METHOD_NOT_ALLOWED;
        case 5:
            *lengthP = lwm2m_int32ToPlainText(targetP->disableTimeout, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;
        case 6:
            *lengthP = lwm2m_int32ToPlainText(targetP->notifyStoring, bufferP);
            if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
            return COAP_205_CONTENT;
        case 7:
            *lengthP = lwm2m_int32ToPlainText(targetP->binding, bufferP);
			if (*lengthP <= 0) return COAP_500_INTERNAL_SERVER_ERROR;
			return COAP_205_CONTENT;
        case 8:
            return COAP_405_METHOD_NOT_ALLOWED;
        default:
            return COAP_404_NOT_FOUND;
        }
    }
}

static uint8_t prv_write(lwm2m_uri_t * uriP,
                         char * buffer,
                         int length,
                         lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int64_t value;

    // for write, instance ID is always set
    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, uriP->instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) return COAP_501_NOT_IMPLEMENTED;

    switch (uriP->resourceId)
    {
    case 1:
        if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
        {
            if (value >= 0 && value <= 0xFF)
            {
                targetP->lifetime = value;
                return COAP_204_CHANGED;
            }
        }
    break;
    case 2:
            if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
            {
                if (value >= 0 && value <= 0xFF)
                {
                    targetP->defMinPeriod = value;
                    return COAP_204_CHANGED;
                }
            }
        break;
    case 3:
            if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
            {
                if (value >= 0 && value <= 0xFF)
                {
                    targetP->defMaxPeriod = value;
                    return COAP_204_CHANGED;
                }
            }
        break;
    case 5:
            if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
            {
                if (value >= 0 && value <= 0xFF)
                {
                    targetP->disableTimeout = value;
                    return COAP_204_CHANGED;
                }
            }
        break;
    case 6:
            if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
            {
                if (value >= 0 && value <= 1)
                {
                    targetP->notifyStoring = value;
                    return  COAP_400_BAD_REQUEST;;
                }
            }
        break;
    case 7:
            if (1 == lwm2m_PlainTextToInt64(buffer, length, &value))
            {
                if (value <BINDING_U || value >BINDING_US )
                {
                	return COAP_400_BAD_REQUEST;
                }
                else if ((lwm2m_status_t)value == BINDING_U)
                {
                    targetP->binding = value;
                    return COAP_204_CHANGED;
                }else{
                	  return COAP_501_NOT_IMPLEMENTED;
                }
            }
        break;
    case 8:
    	return COAP_405_METHOD_NOT_ALLOWED;
        break;
    case 9:
    	return COAP_405_METHOD_NOT_ALLOWED;
        break;

    default:
        return COAP_404_NOT_FOUND;
    }
    return COAP_400_BAD_REQUEST;
}


static uint8_t prv_exec(lwm2m_uri_t * uriP,
                        char * buffer,
                        int length,
                        lwm2m_object_t * objectP)
{
    int64_t value;
    prv_instance_t* list_Elem= (prv_instance_t*)lwm2m_list_find(objectP->instanceList, uriP->instanceId); //Get Obj Instance

    if (NULL == list_Elem) return COAP_404_NOT_FOUND;

    switch (uriP->resourceId)
    {
    case 0:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 1:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 2:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 3:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 4:
    	//TODO:Implement disable method
        printf(stdout, "\r\n------- Disable Server Not Implemented----------\r\n");

        return COAP_501_NOT_IMPLEMENTED;
    case 5:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 6:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 7:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 8:
        fprintf(stdout, "\r\n-----------------\r\n"
                        "Execute Update Trigger on %hu/%d/%d\r\n"
                        " Parameter (%d bytes):\r\n",
                        uriP->objectId, uriP->instanceId, uriP->resourceId, length);
        fprintf(stdout, "-----------------\r\n\r\n");

        // get context
        lwm2m_context_t* lwm2mH = (lwm2m_context_t*)objectP->userData;
        if(NULL == lwm2mH) fprintf(stderr,"CONTEXT NULL\n");

        //Check if Update is available
        if(list_Elem->updateAvailable == 1)
            registration_update(lwm2mH, (lwm2m_server_t*)list_Elem);

       return COAP_204_CHANGED;
    default:
        return COAP_404_NOT_FOUND;
    }
}

lwm2m_object_t * get_object_server()
{
    lwm2m_object_t * serverObj;

    serverObj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));

    if (NULL != serverObj)
    {
        int i;
        prv_instance_t * targetP;

        memset(serverObj, 0, sizeof(lwm2m_object_t));

        serverObj->objID = 2;

		targetP = (prv_instance_t *)malloc(sizeof(prv_instance_t));
		if (NULL == targetP) return NULL;
		memset(targetP, 0, sizeof(prv_instance_t));
		targetP->shortID = 0;
		targetP->updateAvailable= 0;
		targetP->lifetime = 86400; //Random Number - TODO Define a value
		targetP->defMinPeriod= 1;  //Default value
		targetP->defMaxPeriod= 86400; //Random Number - TODO Define a value
		targetP->disableTimeout = 86400; // Default Value
		targetP->notifyStoring = 1; //Default value
		targetP->binding= BINDING_U;


		serverObj->instanceList = LWM2M_LIST_ADD(serverObj->instanceList, targetP);

        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
		serverObj->readFunc = prv_read;
        serverObj->writeFunc = prv_write;
        //serverObj->createFunc = prv_create;  //TODO: implementing for bootstrap?
        //serverObj->deleteFunc = prv_delete;  //TODO: implementing for bootstrap?
		serverObj->executeFunc = prv_exec;
    }

    return serverObj;
}



