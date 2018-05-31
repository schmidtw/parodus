/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../src/ParodusInternal.h"
#include "../src/thread_tasks.h"
#include "../src/client_list.h"
#include "../src/peer2peer.h"


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ParodusMsg *ParodusMsgQ;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
int numLoops;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

int get_numOfClients()
{
    function_called();
    return (int)mock();
}

reg_list_item_t * get_global_node(void)
{
    function_called();
    return mock_ptr_type(reg_list_item_t *);
}

void listenerOnMessage(void * msg, size_t msgSize )
{
    check_expected((intptr_t)msg);
    check_expected(msgSize);
    function_called();
}

void handle_upstream(void *args)
{
    function_called();
    UNUSED(args);
}

void processUpstreamMessage()
{
    function_called();
}

void CRUDHandlerTask()
{
    function_called();
}

void handle_P2P_Incoming(void *args)
{
    function_called();
    UNUSED(args);
}

void process_P2P_IncomingMessage(void *args)
{
    UNUSED(args);
    function_called();
}

void process_P2P_OutgoingMessage(void *args)
{
    UNUSED(args);
    function_called();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_messageHandlerTask()
{
    ParodusMsgQ = (ParodusMsg *) malloc (sizeof(ParodusMsg));
    ParodusMsgQ->payload = "First message";
    ParodusMsgQ->len = 9;
    ParodusMsgQ->next = NULL;
    
    numLoops = 1;

    expect_value(listenerOnMessage, (intptr_t)msg, (intptr_t)ParodusMsgQ->payload);
    expect_value(listenerOnMessage, msgSize, ParodusMsgQ->len);
    expect_function_call(listenerOnMessage);
    
    messageHandlerTask();
}

void err_messageHandlerTask()
{
    numLoops = 1;
    
    messageHandlerTask();
}

void test_handle_and_process_message()
{
    socket_handles_t sock;
    sock.pipeline = 1;
    sock.pubsub = 0;
    sock.local = 2;
    numLoops = 1;

    expect_function_call(CRUDHandlerTask);
    expect_function_call(handle_upstream);
    expect_function_call(processUpstreamMessage);
    expect_function_call(process_P2P_OutgoingMessage);
    expect_function_call(handle_P2P_Incoming);
    expect_function_call(process_P2P_IncomingMessage);
    handle_and_process_message((void *)&sock);
}

void test_handle_and_process_message_null()
{
    numLoops = 0;
    handle_and_process_message(NULL);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_messageHandlerTask),
        cmocka_unit_test(err_messageHandlerTask),
        cmocka_unit_test(test_handle_and_process_message),
        cmocka_unit_test(test_handle_and_process_message_null),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
