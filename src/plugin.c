/*
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <faux/faux.h>
#include <klish/kplugin.h>
#include <klish/kcontext.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#include "syms.h"


const uint8_t kplugin_sysrepo_major = KPLUGIN_MAJOR;
const uint8_t kplugin_sysrepo_minor = KPLUGIN_MINOR;


int kplugin_sysrepo_init(kcontext_t *context)
{
	kplugin_t *plugin = NULL;
	ksym_t *sym = NULL;
	int err = SR_ERR_OK;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;

	assert(context);
	plugin = kcontext_plugin(context);
	assert(plugin);

	err = sr_connect(SR_CONN_DEFAULT, &conn);
	if (err) {
		fprintf(stderr, "Can't connect to Sysrepo\n");
		return -1;;
	}
	err = sr_session_start(conn, SR_DS_RUNNING, &sess);
	if (err) {
		fprintf(stderr, "Can't start Sysrepo session\n");
		sr_disconnect(conn);
		return -1;
	}

	kplugin_set_udata(plugin, sess);

	// Symbols
	kplugin_add_syms(plugin, ksym_new("srp_compl", srp_compl));

	return 0;
}


int kplugin_sysrepo_fini(kcontext_t *context)
{
	kplugin_t *plugin = NULL;
	sr_session_ctx_t *sess = NULL;

	if (!context)
		return -1;

	plugin = kcontext_plugin(context);
	sess = (sr_session_ctx_t *)kplugin_udata(plugin);

	sr_disconnect(sr_session_get_connection(sess));

	return 0;
}
