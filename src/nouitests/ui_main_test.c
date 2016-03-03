#include <string.h>

#include "case.h"
#include "core/sds.h"
#include "core/dict.h"
#include "core/adlist.h"
#include "core/util.h"

#include "event/event.h"
#include "ui/ui.h"

TEST_CASE("fail ui prepare test")
{
    UI_PrepareDocument();
}
