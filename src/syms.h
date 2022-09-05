/*
 * syms.h
 */

#ifndef _syms_h
#define _syms_h

#include <faux/faux.h>
#include <klish/kcontext_base.h>

// Repository to edit with srp commands
#define SRP_REPO_EDIT SR_DS_CANDIDATE

C_DECL_BEGIN

// Types
int srp_PLINE_SET(kcontext_t *context);
int srp_PLINE_DEL(kcontext_t *context);
int srp_PLINE_EDIT(kcontext_t *context);

// Completion/Help/Prompt
int srp_compl(kcontext_t *context);
int srp_help(kcontext_t *context);
int srp_prompt_edit_path(kcontext_t *context);

// Operations
int srp_set(kcontext_t *context);
int srp_del(kcontext_t *context);
int srp_edit(kcontext_t *context);
int srp_top(kcontext_t *context);
int srp_up(kcontext_t *context);

C_DECL_END


#endif // _syms_h
