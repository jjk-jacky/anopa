
#include <anopa/err.h>

const char const *errmsg[_NB_ERR] = {
    "",
    "Invalid name",
    "Unknown service",
    "Failed dependency",
    "I/O error",
    "Uable to write service status file",
    "Unable to get into service directory",
    "Unable to exec",
    "Unable to setup pipes",
    "Failed to communicate with s6",
    "Failed",
    "Timed out",
    "Failed to create repository directory",
    "Failed to create scandir directory",
    "Failed to enable/create servicedir",

    "Already up",
    "Not up"
};
