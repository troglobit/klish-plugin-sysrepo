/*
 * syms.h
 */

#ifndef _syms_h
#define _syms_h

#include <faux/faux.h>
#include <klish/kcontext_base.h>


C_DECL_BEGIN

int srp_PLINE_SET(kcontext_t *context);
int srp_PLINE_DEL(kcontext_t *context);
int srp_PLINE_EDIT(kcontext_t *context);

int srp_compl(kcontext_t *context);
int srp_help(kcontext_t *context);

int srp_set(kcontext_t *context);

C_DECL_END


#endif // _syms_h
