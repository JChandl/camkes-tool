/*#
 *# Copyright 2014, NICTA
 *#
 *# This software may be distributed and modified according to the terms of
 *# the BSD 2-Clause license. Note that NO WARRANTY is provided.
 *# See "LICENSE_BSD2.txt" for details.
 *#
 *# @TAG(NICTA_BSD)
 #*/

/*- set p = Perspective(instance=me.to_instance.name, interface=me.to_interface.name) -*/
/*- if parse_bool(configuration[me.to_instance.name].get(p['passive_attribute'], 'false')) -*/
    /*? raise(Exception("Interfaces connected with %s may not be declared passive." % me.type.name)) ?*/
/*- endif -*/

#include <assert.h>
#include <sel4/sel4.h>
#include <stddef.h>
#include <utils/util.h>

/*? macros.show_includes(me.to_instance.type.includes) ?*/

/*- set notification = alloc('notification', seL4_NotificationObject, read=True) -*/
/*- set lock = alloc('lock', seL4_NotificationObject, read=True, write=True) -*/

#define MAX_CALLBACKS 10

static void (*volatile callbacks[MAX_CALLBACKS])(void*);
static void *callback_args[MAX_CALLBACKS];
static volatile int event_pending;
static volatile int sleepers;

#define CAS __sync_val_compare_and_swap
#define ATOMIC_INCREMENT(ptr) __sync_fetch_and_add((ptr), 1)
#define ATOMIC_DECREMENT(ptr) __sync_fetch_and_sub((ptr), 1)

#define SLEEP() \
    do { \
        ATOMIC_INCREMENT(&sleepers); \
        assert(sleepers > 0); \
        (void)seL4_Recv(/*? lock ?*/, NULL); \
        assert(sleepers > 0); \
        ATOMIC_DECREMENT(&sleepers); \
    } while (0)

#define WAKE() seL4_Signal(/*? lock ?*/)

int /*? me.to_interface.name ?*/__run(void) {
    while (1) {
        int handled = 0;

        (void)seL4_Recv(/*? notification ?*/, NULL);

        /* First preference: callbacks. */
        if (!handled) {
            for (int i = 0; i < MAX_CALLBACKS; ++i) {
                void (*callback)(void*) = callbacks[i];
                if (callback != NULL) {
                    callbacks[i] = NULL; /* No need for CAS. */
                    callback(callback_args[i]);
                    handled = 1;
                    break;
                }
            }
        }

        /* There may in fact already be a pending event, but we don't care. */
        event_pending = 1;

        /* Second preference: waiters. */
        if (!handled) {
            if (sleepers > 0) { /* No lock required. */
                WAKE();
                /* Assume one of them will grab it. */
                handled = 1;
            }
        }

        /* Else, leave it for polling. */
    }

    UNREACHABLE();
}

int /*? me.to_interface.name ?*/_poll(void) {
    return CAS(&event_pending, 1, 0);
}

void /*? me.to_interface.name ?*/_wait(void) {
    while (!/*? me.to_interface.name ?*/_poll()) {
        SLEEP();
    }
}

int /*? me.to_interface.name ?*/_reg_callback(void (*callback)(void*), void *arg) {
    for (int i = 0; i < MAX_CALLBACKS; ++i) {
        if (CAS(&callbacks[i], NULL, callback) == NULL) {
            callback_args[i] = arg;
            return 0;
        }
    }
    /* We didn't find an empty slot. */
    return -1;
}
