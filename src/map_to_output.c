#include <stdio.h>
#include <stdbool.h>

#include <xorg/xf86.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xinerama.h>

static bool is_pointer(int use)
{
    return use == XIMasterPointer || use == XISlavePointer;
}

static bool is_keyboard(int use)
{
    return use == XIMasterKeyboard || use == XISlaveKeyboard;
}

Bool device_matches(XIDeviceInfo *info, char *name)
{
    if (strcmp(info->name, name) == 0) {
        return True;
    }

    if (strncmp(name, "pointer:", strlen("pointer:")) == 0 &&
        strcmp(info->name, name + strlen("pointer:")) == 0 &&
        is_pointer(info->use)) {
        return True;
    }

    if (strncmp(name, "keyboard:", strlen("keyboard:")) == 0 &&
        strcmp(info->name, name + strlen("keyboard:")) == 0 &&
        is_keyboard(info->use)) {
        return True;
    }

    return False;
}

XIDeviceInfo*
xi2_find_device_info(Display *display, char *name)
{
    XIDeviceInfo *info;
    XIDeviceInfo *found = NULL;
    int ndevices;
    Bool is_id = True;
    int i, id = -1;

    for(i = 0; i < strlen(name); i++) {
	if (!isdigit(name[i])) {
	    is_id = False;
	    break;
	}
    }

    if (is_id) {
	id = atoi(name);
    }

    info = XIQueryDevice(display, XIAllDevices, &ndevices);
    for(i = 0; i < ndevices; i++)
    {
        if (is_id ? info[i].deviceid == id : device_matches (&info[i], name)) {
            if (found) {
                fprintf(stderr,
                        "Warning: There are multiple devices matching '%s'.\n"
                        "To ensure the correct one is selected, please use "
                        "the device ID, or prefix the\ndevice name with "
                        "'pointer:' or 'keyboard:' as appropriate.\n\n", name);
                XIFreeDeviceInfo(info);
                return NULL;
            } else {
                found = &info[i];
            }
        }
    }

    return found;
}

int
map_to_output(Display *dpy, int argc, char *argv[], char *name, char *desc)
{
    char *output_name;
    XIDeviceInfo *info;
    XRROutputInfo *output_info;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: xinput %s %s\n", name, desc);
        return EXIT_FAILURE;
    }

    info = xi2_find_device_info(dpy, argv[0]);
    if (!info)
    {
        fprintf(stderr, "unable to find device '%s'\n", argv[0]);
        return EXIT_FAILURE;
    }

    output_name = argv[1];
    output_info = find_output_xrandr(dpy, output_name);
    if (!output_info)
    {
        /* Output doesn't exist. Is this a (partial) non-RandR setup?  */
        output_info = find_output_xrandr(dpy, "default");
        if (output_info)
        {
            XRRFreeOutputInfo(output_info);
            if (strncmp("HEAD-", output_name, strlen("HEAD-")) == 0)
                return map_output_xinerama(dpy, info->deviceid, output_name);
        }
    } else
        XRRFreeOutputInfo(output_info);

    return map_output_xrandr(dpy, info->deviceid, output_name);
}
