/*
 * This file is part of the ESO C Extension Library
 * Copyright (C) 2001-2026 European Southern Observatory
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#undef CX_DISABLE_ASSERT
#undef CX_LOG_DOMAIN

#include "cxmessages.h"

#include <pthread.h>
#include <stdio.h>


#define CX_TEST_NTHREADS 2


typedef struct cx_test_thread_data cx_test_thread_data;

struct cx_test_thread_data
{
    cxsize tid;
    cxsize repetitions;
};


static cx_test_thread_data tdata[6] = {{3, 1000}, {6, 1000}, {0, 1000},
                                       {0, 1000}, {0, 1000}, {0, 1000}};


static cxptr
_cx_test_worker(cxptr args)
{
    register cxsize i = 0;

    cx_test_thread_data *data = (cx_test_thread_data *)args;


    for (i = 0; i < data->repetitions; ++i) {
        cx_warning("TID %" CX_PRINTF_FORMAT_SIZE_TYPE ": Hello, world!\n",
                   data->tid);
    }

    return NULL;
}


int
main(void)
{
    /*
     * Test 1: Performance test of cxmessages calls
     */

#if 0

    cxsize i = 0;
    cxsize nthreads = CX_TEST_NTHREADS;

    pthread_t threads[CX_TEST_NTHREADS];
    pthread_attr_t attr;


    fprintf(stderr, "Using %" CX_PRINTF_FORMAT_SIZE_TYPE " threads\n", nthreads);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < nthreads; ++i)
    {
        cxint status = 0;

        tdata[i].tid = i;

        status = pthread_create(&threads[i], &attr, _cx_test_worker, &tdata[i]);

        if (status != 0) {
            fprintf(stderr, "error: cannot create thread (error code %d)\n",
                    status);
            return -1;
        }

    }

    pthread_attr_destroy(&attr);

    for (i = 0; i < nthreads; ++i)
    {
        cxint status = 0;

        cxptr result = NULL;


        status = pthread_join(threads[i], &result);

        if (status != 0) {
            fprintf(stderr, "error: joining threads failed (error code %d)\n",
                    status);
            return -1;
        }

    }

    return 0;

#else
    cxsize i = 0;


    fprintf(stderr, "Using no threads\n");

    for (i = 0; i < CX_TEST_NTHREADS; ++i) {
        _cx_test_worker(&tdata[i]);
    }

    return 0;
#endif

    /*
     * All tests succeeded
     */

    //return 0;
}
