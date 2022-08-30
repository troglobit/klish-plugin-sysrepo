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

#include "private.h"


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

/*
	// Misc
	kplugin_add_syms(plugin, ksym_new_ext("nop", klish_nop,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new("tsym", klish_tsym));
	kplugin_add_syms(plugin, ksym_new("print", klish_print));
	kplugin_add_syms(plugin, ksym_new_ext("pwd", klish_pwd,
		KSYM_PERMANENT, KSYM_SYNC));

	// Navigation
	// Navigation must be permanent (no dry-run) and sync. Because unsync
	// actions will be fork()-ed so it can't change current path.
	kplugin_add_syms(plugin, ksym_new_ext("nav", klish_nav,
		KSYM_PERMANENT, KSYM_SYNC));

	// PTYPEs
	// These PTYPEs are simple and fast so set SYNC flag
	kplugin_add_syms(plugin, ksym_new_ext("COMMAND", klish_ptype_COMMAND,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("completion_COMMAND", klish_completion_COMMAND,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("COMMAND_CASE", klish_ptype_COMMAND_CASE,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("INT", klish_ptype_INT,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("UINT", klish_ptype_UINT,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("STRING", klish_ptype_STRING,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
*/
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
