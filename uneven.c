#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>

// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_devicelist {
    uint8_t initialized;
    char name[512];
    uint32_t index;
    char description[256];
} pa_devicelist_t;

void pa_state_cb(pa_context *c, void *userdata);
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output);

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata) {
    pa_context_state_t state;
    int *pa_ready = userdata;

    state = pa_context_get_state(c);
    switch  (state) {
        // There are just here for reference
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

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata) {
    pa_devicelist_t *pa_devicelist = userdata;
    int ctr = 0;

    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0) {
        return;
    }

    // We know we've allocated 16 slots to hold devices.  Loop through our
    // structure and find the first one that's "uninitialized."  Copy the
    // contents into it and we're done.  If we receive more than 16 devices,
    // they're going to get dropped.  You could make this dynamically allocate
    // space for the device list, but this is a simple example.
    for (ctr = 0; ctr < 16; ctr++) {
        if (! pa_devicelist[ctr].initialized) {
            strncpy(pa_devicelist[ctr].name, l->name, 511);
            strncpy(pa_devicelist[ctr].description, l->description, 255);
            pa_devicelist[ctr].index = l->index;
            pa_devicelist[ctr].initialized = 1;
            break;
        }
    }
}

// See above.  This callback is pretty much identical to the previous
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) {
    pa_devicelist_t *pa_devicelist = userdata;
    int ctr = 0;

    if (eol > 0) {
        return;
    }

    for (ctr = 0; ctr < 16; ctr++) {
        if (! pa_devicelist[ctr].initialized) {
            strncpy(pa_devicelist[ctr].name, l->name, 511);
            strncpy(pa_devicelist[ctr].description, l->description, 255);
            pa_devicelist[ctr].index = l->index;
            pa_devicelist[ctr].initialized = 1;
            break;
        }
    }
}

int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output) {
    // Initialize our device lists
    memset(input, 0, sizeof(pa_devicelist_t) * 16);
    memset(output, 0, sizeof(pa_devicelist_t) * 16);

    // Create a mainloop
    pa_mainloop* pa_ml = pa_mainloop_new();
    pa_mainloop_api* pa_mlapi = pa_mainloop_get_api(pa_ml);

    // Establish PA context
    pa_context* pa_ctx = pa_context_new(pa_mlapi, "uneven");
    pa_context_connect(pa_ctx, NULL, 0, NULL);

    // Set up PA state callback
    int pa_state = 0;
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_state);

    // Iterate PA mainloop until we're done or the context enters an error state
    int state = 0;
    int done = 0;
    pa_operation* pending_op = NULL;
    while (done == 0 && pa_state != 2) {
        pa_mainloop_iterate(pa_ml, 1, NULL);

        // Can't do anything unless PA is ready
        if (pa_state == 1) {
            // Wait for any pending operation to complete
            if (!pending_op || (pending_op && pa_operation_get_state(pending_op) == PA_OPERATION_DONE)) {
                if (pending_op) {
                    pa_operation_unref(pending_op);
                    pending_op = NULL;
                }
                switch (state) {
                    case 0:
                        pending_op = pa_context_get_sink_info_list(
                            pa_ctx,
                            pa_sinklist_cb,
                            output
                        );

                        state++;
                        break;
                    case 1:
                        pending_op = pa_context_get_source_info_list(
                            pa_ctx,
                            pa_sourcelist_cb,
                            input
                        );

                        state++;
                        break;
                    case 2:
                        done = 1;
                        break;
                    default:
                        fprintf(stderr, "in state %d\n", state);
                        done = -1;
                }
            }
        }
    }

    // Clean up
    pa_context_disconnect(pa_ctx);
    pa_context_unref(pa_ctx);
    pa_mainloop_free(pa_ml);

    return done == 1 && pa_state != 2;
}

int main(int argc, char *argv[]) {
    int ctr;

    // This is where we'll store the input device list
    pa_devicelist_t pa_input_devicelist[16];

    // This is where we'll store the output device list
    pa_devicelist_t pa_output_devicelist[16];

    if (pa_get_devicelist(pa_input_devicelist, pa_output_devicelist) < 0) {
        fprintf(stderr, "failed to get device list\n");
        return 1;
    }

    for (ctr = 0; ctr < 16; ctr++) {
        if (! pa_output_devicelist[ctr].initialized) {
            break;
        }
        printf("=======[ Output Device #%d ]=======\n", ctr+1);
        printf("Description: %s\n", pa_output_devicelist[ctr].description);
        printf("Name: %s\n", pa_output_devicelist[ctr].name);
        printf("Index: %d\n", pa_output_devicelist[ctr].index);
        printf("\n");
    }

    for (ctr = 0; ctr < 16; ctr++) {
        if (! pa_input_devicelist[ctr].initialized) {
            break;
        }
        printf("=======[ Input Device #%d ]=======\n", ctr+1);
        printf("Description: %s\n", pa_input_devicelist[ctr].description);
        printf("Name: %s\n", pa_input_devicelist[ctr].name);
        printf("Index: %d\n", pa_input_devicelist[ctr].index);
        printf("\n");
    }
    return 0;
}
