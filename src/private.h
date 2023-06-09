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
	pline_opts_t opts; // Settings
} srp_udata_t;


// Repository to edit with srp commands
#define SRP_REPO_EDIT SR_DS_CANDIDATE


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
int srp_compl_xpath_running(kcontext_t *context);
int srp_compl_xpath_candidate(kcontext_t *context);

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
int srp_diff(kcontext_t *context);
int srp_deactivate(kcontext_t *context);

// Logging
int srp_set_log_func(void);

// Plugin's user-data service functions
pline_opts_t *srp_udata_opts(kcontext_t *context);
faux_argv_t *srp_udata_path(kcontext_t *context);
void srp_udata_set_path(kcontext_t *context, faux_argv_t *path);

// Private
enum diff_op {
    DIFF_OP_CREATE,
    DIFF_OP_DELETE,
    DIFF_OP_REPLACE,
    DIFF_OP_NONE,
};

bool_t show_xpath(sr_session_ctx_t *sess, const char *xpath, pline_opts_t *opts);
void show_subtree(const struct lyd_node *nodes_list, size_t level,
	enum diff_op op, pline_opts_t *opts);

// Sysrepo copy-paste
int sr_module_is_internal(const struct lys_module *ly_mod);

C_DECL_END


#endif // _plugin_sysrepo_private_h
