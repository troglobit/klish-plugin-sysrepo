/** @file ipath.h
 * @brief Internal path structures.
 */

#ifndef _ipath_h
#define _ipath_h

#include <faux/faux.h>
#include <faux/list.h>

typedef struct ipath_s ipath_t;
typedef faux_list_node_t ipath_node_t;

C_DECL_BEGIN

ipath_t *ipath_new(void);
void ipath_free(ipath_t *fargv);
void ipath_set_quotes(ipath_t *fargv, const char *quotes);

ssize_t ipath_len(ipath_t *fargv);
ipath_node_t *ipath_iter(const ipath_t *fargv);
const char *ipath_each(ipath_node_t **iter);
const char *ipath_current(ipath_node_t *iter);
const char *ipath_index(const ipath_t *fargv, size_t index);

ssize_t ipath_parse(ipath_t *fargv, const char *str);
bool_t ipath_add(ipath_t *fargv, const char *arg);

bool_t ipath_is_continuable(const ipath_t *fargv);
void ipath_set_continuable(ipath_t *fargv, bool_t continuable);

bool_t ipath_is_last(ipath_node_t *iter);

C_DECL_END

#endif				/* _ipath_h */
