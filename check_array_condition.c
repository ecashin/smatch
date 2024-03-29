/*
 * smatch/check_array_condition.c
 *
 * Copyright (C) 2013 Oracle.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

/*
 * struct foo { char buf[10]; };
 *
 * struct foo *p = something();
 * if (p->buf) { ...
 *
 */

#include "smatch.h"

static int my_id;

static void match_condition(struct expression *expr)
{
	struct symbol *type;
	char *str;

	if (expr->type != EXPR_DEREF)
		return;
	type = get_type(expr);
	if (!type || type->type != SYM_ARRAY)
		return;
	if (get_macro_name(expr->pos))
		return;

	str = expr_to_str(expr);
	sm_msg("warn: this array is probably non-NULL. '%s'", str);
	free_string(str);
}

void check_array_condition(int id)
{
	my_id = id;
	add_hook(&match_condition, CONDITION_HOOK);
}
