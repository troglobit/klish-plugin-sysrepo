/** @file pline.h
 * @brief Plain line.
 */

#ifndef _pline_h
#define _pline_h

#include <faux/list.h>
#include <faux/argv.h>

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
	sr_session_ctx_t *sess;
	faux_list_t *exprs;
	faux_list_t *compls;
} pline_t;


C_DECL_BEGIN

pline_t *pline_new(sr_session_ctx_t *sess);
pline_t *pline_parse(sr_session_ctx_t *sess, faux_argv_t *argv, uint32_t flags);

void pline_free(pline_t *pline);

void pline_debug(pline_t *pline);
void pline_print_completions(const pline_t *pline, bool_t help);

C_DECL_END

#endif				/* _pline_h */
