/**
 *  Copyright 2018 Comcast Cable Communications Management, LLC
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
#include <stdarg.h>

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <CUnit/Basic.h>

#include "../src/ParodusInternal.h"
#include "../src/parodus_interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define HUB   "tcp://127.0.0.1:7777"
#define SPOKE "tcp://127.0.0.1:8888"

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    char   *d;
    char   *n;
    ssize_t nsz;
} test_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void *check_hub();
static void *check_spoke();

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static bool g_execute = true;
static test_t tests[] = {
    {   // 0 
        .d = HUB,
        .n = "Some binary",
        .nsz = 11,
     },
    {   // 1 
        .d = SPOKE,
        .n = "Some other binary",
        .nsz = 17,
     },
};

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_push_pull()
{
    bool result;
    pthread_t t;

    g_execute = true;
    pthread_create(&t, NULL, check_hub, NULL);

    result = spoke_send_msg(tests[0].d, tests[0].n, tests[0].nsz);
    CU_ASSERT(true == result);

    g_execute = false;
}

void test_pub_sub()
{
    bool result;
    pthread_t t;

    g_execute = true;
    pthread_create(&t, NULL, check_spoke, NULL);

    result = hub_send_msg(tests[1].d, tests[1].n, tests[1].nsz);
    CU_ASSERT(true == result);

    g_execute = false;
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        printf("--------Start of Test Cases Execution ---------\n");
        suite = CU_add_suite( "tests", NULL, NULL );
        CU_add_test( suite, "Test Push/Pull", test_push_pull );
        CU_add_test( suite, "Test Pub/Sub",   test_pub_sub );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
static void *check_hub()
{
    char *msg;
    ssize_t msg_sz;

    set_parodus_to_parodus_listener_url( HUB );
    msg_sz = hub_check_inbox(&msg);
    if( 0 < msg_sz ) {
        CU_ASSERT_EQUAL( (tests[0].nsz + 1), msg_sz );
        CU_ASSERT_STRING_EQUAL( tests[0].n, msg );
        free(msg);
    } else {
        printf("Spoke sent msg not received in Hub inbox!!!\n");
    }
    return NULL;
}

static void *check_spoke()
{
    char *msg;
    ssize_t msg_sz;

    set_parodus_to_parodus_listener_url( SPOKE );
    msg_sz = spoke_check_inbox(&msg);
    if( 0 < msg_sz ) {
        CU_ASSERT_EQUAL( (tests[1].nsz + 1), msg_sz );
        CU_ASSERT_STRING_EQUAL( tests[1].n, msg );
        free(msg);
    } else {
        printf("Hub sent msg not received in Spoke inbox!!!\n");
    }
    return NULL;
}