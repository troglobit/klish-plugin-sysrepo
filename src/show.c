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

#include "sr_copypaste.h"
#include "pline.h"
#include "show.h"

#define LEVEL_SPACES_NUM 4

#define JUN(flags) (flags & PPARSE_JUNIPER_SHOW)


static void show_container(const struct lyd_node *node, size_t level, uint32_t flags);
static void show_list(const struct lyd_node *node, size_t level, uint32_t flags);
static void show_leaf(const struct lyd_node *node, size_t level, uint32_t flags);
static void show_leaflist(const struct lyd_node *node, size_t level, uint32_t flags);
static void show_node(const struct lyd_node *node, size_t level, uint32_t flags);
static void show_subtree(const struct lyd_node *nodes_list, size_t level, uint32_t flags);


static const char *get_value(const struct lyd_node *node)
{
	const struct lysc_node *schema = NULL;
	const struct lysc_type *type = NULL;
	const struct lyd_value *value = NULL;

	if (!node)
		return NULL;

	schema = node->schema;
	if (!(schema->nodetype & (LYS_LEAF | LYS_LEAFLIST)))
		return NULL;

	if (schema->nodetype & LYS_LEAF)
		type = ((const struct lysc_node_leaf *)schema)->type;
	else
		type = ((const struct lysc_node_leaflist *)schema)->type;

	if (type->basetype != LY_TYPE_IDENT)
		return lyd_get_value(node);

	// Identity
	value = &((const struct lyd_node_term *)node)->value;

	return value->ident->name;
}


static void show_container(const struct lyd_node *node, size_t level, uint32_t flags)
{
	if (!node)
		return;

	printf("%*s%s%s\n", (int)(level * LEVEL_SPACES_NUM), "",
		node->schema->name, JUN(flags) ? " {" : "");
	show_subtree(lyd_child(node), level + 1, flags);
	if (JUN(flags))
		printf("%*s%s\n", (int)(level * LEVEL_SPACES_NUM), "", "}");
}


static void show_list(const struct lyd_node *node, size_t level, uint32_t flags)
{
	size_t keys_num = 0;
	const struct lyd_node *iter = NULL;
	bool_t first_key = BOOL_TRUE;

	if (!node)
		return;

	printf("%*s%s", (int)(level * LEVEL_SPACES_NUM), "", node->schema->name);

	LY_LIST_FOR(lyd_child(node), iter) {
		if (!(iter->schema->nodetype & LYS_LEAF))
			continue;
		if (!(iter->schema->flags & LYS_KEY))
			continue;
		if ((first_key && (flags & PPARSE_FIRST_KEY_W_STMT)) ||
			(!first_key && (flags & PPARSE_MULTI_KEYS_W_STMT)))
			printf(" %s", iter->schema->name);
		printf(" %s", get_value(iter));
		first_key = BOOL_FALSE;
	}
	printf("%s\n", JUN(flags) ? " {" : "");
	show_subtree(lyd_child(node), level + 1, flags);
	if (JUN(flags))
		printf("%*s%s\n", (int)(level * LEVEL_SPACES_NUM), "", "}");
}


static void show_leaf(const struct lyd_node *node, size_t level, uint32_t flags)
{
	struct lysc_node_leaf *leaf = (struct lysc_node_leaf *)node;

	if (!node)
		return;
	if (node->schema->flags & LYS_KEY)
		return;

	printf("%*s%s", (int)(level * LEVEL_SPACES_NUM), "", node->schema->name);

	leaf = (struct lysc_node_leaf *)node->schema;
	if (leaf->type->basetype != LY_TYPE_EMPTY)
		printf(" %s", get_value(node));

	printf("%s\n", JUN(flags) ? ";" : "");
}


static void show_leaflist(const struct lyd_node *node, size_t level, uint32_t flags)
{
	if (!node)
		return;

	printf("%*s%s %s%s\n", (int)(level * LEVEL_SPACES_NUM), "", node->schema->name,
		get_value(node), JUN(flags) ? ";" : "");
}


static void show_node(const struct lyd_node *node, size_t level, uint32_t flags)
{
	const struct lysc_node *schema = NULL;

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

	// Container
	if (schema->nodetype & LYS_CONTAINER) {
		show_container((const struct lyd_node *)node, level, flags);

	// List
	} else if (schema->nodetype & LYS_LIST) {
		show_list((const struct lyd_node *)node, level, flags);

	// Leaf
	} else if (schema->nodetype & LYS_LEAF) {
		show_leaf((const struct lyd_node *)node, level, flags);

	// Leaf-list
	} else if (schema->nodetype & LYS_LEAFLIST) {
		show_leaflist((const struct lyd_node *)node, level, flags);

	} else {
		return;
	}
}


static void show_sorted_list(faux_list_t *list, size_t level, uint32_t flags)
{
	faux_list_node_t *iter = NULL;
	const struct lyd_node *lyd = NULL;

	if (!list)
		return;

	iter = faux_list_head(list);
	while ((lyd = (const struct lyd_node *)faux_list_each(&iter)))
		show_node(lyd, level, flags);
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
		faux_str_cat(&keys, get_value(iter));
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

	return faux_str_numcmp(get_value(f), get_value(s));
}


static void show_subtree(const struct lyd_node *nodes_list, size_t level, uint32_t flags)
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
			show_sorted_list(list, level, flags);
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

		show_node(iter, level, flags);
	}

	if (list) {
		show_sorted_list(list, level, flags);
		faux_list_free(list);
	}
}


bool_t show_xpath(sr_session_ctx_t *sess, const char *xpath, uint32_t flags)
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

	show_subtree(nodes_list, 0, flags);
	sr_release_data(data);

	return BOOL_TRUE;
}
