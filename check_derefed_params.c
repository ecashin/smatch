/*
 * sparse/check_deference.c
 *
 * Copyright (C) 2006 Dan Carpenter.
 *
 *  Licensed under the Open Software License version 1.1
 *
 */

#include "token.h"
#include "smatch.h"

static int my_id;

STATE(argument);
STATE(ignore);
STATE(isnull);
STATE(nonnull);

struct param {
	struct symbol *sym;
	int used;
};
ALLOCATOR(param, "parameters");
#define MAX_PARAMS 16
struct param *params[MAX_PARAMS];

static struct smatch_state *merge_func(const char *name, struct symbol *sym,
				       struct smatch_state *s1,
				       struct smatch_state *s2)
{
	if (s1 == &ignore || s2 == &ignore)
		return &ignore;
	if (s1 == &argument)
		return s2;
	return &undefined;
}

static struct param *new_param(struct symbol *arg)
{
	struct param *new;

	new = __alloc_param(0);
	if (!new) {
		printf("Internal error:  Unable to allocate memory in new_param()\n");
		exit(1);
	}
	new->sym = arg;
	new->used = 0;
       	return new;
}

static void match_function_def(struct symbol *sym)
{
	struct symbol *arg;
	int i = 0;
	FOR_EACH_PTR(sym->ctype.base_type->arguments, arg) {
		set_state("", my_id, arg, &argument);
		params[i] = new_param(arg);
		i++;
		if (i >= MAX_PARAMS - 1) {
			printf("Error function has too many params.\n");
			break;
		}
	} END_FOR_EACH_PTR(arg);
	params[i] = NULL;
}

static void print_unchecked_param(struct symbol *sym) {
	int i = 0;

	while (params[i]) {
		if (params[i]->sym == sym && !params[i]->used) {
			smatch_msg("unchecked param:  %s %d", get_function(),
				   i);
		}
		i++;
	}
}

static void match_deref(struct expression *expr)
{
	char *deref = NULL;
	struct symbol *sym = NULL;
	struct smatch_state *state;

	if (strcmp(show_special(expr->deref->op), "*"))
		return;

	deref = get_variable_from_expr_simple(expr->deref->unop, &sym);
	if (!deref)
		return;
	state = get_state("", my_id, sym);
	if (state == &argument) {
		print_unchecked_param(sym);
		/* this doesn't actually work because we'll still see
		   the same variable get derefed on other paths */
		set_state("", my_id, sym, &ignore);
	} else if (state == &isnull)
		smatch_msg("Error dereferencing NULL:  %s", deref);
}

static void match_function_call_after(struct expression *expr)
{
	struct expression *tmp;
	struct symbol *sym;
	struct smatch_state *state;

	FOR_EACH_PTR(expr->args, tmp) {
		if (tmp->op == '&') {
			get_variable_from_expr(tmp, &sym);
			state = get_state("", my_id, sym);
			if (state) {
				set_state("", my_id, sym, &nonnull);
			}
		}
	} END_FOR_EACH_PTR(tmp);
}

static void match_assign(struct expression *expr)
{
	struct smatch_state *state;

	/* Since we're only tracking arguments, we only want 
	   EXPR_SYMBOLs.  */
	if (expr->left->type != EXPR_SYMBOL)
		return;
	state = get_state("", my_id, expr->left->symbol);
	if (!state)
		return;
	/* probably it's non null */
	set_state("", my_id, expr->left->symbol, &nonnull);
}

static void match_condition(struct expression *expr)
{
	switch(expr->type) {
	case EXPR_COMPARE: {
		struct symbol *sym = NULL;

		if (expr->left->type == EXPR_SYMBOL) {
			sym = expr->left->symbol;
			if (expr->right->type != EXPR_VALUE 
			    || expr->right->value != 0)
				return;
		}
		if (expr->right->type == EXPR_SYMBOL) {
			sym = expr->right->symbol;
			if (expr->left->type != EXPR_VALUE 
			    || expr->left->value != 0)
				return;
		}
		if (!sym)
			return;

		if (expr->op == SPECIAL_EQUAL)
			set_true_false_states("", my_id, sym, &isnull,
					      &nonnull);
		else if (expr->op == SPECIAL_NOTEQUAL)
			set_true_false_states("", my_id, sym, &nonnull,
					      &isnull);
		return;
	}
	case EXPR_SYMBOL:
		if (get_state("", my_id, expr->symbol) == &argument) {
			set_true_false_states("", my_id, expr->symbol, 
		  			      &nonnull, &isnull);
		}
		return;
	default:
		return;
	}
}

static void end_of_func_cleanup(struct symbol *sym)
{
	int i = 0;

	while (params[i]) {
		__free_param(params[i]);
		i++;
	}
}

void register_derefed_params(int id)
{
	my_id = id;
	add_merge_hook(my_id, &merge_func);
	add_hook(&match_function_def, FUNC_DEF_HOOK);
	add_hook(&match_function_call_after, FUNCTION_CALL_AFTER_HOOK);
	add_hook(&match_deref, DEREF_HOOK);
	add_hook(&match_assign, ASSIGNMENT_HOOK);
	add_hook(&match_condition, CONDITION_HOOK);
	add_hook(&end_of_func_cleanup, END_FUNC_HOOK);
}