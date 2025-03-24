#ifndef OPENCCA_DEBUG_H
#define OPENCCA_DEBUG_H
#include <common/debug.h>

#define HERE VERBOSE("[tfa] %s/%s: %d, core:%d, el:%d\n", \
    __FILE__, __FUNCTION__, __LINE__, (int) plat_my_core_pos(), (int) get_current_el())

#endif /* OPENCCA_DEBUG_H */