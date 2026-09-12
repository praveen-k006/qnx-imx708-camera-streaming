/*
 * Copyright (c) 2024, BlackBerry Limited. All rights reserved.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>

#include <camera/camera_api.h>
#include "FrameQueue.h"
#include "FrameUtils.h"
#include "worker.h"
#include "UdpSender.h"
#include <pthread.h>
FrameQueue g_frameQueue;

/**
 * @brief Number of channels for supported frametypes
 */
#define NUM_CHANNELS (3)

/**
 * @brief List of frametypes that @c processCameraData can operate on
 */
const camera_frametype_t cSupportedFrametypes[] = {
    /* NV12 added: it is the default_video_format in the actually-installed
     * rpi4_camera_module3.conf for this IMX708 sensor unit (confirmed by
     * reading that file directly), not one of the originally assumed
     * formats. Without this, the camera would be rejected as unsupported
     * at startup on this exact target. */
    CAMERA_FRAMETYPE_NV12,
    CAMERA_FRAMETYPE_YCBYCR,
    CAMERA_FRAMETYPE_CBYCRY,
    CAMERA_FRAMETYPE_RGB8888,
    CAMERA_FRAMETYPE_BGR8888,
};
#define NUM_SUPPORTED_FRAMETYPES (sizeof(cSupportedFrametypes) / sizeof(cSupportedFrametypes[0]))

/**
 * @brief Prints a list of available cameras
 */
static void listAvailableCameras(void);

/**
 * @brief Callback function to be called when camera data is available
 *
 * @param handle Handle to the camera providing the data
 * @param buffer Buffer of camera data
 * @param arg Argument provided when starting streaming
 */
static void processCameraData(camera_handle_t handle, camera_buffer_t* buffer, void* arg);

/**
 * @brief Blocks until the user presses any key
 */
static void blockOnKeyPress(void);

