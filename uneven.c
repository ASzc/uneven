#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pulse/pulseaudio.h>

void pa_state_cb(pa_context *c, void *userdata) {
    pa_context_state_t state;
    int *pa_ready = userdata;

    state = pa_context_get_state(c);
    switch  (state) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        default:
            *pa_ready = 0;
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            *pa_ready = 2;
            break;
        case PA_CONTEXT_READY:
            *pa_ready = 1;
            break;
    }
}

void source_state_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
    int* state = userdata;

    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SOURCE) {
        if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE) {
            *state = 2;
        }
    }
}

void get_volume_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    printf("get_volume_cb\n");
    pa_cvolume* volume = userdata;

    if (eol < 0) {
        fprintf(stderr, "Failed to get source information: %s\n", pa_strerror(pa_context_errno(c)));
    }

    printf("name: %s, #chan: %d, level0: %u, level1: %u\n", i->name, i->volume.channels, i->volume.values[0], i->volume.values[1]);

    *volume = i->volume;
    printf("fetched volume ok\n");
}

volatile sig_atomic_t done = -1;

void cleanup_pa(int signum) {
    done = 0;
}

int force_volume(char* source_name, pa_volume_t desired_volume) {
    // Create a mainloop
    pa_mainloop* pa_ml = pa_mainloop_new();
    pa_mainloop_api* pa_mlapi = pa_mainloop_get_api(pa_ml);

    // Establish PA context
    pa_context* pa_ctx = pa_context_new(pa_mlapi, "uneven");
    pa_context_connect(pa_ctx, NULL, 0, NULL);

    // Set up PA state callback
    int pa_state = 0;
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_state);

    // Set up event subscription
    int state = 0;
    pa_context_set_subscribe_callback(pa_ctx, source_state_cb, &state);

    // Iterate PA mainloop until we're done or the context enters an error state
    pa_cvolume* current_volume = NULL;
    pa_operation* pending_op = NULL;
    int do_update = 0;
    int i;
    while (done == -1 && pa_state != 2) {
        pa_mainloop_iterate(pa_ml, 1, NULL);

        // Can't do anything unless PA is ready
        if (pa_state == 1) {
            // Wait for any pending operation to complete
            if (!pending_op || (pending_op && pa_operation_get_state(pending_op) == PA_OPERATION_DONE)) {
                if (pending_op) {
                    printf("<< done\n");
                    pa_operation_unref(pending_op);
                    pending_op = NULL;
                }
                switch (state) {
                    case 0:
                        printf(">> 0\n");
                        // Subscribe to source events
                        pending_op = pa_context_subscribe(pa_ctx, PA_SUBSCRIPTION_MASK_SOURCE, NULL, NULL);

                        state = 2;
                        break;
                    case 1:
                        printf(">> 1\n");
                        // Idle
                        break;
                    case 2:
                        printf(">> 2\n");
                        printf("%s\n", source_name);
                        // Source change detected, retrive volume information
                        pending_op = pa_context_get_source_info_by_name(pa_ctx, source_name, get_volume_cb, current_volume);

                        state++;
                        break;
                    case 3:
                        printf(">> 3\n");
                        // Check channel volumes and reset if required
                        do_update = 0;
                        for (i = 0; i < current_volume->channels; i++) {
                            if (current_volume->values[i] != desired_volume) {
                                do_update = 1;
                                current_volume->values[i] = desired_volume;
                            }
                        }

                        if (do_update) {
                            pending_op = pa_context_set_source_volume_by_name(pa_ctx, source_name, current_volume, NULL, NULL);
                        }

                        state = 1;
                        break;
                    default:
                        fprintf(stderr, "in state %d\n", state);
                        done = 2;
                }
            }
        }
    }

    // Clean up
    pa_context_disconnect(pa_ctx);
    pa_context_unref(pa_ctx);
    pa_mainloop_free(pa_ml);

    return done;
}

int main(int argc, char* argv[]) {
    pa_volume_t volume = (pa_volume_t) (atoi(argv[2]) * (double) PA_VOLUME_NORM / 100);
    if (!PA_VOLUME_IS_VALID(volume)) {
        return 3;
    }
    char* source_name = pa_xstrdup(argv[1]);

    signal(SIGINT, cleanup_pa);
    signal(SIGTERM, cleanup_pa);

    int retcode = force_volume(source_name, volume);

    pa_xfree(source_name);

    return retcode;
}
