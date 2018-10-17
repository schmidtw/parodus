/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
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
#include "thread_tasks.h"
#include "downstream.h"
#include "ParodusInternal.h"
#include "client_list.h"
#include "peer2peer.h"
#include "upstream.h"
#include "time.h"
#include "service_alive.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void messageHandlerTask()
{
    pthread_mutex_lock (&g_mutex);
    ParodusPrint("mutex lock in consumer thread\n");
    if(ParodusMsgQ != NULL)
    {
        ParodusMsg *message = ParodusMsgQ;
        ParodusMsgQ = ParodusMsgQ->next;
        pthread_mutex_unlock (&g_mutex);
        ParodusPrint("mutex unlock in consumer thread\n");

        listenerOnMessage(message->payload, message->len);

        nopoll_msg_unref(message->msg);
        free(message);
        message = NULL;
    }
    else
    {
        ParodusPrint("Before pthread cond wait in consumer thread\n");
        pthread_mutex_unlock (&g_mutex);
        ParodusPrint("mutex unlock in consumer thread after cond wait\n");
    }
    ParodusPrint ("Ended messageHandlerTask\n");
}

void *handle_and_process_message(void *args)
{
    ParodusInfo("****** %s *******\n",__FUNCTION__);
    uint64_t startTime = 0, endTime = 0;
    struct timespec start, end;
    uint32_t diffTime = 0;

    while( FOREVER() )
    {
        startTime = getCurrentTimeInMicroSeconds(&start);
        serviceAliveTask();
        messageHandlerTask();
        CRUDHandlerTask();
        handle_upstream(args);
        processUpstreamMessage();
        process_P2P_OutgoingMessage(args);
        handle_P2P_Incoming(args);
        process_P2P_IncomingMessage(args);
        endTime = getCurrentTimeInMicroSeconds(&end);
        diffTime = endTime - startTime;
        ParodusPrint("Elapsed time : %lu \n", diffTime);
        if(diffTime < 50)
        {
            ParodusPrint("Sleeping for 0.5 s\n");
            usleep(1);
        }
    }
    return NULL;
}
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
