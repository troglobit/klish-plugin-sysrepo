#include <stdlib.h>
#include <stdio.h>

#include <faux/faux.h>
#include <faux/argv.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#include "pline.h"


int main(int argc, char **argv)
{
	int ret = -1;
	int err = SR_ERR_OK;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	faux_argv_t *args = faux_argv_new();
	pline_t *pline = NULL;

	err = sr_connect(SR_CONN_DEFAULT, &conn);
	if (err) {
		printf("Error\n");
		goto out;
	}
	err = sr_session_start(conn, SR_DS_RUNNING, &sess);
	if (err) {
		printf("Error2\n");
		goto out;
	}

	faux_argv_parse(args, argv[1]);
	faux_argv_del_continuable(args);
	pline = pline_parse(sess, args, 0);
	faux_argv_free(args);
//	pline_debug(pline);
	pline_print_completions(pline, BOOL_TRUE);
	pline_free(pline);

	ret = 0;
out:
	sr_disconnect(conn);

	return ret;
}
