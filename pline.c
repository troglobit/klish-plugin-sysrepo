/** @file pline.c
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sysrepo.h>
#include <sysrepo/xpath.h>
#include <libyang/tree_edit.h>

#include <faux/faux.h>
#include <faux/str.h>
#include <faux/list.h>
#include <faux/argv.h>

#include "pline.h"

#define NODETYPE_CONF (LYS_CONTAINER | LYS_LIST | LYS_LEAF | LYS_LEAFLIST)


pexpr_t *pexpr_new(void)
{
	pexpr_t *pexpr = NULL;

	pexpr = faux_zmalloc(sizeof(*pexpr));
	assert(pexpr);
	if (!pexpr)
		return NULL;

	return pexpr;
}


void pexpr_free(pexpr_t *pexpr)
{
	if (!pexpr)
		return;

	faux_str_free(pexpr->xpath);
	faux_str_free(pexpr->value);

	free(pexpr);
}


pcompl_t *pcompl_new(void)
{
	pcompl_t *pcompl = NULL;

	pcompl = faux_zmalloc(sizeof(*pcompl));
	assert(pcompl);
	if (!pcompl)
		return NULL;

	return pcompl;
}


void pcompl_free(pcompl_t *pcompl)
{
	if (!pcompl)
		return;

	faux_str_free(pcompl->xpath);

	free(pcompl);
}


pline_t *pline_new(void)
{
	pline_t *pline = NULL;

	pline = faux_zmalloc(sizeof(*pline));
	assert(pline);
	if (!pline)
		return NULL;

	// Init
	pline->exprs = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (faux_list_free_fn)pexpr_free);
	pline->compls = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (faux_list_free_fn)pcompl_free);

	return pline;
}


void pline_free(pline_t *pline)
{
	LY_ARRAY_COUNT_TYPE u = 0;

	if (!pline)
		return;

	faux_list_free(pline->exprs);
	faux_list_free(pline->compls);

	faux_free(pline);
}


pexpr_t *pline_current_expr(pline_t *pline)
{
	assert(pline);

	if (faux_list_len(pline->exprs) == 0)
		faux_list_add(pline->exprs, pexpr_new());

	return (pexpr_t *)faux_list_data(faux_list_tail(pline->exprs));
}


static int
sr_ly_module_is_internal(const struct lys_module *ly_mod);

int
sr_module_is_internal(const struct lys_module *ly_mod);


// Don't use standard lys_find_child() because it checks given module to be
// equal to found node's module. So augmented nodes will not be found.
static const struct lysc_node *find_child(const struct lysc_node *node,
	const char *name)
{
	const struct lysc_node *iter = NULL;

	if (!node)
		return NULL;

	LY_LIST_FOR(node, iter) {
		if (!(iter->nodetype & NODETYPE_CONF))
			continue;
		if (!(iter->flags & LYS_CONFIG_W))
			continue;
		if (!faux_str_cmp(iter->name, name))
			return iter;
	}

	return NULL;
}






bool_t parse_module(const struct lys_module *module, faux_argv_node_t **arg,
	pline_t *pline)
{
	const struct lysc_node *node = NULL;

	do {
		pexpr_t *pexpr = pline_current_expr(pline);
		const char *str = (const char *)faux_argv_current(*arg);

		if (node) {
			char *tmp = faux_str_sprintf("/%s:%s",
				node->module->name, node->name);
			faux_str_cat(&pexpr->xpath, tmp);
			faux_str_free(tmp);
			printf("%s\n", pexpr->xpath);
		}

printf("for str = %s\n", str);

		if (!str)
			break;

		// Root of the module
		if (!node) {
printf("Module\n");
			node = find_child(module->compiled->data, str);
			if (!node)
				return BOOL_FALSE;
//			continue; // Don't get next arg

		// Container
		} else if (node->nodetype & LYS_CONTAINER) {
printf("Container\n");
			node = find_child(lysc_node_child(node), str);

		// List
		} else if (node->nodetype & LYS_LIST) {
printf("List\n");
			const struct lysc_node *iter = NULL;
			printf("str = %s\n", str);
			LY_LIST_FOR(lysc_node_child(node), iter) {
				char *tmp = NULL;
				struct lysc_node_leaf *leaf =
					(struct lysc_node_leaf *)iter;
				if (!(iter->nodetype & LYS_LEAF))
					continue;
				if (!(iter->flags & LYS_KEY))
					continue;
				assert (leaf->type->basetype != LY_TYPE_EMPTY);
				tmp = faux_str_sprintf("[%s='%s']",
					leaf->name, str);
				faux_str_cat(&pexpr->xpath, tmp);
				faux_str_free(tmp);
				printf("%s\n", pexpr->xpath);
				faux_argv_each(arg);
				str = (const char *)faux_argv_current(*arg);
printf("list str = %s\n", str);
				if (!str)
					break;
			}
			if (!str)
				break;
			node = find_child(lysc_node_child(node), str);
printf("list next node = %s\n", node ? node->name : "NULL");

		// Leaf
		} else if (node->nodetype & LYS_LEAF) {
printf("Leaf\n");
			struct lysc_node_leaf *leaf =
				(struct lysc_node_leaf *)node;
			if (leaf->type->basetype != LY_TYPE_EMPTY) {
				pexpr->value = faux_str_dup(str);
printf("value=%s\n", pexpr->value);
			}
			// Expression was completed
			node = node->parent; // For oneliners
			faux_list_add(pline->exprs, pexpr_new());
		}

		faux_argv_each(arg);
	} while (node);

	return BOOL_TRUE;
}


/*
bool_t parse_tree(const struct lysc_node *tree, faux_argv_node_t **arg,
	pline_t *pline)
{
	const struct lysc_node *node = NULL;
	const char *str = (const char *)faux_argv_current(*arg);

	node = find_child(tree, str);
	if (node) {
		parse_node(node, arg, pline);
		return BOOL_TRUE;
	}

	return BOOL_FALSE;
}
*/


pline_t *pline_parse(const struct ly_ctx *ctx, faux_argv_t *argv, uint32_t flags)
{
	struct lys_module *module = NULL;
	pline_t *pline = pline_new();
	uint32_t i = 0;
	faux_argv_node_t *arg = faux_argv_iter(argv);

	assert(ctx);
	if (!ctx)
		return NULL;

	// Iterate all modules
	i = 0;
	while ((module = ly_ctx_get_module_iter(ctx, &i))) {
		if (sr_module_is_internal(module))
			continue;
		if (!module->compiled)
			continue;
		if (!module->implemented)
			continue;
		if (!module->compiled->data)
			continue;
		if (parse_module(module, &arg, pline))
			break; // Found
	}

	return pline;
}
