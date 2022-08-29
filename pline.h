/** @file pline.h
 * @brief Plain line.
 */

#ifndef _pline_h
#define _pline_h

#include <sysrepo.h>
#include <sysrepo/xpath.h>

// Plain EXPRession
typedef struct {
	char *xpath;
	char *value;
	bool_t active;
} pexpr_t;


// Possible types of completion source
typedef enum {
	PCOMPL_NODE = 0,
	PCOMPL_TYPE = 1,
} pcompl_type_e;


// Plain COMPLetion
typedef struct {
	pcompl_type_e type;
	const struct lysc_node *node;
	char *xpath;
} pcompl_t;


// Plain LINE
typedef struct pline_s {
	faux_list_t *exprs;
	faux_list_t *compls;
} pline_t;

C_DECL_BEGIN


pline_t *pline_new(void);
void pline_free(pline_t *pline);
pline_t *pline_parse(const struct ly_ctx *ctx, faux_argv_t *argv, uint32_t flags);
pexpr_t *pline_current_expr(pline_t *pline);
void pline_debug(pline_t *pline);
void pline_print_completions(const pline_t *pline,
	sr_session_ctx_t *sess, bool_t help);

//void pline_set_quotes(pline_t *fargv, const char *quotes);

//ssize_t pline_len(const pline_t *fargv);
//pline_node_t *pline_iter(const pline_t *fargv);
//const char *pline_each(pline_node_t **iter);
//const char *pline_current(pline_node_t *iter);
//const char *pline_index(const pline_t *fargv, size_t index);

//ssize_t pline_parse(pline_t *fargv, const char *str);
//bool_t pline_add(pline_t *fargv, const char *arg);

//bool_t pline_is_continuable(const pline_t *fargv);
//void pline_set_continuable(pline_t *fargv, bool_t continuable);

//bool_t pline_is_last(pline_node_t *iter);

C_DECL_END

#endif				/* _pline_h */
