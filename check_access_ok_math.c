/*
 * smatch/check_access_ok_math.c
 *
 * Copyright (C) 2010 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

#include "smatch.h"

static int my_id;

static int can_overflow(struct expression *expr)
{
	sval_t max;
	int uncapped = 0;

	expr = strip_expr(expr);

	if (expr->type == EXPR_BINOP) {
		uncapped += can_overflow(expr->left);
		uncapped += can_overflow(expr->right);

		if (uncapped &&
			(expr->op == '+' || expr->op == '*' || expr->op == SPECIAL_LEFTSHIFT))
			return 1;

		return 0;
	}

	if (get_implied_max(expr, &max))
		return 0;
	if (get_absolute_max(expr, &max) && sval_cmp_val(max, 4096) <= 0)
		return 0;
	return 1;
}

static void match_size(struct expression *size_expr)
{
	char *name;

	size_expr = strip_expr(size_expr);
	if (!size_expr)
		return;
	if (size_expr->type != EXPR_BINOP) {
		size_expr = get_assigned_expr(size_expr);
		if (!size_expr || size_expr->type != EXPR_BINOP)
			return;
	}
	if (!can_overflow(size_expr))
		return;

	name = expr_to_str(size_expr);
	sm_msg("warn: math in access_ok() is dangerous '%s'", name);

	free_string(name);
}

static void match_access_ok(const char *fn, struct expression *expr, void *data)
{
	struct expression *size_expr;

	size_expr = get_argument_from_call_expr(expr->args, 1);
	match_size(size_expr);
}

static void split_asm_constraints(struct expression_list *expr_list)
{
	struct expression *expr;
        int state = 0;
	int i;

	i = 0;
        FOR_EACH_PTR(expr_list, expr) {

                switch (state) {
                case 0: /* identifier */
                case 1: /* constraint */
                        state++;
                        continue;
                case 2: /* expression */
                        state = 0;
			if (i == 1)
				match_size(expr);
			i++;
                        continue;
                }
        } END_FOR_EACH_PTR(expr);
}

static void match_asm_stmt(struct statement *stmt)
{
	char *name;

	name = get_macro_name(stmt->pos);
	if (!name || strcmp(name, "access_ok") != 0)
		return;
	split_asm_constraints(stmt->asm_inputs);
}

void check_access_ok_math(int id)
{
	my_id = id;
	if (option_project != PROJ_KERNEL)
		return;
	if (!option_spammy)
		return;
	add_function_hook("__access_ok", &match_access_ok, NULL);
	add_hook(&match_asm_stmt, ASM_HOOK);
}
