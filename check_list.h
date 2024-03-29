#ifndef CK
#define CK(_x) void _x(int id);
#define __undo_CK_def
#endif

CK(register_smatch_extra) /* smatch_extra always has to be first */
CK(register_modification_hooks)
CK(register_definition_db_callbacks)
CK(register_project)

CK(register_smatch_ignore)
CK(register_buf_size)
CK(register_strlen)
CK(register_strlen_equiv)
CK(register_capped)
CK(register_parse_call_math)
CK(register_param_limit)
CK(register_param_filter)
CK(register_param_set)
CK(register_param_cleared)
CK(register_clear_buffer)
CK(register_comparison)
CK(register_comparison_links)
CK(register_local_values)
CK(register_function_ptrs)
CK(register_annotate)
CK(register_start_states)
CK(register_type_val)
CK(register_data_source)
CK(register_common_functions)

CK(check_debug)
CK(check_assigned_expr)
CK(check_user_data)

CK(check_bogus_loop)

CK(check_deref)
CK(check_check_deref)
CK(check_dereferences_param)
CK(check_overflow)
CK(check_leaks)
CK(check_type)
CK(check_allocation_funcs)
CK(check_frees_argument)
CK(check_balanced)
CK(check_deref_check)
CK(check_redundant_null_check)
CK(check_signed)
CK(check_precedence)
CK(check_format_string)
CK(check_unused_ret)
CK(check_dma_on_stack)
CK(check_param_mapper)
CK(check_call_tree)
CK(check_dev_queue_xmit)
CK(check_stack)
CK(check_no_return)
CK(check_mod_timer)
CK(check_return)
CK(check_resource_size)
CK(check_release_resource)
CK(check_proc_create)
CK(check_freeing_null)
CK(check_free)
CK(check_no_effect)
CK(check_kunmap)
CK(check_snprintf)
CK(check_macros)
CK(check_propagate)
CK(check_return_efault)
CK(check_gfp_dma)
CK(check_unwind)
CK(check_kmalloc_to_bugon)
CK(check_platform_device_put)
CK(check_info_leak)
CK(check_return_enomem)
CK(check_get_user_overflow)
CK(check_get_user_overflow2)
CK(check_access_ok_math)
CK(check_container_of)
CK(check_input_free_device)
CK(check_select)
CK(check_memset)
CK(check_logical_instead_of_bitwise)
CK(check_kmalloc_wrong_size)
CK(check_pointer_math)
CK(check_bit_shift)
CK(check_macro_side_effects)
CK(check_sizeof)
CK(check_or_vs_and)
CK(check_passes_sizeof)
CK(check_assign_vs_compare)
CK(check_missing_break)
CK(check_array_condition)
CK(check_struct_type)
CK(check_cast_assign)

/* <- your test goes here */
/* CK(register_template) */

/* kernel specific */
CK(check_locking)
CK(check_puts_argument)
CK(check_err_ptr)
CK(check_err_ptr_deref)
CK(check_expects_err_ptr)
CK(check_held_dev)
CK(check_return_negative_var)
CK(check_rosenberg)
CK(check_rosenberg2)
CK(check_wait_for_common)
CK(check_bogus_irqrestore)

/* wine specific stuff */
CK(check_wine_filehandles)
CK(check_wine_WtoA)

#include "check_list_local.h"

CK(register_sval)
CK(register_buf_size_late)
CK(register_modification_hooks_late)
CK(register_smatch_extra_late)
CK(check_kernel)  /* this is overwriting stuff from smatch_extra_late */
CK(register_function_hooks)
CK(register_returns)
CK(register_db_call_marker) /* always second last */
CK(register_implications) /* implications always has to be last */

#ifdef __undo_CK_def
#undef CK
#undef __undo_CK_def
#endif
