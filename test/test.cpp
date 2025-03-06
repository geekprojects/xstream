/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/gst.h>
#include <gst/app/app.h>

#include <gst/rtsp-server/rtsp-server.h>

#include <stdio.h>

typedef struct
{
    uint8_t col;
    GstClockTime timestamp;
    uint8_t* buffer;
} MyContext;

uint32_t palette[] = {
    0xff000000,
    0xff0000D7,
    0xffD70000,
    0xffD700D7,
    0xff00D700,
    0xff00D7D7,
    0xffD7D700,
    0xffD7D7D7,
    0xffffff00,
    0xffffffff,
};

/* called when we need to give data to appsrc */
static void
need_data(GstElement* appsrc, guint unused, MyContext* ctx)
{
    guint size;

    size = 1024 * 768 * 4;

    GstBuffer* buffer;
    buffer = gst_buffer_new_allocate(NULL, size, NULL);

    time_t timestamp;
    time(&timestamp);
    struct tm local_time = {};
    localtime_r(&timestamp, &local_time);

    int mins = local_time.tm_min;
    int secs = local_time.tm_sec;

    printf("need_data: %02d:%02d\n", mins, secs);

    int h1 = mins / 10;
    int h2 = mins % 10;
    int m1 = secs / 10;
    int m2 = secs % 10;

    int quarter = size / 4;
    for (guint i = 0; i < quarter; i += 4)
    {
        uint32_t c = palette[h1];
        ctx->buffer[i + 0] = c & 0xff;
        ctx->buffer[i + 1] = (c >> 8) & 0xff;
        ctx->buffer[i + 2] = (c >> 16) & 0xff;
        ctx->buffer[i + 3] = (c >> 24) & 0xff;

        c = palette[h2];
        ctx->buffer[(quarter * 1) + i + 0] = c & 0xff;
        ctx->buffer[(quarter * 1) + i + 1] = (c >> 8) & 0xff;
        ctx->buffer[(quarter * 1) + i + 2] = (c >> 16) & 0xff;
        ctx->buffer[(quarter * 1) + i + 3] = (c >> 24) & 0xff;

        c = palette[m1];
        ctx->buffer[(quarter * 2) + i + 0] = c & 0xff;
        ctx->buffer[(quarter * 2) + i + 1] = (c >> 8) & 0xff;
        ctx->buffer[(quarter * 2) + i + 2] = (c >> 16) & 0xff;
        ctx->buffer[(quarter * 2) + i + 3] = (c >> 24) & 0xff;

        c = palette[m2];
        ctx->buffer[(quarter * 3) + i + 0] = c & 0xff;
        ctx->buffer[(quarter * 3) + i + 1] = (c >> 8) & 0xff;
        ctx->buffer[(quarter * 3) + i + 2] = (c >> 16) & 0xff;
        ctx->buffer[(quarter * 3) + i + 3] = (c >> 24) & 0xff;

    }
    gst_buffer_fill(buffer, 0, ctx->buffer, size);

    /* increment the timestamp every 1/2 second */
    //GST_BUFFER_PTS(buffer) = ctx->timestamp;
    //GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 2);
    //GST_BUFFER_FLAGS(buffer) |= GST_BUFFER_FLAG_LIVE;
    //ctx->timestamp += GST_BUFFER_DURATION(buffer);

    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void
media_configure(
    GstRTSPMediaFactory* factory,
    GstRTSPMedia* media,
    gpointer user_data)
{
    GstElement* element,* appsrc;
    MyContext* ctx;

    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element(media);
    //gst_rtsp_media_set_suspend_mode(media, GST_RTSP_SUSPEND_MODE_NONE);
    //gst_rtsp_media_set_latency(media, 0);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
    /* configure the caps of the video */
    g_object_set(
        G_OBJECT(appsrc),
        "caps",
        gst_caps_new_simple(
            "video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, 1024,
            "height", G_TYPE_INT, 768,
            "framerate", GST_TYPE_FRACTION, 0, 1,
            NULL),
        NULL);

    ctx = g_new0(MyContext, 1);
    ctx->col = 0;
    ctx->timestamp = 0;
    ctx->buffer = new uint8_t[1024 * 768 * 4];
    /* make sure ther datais freed when the media is gone */
    g_object_set_data_full(
        G_OBJECT(media),
        "my-extra-data",
        ctx,
        (GDestroyNotify) g_free);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect(appsrc, "need-data", (GCallback) need_data, ctx);
    gst_object_unref(appsrc);
    gst_object_unref(element);
}

int main(int argc, char* argv[])
{

    gst_init(&argc, &argv);

    GMainLoop* loop = g_main_loop_new(NULL, FALSE);

    /* create a server instance */
    GstRTSPServer* server = gst_rtsp_server_new();

    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server);

    /* make a media factory for a test stream. The default media factory can use
     * gst-launch syntax to create pipelines.
     * any launch line works as long as it contains elements named pay%d. Each
     * element with pay%d names will be a stream */
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(
        factory,
        "( appsrc name=mysrc block=false is-live=1 do-timestamp=1 min-latency=0 ! videoconvert ! video/x-raw,format=I420 ! x264enc ! rtph264pay name=pay0 pt=96 )");

    /* notify when our media is ready, This is called whenever someone asks for
     * the media and a new pipeline with our appsrc is created */
    g_signal_connect(
        factory,
        "media-configure",
        (GCallback) media_configure,
        NULL);

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

    /* don't need the ref to the mounts anymore */
    g_object_unref(mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);

    /* start serving */
    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run(loop);

    return 0;
}
