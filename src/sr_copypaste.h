#ifndef _sr_copypaste_h
#define _sr_copypaste_h


#include <sysrepo.h>
#include <sysrepo/xpath.h>


int
sr_ly_module_is_internal(const struct lys_module *ly_mod);

int
sr_module_is_internal(const struct lys_module *ly_mod);


#endif // _sr_copypaste_h
