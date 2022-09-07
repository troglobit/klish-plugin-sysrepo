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

	// Types
	kplugin_add_syms(plugin, ksym_new_ext("PLINE_SET", srp_PLINE_SET,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("PLINE_DEL", srp_PLINE_DEL,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("PLINE_EDIT", srp_PLINE_EDIT,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("PLINE_INSERT_FROM", srp_PLINE_INSERT_FROM,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("PLINE_INSERT_TO", srp_PLINE_INSERT_TO,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));

	// Completion/Help/Prompt
	kplugin_add_syms(plugin, ksym_new_ext("srp_compl", srp_compl,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("srp_help", srp_help,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("srp_compl_insert_to", srp_compl_insert_to,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("srp_help_insert_to", srp_help_insert_to,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));
	kplugin_add_syms(plugin, ksym_new_ext("srp_prompt_edit_path", srp_prompt_edit_path,
		KSYM_USERDEFINED_PERMANENT, KSYM_UNSYNC));

	// Operations
	kplugin_add_syms(plugin, ksym_new("srp_set", srp_set));
	kplugin_add_syms(plugin, ksym_new("srp_del", srp_del));
	// Note: 'edit', 'top', 'up'  must be sync to set current path
	kplugin_add_syms(plugin, ksym_new_ext("srp_edit", srp_edit,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("srp_top", srp_top,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new_ext("srp_up", srp_up,
		KSYM_USERDEFINED_PERMANENT, KSYM_SYNC));
	kplugin_add_syms(plugin, ksym_new("srp_insert", srp_insert));

	return 0;
}


int kplugin_sysrepo_fini(kcontext_t *context)
{
	context = context; // Happy compiler

	return 0;
}
