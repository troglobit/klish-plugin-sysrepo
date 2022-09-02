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


static int srp_check_type(kcontext_t *context,
	pt_e not_accepted_nodes,
	size_t max_expr_num)
{
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	const char *entry_name = NULL;
	const char *value = NULL;
	pexpr_t *expr = NULL;
	size_t expr_num = 0;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SR_DS_RUNNING, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	entry_name = kentry_name(kcontext_candidate_entry(context));
	value = kcontext_candidate_value(context);
	args = param2argv(kcontext_parent_pargv(context), entry_name);
	if (value)
		faux_argv_add(args, value);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);

	if (pline->invalid)
		return -1;
	expr_num = faux_list_len(pline->exprs);
	if (expr_num < 1)
		return -1;
	if ((max_expr_num > 0) &&  // '0' means unlimited
		(expr_num > max_expr_num))
		return -1;
	expr = pline_current_expr(pline);
	if (expr->pat & not_accepted_nodes)
		return -1;

	pline_free(pline);
	sr_disconnect(conn);

	return 0;
}


int srp_PLINE_SET(kcontext_t *context)
{
	return srp_check_type(context, PT_NOT_SET, 0);
}


int srp_PLINE_DEL(kcontext_t *context)
{
	return srp_check_type(context, PT_NOT_DEL, 1);
}


int srp_PLINE_EDIT(kcontext_t *context)
{
	return srp_check_type(context, PT_NOT_EDIT, 1);
}


int srp_set(kcontext_t *context)
{
	int ret = 0;
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

	if (pline->invalid) {
		fprintf(stderr, "Invalid set request\n");
		ret = -1;
		goto cleanup;
	}

	iter = faux_list_head(pline->exprs);
	while ((expr = (pexpr_t *)faux_list_each(&iter))) {
		if (!(expr->pat & PT_SET)) {
			err_num++;
			fprintf(stderr, "Illegal expression for set operation\n");
			break;
		}
		if (sr_set_item_str(sess, expr->xpath, expr->value, NULL, 0) !=
			SR_ERR_OK) {
			err_num++;
			fprintf(stderr, "Can't set data\n");
			break;
		}
	}

	if (sr_has_changes(sess)) {
		if (err_num > 0)
			sr_discard_changes(sess);
		else
			sr_apply_changes(sess, 0);
	}

	if (err_num > 0)
		ret = -1;

cleanup:
	pline_free(pline);
	sr_disconnect(conn);

	return ret;
}


int srp_del(kcontext_t *context)
{
	int ret = 0;
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
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

	if (pline->invalid) {
		fprintf(stderr, "Invalid 'del' request\n");
		ret = -1;
		goto cleanup;
	}

	if (faux_list_len(pline->exprs) > 1) {
		fprintf(stderr, "Can't delete more than one object\n");
		ret = -1;
		goto cleanup;
	}

	expr = (pexpr_t *)faux_list_data(faux_list_head(pline->exprs));

	if (!(expr->pat & PT_DEL)) {
		fprintf(stderr, "Illegal expression for 'del' operation\n");
		ret = -1;
		goto cleanup;
	}

	if (sr_delete_item(sess, expr->xpath, 0) != SR_ERR_OK) {
		fprintf(stderr, "Can't delete data\n");
		ret = -1;
		goto cleanup;
	}

	sr_apply_changes(sess, 0);

cleanup:
	pline_free(pline);
	sr_disconnect(conn);

	return ret;
}
