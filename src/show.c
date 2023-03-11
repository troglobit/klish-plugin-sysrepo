#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>
#include <faux/argv.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>
#include <sysrepo/values.h>
#include <libyang/tree_edit.h>

#include "pline.h"
#include "private.h"


static void show_container(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts);
static void show_list(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts);
static void show_leaf(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts);
static void show_leaflist(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts);
static void show_node(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts);
static enum diff_op str2diff_op(const char *str);


static char *get_value(const struct lyd_node *node)
{
	const struct lysc_node *schema = NULL;
	const struct lysc_type *type = NULL;
	const char *origin_value = NULL;
	char *space = NULL;
	char *escaped = NULL;
	char *result = NULL;

	if (!node)
		return NULL;

	schema = node->schema;
	if (!(schema->nodetype & (LYS_LEAF | LYS_LEAFLIST)))
		return NULL;

	if (schema->nodetype & LYS_LEAF)
		type = ((const struct lysc_node_leaf *)schema)->type;
	else
		type = ((const struct lysc_node_leaflist *)schema)->type;

	if (type->basetype != LY_TYPE_IDENT) {
		origin_value = lyd_get_value(node);
	} else {
		// Identity
		const struct lyd_value *value = NULL;
		value = &((const struct lyd_node_term *)node)->value;
		origin_value = value->ident->name;
	}

	escaped = faux_str_c_esc(origin_value);
	// String with space must have quotes
	space = strchr(origin_value, ' ');
	if (space) {
		result = faux_str_sprintf("\"%s\"", escaped);
		faux_str_free(escaped);
	} else {
		result = escaped;
	}

	return result;
}


static const char *diff_prefix(enum diff_op op, pline_opts_t *opts)
{
	bool_t c = opts->colorize;

	if (DIFF_OP_CREATE == op)
		return c ? "\x1b[32m+" : "+";
	else if (DIFF_OP_DELETE == op)
		return c ? "\x1b[31m-" : "-";
	else if (DIFF_OP_REPLACE == op)
		return c ? "\x1b[33m=" : "=";

	return "";
}


static const char *diff_suffix(enum diff_op op, pline_opts_t *opts)
{
	if (opts->colorize && (DIFF_OP_NONE != op))
		return "\x1b[0m";

	return "";
}


static void show_container(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	char begin_bracket[3] = {' ', opts->begin_bracket, '\0'};

	if (!node)
		return;

	printf("%s%*s%s%s%s\n",
		diff_prefix(op, opts),
		(int)(level * opts->indent), "",
		node->schema->name,
		opts->show_brackets ? begin_bracket : "",
		diff_suffix(op, opts));
	show_subtree(lyd_child(node), level + 1, op, opts);
	if (opts->show_brackets) {
		printf("%s%*s%c%s\n",
			diff_prefix(op, opts),
			(int)(level * opts->indent), "",
			opts->end_bracket,
			diff_suffix(op, opts));
	}
}


static void show_list(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	char begin_bracket[3] = {' ', opts->begin_bracket, '\0'};
	size_t keys_num = 0;
	const struct lyd_node *iter = NULL;
	bool_t first_key = BOOL_TRUE;

	if (!node)
		return;

	printf("%s%*s%s",
		diff_prefix(op, opts),
		(int)(level * opts->indent), "",
		node->schema->name);

	LY_LIST_FOR(lyd_child(node), iter) {
		char *value = NULL;

		if (!(iter->schema->nodetype & LYS_LEAF))
			continue;
		if (!(iter->schema->flags & LYS_KEY))
			continue;
		if ((first_key && opts->first_key_w_stmt) ||
			(!first_key && opts->multi_keys_w_stmt))
			printf(" %s", iter->schema->name);
		value = get_value(iter);
		printf(" %s", value);
		faux_str_free(value);
		first_key = BOOL_FALSE;
	}
	printf("%s%s\n",
		opts->show_brackets ? begin_bracket : "",
		diff_suffix(op, opts));
	show_subtree(lyd_child(node), level + 1, op, opts);
	if (opts->show_brackets) {
		printf("%s%*s%c%s\n",
			diff_prefix(op, opts),
			(int)(level * opts->indent), "",
			opts->end_bracket,
			diff_suffix(op, opts));
	}
}


static void show_leaf(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	struct lysc_node_leaf *leaf = (struct lysc_node_leaf *)node;

	if (!node)
		return;
	if (node->schema->flags & LYS_KEY)
		return;

	printf("%s%*s%s",
		diff_prefix(op, opts),
		(int)(level * opts->indent), "",
		node->schema->name);

	leaf = (struct lysc_node_leaf *)node->schema;
	if (leaf->type->basetype != LY_TYPE_EMPTY) {
		char *value = get_value(node);
		printf(" %s", value);
		faux_str_free(value);
	}

	printf("%s%s\n",
		opts->show_semicolons ? ";" : "",
		diff_suffix(op, opts));
}


static void show_leaflist(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	char *value = NULL;

	if (!node)
		return;

	value = get_value(node);
	printf("%s%*s%s %s%s%s\n",
		diff_prefix(op, opts),
		(int)(level * opts->indent), "",
		node->schema->name,
		value,
		opts->show_semicolons ? ";" : "",
		diff_suffix(op, opts));
	faux_str_free(value);
}


