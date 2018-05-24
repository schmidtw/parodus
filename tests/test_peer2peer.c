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

#include <CUnit/Basic.h>

#include "../src/config.h"
#include "../src/peer2peer.h"
#include "../src/ParodusInternal.h"

extern P2P_Msg *outMsgQ;
extern P2P_Msg *inMsgQ;
static ParodusCfg parodusCfg;
int numLoops;
char *notification;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
ParodusCfg *get_parodus_cfg(void) 
{
    return &parodusCfg;
}

ssize_t check_inbox(int sock, void **msg)
{
    UNUSED(sock); //UNUSED(msg);
    *msg = notification;
    function_called();
    return (ssize_t)mock();
}

void free_msg(void *msg)
{
    UNUSED(msg);
    function_called();
}

bool send_msg(int sock, const void *msg, size_t size)
{
    UNUSED(sock); UNUSED(msg); UNUSED(size);
    function_called();
    return (bool)mock();
}

void sendToAllRegisteredClients(void **resp_bytes, size_t resp_size)
{
    UNUSED(resp_bytes); UNUSED(resp_size);
    function_called();
}

void cleanup_sock(int *sock)
{
    UNUSED(sock);
    function_called();
}

bool spoke_setup_pipeline(const char *pipeline_url, int *pipeline_sock)
{
    UNUSED(pipeline_url); UNUSED(pipeline_sock);
    function_called();
    return (bool)mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_handle_P2P_Incoming_hub()
{
    socket_handles_t sock;
    sock.pipeline.sock = 1;
    sock.pubsub.sock = 0;
    numLoops = 2;
    notification = "Hello";
    strcpy(parodusCfg.hub_or_spk, "hub");
    expect_function_call(check_inbox);
    will_return(check_inbox, 6);
    expect_function_call(free_msg);
    expect_function_call(check_inbox);
    will_return(check_inbox, 6);
    expect_function_call(free_msg);
    handle_P2P_Incoming((void *)&sock);
    assert_non_null(inMsgQ);
    assert_int_equal(6, inMsgQ->len);
    assert_non_null(inMsgQ->next);
    assert_int_equal(6, inMsgQ->next->len);
    assert_null(inMsgQ->next->next);
    assert_string_equal("Hello", (char *)inMsgQ->msg);
    assert_string_equal("Hello", (char *)inMsgQ->next->msg);
}

void test_handle_P2P_Incoming_spoke()
{
    socket_handles_t sock;
    sock.pipeline.sock = 0;
    sock.pubsub.sock = 1;
    numLoops = 1;
    notification = "Welcome";
    strcpy(parodusCfg.hub_or_spk, "spk");
    expect_function_call(check_inbox);
    will_return(check_inbox, 8);
    expect_function_call(free_msg);
    handle_P2P_Incoming((void *)&sock);
    assert_non_null(inMsgQ->next->next);
    assert_int_equal(8, inMsgQ->next->next->len);
    assert_null(inMsgQ->next->next->next);
    assert_string_equal("Welcome", (char *)inMsgQ->next->next->msg);
}

void test_process_P2P_IncomingMessage_hub()
{
    socket_handles_t sock;
    sock.pipeline.sock = 1;
    sock.pubsub.sock = 0;
    numLoops = 2;
    strcpy(parodusCfg.hub_or_spk, "hub");
    expect_function_call(send_msg);
    will_return(send_msg, true);
    expect_function_call(sendToAllRegisteredClients);
    expect_function_call(send_msg);
    will_return(send_msg, false);
    expect_function_call(sendToAllRegisteredClients);
    process_P2P_IncomingMessage(&sock);
}

void test_process_P2P_IncomingMessage_spoke()
{
    socket_handles_t sock;
    sock.pipeline.sock = 0;
    sock.pubsub.sock = 1;
    numLoops = 1;
    strcpy(parodusCfg.hub_or_spk, "spk");
    expect_function_call(sendToAllRegisteredClients);
    process_P2P_IncomingMessage(&sock);
}

void test_add_P2P_OutgoingMessage()
{
    wrp_msg_t wrp_msg;
    wrp_msg_t *msg = NULL;
    void *bytes = NULL;
    int rv = -1;
    memset(&wrp_msg, 0, sizeof(wrp_msg_t));
    wrp_msg.msg_type = WRP_MSG_TYPE__EVENT;     
    wrp_msg.u.event.source = strdup("mac:14cfe214266");
    wrp_msg.u.event.dest   = strdup("event:node-change");
    wrp_msg.u.event.payload = "Hello world";
    wrp_msg.u.event.payload_size = 12;
    
    int size = wrp_struct_to( &wrp_msg, WRP_BYTES, &bytes );
    add_P2P_OutgoingMessage(&bytes, size);
    assert_non_null(outMsgQ);
    assert_int_equal(size, outMsgQ->len);
    assert_null(outMsgQ->next);
    
    rv = wrp_to_struct( outMsgQ->msg, outMsgQ->len, WRP_BYTES, &msg );
    assert_true(rv > 0);
    assert_non_null(msg);
    assert_int_equal(wrp_msg.msg_type, msg->msg_type);
    assert_string_equal(wrp_msg.u.event.source, msg->u.event.source);
    assert_string_equal(wrp_msg.u.event.dest, msg->u.event.dest);
    assert_string_equal((char *)wrp_msg.u.event.payload, (char *)msg->u.event.payload);
    assert_int_equal(wrp_msg.u.event.payload_size, msg->u.event.payload_size);
    wrp_free_struct(msg);
}

void test_process_P2P_OutgoingMessage_hub()
{
    socket_handles_t sock;
    sock.pipeline.sock = 1;
    sock.pubsub.sock = 0;
    numLoops = 2;
    void *bytes = NULL;
    wrp_msg_t wrp_msg;
    memset(&wrp_msg, 0, sizeof(wrp_msg_t));
    wrp_msg.msg_type = WRP_MSG_TYPE__EVENT;
    wrp_msg.u.event.source = strdup("mac:14cfe654385");
    wrp_msg.u.event.dest   = strdup("event:connected-client");
    wrp_msg.u.event.payload = "Hello world";
    wrp_msg.u.event.payload_size = 12;
    
    int size = wrp_struct_to( &wrp_msg, WRP_BYTES, &bytes );
    outMsgQ->next = (P2P_Msg *)malloc(sizeof(P2P_Msg));
    outMsgQ->next->msg = bytes;
    outMsgQ->next->len = size;
    outMsgQ->next->next = NULL;
    
    strcpy(parodusCfg.hub_or_spk, "hub");
    expect_function_call(send_msg);
    will_return(send_msg, true);
    expect_function_call(send_msg);
    will_return(send_msg, false);
    process_P2P_OutgoingMessage(&sock);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_handle_P2P_Incoming_hub),
        cmocka_unit_test(test_handle_P2P_Incoming_spoke),
        cmocka_unit_test(test_process_P2P_IncomingMessage_hub),
        cmocka_unit_test(test_process_P2P_IncomingMessage_spoke),
        cmocka_unit_test(test_add_P2P_OutgoingMessage),
        cmocka_unit_test(test_process_P2P_OutgoingMessage_hub),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
