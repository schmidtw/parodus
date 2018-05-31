/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @file crud_interface.c
 *
 * @description This file is used to manage incoming and outgoing CRUD messages.
 *
 */

#include "ParodusInternal.h"
#include "crud_tasks.h"
#include "crud_interface.h"
#include "upstream.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Internal variables                             */
/*----------------------------------------------------------------------------*/

static CrudMsg *crudMsgQ = NULL;

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	CrudMsg * crudMessage;
	crudMessage = (CrudMsg *)malloc(sizeof(CrudMsg));
	ParodusPrint("----------- %s ------------\n",__FUNCTION__);
	if(crudMessage)
	{
		crudMessage->msg = crudMsg;
		crudMessage->next = NULL;
		if(crudMsgQ ==NULL)
		{
			crudMsgQ = crudMessage;
		}
		else
		{
			CrudMsg *temp = crudMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = crudMessage;
		}
	}
	else
	{
		ParodusError("Memory allocation failed for CRUD\n");
	}
}


void CRUDHandlerTask()
{
    int ret = 0;
    ssize_t resp_size = 0;
    void *resp_bytes;
    wrp_msg_t *crud_response = NULL;
    ParodusPrint("----------- %s ------------\n",__FUNCTION__);

    if(crudMsgQ !=NULL)
    {
        CrudMsg *message = crudMsgQ;
        crudMsgQ = crudMsgQ->next;

        ret = processCrudRequest(message->msg, &crud_response);

        if(ret == 0)
        {
            ParodusInfo("CRUD processed successfully\n");
            ParodusPrint("msgpack encode to send to upstream\n");
            resp_size = wrp_struct_to( crud_response, WRP_BYTES, &resp_bytes );
            ParodusPrint("Encoded CRUD resp_size :%lu\n", resp_size);
            ParodusInfo("Adding CRUD response to upstreamQ\n");
            addCRUDresponseToUpstreamQ(resp_bytes, resp_size);
            wrp_free_struct(crud_response);
        }
        else
        {
            ParodusError("Failure in CRUD request processing !!\n");
        }
    }
}


//CRUD Producer adds the response into common UpStreamQ
void addCRUDresponseToUpstreamQ(void *response_bytes, ssize_t response_size)
{
	UpStreamMsg *response;
	
	response = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
	if(response)
	{
	    response->msg =(void *)response_bytes;
	    response->len =(int)response_size;
	    response->next=NULL;
	    
	    if(get_global_UpStreamMsgQ() == NULL)
	    {
		set_global_UpStreamMsgQ(response);
	    }
	    else
	    {
		ParodusPrint("Producer adding CRUD response to UpStreamQ\n");
		UpStreamMsg *temp = get_global_UpStreamMsgQ();
		while(temp->next)
		{
		    temp = temp->next;
		}
		temp->next = response;
	    }
	}
	else
	{
		ParodusError("failure in allocation for CRUD response\n");
	}
	
			   
}

