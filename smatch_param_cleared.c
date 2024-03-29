/*
 * sparse/smatch_param_cleared.c
 *
 * Copyright (C) 2012 Oracle.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

/*
 * This works together with smatch_clear_buffer.c.  This one is only for
 * tracking the information and smatch_clear_buffer.c changes SMATCH_EXTRA.
 *
 * This tracks functions like memset() which clear out a chunk of memory.
 * It fills in a gap that smatch_param_set.c can't handle.  It only handles
 * void pointers because smatch_param_set.c should handle the rest.
 */

#include "scope.h"
#include "smatch.h"
#include "smatch_slist.h"
#include "smatch_extra.h"

static int my_id;

STATE(cleared);
STATE(zeroed);

static void db_param_cleared(struct expression *expr, int param, char *key, char *value)
{
	struct expression *arg;

	while (expr->type == EXPR_ASSIGNMENT)
		expr = strip_expr(expr->right);
	if (expr->type != EXPR_CALL)
		return;

	arg = get_argument_from_call_expr(expr->args, param);
	arg = strip_expr(arg);
	if (!arg)
		return;
	if (arg->type != EXPR_SYMBOL)
		return;
	if (get_param_num_from_sym(arg->symbol) < 0)
		return;

	if (strcmp(value, "0") == 0)
		set_state_expr(my_id, arg, &zeroed);
	else
		set_state_expr(my_id, arg, &cleared);
}

static void match_memset(const char *fn, struct expression *expr, void *arg)
{
	db_param_cleared(expr, PTR_INT(arg), (char *)"$$", (char *)"0");
}

static void match_memcpy(const char *fn, struct expression *expr, void *arg)
{
	db_param_cleared(expr, PTR_INT(arg), (char *)"$$", (char *)"");
}

static void print_return_value_param(int return_id, char *return_ranges, struct expression *expr)
{
	struct state_list *my_slist;
	struct sm_state *sm;
	int param;

	my_slist = get_all_states(my_id);

	FOR_EACH_PTR(my_slist, sm) {
		param = get_param_num_from_sym(sm->sym);
		if (param < 0)
			continue;

		if (sm->state == &zeroed) {
			sql_insert_return_states(return_id, return_ranges,
						 PARAM_CLEARED, param, "$$", "0");
		}

		if (sm->state == &cleared) {
			sql_insert_return_states(return_id, return_ranges,
						 PARAM_CLEARED, param, "$$", "");
		}
	} END_FOR_EACH_PTR(sm);
}

static void register_clears_param(void)
{
	struct token *token;
	char name[256];
	const char *function;
	int param;

	if (option_project == PROJ_NONE)
		return;

	snprintf(name, 256, "%s.clears_argument", option_project_str);

	token = get_tokens_file(name);
	if (!token)
		return;
	if (token_type(token) != TOKEN_STREAMBEGIN)
		return;
	token = token->next;
	while (token_type(token) != TOKEN_STREAMEND) {
		if (token_type(token) != TOKEN_IDENT)
			return;
		function = show_ident(token->ident);
		token = token->next;
		if (token_type(token) != TOKEN_NUMBER)
			return;
		param = atoi(token->number);
		add_function_hook(function, &match_memcpy, INT_PTR(param));
		token = token->next;
	}
	clear_token_alloc();
}

#define USB_DIR_IN 0x80
static void match_usb_control_msg(const char *fn, struct expression *expr, void *_size_arg)
{
	struct expression *inout;
	sval_t sval;

	inout = get_argument_from_call_expr(expr->args, 3);

	if (get_value(inout, &sval) && !(sval.uvalue & USB_DIR_IN))
		return;

	db_param_cleared(expr, 6, (char *)"$$", (char *)"");
}

void register_param_cleared(int id)
{
	if (!option_info)
		return;

	my_id = id;

	add_function_hook("memset", &match_memset, INT_PTR(0));

	add_function_hook("memcpy", &match_memcpy, INT_PTR(0));
	add_function_hook("memmove", &match_memcpy, INT_PTR(0));
	add_function_hook("strcpy", &match_memcpy, INT_PTR(0));
	add_function_hook("strncpy", &match_memcpy, INT_PTR(0));
	add_function_hook("sprintf", &match_memcpy, INT_PTR(0));
	add_function_hook("snprintf", &match_memcpy, INT_PTR(0));

	register_clears_param();

	select_return_states_hook(PARAM_CLEARED, &db_param_cleared);
	add_split_return_callback(&print_return_value_param);

	if (option_project == PROJ_KERNEL) {
		add_function_hook("usb_control_msg", &match_usb_control_msg, NULL);
	}

}

