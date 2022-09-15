/*
 * private.h
 */

#ifndef _pligin_sysrepo_private_h
#define _plugin_sysrepo_private_h

#include <faux/faux.h>
#include <faux/argv.h>
#include <klish/kcontext_base.h>

#include "pline.h"


// Plugin's user-data structure
typedef struct {
	faux_argv_t *path; // Current data hierarchy path ('edit' operation)
	uint32_t flags; // Parse/Show flags
} srp_udata_t;


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
int srp_verify(kcontext_t *context);
int srp_commit(kcontext_t *context);
int srp_rollback(kcontext_t *context);
int srp_show(kcontext_t *context);
int srp_show_running(kcontext_t *context);
int srp_deactivate(kcontext_t *context);

// Sysrepo copy-paste
int sr_ly_module_is_internal(const struct lys_module *ly_mod);
int sr_module_is_internal(const struct lys_module *ly_mod);

// Plugin's user-data service functions
uint32_t srp_udata_flags(kcontext_t *context);
faux_argv_t *srp_udata_path(kcontext_t *context);
void srp_udata_set_path(kcontext_t *context, faux_argv_t *path);

// Private
bool_t show_xpath(sr_session_ctx_t *sess, const char *xpath, uint32_t flags);

C_DECL_END


#endif // _plugin_sysrepo_private_h