static void show_node(const struct lyd_node *node, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	const struct lysc_node *schema = NULL;
	struct lyd_meta *meta = NULL;
	enum diff_op cur_op = op;

	if (!node)
		return;

	if (node->flags & LYD_DEFAULT)
		return;
	schema = node->schema;
	if (!schema)
		return;
	if (!(schema->nodetype & SRP_NODETYPE_CONF))
		return;
	if (!(schema->flags & LYS_CONFIG_W))
		return;

	meta = lyd_find_meta(node->meta, NULL, "yang:operation");
	if (meta)
		cur_op = str2diff_op(lyd_get_meta_value(meta));

	// Container
	if (schema->nodetype & LYS_CONTAINER) {
		show_container(node, level, cur_op, opts);

	// List
	} else if (schema->nodetype & LYS_LIST) {
		show_list(node, level, cur_op, opts);

	// Leaf
	} else if (schema->nodetype & LYS_LEAF) {
		show_leaf(node, level, cur_op, opts);

	// Leaf-list
	} else if (schema->nodetype & LYS_LEAFLIST) {
		show_leaflist(node, level, cur_op, opts);

	} else {
		return;
	}
}


static void show_sorted_list(faux_list_t *list, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	faux_list_node_t *iter = NULL;
	const struct lyd_node *lyd = NULL;

	if (!list)
		return;

	iter = faux_list_head(list);
	while ((lyd = (const struct lyd_node *)faux_list_each(&iter)))
		show_node(lyd, level, op, opts);
}


static char *list_keys_str(const struct lyd_node *node)
{
	char *keys = NULL;
	const struct lyd_node *iter = NULL;

	if (!node)
		return NULL;
	if (node->schema->nodetype != LYS_LIST)
		return NULL;

	LY_LIST_FOR(lyd_child(node), iter) {
		if (!(iter->schema->nodetype & LYS_LEAF))
			continue;
		if (!(iter->schema->flags & LYS_KEY))
			continue;
		if (keys)
			faux_str_cat(&keys, " ");
		faux_str_cat(&keys, lyd_get_value(iter));
	}

	return keys;
}


static int list_compare(const void *first, const void *second)
{
	int rc = 0;
	const struct lyd_node *f = (const struct lyd_node *)first;
	const struct lyd_node *s = (const struct lyd_node *)second;
	char *f_keys = list_keys_str(f);
	char *s_keys = list_keys_str(s);

	rc = faux_str_numcmp(f_keys, s_keys);
	faux_str_free(f_keys);
	faux_str_free(s_keys);

	return rc;
}


static int leaflist_compare(const void *first, const void *second)
{
	const struct lyd_node *f = (const struct lyd_node *)first;
	const struct lyd_node *s = (const struct lyd_node *)second;

	return faux_str_numcmp(lyd_get_value(f), lyd_get_value(s));
}


void show_subtree(const struct lyd_node *nodes_list, size_t level,
	enum diff_op op, pline_opts_t *opts)
{
	const struct lyd_node *iter = NULL;
	faux_list_t *list = NULL;
	const struct lysc_node *saved_lysc = NULL;

	if(!nodes_list)
		return;

	LY_LIST_FOR(nodes_list, iter) {

		if (saved_lysc) {
			if (saved_lysc == iter->schema) {
				faux_list_add(list, (void *)iter);
				continue;
			}
			show_sorted_list(list, level, op, opts);
			faux_list_free(list);
			list = NULL;
			saved_lysc = NULL;
		}

		if (((LYS_LIST == iter->schema->nodetype) ||
			(LYS_LEAFLIST == iter->schema->nodetype)) &&
			(iter->schema->flags & LYS_ORDBY_SYSTEM)) {
			saved_lysc = iter->schema;
			if (LYS_LIST == iter->schema->nodetype) {
				list = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
					list_compare, NULL, NULL);
			} else { // LEAFLIST
				list = faux_list_new(FAUX_LIST_SORTED, FAUX_LIST_UNIQUE,
					leaflist_compare, NULL, NULL);
			}
			faux_list_add(list, (void *)iter);
			continue;
		}

		show_node(iter, level, op, opts);
	}

	if (list) {
		show_sorted_list(list, level, op, opts);
		faux_list_free(list);
	}
}


bool_t show_xpath(sr_session_ctx_t *sess, const char *xpath, pline_opts_t *opts)
{
	sr_data_t *data = NULL;
	struct lyd_node *nodes_list = NULL;

	assert(sess);

	if (xpath) {
		if (sr_get_subtree(sess, xpath, 0, &data) != SR_ERR_OK)
			return BOOL_FALSE;
		nodes_list = lyd_child(data->tree);
	} else {
		if (sr_get_data(sess, "/*", 0, 0, 0, &data) != SR_ERR_OK)
			return BOOL_FALSE;
		nodes_list = data->tree;
	}

	show_subtree(nodes_list, 0, DIFF_OP_NONE, opts);
	sr_release_data(data);

	return BOOL_TRUE;
}


static enum diff_op str2diff_op(const char *str)
{
	if (!strcmp(str, "create"))
		return DIFF_OP_CREATE;
	else if (!strcmp(str, "delete"))
		return DIFF_OP_DELETE;
	else if (!strcmp(str, "replace"))
		return DIFF_OP_REPLACE;

	return DIFF_OP_NONE;
}
