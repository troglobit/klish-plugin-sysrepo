#include <stdlib.h>
#include <stdio.h>

//#include <libyang/libyang.h>
//#include <libyang/tree_schema.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#include <faux/faux.h>
#include <faux/argv.h>

#include "pline.h"


static int
sr_ly_module_is_internal(const struct lys_module *ly_mod)
{
    if (!ly_mod->revision) {
        return 0;
    }

    if (!strcmp(ly_mod->name, "ietf-yang-metadata") && !strcmp(ly_mod->revision, "2016-08-05")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "yang") && !strcmp(ly_mod->revision, "2021-04-07")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-inet-types") && !strcmp(ly_mod->revision, "2013-07-15")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-yang-types") && !strcmp(ly_mod->revision, "2013-07-15")) {
        return 1;
    }

    return 0;
}


int
sr_module_is_internal(const struct lys_module *ly_mod)
{
    if (!ly_mod->revision) {
        return 0;
    }

    if (sr_ly_module_is_internal(ly_mod)) {
        return 1;
    }

    if (!strcmp(ly_mod->name, "ietf-datastores") && !strcmp(ly_mod->revision, "2018-02-14")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-yang-schema-mount")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-yang-library")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-netconf")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-netconf-with-defaults") && !strcmp(ly_mod->revision, "2011-06-01")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-origin") && !strcmp(ly_mod->revision, "2018-02-14")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-netconf-notifications") && !strcmp(ly_mod->revision, "2012-02-06")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "sysrepo")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "sysrepo-monitoring")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "sysrepo-plugind")) {
        return 1;
    } else if (!strcmp(ly_mod->name, "ietf-netconf-acm")) {
        return 1;
    }

    return 0;
}


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

	return 0;
}
