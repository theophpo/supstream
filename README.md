
![alt text](https://gitlab.com/ouzb64ty/supstream/-/raw/main/images/supstream-logo.png)

Gstreamer-based stream supervisor using YAML

**Project is under development, looking forward to getting help to enrich the project.**

To better understand, look at the _samples_ directory

TODO list : https://trello.com/b/gAa0tKvO/supstream

# Install

## Compilation

### Ubuntu 21.10

1. You need to install gstreamer before (https://gstreamer.freedesktop.org/documentation/installing/index.html?gi-language=c)
2. type `apt install libyaml-dev && apt-install libczmq-dev && libcjson-dev`
3. type `git clone https://gitlab.com/ouzb64ty/supstream.git supstream && cd supstream && make`

## Docker

This project contains a Dockerfile for easy development mode based on latest Ubuntu.

- Automatic dependancies installations
- Gstreamer compilation using meson and ninja
- Gstreamer plugins compilations
- Vim and Zsh configuration

## Building

```bash
docker build . -t docker-gstreamer-compiler
```

## Running

```bash
xhost +
docker run --ipc=host -v=/tmp:/tmp --rm -it --device /dev/video0 --net host -e DISPLAY=$DISPLAY supstream
```

/tmp/tmp to getting visual pipeline graph

# Running

Just type `./bin/supstream samples/matroska_video_0.yaml`

```yaml
pipelines:

    matroska_video_0:

        init_state: play
        type_exec: sync

        elements:
            souphttpsrc:
                element: souphttpsrc
                properties:
                    location: https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm
                element_link: matroskademux
            matroskademux:
                element: matroskademux
                pad_link:
                    pad_video:
                        from: video_0
                        to:
                            name: vp8dec
                            pad: sink
            vp8dec:
                element: vp8dec
                element_link: videoconvert
            videoconvert:
                element: videoconvert
                element_link: autovideosink
            autovideosink:
                element: autovideosink
```
This configuration allows you to easily display a WEBM video from an MKV demuxer

# Plugin

**The formalization of the plugins is not yet very clear because for the moment there is only the bus_fn function**

For using your own C functions with Supstream,
you can call _plugin_ property in your YAML cofiguration.

Useful to write your own Gstreamer bus manager, there is a sample at _samples/matroska_video_0_ld.yaml_

```yaml
pipelines:
    plugin: template.so
    bus_fn: bus # bus management function in template.so
```

## Example

Useful to write your own Gstreamer bus manager, there is a sample in _samples/matroska_video_0_ld.yaml_ and example plugin in plugin/template.so

```c
/* Example of bus manager in C, called by Supstream after pipelines initialization */

#include <gst/gst.h>

int				bus(GstBus *bus, GstPipeline *pipeline) {

    GstMessage	*msg = NULL;

    g_print("Initialize from plugin !\n");
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        g_error ("An error occurred! Re-run with the GST_DEBUG=*:WARN environment "
                "variable set for more details.");
    }
    gst_message_unref (msg);
    gst_object_unref (bus);
    gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
    gst_object_unref (pipeline);
    return (0);
}
```

## Compilation

You must export your plugin directory in _LD_LIBRARY_PATH_ and compile your template :
```bash
gcc -shared `pkg-config --libs --cflags gstreamer-1.0` plugin/template.c -o plugin/template.so
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:plugin
```

# ZeroMQ API

This textual documentation describes how Supstream dialogues externally with ZeroMQ IPCs

- codes :
    - 0 success
    - 1 failure

The default ZMQ uri for IPC/TCP sharing is _ipc:///tmp/supstream.pipe_, you can update your own ZMQ path with _zmq_path_ property.

## Requests

Mini pygame client available in https://gitlab.com/ouzb64ty/supstream-gateway

### Play

Update state to PLAYING for a specific pipeline or rather all

```json
{
    command: "play",
    pipelines: ["matroska-video_0"]
}
```

### Pause

Update state to PAUSED for a specific pipeline or rather all

```json
{
    command: "pause",
    pipelines: ["matroska-video_0"]
}
```

### Version

```json
{
    command: "version"
}
```

### Exit

This request exit a supstream process

```json
{
    command: "exit"
}
```

### Show

Used to get general pipelines configuration, saved in cache

```json
{
    command: "show"
}
```

# Autor

tpoac <t.poac@cyim.com>
