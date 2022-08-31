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


static faux_argv_t *pargv2argv(const kpargv_t *pargv)
{
	const kentry_t *candidate = NULL;
	faux_list_node_t *iter = NULL;
	faux_list_t *pargs = NULL;
	faux_argv_t *args = NULL;

	assert(pargv);
	if (!pargv)
		return NULL;
	pargs = kpargv_pargs(pargv);
	candidate = kparg_entry(kpargv_candidate_parg(pargv));

	iter = faux_list_tail(pargs);
	while (iter) {
		faux_list_node_t *prev = faux_list_prev_node(iter);
		kparg_t *parg = (kparg_t *)faux_list_data(iter);
		if (kparg_entry(parg) != candidate) {
				iter = faux_list_next_node(iter);
				break;
		}
		if (!prev)
			break;
		iter = prev;
	}

	args = faux_argv_new();
	while (iter) {
		kparg_t *parg = (kparg_t *)faux_list_data(iter);
		faux_argv_add(args, kparg_value(parg));
		iter = faux_list_next_node(iter);
	}

	faux_argv_set_continuable(args, kpargv_continuable(pargv));

	return args;
}




// Candidate from pargv contains possible begin of current word (that must be
// completed). kpargv's list don't contain candidate but only already parsed
// words.
int srp_compl(kcontext_t *context)
{
	faux_argv_t *args = NULL;
	pline_t *pline = NULL;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;

	assert(context);

	if (sr_connect(SR_CONN_DEFAULT, &conn))
		return -1;
	if (sr_session_start(conn, SR_DS_RUNNING, &sess)) {
		sr_disconnect(conn);
		return -1;
	}

	args = pargv2argv(kcontext_parent_pargv(context));
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);
	pline_print_completions(pline, BOOL_FALSE);
	pline_free(pline);

	sr_disconnect(conn);

	return 0;
}
