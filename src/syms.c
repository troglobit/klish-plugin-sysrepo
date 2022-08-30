#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#include <faux/faux.h>
#include <faux/argv.h>
#include <faux/str.h>
#include <faux/list.h>
#include <faux/error.h>
#include <klish/khelper.h>
#include <klish/kplugin.h>
#include <klish/kentry.h>
#include <klish/kscheme.h>
#include <klish/kcontext.h>
#include <klish/kpargv.h>

#include "pline.h"


int srp_compl(kcontext_t *context)
{
	const kpargv_t *pargv = NULL;
	const kentry_t *candidate = NULL;
	faux_list_node_t *iter = NULL;
	faux_list_t *pargs = NULL;
	faux_argv_t *args = faux_argv_new();
	pline_t *pline = NULL;

	pargv = kcontext_parent_pargv(context);
	pargs = kpargv_pargs(pargv);
	candidate = kparg_entry(kpargv_candidate_parg(pargv));
	sr_session_ctx_t *sess = NULL;

	iter = faux_list_tail(pargs);
	do {
		faux_list_node_t *prev = faux_list_prev_node(iter);
		if (prev) {
			kparg_t *parg = (kparg_t *)faux_list_data(prev);
			if (kparg_entry(parg) != candidate)
				break;
		} else {
			break;
		}
		iter = prev;
	} while (iter);

	while (iter) {
		kparg_t *parg = (kparg_t *)faux_list_data(iter);
		faux_argv_add(args, kparg_value(parg));
		iter = faux_list_next_node(iter);
	}

	faux_argv_set_continuable(args, kpargv_continuable(pargv));


	sess = (sr_session_ctx_t *)kplugin_udata(kcontext_plugin(context));

	faux_argv_del_continuable(args);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);
	pline_print_completions(pline, BOOL_TRUE);
	pline_free(pline);

	faux_argv_free(args);

	return 0;
}
