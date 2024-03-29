/*
 * sparse/check_rosenberg.c
 *
 * Copyright (C) 2011 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

/* Does a search for Dan Rosenberg style info leaks */

/* fixme: struct includes a struct with a hole in it */
/* function is called that clears the struct */

#include "scope.h"
#include "smatch.h"
#include "smatch_function_hashtable.h"
#include "smatch_slist.h"
#include "smatch_extra.h"

static int my_whole_id;
static int my_member_id;

STATE(cleared);

static void extra_mod_hook(const char *name, struct symbol *sym, struct smatch_state *state)
{
	set_state(my_member_id, name, sym, state);
}

static void print_holey_warning(struct expression *data, const char *member)
{
	char *name;

	name = expr_to_str(data);
	if (member) {
		sm_msg("warn: check that '%s' doesn't leak information (struct has a hole after '%s')",
		       name, member);
	} else {
		sm_msg("warn: check that '%s' doesn't leak information (struct has holes)",
		       name);
	}
	free_string(name);
}

static int check_struct(struct expression *expr, struct symbol *type)
{
	struct symbol *tmp, *base_type;
	const char *prev = NULL;
	int align;

	align = 0;
	FOR_EACH_PTR(type->symbol_list, tmp) {
		base_type = get_real_base_type(tmp);
		if (base_type && base_type->type == SYM_STRUCT) {
			if (check_struct(expr, base_type))
				return 1;
		}
		if (type->ctype.attribute->is_packed)
			continue;

		if (!tmp->ctype.alignment) {
			sm_msg("warn: cannot determine the alignment here\n");
		} else if (align % tmp->ctype.alignment) {
			print_holey_warning(expr, prev);
			return 1;
		}

		if (base_type == &bool_ctype)
			align += 1;
		else if (tmp->bit_size <= 0)
			align = 0;
		else
			align += bits_to_bytes(tmp->bit_size);

		if (tmp->ident)
			prev = tmp->ident->name;
		else
			prev = NULL;
	} END_FOR_EACH_PTR(tmp);

	return 0;
}

static int warn_on_holey_struct(struct expression *expr)
{
	struct symbol *type;
	type = get_type(expr);
	if (!type || type->type != SYM_STRUCT)
		return 0;

	return check_struct(expr, type);
}

static int has_global_scope(struct expression *expr)
{
	struct symbol *sym;

	if (expr->type != EXPR_SYMBOL)
		return FALSE;
	sym = expr->symbol;
	return toplevel(sym->scope);
}

static int was_initialized(struct expression *expr)
{
	struct symbol *sym;
	char *name;

	name = expr_to_var_sym(expr, &sym);
	if (!name)
		return 0;
	if (sym->initializer)
		return 1;
	return 0;
}

static void match_clear(const char *fn, struct expression *expr, void *_arg_no)
{
	struct expression *ptr;
	int arg_no = PTR_INT(_arg_no);

	ptr = get_argument_from_call_expr(expr->args, arg_no);
	if (!ptr)
		return;
	if (ptr->type != EXPR_PREOP || ptr->op != '&')
		return;
	ptr = strip_expr(ptr->unop);
	set_state_expr(my_whole_id, ptr, &cleared);
}

static int was_memset(struct expression *expr)
{
	if (get_state_expr(my_whole_id, expr) == &cleared)
		return 1;
	return 0;
}

static int member_initialized(char *name, struct symbol *outer, struct symbol *member)
{
	char buf[256];
	struct symbol *base;

	base = get_base_type(member);
	if (!base || base->type != SYM_BASETYPE || !member->ident)
		return FALSE;

	snprintf(buf, 256, "%s.%s", name, member->ident->name);
	if (get_state(my_member_id, buf, outer))
		return TRUE;

	return FALSE;
}

