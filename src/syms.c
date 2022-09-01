#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/argv.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/kentry.h>
#include <klish/kscheme.h>
#include <klish/kcontext.h>
#include <klish/kpargv.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#include "pline.h"


static faux_argv_t *param2argv(const kpargv_t *pargv, const char *entry_name)
{
	faux_list_node_t *iter = NULL;
	faux_list_t *pargs = NULL;
	faux_argv_t *args = NULL;
	kparg_t *parg = NULL;

	assert(pargv);
	if (!pargv)
		return NULL;

	pargs = kpargv_find_multi(pargv, entry_name);
	args = faux_argv_new();

	iter = faux_list_head(pargs);
	while ((parg = (kparg_t *)faux_list_each(&iter))) {
		faux_argv_add(args, kparg_value(parg));
	}
	faux_list_free(pargs);

	return args;
}


// Candidate from pargv contains possible begin of current word (that must be
// completed). kpargv's list don't contain candidate but only already parsed
// words.
static int srp_compl_or_help(kcontext_t *context, bool_t help)
{
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	const char *entry_name = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SR_DS_RUNNING, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	entry_name = kentry_name(kcontext_candidate_entry(context));
	args = param2argv(kcontext_parent_pargv(context), entry_name);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);
	pline_print_completions(pline, help);
	pline_free(pline);

	sr_disconnect(conn);

	return 0;
}


int srp_compl(kcontext_t *context)
{
	return srp_compl_or_help(context, BOOL_FALSE);
}


int srp_help(kcontext_t *context)
{
	return srp_compl_or_help(context, BOOL_TRUE);
}


int srp_set(kcontext_t *context)
{
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	faux_list_node_t *iter = NULL;
	pexpr_t *expr = NULL;
	size_t err_num = 0;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SR_DS_RUNNING, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	args = param2argv(kcontext_pargv(context), "path");
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);

	iter = faux_list_head(pline->exprs);
	while ((expr = (pexpr_t *)faux_list_each(&iter))) {
		if (!expr->active)
			break;
		if (sr_set_item_str(sess, expr->xpath, expr->value, NULL, 0) !=
			SR_ERR_OK) {
			err_num++;
			fprintf(stderr, "Can't set data");
			break;
		}
	}

	if (sr_has_changes(sess)) {
		if (err_num > 0)
			sr_discard_changes(sess);
		else
			sr_apply_changes(sess, 0);
	}

	pline_free(pline);
	sr_disconnect(conn);

	if (err_num > 0)
		return -1;

	return 0;
}
