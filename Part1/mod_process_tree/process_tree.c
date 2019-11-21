#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ROHIT");
MODULE_DESCRIPTION("Display Process Hierarchy");
MODULE_VERSION("0.01");

int ppid = 0;
int level = 0;

static void display_hierarchy(struct task_struct *process) {
	struct task_struct *pchild;
	struct list_head *list;
	for_each_process(process) {
		if(process->pid == ppid) {
			level++;
		}
		else {
			level = 1;
                	printk(KERN_INFO "%s[%d]\n", process->comm, process->pid);
		}
                list_for_each(list, &process->children) {
                        pchild = list_entry(list, struct task_struct, sibling);
			ppid = pchild->pid;
                        printk(KERN_INFO "%*c%s[%d]\n", level, ' ', pchild->comm, pchild->pid);
                }
        }
}

static int __init run_modprocesstree(void) {
	printk(KERN_INFO "\nProcessTree: Loading module\n");
	struct task_struct *process;
	display_hierarchy(process);
	return 0;
}

static void __exit exit_modprocesstree(void) {
	printk(KERN_INFO "\nProcessTree: Removing module\n");
}

module_init(run_modprocesstree);
module_exit(exit_modprocesstree);
