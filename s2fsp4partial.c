#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/init_task.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ROHIT");
MODULE_DESCRIPTION("s2fs kernel module");
MODULE_VERSION("0.01");

#define S2FS_MAGIC 0x19980122

struct task_struct *process;
struct task_struct *pchild;

static struct inode *s2fs_make_inode(struct super_block *sb, int mode) {
	struct inode *ret = new_inode(sb);

	if(ret) {
		ret->i_mode = mode;
		ret->i_uid.val = ret->i_gid.val = 0;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = current_kernel_time64();
	}
	return ret;
}

static int s2fs_open(struct inode *inode, struct file *filp) {
	filp->private_data = inode->i_private;
	return 0;
}

#define TMPSIZE 20

static ssize_t s2fs_read_file(struct file *filp, char __user *buf, size_t count, loff_t *offset) {
	
	int len;
	char tmp[TMPSIZE];

	len = snprintf(tmp, TMPSIZE, "Hello World!\n");
	if(*offset > len) {
		return 0;
	}
	if (count > len - *offset) {
		count = len - *offset;
	}

	if (copy_to_user(buf, tmp + *offset, count)) {
		return -EFAULT;
	}
	*offset += count;
	return count;
}

static ssize_t s2fs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset) {
	atomic_t *counter = (atomic_t *) filp->private_data;
	char tmp[TMPSIZE];
	
	if (*offset != 0) {
		return -EINVAL;
	}
	
	if (count >= TMPSIZE) {
		return -EINVAL;
	}

	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count)) {
		return -EFAULT;
	}
	
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	
	return count;
}

static struct file_operations s2fs_file_ops = {
	.open = s2fs_open,
	.read = s2fs_read_file,
	.write = s2fs_write_file
};

static struct dentry *s2fs_create_file(struct super_block *sb, struct dentry *dir, const char *name, atomic_t *counter) {
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;
	
	qname.name = name;
	qname.len = strlen(name);
	qname.hash = full_name_hash(dir, name, qname.len);

	dentry = d_alloc(dir, &qname);
	if(!dentry)
		goto out;

	inode = s2fs_make_inode(sb, S_IFREG | 0644);
	if(!inode) {
		goto out_dput;
	}
	inode->i_fop = &s2fs_file_ops;
	inode->i_private = counter;

	d_add(dentry, inode);
	return dentry;

	out_dput:
		dput(dentry);
	out:
		return 0;
}

static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *name) {
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen(name);
	qname.hash = full_name_hash(parent, name, qname.len);
	dentry = d_alloc(parent, &qname);
	if(!dentry) {
		goto out;
	}

	inode = s2fs_make_inode(sb, S_IFDIR | 0644);
	if(!inode) {
		goto out_dput;
	}
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

	out_dput:
		dput(dentry);
	out:
		return 0;
}

static atomic_t counter, subcounter;

static void s2fs_create_files(struct super_block *sb, struct dentry *root, struct task_struct *process) {
	struct dentry *subdir;

	atomic_set(&counter, 0);

	char fileName[10];

        sprintf(fileName, "%d", process->pid);

	struct list_head *list;

	if (!list_empty(&process->children)) {
                subdir = s2fs_create_dir(sb, root, fileName);
		s2fs_create_file(sb, subdir, fileName, &subcounter);
        }
	else {
                s2fs_create_file(sb, root, fileName, &subcounter);
        }

	// Iterating the list of all children
        list_for_each(list, &process->children) {

                // Reading child process
                pchild = list_entry(list, struct task_struct, sibling);

                // Recursive call for child process with incremented indentation level
		s2fs_create_files(sb, subdir, pchild);
        }
//s2fs_create_file(sb, subdir, fileName, &subcounter);
}

static struct super_operations s2fs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static int s2fs_fill_super(struct super_block *sb, void *data, int silent) {
	struct inode *root;
	struct dentry *root_dentry;

//	sb->s_blocksize = PAGE_CACHE_SIZE;
//	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = S2FS_MAGIC;
	sb->s_op = &s2fs_s_ops;

	root = s2fs_make_inode(sb, S_IFDIR | 0755);
	if(!root) {
		goto out;
	}
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_make_root(root);
	if(!root_dentry) {
		goto out_iput;
	}
	sb->s_root = root_dentry;

	for(process=current; process!=&init_task; process=process->parent);
	
	s2fs_create_files(sb, root_dentry, process);
	return 0;

	out_iput:
		iput(root);
	out:
		return -ENOMEM;
}

static struct dentry *s2fs_get_super(struct file_system_type *fst, int flags, const char *devname, void *data) {
	return mount_nodev(fst, flags, data, s2fs_fill_super);
}

struct file_system_type s2fs_type = {
	.owner		= THIS_MODULE,
	.name		= "s2fs",
	.mount		= s2fs_get_super,
	.kill_sb	= kill_litter_super
};

static int __init s2fs_init(void) {
	return register_filesystem(&s2fs_type);
}

static void __exit s2fs_exit(void) {
	unregister_filesystem(&s2fs_type);
}

module_init(s2fs_init);
module_exit(s2fs_exit);

