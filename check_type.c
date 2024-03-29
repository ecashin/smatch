/*
 * sparse/check_type.c
 *
 * Copyright (C) 2009 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

#include "smatch.h"

static int my_id;

static int in_function(const char *fn)
{
	char *cur_func = get_function();

	if (!cur_func)
		return 0;
	if (!strcmp(cur_func, fn))
		return 1;
	return 0;
}

static void match_free(const char *fn, struct expression *expr, void *data)
{
	struct expression *arg_expr;
	char *name;
	struct symbol *type;

	arg_expr = get_argument_from_call_expr(expr->args, 0);
	type = get_pointer_type(arg_expr);
	if (!type || !type->ident)
		return;

	name = expr_to_str(arg_expr);

	if (!strcmp("sk_buff", type->ident->name)) {
		sm_msg("error: use kfree_skb() here instead of kfree(%s)", name);
	} else if (!strcmp("net_device", type->ident->name)) {
		if (in_function("alloc_netdev"))
			return;
		if (in_function("alloc_netdev_mqs"))
			return;
		sm_msg("error: use free_netdev() here instead of kfree(%s)", name);
	}

	free_string(name);
}

void check_type(int id)
{
	my_id = id;
	if (option_project == PROJ_KERNEL)
		add_function_hook("kfree", &match_free, NULL);
}
