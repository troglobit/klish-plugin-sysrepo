#include <stdlib.h>
#include <stdio.h>

//#include <libyang/libyang.h>
//#include <libyang/tree_schema.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>


static void process_node(const struct lysc_node *node, size_t level);
static void iterate_nodes(const struct lysc_node *node, size_t level);


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

static void identityref(struct lysc_ident *ident)
{
	LY_ARRAY_COUNT_TYPE u = 0;

	if (!ident)
		return;

	if (!ident->derived) {
		printf(" %s", ident->name);
		return;
	}

	LY_ARRAY_FOR(ident->derived, u) {
		identityref(ident->derived[u]);
	}

}

static void process_node(const struct lysc_node *node, size_t level)
{
	if (!node)
		return;

	printf("%*c %s [%s]",
		(int)(level * 2), ' ',
		node->name,
		lys_nodetype2str(node->nodetype));

	if (node->nodetype & LYS_LIST) {
		const struct lysc_node *iter = NULL;
		LY_LIST_FOR(lysc_node_child(node), iter) {
			if (lysc_is_key(iter))
				printf(" %s", iter->name);
		}

	} else if (node->nodetype & LYS_LEAF) {
		const struct lysc_node_leaf *leaf =
			(const struct lysc_node_leaf *)node;
		const struct lysc_type *type = leaf->type;
		if (type->basetype == LY_TYPE_IDENT) {
			struct lysc_type_identityref *iref =
				(struct lysc_type_identityref *)type;
			LY_ARRAY_COUNT_TYPE u = 0;
			LY_ARRAY_FOR(iref->bases, u) {
				identityref(iref->bases[u]);
			}
		}
	}


	printf("\n");
	iterate_nodes(lysc_node_child(node), level + 1);
}


static void iterate_nodes(const struct lysc_node *node, size_t level)
{
	const struct lysc_node *iter = NULL;

	if (!node)
		return;

	LY_LIST_FOR(node, iter) {
		if (!(iter->nodetype & (
			LYS_CONTAINER |
			LYS_LIST |
			LYS_LEAF |
			LYS_LEAFLIST
			)))
			continue;
		if (!(iter->flags & LYS_CONFIG_W))
			continue;
		process_node(iter, level);
	}
}


int main(void)
{
	int ret = -1;
	int err = SR_ERR_OK;
	sr_conn_ctx_t *conn = NULL;
	sr_session_ctx_t *sess = NULL;
	const struct ly_ctx *lyctx = NULL;
	uint32_t i = 0;
	struct lys_module *module = NULL;

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
	lyctx = sr_acquire_context(conn);
	if (!lyctx) {
		printf("Cannot acquire context\n");
		goto out;
	}

	// Iterate all modules
	i = 0;
	while ((module = ly_ctx_get_module_iter(lyctx, &i))) {
		if (sr_module_is_internal(module))
			continue;
		if (!module->compiled)
			continue;
		if (!module->implemented)
			continue;
		if (!module->compiled->data)
			continue;
		printf("%s\n", module->name);
		iterate_nodes(module->compiled->data, 1);
	}


//	printf("Ok\n");
	ret = 0;
out:
	sr_release_context(conn);
	sr_disconnect(conn);

	return 0;
}
