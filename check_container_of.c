/*
 * smatch/check_container_of.c
 *
 * Copyright (C) 2010 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */
/*
 * Some macros don't return NULL pointers.  Complain if people
 * check the results for NULL because obviously the programmers
 * don't know what the pants they're doing.
 */

#include "smatch.h"

static int my_id;

STATE(non_null);

static void is_ok(struct sm_state *sm, struct expression *mod_expr)
{
	set_state(my_id, sm->name, sm->sym, &undefined);
}

static void match_non_null(const char *fn, struct expression *expr, void *unused)
{
	set_state_expr(my_id, expr->left, &non_null);
}

static void match_condition(struct expression *expr)
{
	if (__in_pre_condition)
		return;

	if (get_macro_name(expr->pos))
		return;

	if (get_state_expr(my_id, expr) == &non_null) {
		char *name;

		name = expr_to_var(expr);
		sm_msg("warn: can '%s' even be NULL?", name);
		set_state_expr(my_id, expr, &undefined);
		free_string(name);
	}
}

void check_container_of(int id)
{
	if (option_project != PROJ_KERNEL)
		return;

	my_id = id;
	add_macro_assign_hook("container_of", &match_non_null, NULL);
	add_macro_assign_hook("list_first_entry", &match_non_null, NULL);
	add_function_assign_hook("nla_data", &match_non_null, NULL);
	add_modification_hook(my_id, &is_ok);
	add_hook(&match_condition, CONDITION_HOOK);
}
