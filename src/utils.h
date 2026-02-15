#pragma once

#define HOMEKIT_MIN(a, b) ((b) < (a) ? (b) : (a))
#define HOMEKIT_MAX(a, b) ((a) < (b) ? (b) : (a))
#define HOMEKIT_COUNTOF(items) (sizeof(items) / sizeof((items)[0]))
