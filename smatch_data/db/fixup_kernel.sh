#!/bin/bash

# mark some paramaters as coming from user space
cat << EOF | sqlite3 smatch_db.sqlite
/* we only care about the main ->read/write() functions. */
delete from caller_info where function = '(struct file_operations)->read' and file != 'fs/read_write.c';
delete from caller_info where function = '(struct file_operations)->write' and file != 'fs/read_write.c';
delete from function_ptr where function = '(struct file_operations)->read';
delete from function_ptr where function = '(struct file_operations)->write';

/* delete these function pointers which cause false positives */
delete from caller_info where function = '(struct notifier_block)->notifier_call' and type != 0;
delete from caller_info where function = '(struct mISDNchannel)->send' and type != 0;
delete from caller_info where function = '(struct irq_router)->get' and type != 0;
delete from caller_info where function = '(struct irq_router)->set' and type != 0;
delete from caller_info where function = '(struct net_device_ops)->ndo_change_mtu' and caller = 'i40e_dbg_netdev_ops_write';
delete from caller_info where function = '(struct timer_list)->function' and type != 0;

/* type 1003 is USER_DATA */
delete from caller_info where caller = 'hid_input_report' and type = 1003;
delete from caller_info where caller = 'nes_process_iwarp_aeqe' and type = 1003;
delete from caller_info where caller = 'oz_process_ep0_urb' and type = 1003;
delete from caller_info where function = 'dev_hard_start_xmit' and key = '\$\$' and type = 1003;
delete from caller_info where function like '%->ndo_start_xmit' and key = '\$\$' and type = 1003;
delete from caller_info where caller = 'packet_rcv_fanout' and function = '(struct packet_type)->func' and parameter = 1 and type = 1003;
delete from caller_info where caller = 'hptiop_probe' and type = 1003;
delete from caller_info where caller = 'p9_fd_poll' and function = '(struct file_operations)->poll' and type = 1003;
delete from caller_info where caller = 'proc_reg_poll' and function = 'proc_reg_poll ptr poll' and type = 1003;
delete from caller_info where function = 'blkdev_ioctl' and type = 1003 and parameter = 0 and key = '\$\$';

insert into caller_info values ('userspace', '', 'compat_sys_ioctl', 0, 0, 3, 0, '$$', '1');
insert into caller_info values ('userspace', '', 'compat_sys_ioctl', 0, 0, 3, 1, '$$', '1');
insert into caller_info values ('userspace', '', 'compat_sys_ioctl', 0, 0, 3, 2, '$$', '1');

delete from caller_info where function = '(struct timer_list)->function' and parameter = 0;

/*
 * rw_verify_area is a very central function for the kernel.  The 1000000 isn't
 * accurate but I've picked it so that we can add "pos + count" without wrapping
 * on 32 bits.
 */
delete from return_states where function = 'rw_verify_area';
insert into return_states values ('faked', 'rw_verify_area', 0, 1, '0-1000000[<=p3]', 0, 0, -1, '', '');
insert into return_states values ('faked', 'rw_verify_area', 0, 1, '0-1000000[<=p3]', 0, 11, 2, '*\$\$', '0-1000000');
insert into return_states values ('faked', 'rw_verify_area', 0, 1, '0-1000000[<=p3]', 0, 11, 3, '\$\$', '0-1000000');
insert into return_states values ('faked', 'rw_verify_area', 0, 2, '(-4095)-(-1)', 0, 0, -1, '', '');


/* store a bunch of capped functions */
update return_states set return = '0-u32max[<=p2]' where function = 'copy_to_user';
update return_states set return = '0-u32max[<=p2]' where function = '_copy_to_user';
update return_states set return = '0-u32max[<=p2]' where function = '__copy_to_user';
update return_states set return = '0-u32max[<=p2]' where function = 'copy_from_user';
update return_states set return = '0-u32max[<=p2]' where function = '_copy_from_user';
update return_states set return = '0-u32max[<=p2]' where function = '__copy_from_user';

/* 64 CPUs aught to be enough for anyone */
update return_states set return = '1-64' where function = 'cpumask_weight';

update return_states set return = '0-8' where function = '__arch_hweight8';
update return_states set return = '0-16' where function = '__arch_hweight16';
update return_states set return = '0-32' where function = '__arch_hweight32';
update return_states set return = '0-64' where function = '__arch_hweight64';

/*
 * Preserve the value across byte swapping.  By the time we use it for math it
 * will be byte swapped back to CPU endian.
 */
update return_states set return = '[==p0]' where function = '__fswab64';
update return_states set return = '[==p0]' where function = '__fswab32';
update return_states set return = '[==p0]' where function = '__fswab16';

EOF

call_id=$(echo "select distinct call_id from caller_info where function = '__kernel_write';" | sqlite3 smatch_db.sqlite)
for id in $call_id ; do
    echo "insert into caller_info values ('fake', '', '__kernel_write', $id, 0, 1, 3, '*\$\$', '0-1000000');" | sqlite3 smatch_db.sqlite
done

for i in $(echo "select distinct return from return_states where function = 'clear_user';" | sqlite3 smatch_db.sqlite ) ; do
    echo "update return_states set return = \"$i[<=p1]\" where return = \"$i\" and function = 'clear_user';" | sqlite3 smatch_db.sqlite
done


