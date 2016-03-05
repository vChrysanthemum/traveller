#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

#include "lua.h"

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/ini.h"
#include "core/errors.h"

#include "net/networking.h"
#include "event/event.h"
#include "script/script.h"
#include "net/resp/service/service.h"

#include "core/extern.h"

aeLooper_t   *nt_el;
ntRespServer_t nt_RespServer;
