/*
 * smatch/check_template.c
 *
 * Copyright (C) 20XX Your Name.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

/*
 * First of all, it's best if you lower your expectations from finding
 * errors to just finding suspicious code.  There tends to be a lot
 * of false positives so having low expectations helps.
 *
 * For this test let's look for functions that return a negative value
 * with a semaphore held.
 *
 * This is just a template check.  It's designed for teaching
 * only and is deliberately less useful than it could be.  check_locking.c
 * is a better real world test.
 *
 * The biggest short coming is that it assumes a function isn't supposed
 * to return negative with a lock held.  Also it assumes the function was
 * called without the lock held. It would be better if it handled the stuff
 * like this:
 *     ret = -ENOMEM;
 *     return ret;
 * Another idea would be to test other kinds of locks besides just semaphores.
 *
 */

#include "smatch.h"
#include "smatch_slist.h"

static int my_id;

STATE(lock);
STATE(unlock);

/*
 * unmatched_state() deals with the case where code is known to be
 * locked on one path but not known on the other side of a merge.  Here
 * we assume it's the opposite.
 */

static struct smatch_state *unmatched_state(struct sm_state *sm)
{
	if (sm->state == &lock)
		return &unlock;
	if (sm->state == &unlock)
		return &lock;
	return &undefined;
}

static void match_call(struct expression *expr)
{
	char *fn_name;
	struct expression *sem_expr;
	char *sem_name;

	fn_name = expr_to_var(expr->fn);
	if (!fn_name || (strcmp(fn_name, "down") && strcmp(fn_name, "up")))
		goto free_fn;

	sem_expr = get_argument_from_call_expr(expr->args, 0);
	sem_name = expr_to_var(sem_expr);
	if (!strcmp(fn_name, "down")) {
		set_state(my_id, sem_name, NULL, &lock);
	} else {
		set_state(my_id, sem_name, NULL, &unlock);
	}
	free_string(sem_name);
free_fn:
	free_string(fn_name);
}

static void match_return(struct expression *ret_value)
{
	sval_t ret_val;
	struct state_list *slist;
	struct sm_state *tmp;

	if (!get_value(ret_value, &ret_val) || sval_cmp_val(ret_val, 0) >= 0)
		return;

	slist = get_all_states(my_id);
	FOR_EACH_PTR(slist, tmp) {
		if (tmp->state != &unlock)
			sm_msg("warn: returned negative with %s semaphore held",
				   tmp->name);
	} END_FOR_EACH_PTR(tmp);
	free_slist(&slist);
}

void check_template(int id)
{
	my_id = id;
	add_unmatched_state_hook(my_id, &unmatched_state);
	add_hook(&match_call, FUNCTION_CALL_HOOK);
	add_hook(&match_return, RETURN_HOOK);
}
