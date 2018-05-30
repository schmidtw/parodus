/**
 * @file peer2peer.c
 *
 * @description This describes functions required
 * to manage parodus peer to peer messages.
 *
 * Copyright (c) 2018  Comcast
 */

#include "ParodusInternal.h"
#include "config.h"
#include "upstream.h"
#include "parodus_interface.h"
#include "peer2peer.h"

P2P_Msg *inMsgQ = NULL;
pthread_mutex_t inMsgQ_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t inMsgQ_con=PTHREAD_COND_INITIALIZER;

P2P_Msg *outMsgQ = NULL;
pthread_mutex_t outMsgQ_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outMsgQ_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

void handle_P2P_Incoming(void *args)
{
    void *ptr;
    int l;
    P2P_Msg *inMsg = NULL;
    socket_handles_t *p_sock;
    ParodusPrint("****** %s *******\n",__FUNCTION__);
    int msgAdded=0;
    p_sock = (socket_handles_t *) args;
    if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) ) 
    {
        l = check_inbox(p_sock->pipeline, &ptr);
    } 
    else 
    {
        l = check_inbox(p_sock->pubsub, &ptr);
    }
    if (l > 0 && ptr != NULL)
    {
        inMsg = (P2P_Msg *)malloc(sizeof(P2P_Msg));
        inMsg->msg = malloc(l);
        memcpy(inMsg->msg,ptr,l);
        inMsg->len = l;
        inMsg->next = NULL;
        free_msg(ptr);
        msgAdded = 1;
    }

    if(msgAdded)
    {
        if(inMsgQ == NULL)
        {
            inMsgQ = inMsg;
        }
        else
        {
            P2P_Msg *temp = inMsgQ;
            while(temp->next)
            {
                temp = temp->next;
            }
            temp->next = inMsg;
        }
        msgAdded = 0;
    }
}

void process_P2P_IncomingMessage(void *args)
{
    int rv=-1; (void) rv;
    bool status;
    socket_handles_t *p_sock;
    ParodusPrint("****** %s *******\n",__FUNCTION__);

    p_sock = (socket_handles_t *) args;
    if(inMsgQ != NULL)
    {
        P2P_Msg *message = inMsgQ;
        inMsgQ = inMsgQ->next;
        // For incoming of type HUB, use hub_send_msg() to propagate message to hardcoded spoke
        if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) )
        {
            status = send_msg(p_sock->pubsub, message->msg, message->len);
            if(status == true)
            {
                ParodusInfo("Successfully sent event to spoke\n");
            }
            else
            {
                ParodusError("Failed to send event to spoke\n");
            }
        }
        //Send event to all registered clients for both hub and spoke incoming msg 
        sendToAllRegisteredClients(&message->msg, message->len);
    }
}

/**
 * For outgoing of type HUB, use hub_send_msg() to propagate message to hardcoded spoke
 * For outgoing of type spoke, use spoke_send_msg()
**/
void process_P2P_OutgoingMessage(void *args)
{
    int rv=-1; (void) rv;
    bool status;
    socket_handles_t *p_sock;
    ParodusPrint("****** %s *******\n",__FUNCTION__);

    p_sock = (socket_handles_t *) args;
    if(outMsgQ != NULL)
    {
        P2P_Msg *message = outMsgQ;
        outMsgQ = outMsgQ->next;
        ParodusInfo("process_P2P_OutgoingMessage - message->msg = %p, message->len = %zd\n", message->msg, message->len);
        if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) )
        {
            ParodusInfo("Just before hub send message\n");
            status = send_msg(p_sock->pubsub, message->msg, message->len);
            if(status == true)
            {
                ParodusInfo("Successfully sent event to spoke\n");
            }
            else
            {
                ParodusError("Failed to send event to spoke\n");
            }
        }
        else
        {
            status = send_msg(p_sock->pipeline, message->msg, message->len);
            if(status == true)
            {
                ParodusInfo("Successfully sent event to hub\n");
            }
            else
            {
                ParodusError("Failed to send event to hub\n");
            }
        }
        free(message);
        message = NULL;
    }
}

void add_P2P_OutgoingMessage(void **message, size_t len)
{
    P2P_Msg *outMsg = NULL;
    void *bytes;
    ParodusInfo("****** %s *******\n",__FUNCTION__);

    outMsg = (P2P_Msg *)malloc(sizeof(P2P_Msg));

    if(outMsg)
    {
        ParodusInfo("add_P2P_OutgoingMessage - *message = %p, len = %zd\n", *message, len);
        bytes = malloc(len);
        memcpy(bytes,*message,len);
        outMsg->msg = bytes;
        outMsg->len = len;
        outMsg->next = NULL;
        if(outMsgQ == NULL)
        {
            outMsgQ = outMsg;
        }
        else
        {
            P2P_Msg *temp = outMsgQ;
            while(temp->next)
            {
                temp = temp->next;
            }
            temp->next = outMsg;
        }
    }
    else
    {
        ParodusError("Failed in memory allocation\n");
    }
}

void *handle_and_process_P2P_messages(void *args)
{
    ParodusInfo("****** %s *******\n",__FUNCTION__);
    while( FOREVER() )
    {
        handle_upstream(args);
        processUpstreamMessage();
        process_P2P_OutgoingMessage(args);
        handle_P2P_Incoming(args);
        process_P2P_IncomingMessage(args);
    }
    return NULL;
}
