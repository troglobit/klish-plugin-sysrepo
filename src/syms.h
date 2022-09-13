/*
 * syms.h
 */

#ifndef _syms_h
#define _syms_h

#include <faux/faux.h>
#include <klish/kcontext_base.h>

#include "pline.h"

// Repository to edit with srp commands
#define SRP_REPO_EDIT SR_DS_CANDIDATE

// Defaut parse options
#define SRP_DEFAULT_PARSE_OPTS ( \
	PPARSE_MULTI_KEYS_W_STMT | \
	PPARSE_JUNIPER_SHOW \
	)


C_DECL_BEGIN

// Types
int srp_PLINE_SET(kcontext_t *context);
int srp_PLINE_DEL(kcontext_t *context);
int srp_PLINE_EDIT(kcontext_t *context);
int srp_PLINE_INSERT_FROM(kcontext_t *context);
int srp_PLINE_INSERT_TO(kcontext_t *context);

// Completion/Help/Prompt
int srp_compl(kcontext_t *context);
int srp_help(kcontext_t *context);
int srp_compl_insert_to(kcontext_t *context);
int srp_help_insert_to(kcontext_t *context);
int srp_prompt_edit_path(kcontext_t *context);

// Operations
int srp_set(kcontext_t *context);
int srp_del(kcontext_t *context);
int srp_edit(kcontext_t *context);
int srp_top(kcontext_t *context);
int srp_up(kcontext_t *context);
int srp_insert(kcontext_t *context);
int srp_commit(kcontext_t *context);
int srp_rollback(kcontext_t *context);
int srp_show(kcontext_t *context);
int srp_deactivate(kcontext_t *context);

C_DECL_END


#endif // _syms_h
