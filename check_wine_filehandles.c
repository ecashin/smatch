/*
 * sparse/check_wine_filehandles.c
 *
 * Copyright (C) 2009 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

/*
 * In wine you aren't allowed to compare file handles with 0,
 * only with INVALID_HANDLE_VALUE.
 *
 */

#include "smatch.h"

static int my_id;

STATE(filehandle);
STATE(oktocheck);

/* originally:
 *  "(?:CreateFile|CreateMailslot|CreateNamedPipe|FindFirstFile(?:Ex)?|OpenConsole|SetupOpenInfFile|socket)[AW]?"
 *
 */
static const char *filehandle_funcs[] = {
	"CreateFile",
	"CreateMailslot",
	"CreateNamedPipe",
	"FindFirstFile",
	"FindFirstFileEx",
	"OpenConsole",
	"SetupOpenInfFile",
	"socket",
	NULL,
};

static void ok_to_use(struct sm_state *sm, struct expression *mod_expr)
{
	if (sm->state != &oktocheck)
		set_state(my_id, sm->name, sm->sym, &oktocheck);
}

static void match_returns_handle(const char *fn, struct expression *expr,
			     void *info)
{
	char *left_name = NULL;
	struct symbol *left_sym;

	left_name = expr_to_var_sym(expr->left, &left_sym);
	if (!left_name || !left_sym)
		goto free;
	set_state_expr(my_id, expr->left, &filehandle);
free:
	free_string(left_name);
}

static void match_condition(struct expression *expr)
{
	if (expr->type == EXPR_ASSIGNMENT)
		match_condition(expr->left);

	if (get_state_expr(my_id, expr) == &filehandle) {
		char *name;

		name = expr_to_var(expr);
		sm_msg("error: comparing a filehandle against zero '%s'", name);
		set_state_expr(my_id, expr, &oktocheck);
		free_string(name);
	}
}

void check_wine_filehandles(int id)
{
	int i;

	if (option_project != PROJ_WINE)
		return;

	my_id = id;
	for (i = 0; filehandle_funcs[i]; i++) {
		add_function_assign_hook(filehandle_funcs[i],
					 &match_returns_handle, NULL);
	}
	add_hook(&match_condition, CONDITION_HOOK);
	add_modification_hook(my_id, ok_to_use);
}
