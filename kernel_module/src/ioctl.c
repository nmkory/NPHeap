//////////////////////////////////////////////////////////////////////
//                             University of California, Riverside
//
//
//
//                             Copyright 2020
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Authors:  Nicholas Kory
//
//   Description:
//     NPHeap Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "npheap.h"

#include <asm/processor.h>
#include <asm/segment.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/rbtree.h>

////////////////////////////////////////////////////////////////////////
//
//   Global variables for NPHeap implementation.
//
////////////////////////////////////////////////////////////////////////

// Part of the starting code.
extern struct miscdevice npheap_dev;

// Declares linux/mutex.h mutex and assigns it parameter name.
static DEFINE_MUTEX(np_lock);

// The root node for the rb tree data structure.
struct rb_root mytree = RB_ROOT;

////////////////////////////////////////////////////////////////////////
//
//   Red black tree data structure implementation.
//   Adapted from kernel.org documentation.
//   https://www.kernel.org/doc/Documentation/rbtree.txt
//
////////////////////////////////////////////////////////////////////////

// The rb tree node implementation.
struct mytype {
  	struct rb_node node;
  	char *keystring;
  }; //struct mytype


// my_search() searches through the rb tree for the node.
//
// rb_root: the rb tree rb_root
// string: the data value we're searching for
//
// returns: the node we're looking for if found or null if not found
struct mytype *my_search(struct rb_root *root, char *string)
{
  	struct rb_node *node = root->rb_node;

  	while (node) {
  		struct mytype *data = container_of(node, struct mytype, node);
		int result;

		result = strcmp(string, data->keystring);

		if (result < 0)
  			node = node->rb_left;
		else if (result > 0)
  			node = node->rb_right;
		else
  			return data;
	}
	return NULL;
}  //my_search()


// my_insert() inserts a node into the rb tree.
//
// rb_root: the rb tree rb_root
// data: the node we're inserting
//
// returns: 1 if successful or 0 if the key value is already in the rb tree
int my_insert(struct rb_root *root, struct mytype *data)
{
  struct rb_node **new = &(root->rb_node), *parent = NULL;

  // Figure out where to put new node
  while (*new) {
    struct mytype *this = container_of(*new, struct mytype, node);
    int result = strcmp(data->keystring, this->keystring);

  parent = *new;
    if (result < 0)
      new = &((*new)->rb_left);
    else if (result > 0)
      new = &((*new)->rb_right);
    else
      return 0;
  }

  // Add new node and rebalance tree.
  rb_link_node(&data->node, parent, new);
  rb_insert_color(&data->node, root);

return 1;
}  //my_insert()


// rb_erase() is part of linux/rbtree.h.
//
// victim: node to be removed (found using search)
// rb_root: the rb tree rb_root
//
// return: void
//  void rb_erase(struct rb_node *victim, struct rb_root *tree);
//
// Example:
//
//   struct mytype *data = mysearch(mytree, "walrus");
//
//   if (data) {
//   	rb_erase(data->node, mytree);
//   	myfree(data);
//   }


////////////////////////////////////////////////////////////////////////
//
//   NPHeap implementation.
//
////////////////////////////////////////////////////////////////////////

// This struct defines a memory VMM memory area. There is one of these
// per VM-area/task.  A VM area is any part of the process virtual memory
// space that has a special rule for the page-fault handlers (ie a shared
// library, the executable area etc).
//
// struct vm_area_struct {
//         struct mm_struct * vm_mm;       VM area parameters
//         unsigned long vm_start;
//         unsigned long vm_end;
//         pgprot_t vm_page_prot;
//         unsigned short vm_flags;
// };

// The npheap_cmd struct.
//
// struct npheap_cmd {
//     __u64 op;	// 0 for lock, 1 for unlock
//     __u64 offset;
//     __u64 size;
//     void *data;
// };


// npheap_mmap() creates a new mapping in the virtual address space of the
// calling process.
//
// filp: unused
// vma: the memory VMM memory area we are creating or mapping to
//
// returns: 0 if successful [TODO] maybe more
int npheap_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;
    return 0;
}  //npheap_mmap()


// npheap_init() shouldn't be changed.
int npheap_init(void)
{
    int ret;
    if ((ret = misc_register(&npheap_dev)))
        printk(KERN_ERR "Unable to register \"npheap\" misc device\n");
    else
        printk(KERN_ERR "\"npheap\" misc device installed\n");
    return ret;
}  //npheap_init()


// npheap_exit() shouldn't be changed.
void npheap_exit(void)
{
    misc_deregister(&npheap_dev);
}  //npheap_exit()


// npheap_lock() aquires the mutex lock when available.
//
// user_cmd: unused
//
// returns: 0 when lock aquired
long npheap_lock(struct npheap_cmd __user *user_cmd)
{
  mutex_lock(&np_lock);
    return 0;
}  //npheap_lock()


// npheap_unlock() releases the mutex lock and wakes waiting users.
//
// user_cmd: unused
//
// returns: 0 when lock released
long npheap_unlock(struct npheap_cmd __user *user_cmd)
{
  mutex_unlock(&np_lock);
    return 0;
}  //npheap_unlock()


// npheap_getsize() returns the size of the user_cmd.
//
// user_cmd: the struct we need to find the size of in the rb tree
//
// returns: the size of the struct we're looking for or 0 if not found
long npheap_getsize(struct npheap_cmd __user *user_cmd)
{
    return 0;
}  //npheap_getsize()


// npheap_delete() deletes a node into the rb tree.
//
// user_cmd: the struct we need to find and delete/free
//
// returns: 0 if successful [TODO] maybe more
long npheap_delete(struct npheap_cmd __user *user_cmd)
{
    return 0;
}  //npheap_delete()


// npheap_ioctl() shouldn't be changed.
long npheap_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
{
    switch (cmd) {
    case NPHEAP_IOCTL_LOCK:
        return npheap_lock((void __user *) arg);
    case NPHEAP_IOCTL_UNLOCK:
        return npheap_unlock((void __user *) arg);
    case NPHEAP_IOCTL_GETSIZE:
        return npheap_getsize((void __user *) arg);
    case NPHEAP_IOCTL_DELETE:
        return npheap_delete((void __user *) arg);
    default:
        return -ENOTTY;
    }
}  //npheap_ioctl()
