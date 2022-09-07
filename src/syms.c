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
#include "syms.h"


static faux_argv_t *param2argv(const faux_argv_t *cur_path,
	const kpargv_t *pargv, const char *entry_name)
{
	faux_list_node_t *iter = NULL;
	faux_list_t *pargs = NULL;
	faux_argv_t *args = NULL;
	kparg_t *parg = NULL;

	assert(pargv);
	if (!pargv)
		return NULL;

	pargs = kpargv_find_multi(pargv, entry_name);
	if (cur_path)
		args = faux_argv_dup(cur_path);
	else
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
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	entry_name = kentry_name(kcontext_candidate_entry(context));
	args = param2argv(cur_path, kcontext_parent_pargv(context), entry_name);
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


int srp_prompt_edit_path(kcontext_t *context)
{
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;
	char *path = NULL;

	assert(context);

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	if (cur_path)
		path = faux_argv_line(cur_path);
	printf("[edit%s%s]\n", path ? " " : "", path ? path : "");
	faux_str_free(path);

	return 0;
}


static int srp_check_type(kcontext_t *context,
	pt_e not_accepted_nodes,
	size_t max_expr_num)
{
	int ret = -1;
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	const char *entry_name = NULL;
	const char *value = NULL;
	pexpr_t *expr = NULL;
	size_t expr_num = 0;
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	entry_name = kentry_name(kcontext_candidate_entry(context));
	value = kcontext_candidate_value(context);
	args = param2argv(cur_path, kcontext_parent_pargv(context), entry_name);
	if (value)
		faux_argv_add(args, value);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);

	if (pline->invalid)
		goto err;
	expr_num = faux_list_len(pline->exprs);
	if (expr_num < 1)
		goto err;
	if ((max_expr_num > 0) &&  // '0' means unlimited
		(expr_num > max_expr_num))
		goto err;
	expr = pline_current_expr(pline);
	if (expr->pat & not_accepted_nodes)
		goto err;

	ret = 0;
err:
	pline_free(pline);
	sr_disconnect(conn);

	return ret;
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


int srp_PLINE_INSERT_FROM(kcontext_t *context)
{
	return srp_check_type(context, PT_NOT_INSERT, 1);
}


static faux_argv_t *assemble_insert_to(sr_session_ctx_t *sess, const kpargv_t *pargv,
	faux_argv_t *cur_path, const char *candidate_value)
{
	faux_argv_t *args = NULL;
	faux_argv_t *insert_to = NULL;
	pline_t *pline = NULL;
	pexpr_t *expr = NULL;
	size_t i = 0;

	assert(sess);

	args = param2argv(cur_path, pargv, "from_path");
	pline = pline_parse(sess, args, 0);
	expr = pline_current_expr(pline);
	for (i = 0; i < (expr->args_num - expr->list_pos); i++) {
		faux_argv_node_t *iter = faux_argv_iterr(args);
		faux_argv_del(args, iter);
	}
	insert_to = param2argv(args, pargv, "to_path");
	faux_argv_free(args);
	if (candidate_value)
		faux_argv_add(insert_to, candidate_value);

	pline_free(pline);

	return insert_to;
}


int srp_PLINE_INSERT_TO(kcontext_t *context)
{
	int ret = -1;
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	const char *value = NULL;
	pexpr_t *expr = NULL;
	size_t expr_num = 0;
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	value = kcontext_candidate_value(context);
	args = assemble_insert_to(sess, kcontext_parent_pargv(context),
		cur_path, value);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);

	if (pline->invalid)
		goto err;
	expr_num = faux_list_len(pline->exprs);
	if (expr_num != 1)
		goto err;
	expr = pline_current_expr(pline);
	if (expr->pat & PT_NOT_INSERT)
		goto err;

	ret = 0;
err:
	pline_free(pline);
	sr_disconnect(conn);

	return ret;
}


static int srp_compl_or_help_insert_to(kcontext_t *context, bool_t help)
{
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	args = assemble_insert_to(sess, kcontext_parent_pargv(context),
		cur_path, NULL);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);
	pline_print_completions(pline, help);
	pline_free(pline);

	sr_disconnect(conn);

	return 0;
}


int srp_compl_insert_to(kcontext_t *context)
{
	return srp_compl_or_help_insert_to(context, BOOL_FALSE);
}


