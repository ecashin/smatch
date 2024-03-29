/*
 * sparse/smatch_tracker.c
 *
 * Copyright (C) 2009 Dan Carpenter.
 *
 * Licensed under the Open Software License version 1.1
 *
 */

#include "smatch.h"

ALLOCATOR(tracker, "trackers");

struct tracker *alloc_tracker(int owner, const char *name, struct symbol *sym)
{
	struct tracker *tmp;

	tmp = __alloc_tracker(0);
	tmp->name = alloc_string(name);
	tmp->owner = owner;
	tmp->sym = sym;
	return tmp;
}

void add_tracker(struct tracker_list **list, int owner, const char *name,
		struct symbol *sym)
{
	struct tracker *tmp;

	if (in_tracker_list(*list, owner, name, sym))
		return;
	tmp = alloc_tracker(owner, name, sym);
	add_ptr_list(list, tmp);
}

void add_tracker_expr(struct tracker_list **list, int owner, struct expression *expr)
{
	char *name;
	struct symbol *sym;

	name = expr_to_var_sym(expr, &sym);
	if (!name || !sym)
		goto free;
	add_tracker(list, owner, name, sym);
free:
	free_string(name);
}

static void free_tracker(struct tracker *t)
{
	free_string(t->name);
	__free_tracker(t);
}

void del_tracker(struct tracker_list **list, int owner, const char *name,
		struct symbol *sym)
{
	struct tracker *tmp;

	FOR_EACH_PTR(*list, tmp) {
		if (tmp->owner == owner && tmp->sym == sym
		    && !strcmp(tmp->name, name)) {
			DELETE_CURRENT_PTR(tmp);
			free_tracker(tmp);
			return;
		}
	} END_FOR_EACH_PTR(tmp);
}

int in_tracker_list(struct tracker_list *list, int owner, const char *name,
		struct symbol *sym)
{
	struct tracker *tmp;

	FOR_EACH_PTR(list, tmp) {
		if (tmp->owner == owner && tmp->sym == sym
		    && !strcmp(tmp->name, name))
			return 1;
	} END_FOR_EACH_PTR(tmp);
	return 0;
}

void free_tracker_list(struct tracker_list **list)
{
	__free_ptr_list((struct ptr_list **)list);
}

void free_trackers_and_list(struct tracker_list **list)
{
	struct tracker *tmp;

	FOR_EACH_PTR(*list, tmp) {
		free_tracker(tmp);
	} END_FOR_EACH_PTR(tmp);
	free_tracker_list(list);
}

