/*
 * sparse/check_prempt.c
 *
 * Copyright (C) 2009 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

/*
 * This test checks that prempt is enabled at the end of every function.
 */

#include "smatch.h"
#include "smatch_slist.h"

static int my_id;

STATE(left);
STATE(right);
STATE(start_state);

static struct tracker_list *starts_left;
static struct tracker_list *starts_right;

static struct smatch_state *get_start_state(struct sm_state *sm)
{
       int is_left = 0;
       int is_right = 0;

       if (in_tracker_list(starts_left, my_id, sm->name, sm->sym))
               is_left = 1;
       if (in_tracker_list(starts_right, my_id, sm->name, sm->sym))
               is_right = 1;
       if (is_left && is_right)
               return &undefined;
       if (is_left)
               return &left;
       if (is_right)
	       return &right;
	return &undefined;
}

static struct smatch_state *unmatched_state(struct sm_state *sm)
{
	return &start_state;
}

static void match_left(const char *fn, struct expression *expr, void *data)
{
	struct sm_state *sm;
	char *name = (char *)data;

	if (__inline_fn)
		return;

	sm = get_sm_state(my_id, name, NULL);
	if (!sm)
		add_tracker(&starts_right, my_id, name, NULL);
	if (sm && slist_has_state(sm->possible, &left))
		sm_msg("warn: double '%s'", fn);
	set_state(my_id, (char *)data, NULL, &left);
}

static void match_right(const char *fn, struct expression *expr, void *data)
{
	struct sm_state *sm;
	char *name = (char *)data;

	if (__inline_fn)
		return;

	sm = get_sm_state(my_id, name, NULL);
	if (!sm)
		add_tracker(&starts_left, my_id, name, NULL);
	if (sm && slist_has_state(sm->possible, &right))
		sm_msg("warn: double '%s'", fn);
	set_state(my_id, (char *)data, NULL, &right);
}

static void check_possible(struct sm_state *sm)
{
	struct sm_state *tmp;
	int is_left = 0;
	int is_right = 0;
	int undef = 0;

	FOR_EACH_PTR(sm->possible, tmp) {
		if (tmp->state == &left)
			is_left = 1;
		if (tmp->state == &right)
			is_right = 1;
		if (tmp->state == &start_state) {
			struct smatch_state *s;

			s = get_start_state(tmp);
			if (s == &left)
				is_left = 1;
			else if (s == &right)
				is_right = 1;
			else
				undef = 1;
		}
		if (tmp->state == &undefined)
			undef = 1;  // i don't think this is possible any more.
	} END_FOR_EACH_PTR(tmp);
	if ((is_left && is_right) || undef)
		sm_msg("warn: returning with unbalanced %s", sm->name);
}

static void match_return(struct expression *expr)
{
	struct state_list *slist;
	struct sm_state *tmp;

	if (__inline_fn)
		return;

	slist = get_all_states(my_id);
	FOR_EACH_PTR(slist, tmp) {
		if (tmp->state == &merged)
			check_possible(tmp);
	} END_FOR_EACH_PTR(tmp);
	free_slist(&slist);
}

static void clear_lists(void)
{
	free_trackers_and_list(&starts_left);
	free_trackers_and_list(&starts_right);
}

static void match_func_end(struct symbol *sym)
{
	if (__inline_fn)
		return;
	if (is_reachable())
		match_return(NULL);
	clear_lists();
}

static void get_left_funcs(const char *name, struct token **token)
{
	const char *func;

	while (token_type(*token) == TOKEN_IDENT) {
		func = show_ident((*token)->ident);
		add_function_hook(func, &match_left, (char *)name);
		*token = (*token)->next;
	}
	if (token_type(*token) == TOKEN_SPECIAL)
		*token = (*token)->next;
}

static void get_right_funcs(const char *name, struct token **token)
{
	const char *func;

	while (token_type(*token) == TOKEN_IDENT) {
		func = show_ident((*token)->ident);
		add_function_hook(func, &match_right, (char *)name);
		*token = (*token)->next;
	}
	if (token_type(*token) == TOKEN_SPECIAL)
		*token = (*token)->next;
}

static void register_funcs_from_file(void)
{
	struct token *token;
	char *name;

	token = get_tokens_file("kernel.balanced_funcs");
	if (!token)
		return;
	if (token_type(token) != TOKEN_STREAMBEGIN)
		return;
	token = token->next;
	while (token_type(token) != TOKEN_STREAMEND) {
		if (token_type(token) != TOKEN_IDENT)
			return;
		name = alloc_string(show_ident(token->ident));
		token = token->next;
		get_left_funcs(name, &token);
		get_right_funcs(name, &token);
	}
	clear_token_alloc();
}

void check_balanced(int id)
{
	my_id = id;
	add_unmatched_state_hook(my_id, &unmatched_state);
	add_hook(&match_return, RETURN_HOOK);
	add_hook(&match_func_end, END_FUNC_HOOK);
	register_funcs_from_file();
}
