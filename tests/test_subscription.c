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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rebar-c.h"

#include <CUnit/Basic.h>

#include "../src/subscription.h"
#include "../src/ParodusInternal.h"


/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int sendMsgtoRegisteredClients(char *dest,const char **Msg,size_t msgSize)
{
    UNUSED(Msg);
    UNUSED(msgSize);
    check_expected(dest);
    function_called();
    return 0;
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_init_subscription()
{
    init_subscription_list();
    assert_non_null(get_global_subscription_list());
    assert_null(get_global_subscription_list()->head);
    assert_null(get_global_subscription_list()->tail);
}

void test_add_Client_Subscription()
{
    bool status;
    Subscription *sub = NULL;
    status = add_Client_Subscription("config", "node-change");
    assert_true(status);
    assert_int_equal(1, (int)rebar_ll_count(get_global_subscription_list()));
    sub = rebar_ll_get_data(Subscription, sub_node, get_global_subscription_list()->head);
    assert_non_null(sub);
    assert_string_equal("config", sub->service_name);
    assert_string_equal("node-change", sub->regex);
    status = add_Client_Subscription("iot", "device-status");
    assert_int_equal(2, (int)rebar_ll_count(get_global_subscription_list()));
    sub = rebar_ll_get_data(Subscription, sub_node, get_global_subscription_list()->tail);
    assert_non_null(sub);
    assert_string_equal("iot", sub->service_name);
    assert_string_equal("device-status", sub->regex);
    assert_true(status);
    status = add_Client_Subscription("config2", "node-change/*");
    assert_true(status);
    status = add_Client_Subscription("iot2", "device-status/*");
    assert_true(status);
}

void test_get_Client_Subscriptions()
{
    cJSON *json;
    json = get_Client_Subscriptions("config");
    assert_int_equal(1, cJSON_GetArraySize(json));
    assert_string_equal("node-change", cJSON_GetArrayItem(json, 0)->valuestring);
    assert_non_null(json);
    cJSON_Delete(json);
}

void test_filter_clients_and_send()
{
    wrp_msg_t wrp_msg;
    memset(&wrp_msg, 0, sizeof(wrp_msg_t));
    wrp_msg.msg_type = WRP_MSG_TYPE__EVENT;     
    wrp_msg.u.event.source = strdup("mac:14cfe214266");
    wrp_msg.u.event.dest   = strdup("event:node-change");
    wrp_msg.u.event.payload = NULL;
    wrp_msg.u.event.payload_size = 0;
    expect_string(sendMsgtoRegisteredClients, dest, "config");
    expect_function_call(sendMsgtoRegisteredClients);
    filter_clients_and_send(&wrp_msg);
    free(wrp_msg.u.event.source);
    free(wrp_msg.u.event.dest);
}

void test_delete_client_subscriptions()
{
    UserDataCounter_t data;
    memset(&data, 0, sizeof(UserDataCounter_t));
    data.service_name = strdup("config2");
    delete_client_subscriptions(&data);
    assert_true(1 == data.delete_count && 1 == data.hit_count);
    
    data.hit_count = 0;
    data.delete_count = 0;
    delete_client_subscriptions(&data);
    assert_true(0 == data.delete_count && 0 == data.hit_count);
   
    free(data.service_name);
    data.service_name = strdup("foo");
    delete_client_subscriptions(&data);
    assert_true(0 == data.delete_count && 0 == data.hit_count);
    free(data.service_name);
   
    memset(&data, 0, sizeof(UserDataCounter_t));
    data.service_name = strdup("config");
    delete_client_subscriptions(&data);
    assert_true(1 == data.delete_count && 1 == data.hit_count);
    free(data.service_name);
   
    memset(&data, 0, sizeof(UserDataCounter_t));
    data.service_name = strdup("iot2");
    delete_client_subscriptions(&data);
    assert_true(1 == data.delete_count && 1 == data.hit_count);
    free(data.service_name);

    assert_int_equal(1, (int)rebar_ll_count(get_global_subscription_list()));

    memset(&data, 0, sizeof(UserDataCounter_t));
    data.service_name = strdup("iot");
    delete_client_subscriptions(&data);
    assert_true(1 == data.delete_count && 1 == data.hit_count);
    free(data.service_name);
  
    assert_int_equal(0, (int)rebar_ll_count(get_global_subscription_list()));
}

void err_add_Client_Subscription()
{
    bool status;
    status = add_Client_Subscription(NULL, "node-change");
    assert_false(status);
    assert_int_equal(0, (int)rebar_ll_count(get_global_subscription_list()));
    status = add_Client_Subscription("iot", NULL);
    assert_int_equal(0, (int)rebar_ll_count(get_global_subscription_list()));
    assert_false(status);
    status = add_Client_Subscription(NULL, NULL);
    assert_int_equal(0, (int)rebar_ll_count(get_global_subscription_list()));
    assert_false(status);
}

void err_get_Client_Subscriptions()
{
    cJSON *json;
    json = get_Client_Subscriptions("config");
    assert_null(json);
    json = get_Client_Subscriptions(NULL);
    assert_null(json);
}

void err_filter_clients_and_send()
{
    filter_clients_and_send(NULL);
}

void err_delete_client_subscriptions()
{
    UserDataCounter_t data;
    memset(&data, 0, sizeof(UserDataCounter_t));
    data.service_name = strdup("config");
    delete_client_subscriptions(&data);
    assert( 0 == data.delete_count);
    free(data.service_name);
    data.service_name = NULL;
    delete_client_subscriptions(&data);
    assert( 0 == data.delete_count);
}

void err_init_subscription()
{
    init_subscription_list();
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    int ret;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init_subscription),
        cmocka_unit_test(test_add_Client_Subscription),
        cmocka_unit_test(test_get_Client_Subscriptions),
        cmocka_unit_test(test_filter_clients_and_send),
        cmocka_unit_test(test_delete_client_subscriptions),
        cmocka_unit_test(err_add_Client_Subscription),
        cmocka_unit_test(err_get_Client_Subscriptions),
        cmocka_unit_test(err_filter_clients_and_send),
        cmocka_unit_test(err_delete_client_subscriptions),
        cmocka_unit_test(err_init_subscription),
    };

    ret =  cmocka_run_group_tests(tests, NULL, NULL);
    delete_global_subscription_list();

    return ret;
}
