#ifndef __APP_NODE_ROLE_H
#define __APP_NODE_ROLE_H

#include <stdint.h>

#define APP_NODE_ROLE_SENSOR   1U
#define APP_NODE_ROLE_EXECUTOR 2U

#ifndef APP_NODE_ROLE
#define APP_NODE_ROLE APP_NODE_ROLE_SENSOR
#endif

#ifndef APP_NODE_ID
#define APP_NODE_ID 1U
#endif

#ifndef APP_NODE_DOWNLINK_COMPAT
#define APP_NODE_DOWNLINK_COMPAT 1U
#endif

#endif
