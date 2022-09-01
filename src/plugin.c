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

	// Symbols
	kplugin_add_syms(plugin, ksym_new("srp_compl", srp_compl));
	kplugin_add_syms(plugin, ksym_new("srp_help", srp_help));
	kplugin_add_syms(plugin, ksym_new("srp_set", srp_set));

	return 0;
}


int kplugin_sysrepo_fini(kcontext_t *context)
{
	context = context; // Happy compiler

	return 0;
}
