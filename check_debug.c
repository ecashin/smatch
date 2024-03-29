/*
 * sparse/check_debug.c
 *
 * Copyright (C) 2009 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

#include "smatch.h"
#include "smatch_slist.h"
#include "smatch_extra.h"

int local_debug;
static int my_id;

static void match_all_values(const char *fn, struct expression *expr, void *info)
{
	struct state_list *slist;

	slist = get_all_states(SMATCH_EXTRA);
	__print_slist(slist);
	free_slist(&slist);
}

static void match_cur_slist(const char *fn, struct expression *expr, void *info)
{
	__print_cur_slist();
}

static void match_state(const char *fn, struct expression *expr, void *info)
{
	struct expression *check_arg, *state_arg;
	struct sm_state *sm;
	int found = 0;

	check_arg = get_argument_from_call_expr(expr->args, 0);
	if (check_arg->type != EXPR_STRING) {
		sm_msg("error:  the check_name argument to %s is supposed to be a string literal", fn);
		return;
	}
	state_arg = get_argument_from_call_expr(expr->args, 1);
	if (!state_arg || state_arg->type != EXPR_STRING) {
		sm_msg("error:  the state_name argument to %s is supposed to be a string literal", fn);
		return;
	}

	FOR_EACH_PTR(__get_cur_slist(), sm) {
		if (strcmp(check_name(sm->owner), check_arg->string->data) != 0)
			continue;
		if (strcmp(sm->name, state_arg->string->data) != 0)
			continue;
		sm_msg("'%s' = '%s'", sm->name, sm->state->name);
		found = 1;
	} END_FOR_EACH_PTR(sm);

	if (!found)
		sm_msg("%s '%s' not found", check_arg->string->data, state_arg->string->data);
}

static void match_states(const char *fn, struct expression *expr, void *info)
{
	struct expression *check_arg;
	struct sm_state *sm;
	int found = 0;

	check_arg = get_argument_from_call_expr(expr->args, 0);
	if (check_arg->type != EXPR_STRING) {
		sm_msg("error:  the check_name argument to %s is supposed to be a string literal", fn);
		return;
	}

	FOR_EACH_PTR(__get_cur_slist(), sm) {
		if (strcmp(check_name(sm->owner), check_arg->string->data) != 0)
			continue;
		sm_msg("'%s' = '%s'", sm->name, sm->state->name);
		found = 1;
	} END_FOR_EACH_PTR(sm);

	if (!found)
		sm_msg("%s: no states", check_arg->string->data);
}

static void match_print_value(const char *fn, struct expression *expr, void *info)
{
	struct state_list *slist;
	struct sm_state *tmp;
	struct expression *arg_expr;

	arg_expr = get_argument_from_call_expr(expr->args, 0);
	if (arg_expr->type != EXPR_STRING) {
		sm_msg("error:  the argument to %s is supposed to be a string literal", fn);
		return;
	}

	slist = get_all_states(SMATCH_EXTRA);
	FOR_EACH_PTR(slist, tmp) {
		if (!strcmp(tmp->name, arg_expr->string->data))
			sm_msg("%s = %s", tmp->name, tmp->state->name);
	} END_FOR_EACH_PTR(tmp);
	free_slist(&slist);
}

static void match_print_implied(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	struct range_list *rl = NULL;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	get_implied_rl(arg, &rl);

	name = expr_to_str(arg);
	sm_msg("implied: %s = '%s'", name, show_rl(rl));
	free_string(name);
}

static void match_print_implied_min(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);

	if (get_implied_min(arg, &sval))
		sm_msg("implied min: %s = %s", name, sval_to_str(sval));
	else
		sm_msg("implied min: %s = <unknown>", name);

	free_string(name);
}

static void match_print_implied_max(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);

	if (get_implied_max(arg, &sval))
		sm_msg("implied max: %s = %s", name, sval_to_str(sval));
	else
		sm_msg("implied max: %s = <unknown>", name);

	free_string(name);
}

static void match_print_hard_max(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);

	if (get_hard_max(arg, &sval))
		sm_msg("hard max: %s = %s", name, sval_to_str(sval));
	else
		sm_msg("hard max: %s = <unknown>", name);

	free_string(name);
}

static void match_print_fuzzy_max(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);

	if (get_fuzzy_max(arg, &sval))
		sm_msg("fuzzy max: %s = %s", name, sval_to_str(sval));
	else
		sm_msg("fuzzy max: %s = <unknown>", name);

	free_string(name);
}

static void match_print_absolute_min(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);

	if (get_absolute_min(arg, &sval))
		sm_msg("absolute min: %s = %s", name, sval_to_str(sval));
	else
		sm_msg("absolute min: %s = <unknown>", name);

	free_string(name);
}

static void match_print_absolute_max(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	get_absolute_max(arg, &sval);

	name = expr_to_str(arg);
	sm_msg("absolute max: %s = %s", name, sval_to_str(sval));
	free_string(name);
}

static void match_sval_info(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	sval_t sval;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);

	if (!get_implied_value(arg, &sval)) {
		sm_msg("no sval for '%s'", name);
		goto free;
	}

	sm_msg("implied: %s %c%d ->value = %llx", name, sval_unsigned(sval) ? 'u' : 's', sval_bits(sval), sval.value);
free:
	free_string(name);
}

static void match_member_name(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	char *name, *member_name;

	arg = get_argument_from_call_expr(expr->args, 0);
	name = expr_to_str(arg);
	member_name = get_member_name(arg);
	sm_msg("member name: '%s => %s'", name, member_name);
	free_string(member_name);
	free_string(name);
}

static void print_possible(struct sm_state *sm)
{
	struct sm_state *tmp;

	sm_msg("Possible values for %s", sm->name);
	FOR_EACH_PTR(sm->possible, tmp) {
		printf("%s\n", tmp->state->name);
	} END_FOR_EACH_PTR(tmp);
	sm_msg("===");
}

static void match_possible(const char *fn, struct expression *expr, void *info)
{
	struct state_list *slist;
	struct sm_state *tmp;
	struct expression *arg_expr;

	arg_expr = get_argument_from_call_expr(expr->args, 0);
	if (arg_expr->type != EXPR_STRING) {
		sm_msg("error:  the argument to %s is supposed to be a string literal", fn);
		return;
	}

	slist = get_all_states(SMATCH_EXTRA);
	FOR_EACH_PTR(slist, tmp) {
		if (!strcmp(tmp->name, arg_expr->string->data))
			print_possible(tmp);
	} END_FOR_EACH_PTR(tmp);
	free_slist(&slist);
}

static void match_strlen(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	struct range_list *rl;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	get_implied_strlen(arg, &rl);

	name = expr_to_str(arg);
	sm_msg("strlen: '%s' %s characters", name, show_rl(rl));
	free_string(name);
}

static void match_buf_size(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	int elements, bytes;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	elements = get_array_size(arg);
	bytes = get_array_size_bytes(arg);

	name = expr_to_str(arg);
	sm_msg("buf size: '%s' %d elements, %d bytes", name, elements, bytes);
	free_string(name);
}

static void match_buf_size_rl(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg;
	struct range_list *rl;
	int elements, bytes;
	char *name;

	arg = get_argument_from_call_expr(expr->args, 0);
	rl = get_array_size_bytes_rl(arg);
	elements = get_array_size(arg);
	bytes = get_array_size_bytes(arg);

	name = expr_to_str(arg);
	sm_msg("buf size: '%s' %s %d elements, %d bytes", name, show_rl(rl), elements, bytes);
	free_string(name);
}

static void match_note(const char *fn, struct expression *expr, void *info)
{
	struct expression *arg_expr;

	arg_expr = get_argument_from_call_expr(expr->args, 0);
	if (arg_expr->type != EXPR_STRING) {
		sm_msg("error:  the argument to %s is supposed to be a string literal", fn);
		return;
	}
	sm_msg("%s", arg_expr->string->data);
}

static void print_related(struct sm_state *sm)
{
	struct relation *rel;

	if (!estate_related(sm->state))
		return;

	sm_prefix();
	sm_printf("%s: ", sm->name);
	FOR_EACH_PTR(estate_related(sm->state), rel) {
		sm_printf("%s ", rel->name);
	} END_FOR_EACH_PTR(rel);
	sm_printf("\n");
}

static void match_dump_related(const char *fn, struct expression *expr, void *info)
{
	struct state_list *slist;
	struct sm_state *tmp;

	slist = get_all_states(SMATCH_EXTRA);
	FOR_EACH_PTR(slist, tmp) {
		print_related(tmp);
	} END_FOR_EACH_PTR(tmp);
	free_slist(&slist);
}

static void match_compare(const char *fn, struct expression *expr, void *info)
{
	struct expression *one, *two;
	char *one_name, *two_name;
	int comparison;
	char buf[16];

	one = get_argument_from_call_expr(expr->args, 0);
	two = get_argument_from_call_expr(expr->args, 1);

	comparison = get_comparison(one, two);
	if (!comparison)
		snprintf(buf, sizeof(buf), "<none>");
	else
		snprintf(buf, sizeof(buf), "%s", show_special(comparison));

	one_name = expr_to_str(one);
	two_name = expr_to_str(two);

	sm_msg("%s %s %s", one_name, buf, two_name);

	free_string(one_name);
	free_string(two_name);
}

static void match_debug_on(const char *fn, struct expression *expr, void *info)
{
	option_debug = 1;
}

static void match_debug_off(const char *fn, struct expression *expr, void *info)
{
	option_debug = 0;
}

static void match_local_debug_on(const char *fn, struct expression *expr, void *info)
{
	local_debug = 1;
}

static void match_local_debug_off(const char *fn, struct expression *expr, void *info)
{
	local_debug = 0;
}

static void match_debug_implied_on(const char *fn, struct expression *expr, void *info)
{
	option_debug_implied = 1;
}

static void match_debug_implied_off(const char *fn, struct expression *expr, void *info)
{
	option_debug_implied = 0;
}

void check_debug(int id)
{
	my_id = id;
	add_function_hook("__smatch_all_values", &match_all_values, NULL);
	add_function_hook("__smatch_state", &match_state, NULL);
	add_function_hook("__smatch_states", &match_states, NULL);
	add_function_hook("__smatch_value", &match_print_value, NULL);
	add_function_hook("__smatch_implied", &match_print_implied, NULL);
	add_function_hook("__smatch_implied_min", &match_print_implied_min, NULL);
	add_function_hook("__smatch_implied_max", &match_print_implied_max, NULL);
	add_function_hook("__smatch_hard_max", &match_print_hard_max, NULL);
	add_function_hook("__smatch_fuzzy_max", &match_print_fuzzy_max, NULL);
	add_function_hook("__smatch_absolute_min", &match_print_absolute_min, NULL);
	add_function_hook("__smatch_absolute_max", &match_print_absolute_max, NULL);
	add_function_hook("__smatch_sval_info", &match_sval_info, NULL);
	add_function_hook("__smatch_member_name", &match_member_name, NULL);
	add_function_hook("__smatch_possible", &match_possible, NULL);
	add_function_hook("__smatch_cur_slist", &match_cur_slist, NULL);
	add_function_hook("__smatch_strlen", &match_strlen, NULL);
	add_function_hook("__smatch_buf_size", &match_buf_size, NULL);
	add_function_hook("__smatch_buf_size_rl", &match_buf_size_rl, NULL);
	add_function_hook("__smatch_note", &match_note, NULL);
	add_function_hook("__smatch_dump_related", &match_dump_related, NULL);
	add_function_hook("__smatch_compare", &match_compare, NULL);
	add_function_hook("__smatch_debug_on", &match_debug_on, NULL);
	add_function_hook("__smatch_debug_off", &match_debug_off, NULL);
	add_function_hook("__smatch_local_debug_on", &match_local_debug_on, NULL);
	add_function_hook("__smatch_local_debug_off", &match_local_debug_off, NULL);
	add_function_hook("__smatch_debug_implied_on", &match_debug_implied_on, NULL);
	add_function_hook("__smatch_debug_implied_off", &match_debug_implied_off, NULL);
}
