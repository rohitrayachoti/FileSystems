#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <asm/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ROHIT");
MODULE_DESCRIPTION("s2fs kernel module");
MODULE_VERSION("0.01");

#define LFS_MAGIC 0x19980122

static struct inode *lfs_make_inode(struct super_block *sb, int mode) {
	struct inode *ret = new_inode(sb);

	if(ret) {
		ret->i_mode = mode;
		ret->i_uid.val = ret->i_gid.val = 0;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = current_kernel_time64();
	}
	return ret;
}

static int lfs_open(struct inode *inode, struct file *filp) {
	return 0;
}

static ssize_t lfs_read_file(struct file *filp, char __user *buf, size_t count, loff_t *offset) {
	return 0;
}

static ssize_t lfs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset) {
	return 0;
}

static struct file_operations lfs_file_ops = {
	.open = lfs_open,
	.read = lfs_read_file,
	.write = lfs_write_file
};

static struct dentry *lfs_create_file(struct super_block *sb, struct dentry *dir, const char *name, atomic_t *counter) {
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;
	
	qname.name = name;
	qname.len = strlen(name);
	qname.hash = full_name_hash(dir, name, qname.len);

	dentry = d_alloc(dir, &qname);
	if(!dentry)
		goto out;

	inode = lfs_make_inode(sb, S_IFREG | 0644);
	if(!inode) {
		goto out_dput;
	}
	inode->i_fop = &lfs_file_ops;
	inode->i_private = counter;

	d_add(dentry, inode);
	return dentry;

	out_dput:
		dput(dentry);
	out:
		return 0;
}

static struct dentry *lfs_create_dir(struct super_block *sb, struct dentry *parent, const char *name) {
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

	inode = lfs_make_inode(sb, S_IFDIR | 0644);
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

static void lfs_create_files(struct super_block *sb, struct dentry *root) {
	struct dentry *subdir;

	atomic_set(&counter, 0);

	subdir = lfs_create_dir(sb, root, "subdir");

	if(subdir) {
		lfs_create_file(sb, subdir, "subcounter", &subcounter);
	}
}

static struct super_operations lfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static int lfs_fill_super(struct super_block *sb, void *data, int silent) {
	struct inode *root;
	struct dentry *root_dentry;

//	sb->s_blocksize = PAGE_CACHE_SIZE;
//	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &lfs_s_ops;

	root = lfs_make_inode(sb, S_IFDIR | 0755);
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
	
	lfs_create_files(sb, root_dentry);
	return 0;

	out_iput:
		iput(root);
	out:
		return -ENOMEM;
}

static struct dentry *lfs_get_super(struct file_system_type *fst, int flags, const char *devname, void *data) {
	return mount_nodev(fst, flags, data, lfs_fill_super);
}

struct file_system_type lfs_type = {
	.owner		= THIS_MODULE,
	.name		= "s2fs",
	.mount		= lfs_get_super,
	.kill_sb	= kill_litter_super
};

static int __init s2fs_init(void) {
	return register_filesystem(&lfs_type);
}

static void __exit s2fs_exit(void) {
	unregister_filesystem(&lfs_type);
}

module_init(s2fs_init);
module_exit(s2fs_exit);

