#ifndef _show_h
#define _show_h

#include <sysrepo.h>
#include <sysrepo/xpath.h>


bool_t show_xpath(sr_session_ctx_t *sess, const char *xpath, uint32_t flags);


#endif // _show_h