int main(int argc, char* argv[])
{
    int err;
    int opt;
    FrameQueue_init(&g_frameQueue);
    if (UdpSender_init("10.0.0.2", 5000) != 0)
    {
        printf("Failed to initialize UDP sender\n");
        return EXIT_FAILURE;
    }
    pthread_t worker;

    if (pthread_create(&worker,
                       NULL,
                       workerThread,
                       NULL) != 0)
    {
        printf("Failed to create worker thread\n");
        return EXIT_FAILURE;
    }
    camera_unit_t unit = CAMERA_UNIT_NONE;
    camera_handle_t handle = CAMERA_HANDLE_INVALID;
    camera_frametype_t frametype = CAMERA_FRAMETYPE_UNSPECIFIED;

    // Read command line options
    while ((opt = getopt(argc, argv, "u:")) != -1) {
        switch (opt) {
        case 'u':
            unit = (camera_unit_t)strtol(optarg, NULL, 10);
            break;
        default:
            /* optarg is NULL here for a bare unrecognized flag (e.g. -c) --
             * it is only set by getopt() for options declared with ':' in
             * the optstring. Passing NULL to %s previously segfaulted. */
            if (optarg != NULL) {
                printf("Ignoring unrecognized option with argument: %s\n", optarg);
            } else {
                printf("Ignoring unrecognized option: -%c\n", optopt);
            }
            break;
        }
    }

    // If no camera unit has been specified, list the options and exit
    if ((unit == CAMERA_UNIT_NONE) || (unit >= CAMERA_UNIT_NUM_UNITS)) {
        listAvailableCameras();
        printf("Please provide camera unit with -u option\n");
        exit(EXIT_SUCCESS);
    }

    printf("Selected camera unit: CAMERA_UNIT_%d\n", (int)unit);

    // Open a read-only handle for the specified camera unit.
    // CAMERA_MODE_RO doesn't give us access to change camera configuration
    // and we can't modify the memory in a provided buffer.
    err = camera_open(unit, CAMERA_MODE_RO, &handle);
    if ((err != CAMERA_EOK) || (handle == CAMERA_HANDLE_INVALID)) {
        printf("camera_open: FAILED for CAMERA_UNIT_%d: err = %d\n", (int)unit, err);
        exit(EXIT_FAILURE);
    }
    printf("camera_open: OK (handle=%d)\n", (int)handle);

    // Make sure that this camera defaults to a supported frametype
    err = camera_get_vf_property(handle, CAMERA_IMGPROP_FORMAT, &frametype);
    if (err != CAMERA_EOK) {
        printf("Failed to get frametype for CAMERA_UNIT_%d: err = %d\n", (int)unit, err);
        (void)camera_close(handle);
        exit(EXIT_FAILURE);
    }
    bool unsupportedFrametype = true;
    for (uint i = 0; i < NUM_SUPPORTED_FRAMETYPES; i++) {
        if (frametype == cSupportedFrametypes[i]) {
            unsupportedFrametype = false;
            break;
        }
    }
    if (unsupportedFrametype) {
        printf("Camera frametype %d is not supported\n", (int)frametype);
        (void)camera_close(handle);
        exit(EXIT_FAILURE);
    }
    printf("\n");

    // Start the camera streaming: callbacks will start being received
    err = camera_start_viewfinder(handle, processCameraData, NULL, NULL);
    if (err != CAMERA_EOK) {
        printf("camera_start_viewfinder: FAILED for CAMERA_UNIT_%d: err = %d\n", (int)unit, err);
        (void)camera_close(handle);
        exit(EXIT_FAILURE);
    }
    printf("camera_start_viewfinder: OK, streaming CAMERA_UNIT_%d\n", (int)unit);

    blockOnKeyPress();

    // Stop the camera streaming: no more callbacks will be received
    err = camera_stop_viewfinder(handle);
    printf("\r\n");
    if (err != CAMERA_EOK) {
        printf("Failed to stop CAMERA_UNIT_%d: err = %d\n", (int)unit, err);
        (void)camera_close(handle);
        exit(EXIT_FAILURE);
    }

    // Close the camera handle
    err = camera_close(handle);
    if (err != CAMERA_EOK) {
        printf("Failed to close CAMERA_UNIT_%d: err = %d\n", (int)unit, err);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

static void listAvailableCameras(void)
{
    int err;
    uint numSupported;
    camera_unit_t* supportedCameras;

    // Determine how many cameras are supported
    err = camera_get_supported_cameras(0, &numSupported, NULL);
    if (err != CAMERA_EOK) {
        printf("Failed to get number of supported cameras: err = %d\n", err);
        return;
    }

    if (numSupported == 0) {
        printf("No supported cameras detected!\n");
        return;
    }

    // Allocate an array big enough to hold all camera units
    supportedCameras = (camera_unit_t*)calloc(numSupported, sizeof(camera_unit_t));
    if (supportedCameras == NULL) {
        printf("Failed to allocate memory for supported cameras\n");
        return;
    }

    // Get the list of supported cameras
    err = camera_get_supported_cameras(numSupported, &numSupported, supportedCameras);
    if (err != CAMERA_EOK) {
        printf("Failed to get list of supported cameras: err = %d\n", err);
    } else {
        printf("Available camera units:\n");
        for (uint i = 0; i < numSupported; i++) {
            printf("\tCAMERA_UNIT_%d", supportedCameras[i]);
            printf(" (specify -u %d)\n", supportedCameras[i]);
        }
    }

    free(supportedCameras);
    return;
}
static void processCameraData(camera_handle_t handle,
                              camera_buffer_t *buffer,
                              void *arg)
{
    static int printed = 0;

    (void)handle;
    (void)arg;

    if (buffer == NULL)
        return;

    // copyFrame() is the single place that reads the format-specific
    // framedesc union member (see FrameUtils.c); reuse its result here
    // instead of re-deriving width/height/stride from the union again.
    Frame frame = copyFrame(buffer);

    if (!printed && frame.data != NULL)
    {
        printed = 1;

        printf("\n");
        printf("=========== First frame received ===========\n");
        printf("Frame type      : %d (%s)\n", buffer->frametype, frametypeName(buffer->frametype));
        printf("Width           : %u\n", frame.width);
        printf("Height          : %u\n", frame.height);
        printf("Stride          : %u\n", frame.stride);
        printf("Frame size      : %zu bytes\n", frame.size);
        printf("==============================================\n\n");
    }

    if (frame.data != NULL)
    {
        if (!FrameQueue_push(&g_frameQueue, frame))
        {
            // Queue was full: frame was dropped, so free it here since it
            // never got handed off to the worker thread for freeing.
            freeFrame(&frame);
        }
    }
}
static void blockOnKeyPress(void)
{
    struct termios oldterm;
    struct termios newterm;
    char key;

    (void)tcgetattr(STDIN_FILENO, &oldterm);
    newterm = oldterm;
    newterm.c_lflag &= ~(ECHO | ICANON);
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
    // Blocking call: wait for 1 byte of data to become available
    (void)read(STDIN_FILENO, &key, 1);
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);

    return;
    //asdf
}