static int member_uninitialized(char *name, struct symbol *outer, struct symbol *member)
{
	char buf[256];
	struct symbol *base;
	struct sm_state *sm;

	base = get_base_type(member);
	if (!base || base->type != SYM_BASETYPE || !member->ident)
		return FALSE;

	snprintf(buf, 256, "%s.%s", name, member->ident->name);
	sm = get_sm_state(my_member_id, buf, outer);
	if (sm && !slist_has_state(sm->possible, &undefined))
		return FALSE;

	sm_msg("warn: check that '%s' doesn't leak information", buf);
	return TRUE;
}

static int check_members_initialized(struct expression *expr)
{
	char *name;
	struct symbol *outer;
	struct symbol *sym;
	struct symbol *tmp;
	int printed = 0;

	sym = get_type(expr);
	if (!sym || sym->type != SYM_STRUCT)
		return 0;

	name = expr_to_var_sym(expr, &outer);

	if (get_state(my_member_id, name, outer))
		goto out;

	/*
	 * check that at least one member was set.  If all of them were not set
	 * it's more likely a problem in the check than a problem in the kernel
	 * code.
	 */
	FOR_EACH_PTR(sym->symbol_list, tmp) {
		if (member_initialized(name, outer, tmp))
			goto check;
	} END_FOR_EACH_PTR(tmp);
	goto out;

check:
	FOR_EACH_PTR(sym->symbol_list, tmp) {
		if (member_uninitialized(name, outer, tmp)) {
			printed = 1;
			goto out;
		}
	} END_FOR_EACH_PTR(tmp);
out:
	free_string(name);
	return printed;
}

static void match_copy_to_user(const char *fn, struct expression *expr, void *_arg)
{
	int arg = PTR_INT(_arg);
	struct expression *data;

	data = get_argument_from_call_expr(expr->args, arg);
	data = strip_expr(data);
	if (!data)
		return;
	if (data->type == EXPR_PREOP && data->op == '&')
		data = strip_expr(data->unop);
	if (data->type != EXPR_SYMBOL)
		return;

	if (has_global_scope(data))
		return;
	if (was_initialized(data))
		return;
	if (was_memset(data))
		return;
	if (warn_on_holey_struct(data))
		return;
	check_members_initialized(data);
}

static void db_param_cleared(struct expression *expr, int param, char *key, char *value)
{
	while (expr->type == EXPR_ASSIGNMENT)
		expr = strip_expr(expr->right);
	if (expr->type != EXPR_CALL)
		return;

	match_clear(NULL, expr, INT_PTR(param));
}

static void register_clears_argument(void)
{
	struct token *token;
	const char *func;
	int arg;

	token = get_tokens_file("kernel.clears_argument");
	if (!token)
		return;
	if (token_type(token) != TOKEN_STREAMBEGIN)
		return;
	token = token->next;
	while (token_type(token) != TOKEN_STREAMEND) {
		if (token_type(token) != TOKEN_IDENT)
			return;
		func = show_ident(token->ident);
		token = token->next;
		if (token_type(token) != TOKEN_NUMBER)
			return;
		arg = atoi(token->number);

		add_function_hook(func, &match_clear, INT_PTR(arg));
		token = token->next;
	}
	clear_token_alloc();
}

void check_rosenberg(int id)
{
	if (option_project != PROJ_KERNEL)
		return;
	my_whole_id = id;

	add_function_hook("memset", &match_clear, INT_PTR(0));
	add_function_hook("memcpy", &match_clear, INT_PTR(0));
	register_clears_argument();

	add_function_hook("copy_to_user", &match_copy_to_user, INT_PTR(1));
	add_function_hook("__copy_to_user", &match_copy_to_user, INT_PTR(1));
	add_function_hook("nla_put", &match_copy_to_user, INT_PTR(3));
	select_return_states_hook(PARAM_CLEARED, &db_param_cleared);
}

void check_rosenberg2(int id)
{
	my_member_id = id;
	add_extra_mod_hook(&extra_mod_hook);
}