int srp_help_insert_to(kcontext_t *context)
{
	return srp_compl_or_help_insert_to(context, BOOL_TRUE);
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
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	args = param2argv(cur_path, kcontext_pargv(context), "path");
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
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	args = param2argv(cur_path, kcontext_pargv(context), "path");
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


int srp_edit(kcontext_t *context)
{
	int ret = 0;
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	pexpr_t *expr = NULL;
	size_t err_num = 0;
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	args = param2argv(cur_path, kcontext_pargv(context), "path");
	pline = pline_parse(sess, args, 0);

	if (pline->invalid) {
		fprintf(stderr, "Invalid 'edit' request\n");
		ret = -1;
		goto cleanup;
	}

	if (faux_list_len(pline->exprs) > 1) {
		fprintf(stderr, "Can't process more than one object\n");
		ret = -1;
		goto cleanup;
	}

	expr = (pexpr_t *)faux_list_data(faux_list_head(pline->exprs));

	if (!(expr->pat & PT_EDIT)) {
		fprintf(stderr, "Illegal expression for 'edit' operation\n");
		ret = -1;
		goto cleanup;
	}

	if (sr_set_item_str(sess, expr->xpath, NULL, NULL, 0) != SR_ERR_OK) {
		fprintf(stderr, "Can't set editing data\n");
		ret = -1;
		goto cleanup;
	}
	sr_apply_changes(sess, 0);

	// Set new current path
	faux_argv_free(cur_path);
	kplugin_set_udata(plugin, args);

cleanup:
	if (ret < 0)
		faux_argv_free(args);
	pline_free(pline);
	sr_disconnect(conn);

	return ret;
}


int srp_top(kcontext_t *context)
{
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;

	assert(context);

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	faux_argv_free(cur_path);
	kplugin_set_udata(plugin, NULL);

	return 0;
}


int srp_up(kcontext_t *context)
{
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;
	faux_argv_node_t *iter = NULL;

	assert(context);

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	if (!cur_path)
		return -1; // It's top level and can't level up

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	// Remove last arguments one by one and wait for legal edit-like pline
	while (faux_argv_len(cur_path) > 0) {
		pline_t *pline = NULL;
		pexpr_t *expr = NULL;
		size_t len = 0;

		iter = faux_argv_iterr(cur_path);
		faux_argv_del(cur_path, iter);
		pline = pline_parse(sess, cur_path, 0);
		if (pline->invalid) {
			pline_free(pline);
			continue;
		}
		len = faux_list_len(pline->exprs);
		if (len != 1) {
			pline_free(pline);
			continue;
		}
		expr = (pexpr_t *)faux_list_data(faux_list_head(pline->exprs));
		if (!(expr->pat & PT_EDIT)) {
			pline_free(pline);
			continue;
		}
		// Here new path is ok
		pline_free(pline);
		break;
	}

	// Don't store empty path
	while (faux_argv_len(cur_path) == 0) {
		faux_argv_free(cur_path);
		kplugin_set_udata(plugin, NULL);
	}

	sr_disconnect(conn);

	return 0;
}


int srp_insert(kcontext_t *context)
{
	int ret = -1;
	pline_t *pline = NULL;
	pline_t *pline_to = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	faux_list_node_t *iter = NULL;
	pexpr_t *expr = NULL;
	pexpr_t *expr_to = NULL;
	faux_argv_t *cur_path = NULL;
	kplugin_t *plugin = NULL;
	faux_argv_t *insert_from = NULL;
	faux_argv_t *insert_to = NULL;
	sr_move_position_t position = SR_MOVE_LAST;
	kpargv_t *pargv = NULL;
	const char *list_keys = NULL;
	const char *leaflist_value = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SRP_REPO_EDIT, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	plugin = kcontext_plugin(context);
	cur_path = (faux_argv_t *)kplugin_udata(plugin);
	pargv = kcontext_pargv(context);

	// 'from' argument
	insert_from = param2argv(cur_path, pargv, "from_path");
	pline = pline_parse(sess, insert_from, 0);
	faux_argv_free(insert_from);

	if (pline->invalid) {
		fprintf(stderr, "Invalid 'from' expression\n");
		goto err;
	}

	if (faux_list_len(pline->exprs) > 1) {
		fprintf(stderr, "Can't process more than one object\n");
		goto err;
	}

	expr = (pexpr_t *)faux_list_data(faux_list_head(pline->exprs));

	if (!(expr->pat & PT_INSERT)) {
		fprintf(stderr, "Illegal 'from' expression for 'insert' operation\n");
		goto err;
	}

	// Position
	if (kpargv_find(pargv, "first"))
		position = SR_MOVE_FIRST;
	else if (kpargv_find(pargv, "last"))
		position = SR_MOVE_LAST;
	else if (kpargv_find(pargv, "before"))
		position = SR_MOVE_BEFORE;
	else if (kpargv_find(pargv, "after"))
		position = SR_MOVE_AFTER;
	else {
		fprintf(stderr, "Illegal 'position' argument\n");
		goto err;
	}

	// 'to' argument
	if ((SR_MOVE_BEFORE == position) || (SR_MOVE_AFTER == position)) {
		insert_to = assemble_insert_to(sess, pargv, cur_path, NULL);
		pline_to = pline_parse(sess, insert_to, 0);
		faux_argv_free(insert_to);

		if (pline_to->invalid) {
			fprintf(stderr, "Invalid 'to' expression\n");
			goto err;
		}

		if (faux_list_len(pline_to->exprs) > 1) {
			fprintf(stderr, "Can't process more than one object\n");
			goto err;
		}

		expr_to = (pexpr_t *)faux_list_data(faux_list_head(pline_to->exprs));

		if (!(expr_to->pat & PT_INSERT)) {
			fprintf(stderr, "Illegal 'to' expression for 'insert' operation\n");
			goto err;
		}

		if (PAT_LIST_KEY == expr_to->pat)
			list_keys = expr_to->last_keys;
		else // PATH_LEAFLIST_VALUE
			leaflist_value = expr_to->last_keys;
	}

	if (sr_move_item(sess, expr->xpath, position,
		list_keys, leaflist_value, NULL, 0) != SR_ERR_OK) {
		fprintf(stderr, "Can't move element\n");
		goto err;
	}
	sr_apply_changes(sess, 0);

	ret = 0;
err:
	pline_free(pline);
	pline_free(pline_to);
	sr_disconnect(conn);

	return ret;
}
