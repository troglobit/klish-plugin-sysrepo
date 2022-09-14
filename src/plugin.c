/*
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/ini.h>
#include <klish/kplugin.h>
#include <klish/kcontext.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#include "private.h"


const uint8_t kplugin_sysrepo_major = KPLUGIN_MAJOR;
const uint8_t kplugin_sysrepo_minor = KPLUGIN_MINOR;

static uint32_t parse_plugin_conf(const char *conf, uint32_t default_flags);


int kplugin_sysrepo_init(kcontext_t *context)
{
	kplugin_t *plugin = NULL;
	ksym_t *sym = NULL;
	int err = SR_ERR_OK;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	srp_udata_t *udata = NULL;

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
	kplugin_add_syms(plugin, ksym_new("srp_commit", srp_commit));
	kplugin_add_syms(plugin, ksym_new("srp_rollback", srp_rollback));
	kplugin_add_syms(plugin, ksym_new("srp_show", srp_show));
	kplugin_add_syms(plugin, ksym_new("srp_show_running", srp_show_running));
	kplugin_add_syms(plugin, ksym_new("srp_deactivate", srp_deactivate));

	// User-data initialization
	udata = faux_zmalloc(sizeof(*udata));
	assert(udata);
	udata->path = NULL;
	udata->flags = parse_plugin_conf(kplugin_conf(plugin), SRP_DEFAULT_PARSE_OPTS);
	kplugin_set_udata(plugin, udata);

	return 0;
}


int kplugin_sysrepo_fini(kcontext_t *context)
{
	srp_udata_t *udata = NULL;

	assert(context);

	// Free plugin's user-data
	udata = (srp_udata_t *)kcontext_udata(context);
	assert(udata);
	if (udata->path)
		faux_argv_free(udata->path);
	faux_free(udata);

	return 0;
}


uint32_t srp_udata_flags(kcontext_t *context)
{
	srp_udata_t *udata = NULL;

	assert(context);

	udata = (srp_udata_t *)kcontext_udata(context);
	assert(udata);

	return udata->flags;
}


faux_argv_t *srp_udata_path(kcontext_t *context)
{
	srp_udata_t *udata = NULL;

	assert(context);

	udata = (srp_udata_t *)kcontext_udata(context);
	assert(udata);

	return udata->path;
}


void srp_udata_set_path(kcontext_t *context, faux_argv_t *path)
{
	srp_udata_t *udata = NULL;

	assert(context);

	udata = (srp_udata_t *)kcontext_udata(context);
	assert(udata);
	if (udata->path)
		faux_argv_free(udata->path);
	udata->path = path;
}


static uint32_t parse_plugin_conf(const char *conf, uint32_t default_flags)
{
	uint32_t flags = default_flags;
	faux_ini_t *ini = NULL;
	const char *val = NULL;

	if (!conf)
		return flags;

	ini = faux_ini_new();
	if (!faux_ini_parse_str(ini, conf)) {
		faux_ini_free(ini);
		return flags;
	}

	if ((val = faux_ini_find(ini, "JuniperLikeShow"))) {
		if (faux_str_cmp(val, "y") == 0)
			flags = flags | PPARSE_JUNIPER_SHOW;
		else if (faux_str_cmp(val, "n") == 0)
			flags = flags & (~(uint32_t)PPARSE_JUNIPER_SHOW);
	}

	if ((val = faux_ini_find(ini, "FirstKeyWithStatement"))) {
		if (faux_str_cmp(val, "y") == 0)
			flags = flags | PPARSE_FIRST_KEY_W_STMT;
		else if (faux_str_cmp(val, "n") == 0)
			flags = flags & (~(uint32_t)PPARSE_FIRST_KEY_W_STMT);
	}

	if ((val = faux_ini_find(ini, "MultiKeysWithStatement"))) {
		if (faux_str_cmp(val, "y") == 0)
			flags = flags | PPARSE_MULTI_KEYS_W_STMT;
		else if (faux_str_cmp(val, "n") == 0)
			flags = flags & (~(uint32_t)PPARSE_MULTI_KEYS_W_STMT);
	}

	faux_ini_free(ini);

	return flags;
}
