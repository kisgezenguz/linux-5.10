#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/stat.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/export.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/highuid.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/security.h>
#include <linux/cred.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/compat.h>

#include <linux/uaccess.h>
#include <asm/unistd.h>

#include "internal.h"
#include "mount.h"

#ifdef MY_ABC_HERE
#include <linux/syno_acl.h>
#endif /* MY_ABC_HERE */

/**
 * generic_fillattr - Fill in the basic attributes from the inode struct
 * @inode: Inode to use as the source
 * @stat: Where to fill in the attributes
 *
 * Fill in the basic attributes in the kstat structure from data that's to be
 * found on the VFS inode structure.  This is the default if no getattr inode
 * operation is supplied.
 */
void generic_fillattr(struct inode *inode, struct kstat *stat)
{
	stat->dev = inode->i_sb->s_dev;
	stat->ino = inode->i_ino;
	stat->mode = inode->i_mode;
	stat->nlink = inode->i_nlink;
	stat->uid = inode->i_uid;
	stat->gid = inode->i_gid;
	stat->rdev = inode->i_rdev;
	stat->size = i_size_read(inode);
	stat->atime = inode->i_atime;
	stat->mtime = inode->i_mtime;
	stat->ctime = inode->i_ctime;
	stat->blksize = i_blocksize(inode);
	stat->blocks = inode->i_blocks;
}
EXPORT_SYMBOL(generic_fillattr);

/**
 * vfs_getattr_nosec - getattr without security checks
 * @path: file to get attributes from
 * @stat: structure to return attributes in
 * @request_mask: STATX_xxx flags indicating what the caller wants
 * @query_flags: Query mode (AT_STATX_SYNC_TYPE)
 *
 * Get attributes without calling security_inode_getattr.
 *
 * Currently the only caller other than vfs_getattr is internal to the
 * filehandle lookup code, which uses only the inode number and returns no
 * attributes to any user.  Any other code probably wants vfs_getattr.
 */
int vfs_getattr_nosec(const struct path *path, struct kstat *stat,
		      u32 request_mask, unsigned int query_flags)
{
	struct inode *inode = d_backing_inode(path->dentry);

	memset(stat, 0, sizeof(*stat));
	stat->result_mask |= STATX_BASIC_STATS;
	query_flags &= AT_STATX_SYNC_TYPE;

	/* allow the fs to override these if it really wants to */
	/* SB_NOATIME means filesystem supplies dummy atime value */
	if (inode->i_sb->s_flags & SB_NOATIME)
		stat->result_mask &= ~STATX_ATIME;

	/*
	 * Note: If you add another clause to set an attribute flag, please
	 * update attributes_mask below.
	 */
	if (IS_AUTOMOUNT(inode))
		stat->attributes |= STATX_ATTR_AUTOMOUNT;

	if (IS_DAX(inode))
		stat->attributes |= STATX_ATTR_DAX;

	stat->attributes_mask |= (STATX_ATTR_AUTOMOUNT |
				  STATX_ATTR_DAX);
#ifdef MY_ABC_HERE
	if (IS_SYNOACL(path->dentry)) {
		int retval = 0;
		if (inode->i_op->getattr)
			retval = inode->i_op->getattr(path, stat, request_mask, query_flags);
		else
			generic_fillattr(inode, stat);

		synoacl_op_to_mode(path->dentry, stat);
		return retval;
	}
#endif /* MY_ABC_HERE */

	if (inode->i_op->getattr)
		return inode->i_op->getattr(path, stat, request_mask,
					    query_flags);

	generic_fillattr(inode, stat);
	return 0;
}
EXPORT_SYMBOL(vfs_getattr_nosec);

/*
 * vfs_getattr - Get the enhanced basic attributes of a file
 * @path: The file of interest
 * @stat: Where to return the statistics
 * @request_mask: STATX_xxx flags indicating what the caller wants
 * @query_flags: Query mode (AT_STATX_SYNC_TYPE)
 *
 * Ask the filesystem for a file's attributes.  The caller must indicate in
 * request_mask and query_flags to indicate what they want.
 *
 * If the file is remote, the filesystem can be forced to update the attributes
 * from the backing store by passing AT_STATX_FORCE_SYNC in query_flags or can
 * suppress the update by passing AT_STATX_DONT_SYNC.
 *
 * Bits must have been set in request_mask to indicate which attributes the
 * caller wants retrieving.  Any such attribute not requested may be returned
 * anyway, but the value may be approximate, and, if remote, may not have been
 * synchronised with the server.
 *
 * 0 will be returned on success, and a -ve error code if unsuccessful.
 */
int vfs_getattr(const struct path *path, struct kstat *stat,
		u32 request_mask, unsigned int query_flags)
{
	int retval;

	retval = security_inode_getattr(path);
	if (retval)
		return retval;
	return vfs_getattr_nosec(path, stat, request_mask, query_flags);
}
EXPORT_SYMBOL(vfs_getattr);

/**
 * vfs_fstat - Get the basic attributes by file descriptor
 * @fd: The file descriptor referring to the file of interest
 * @stat: The result structure to fill in.
 *
 * This function is a wrapper around vfs_getattr().  The main difference is
 * that it uses a file descriptor to determine the file location.
 *
 * 0 will be returned on success, and a -ve error code if unsuccessful.
 */
int vfs_fstat(int fd, struct kstat *stat)
{
	struct fd f;
	int error;

	f = fdget_raw(fd);
	if (!f.file)
		return -EBADF;
	error = vfs_getattr(&f.file->f_path, stat, STATX_BASIC_STATS, 0);
	fdput(f);
	return error;
}

/**
 * vfs_statx - Get basic and extra attributes by filename
 * @dfd: A file descriptor representing the base dir for a relative filename
 * @filename: The name of the file of interest
 * @flags: Flags to control the query
 * @stat: The result structure to fill in.
 * @request_mask: STATX_xxx flags indicating what the caller wants
 *
 * This function is a wrapper around vfs_getattr().  The main difference is
 * that it uses a filename and base directory to determine the file location.
 * Additionally, the use of AT_SYMLINK_NOFOLLOW in flags will prevent a symlink
 * at the given name from being referenced.
 *
 * 0 will be returned on success, and a -ve error code if unsuccessful.
 */
static int vfs_statx(int dfd, const char __user *filename, int flags,
	      struct kstat *stat, u32 request_mask)
{
	struct path path;
	unsigned lookup_flags = 0;
	int error;

	if (flags & ~(AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT | AT_EMPTY_PATH |
		      AT_STATX_SYNC_TYPE))
		return -EINVAL;

	if (!(flags & AT_SYMLINK_NOFOLLOW))
		lookup_flags |= LOOKUP_FOLLOW;
	if (!(flags & AT_NO_AUTOMOUNT))
		lookup_flags |= LOOKUP_AUTOMOUNT;
	if (flags & AT_EMPTY_PATH)
		lookup_flags |= LOOKUP_EMPTY;

retry:
	error = user_path_at(dfd, filename, lookup_flags, &path);
	if (error)
		goto out;

	error = vfs_getattr(&path, stat, request_mask, flags);
	stat->mnt_id = real_mount(path.mnt)->mnt_id;
	stat->result_mask |= STATX_MNT_ID;
	if (path.mnt->mnt_root == path.dentry)
		stat->attributes |= STATX_ATTR_MOUNT_ROOT;
	stat->attributes_mask |= STATX_ATTR_MOUNT_ROOT;
	path_put(&path);
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
out:
	return error;
}

int vfs_fstatat(int dfd, const char __user *filename,
			      struct kstat *stat, int flags)
{
	return vfs_statx(dfd, filename, flags | AT_NO_AUTOMOUNT,
			 stat, STATX_BASIC_STATS);
}

#ifdef MY_ABC_HERE
int __always_inline syno_vfs_getattr(struct path *path, struct kstat *stat,
		u32 request_mask, unsigned int query_flags, unsigned int syno_flags)
{
	int error = 0;

	error = vfs_getattr(path, stat, request_mask, query_flags);
	if (error)
		goto out;

	error = syno_op_getattr(path->dentry, stat, syno_flags);
out:
	return error;
}

// copy from vfs_fstat
int syno_vfs_fstat(unsigned int fd, struct kstat *stat, unsigned int syno_flags)
{
	struct fd f;
	int error;

	f = fdget_raw(fd);
	if (!f.file)
		return -EBADF;
	error = syno_vfs_getattr(&f.file->f_path, stat,
			STATX_BASIC_STATS, 0, syno_flags);
	fdput(f);
	return error;
}
EXPORT_SYMBOL(syno_vfs_fstat);

// copy from vfs_statx
int syno_vfs_statx(const char __user *filename, struct kstat *stat,
		unsigned int query_flags, unsigned int syno_flags)
{
	struct path path;
	unsigned lookup_flags = 0;
	int error;

	if (query_flags & ~(AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT |
	                   AT_EMPTY_PATH | AT_STATX_SYNC_TYPE))
		return -EINVAL;

	if (!(query_flags & AT_SYMLINK_NOFOLLOW))
		lookup_flags |= LOOKUP_FOLLOW;
	if (!(query_flags & AT_NO_AUTOMOUNT))
		lookup_flags |= LOOKUP_AUTOMOUNT;
	if (query_flags & AT_EMPTY_PATH)
		lookup_flags |= LOOKUP_EMPTY;

retry:
	error = user_path_at(AT_FDCWD, filename, lookup_flags, &path);
	if (error)
		goto out;

	error = syno_vfs_getattr(&path, stat, STATX_BASIC_STATS, query_flags, syno_flags);
	stat->mnt_id = real_mount(path.mnt)->mnt_id;
	stat->result_mask |= STATX_MNT_ID;
	if (path.mnt->mnt_root == path.dentry)
		stat->attributes |= STATX_ATTR_MOUNT_ROOT;
	stat->attributes_mask |= STATX_ATTR_MOUNT_ROOT;
	path_put(&path);
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
out:
	return error;
}

// copy from vfs_fstatat
int syno_vfs_fstatat(const char __user *filename, struct kstat *stat,
		unsigned int query_flags, unsigned int syno_flags)
{
	return syno_vfs_statx(filename, stat, query_flags | AT_NO_AUTOMOUNT,
			syno_flags);
}
EXPORT_SYMBOL(syno_vfs_fstatat);
#endif /* MY_ABC_HERE */

#ifdef __ARCH_WANT_OLD_STAT

/*
 * For backward compatibility?  Maybe this should be moved
 * into arch/i386 instead?
 */
static int cp_old_stat(struct kstat *stat, struct __old_kernel_stat __user * statbuf)
{
	static int warncount = 5;
	struct __old_kernel_stat tmp;

	if (warncount > 0) {
		warncount--;
		printk(KERN_WARNING "VFS: Warning: %s using old stat() call. Recompile your binary.\n",
			current->comm);
	} else if (warncount < 0) {
		/* it's laughable, but... */
		warncount = 0;
	}

	memset(&tmp, 0, sizeof(struct __old_kernel_stat));
	tmp.st_dev = old_encode_dev(stat->dev);
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, from_kuid_munged(current_user_ns(), stat->uid));
	SET_GID(tmp.st_gid, from_kgid_munged(current_user_ns(), stat->gid));
	tmp.st_rdev = old_encode_dev(stat->rdev);
#if BITS_PER_LONG == 32
	if (stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
#endif
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_ctime = stat->ctime.tv_sec;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(stat, const char __user *, filename,
		struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_stat(filename, &stat);
	if (error)
		return error;

	return cp_old_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(lstat, const char __user *, filename,
		struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;

	return cp_old_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(fstat, unsigned int, fd, struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_old_stat(&stat, statbuf);

	return error;
}

#endif /* __ARCH_WANT_OLD_STAT */

#ifdef __ARCH_WANT_NEW_STAT

#if BITS_PER_LONG == 32
#  define choose_32_64(a,b) a
#else
#  define choose_32_64(a,b) b
#endif

#define valid_dev(x)  choose_32_64(old_valid_dev(x),true)
#define encode_dev(x) choose_32_64(old_encode_dev,new_encode_dev)(x)

#ifndef INIT_STRUCT_STAT_PADDING
#  define INIT_STRUCT_STAT_PADDING(st) memset(&st, 0, sizeof(st))
#endif

static int cp_new_stat(struct kstat *stat, struct stat __user *statbuf)
{
	struct stat tmp;

	if (!valid_dev(stat->dev) || !valid_dev(stat->rdev))
		return -EOVERFLOW;
#if BITS_PER_LONG == 32
	if (stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
#endif

	INIT_STRUCT_STAT_PADDING(tmp);
	tmp.st_dev = encode_dev(stat->dev);
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, from_kuid_munged(current_user_ns(), stat->uid));
	SET_GID(tmp.st_gid, from_kgid_munged(current_user_ns(), stat->gid));
	tmp.st_rdev = encode_dev(stat->rdev);
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_ctime = stat->ctime.tv_sec;
#ifdef STAT_HAVE_NSEC
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
#endif
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(newstat, const char __user *, filename,
		struct stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_stat(filename, &stat);

	if (error)
		return error;
	return cp_new_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(newlstat, const char __user *, filename,
		struct stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;

	return cp_new_stat(&stat, statbuf);
}

#if !defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_SYS_NEWFSTATAT)
SYSCALL_DEFINE4(newfstatat, int, dfd, const char __user *, filename,
		struct stat __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_new_stat(&stat, statbuf);
}
#endif

SYSCALL_DEFINE2(newfstat, unsigned int, fd, struct stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_new_stat(&stat, statbuf);

	return error;
}
#endif

static int do_readlinkat(int dfd, const char __user *pathname,
			 char __user *buf, int bufsiz)
{
	struct path path;
	int error;
	int empty = 0;
	unsigned int lookup_flags = LOOKUP_EMPTY;

	if (bufsiz <= 0)
		return -EINVAL;

retry:
	error = user_path_at_empty(dfd, pathname, lookup_flags, &path, &empty);
	if (!error) {
		struct inode *inode = d_backing_inode(path.dentry);

		error = empty ? -ENOENT : -EINVAL;
		/*
		 * AFS mountpoints allow readlink(2) but are not symlinks
		 */
		if (d_is_symlink(path.dentry) || inode->i_op->readlink) {
			error = security_inode_readlink(path.dentry);
			if (!error) {
				touch_atime(&path);
				error = vfs_readlink(path.dentry, buf, bufsiz);
			}
		}
		path_put(&path);
		if (retry_estale(error, lookup_flags)) {
			lookup_flags |= LOOKUP_REVAL;
			goto retry;
		}
	}
	return error;
}

SYSCALL_DEFINE4(readlinkat, int, dfd, const char __user *, pathname,
		char __user *, buf, int, bufsiz)
{
	return do_readlinkat(dfd, pathname, buf, bufsiz);
}

SYSCALL_DEFINE3(readlink, const char __user *, path, char __user *, buf,
		int, bufsiz)
{
	return do_readlinkat(AT_FDCWD, path, buf, bufsiz);
}


/* ---------- LFS-64 ----------- */
#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)

#ifndef INIT_STRUCT_STAT64_PADDING
#  define INIT_STRUCT_STAT64_PADDING(st) memset(&st, 0, sizeof(st))
#endif

static long cp_new_stat64(struct kstat *stat, struct stat64 __user *statbuf)
{
	struct stat64 tmp;

	INIT_STRUCT_STAT64_PADDING(tmp);
#ifdef CONFIG_MIPS
	/* mips has weird padding, so we don't get 64 bits there */
	tmp.st_dev = new_encode_dev(stat->dev);
	tmp.st_rdev = new_encode_dev(stat->rdev);
#else
	tmp.st_dev = huge_encode_dev(stat->dev);
	tmp.st_rdev = huge_encode_dev(stat->rdev);
#endif
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
#ifdef STAT64_HAS_BROKEN_ST_INO
	tmp.__st_ino = stat->ino;
#endif
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	tmp.st_uid = from_kuid_munged(current_user_ns(), stat->uid);
	tmp.st_gid = from_kgid_munged(current_user_ns(), stat->gid);
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime = stat->ctime.tv_sec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
	tmp.st_size = stat->size;
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(stat64, const char __user *, filename,
		struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_stat(filename, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE2(lstat64, const char __user *, filename,
		struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_lstat(filename, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE2(fstat64, unsigned long, fd, struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE4(fstatat64, int, dfd, const char __user *, filename,
		struct stat64 __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_new_stat64(&stat, statbuf);
}
#endif /* __ARCH_WANT_STAT64 || __ARCH_WANT_COMPAT_STAT64 */

static noinline_for_stack int
cp_statx(const struct kstat *stat, struct statx __user *buffer)
{
	struct statx tmp;

	memset(&tmp, 0, sizeof(tmp));

	tmp.stx_mask = stat->result_mask;
	tmp.stx_blksize = stat->blksize;
	tmp.stx_attributes = stat->attributes;
	tmp.stx_nlink = stat->nlink;
	tmp.stx_uid = from_kuid_munged(current_user_ns(), stat->uid);
	tmp.stx_gid = from_kgid_munged(current_user_ns(), stat->gid);
	tmp.stx_mode = stat->mode;
	tmp.stx_ino = stat->ino;
	tmp.stx_size = stat->size;
	tmp.stx_blocks = stat->blocks;
	tmp.stx_attributes_mask = stat->attributes_mask;
	tmp.stx_atime.tv_sec = stat->atime.tv_sec;
	tmp.stx_atime.tv_nsec = stat->atime.tv_nsec;
	tmp.stx_btime.tv_sec = stat->btime.tv_sec;
	tmp.stx_btime.tv_nsec = stat->btime.tv_nsec;
	tmp.stx_ctime.tv_sec = stat->ctime.tv_sec;
	tmp.stx_ctime.tv_nsec = stat->ctime.tv_nsec;
	tmp.stx_mtime.tv_sec = stat->mtime.tv_sec;
	tmp.stx_mtime.tv_nsec = stat->mtime.tv_nsec;
	tmp.stx_rdev_major = MAJOR(stat->rdev);
	tmp.stx_rdev_minor = MINOR(stat->rdev);
	tmp.stx_dev_major = MAJOR(stat->dev);
	tmp.stx_dev_minor = MINOR(stat->dev);
	tmp.stx_mnt_id = stat->mnt_id;

	return copy_to_user(buffer, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}

int do_statx(int dfd, const char __user *filename, unsigned flags,
	     unsigned int mask, struct statx __user *buffer)
{
	struct kstat stat;
	int error;

	if (mask & STATX__RESERVED)
		return -EINVAL;
	if ((flags & AT_STATX_SYNC_TYPE) == AT_STATX_SYNC_TYPE)
		return -EINVAL;

	error = vfs_statx(dfd, filename, flags, &stat, mask);
	if (error)
		return error;

	return cp_statx(&stat, buffer);
}

/**
 * sys_statx - System call to get enhanced stats
 * @dfd: Base directory to pathwalk from *or* fd to stat.
 * @filename: File to stat or "" with AT_EMPTY_PATH
 * @flags: AT_* flags to control pathwalk.
 * @mask: Parts of statx struct actually required.
 * @buffer: Result buffer.
 *
 * Note that fstat() can be emulated by setting dfd to the fd of interest,
 * supplying "" as the filename and setting AT_EMPTY_PATH in the flags.
 */
SYSCALL_DEFINE5(statx,
		int, dfd, const char __user *, filename, unsigned, flags,
		unsigned int, mask,
		struct statx __user *, buffer)
{
	return do_statx(dfd, filename, flags, mask, buffer);
}

#ifdef CONFIG_COMPAT
static int cp_compat_stat(struct kstat *stat, struct compat_stat __user *ubuf)
{
	struct compat_stat tmp;

	if (!old_valid_dev(stat->dev) || !old_valid_dev(stat->rdev))
		return -EOVERFLOW;

	memset(&tmp, 0, sizeof(tmp));
	tmp.st_dev = old_encode_dev(stat->dev);
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, from_kuid_munged(current_user_ns(), stat->uid));
	SET_GID(tmp.st_gid, from_kgid_munged(current_user_ns(), stat->gid));
	tmp.st_rdev = old_encode_dev(stat->rdev);
	if ((u64) stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime = stat->ctime.tv_sec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(ubuf, &tmp, sizeof(tmp)) ? -EFAULT : 0;
}

COMPAT_SYSCALL_DEFINE2(newstat, const char __user *, filename,
		       struct compat_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_stat(filename, &stat);
	if (error)
		return error;
	return cp_compat_stat(&stat, statbuf);
}

COMPAT_SYSCALL_DEFINE2(newlstat, const char __user *, filename,
		       struct compat_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;
	return cp_compat_stat(&stat, statbuf);
}

#ifndef __ARCH_WANT_STAT64
COMPAT_SYSCALL_DEFINE4(newfstatat, unsigned int, dfd,
		       const char __user *, filename,
		       struct compat_stat __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_compat_stat(&stat, statbuf);
}
#endif

COMPAT_SYSCALL_DEFINE2(newfstat, unsigned int, fd,
		       struct compat_stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_compat_stat(&stat, statbuf);
	return error;
}
#endif

#ifdef MY_ABC_HERE
/* This stat is used by caseless protocol.
 * The filename will be convert to real filename and return to user space.
 * In caller, the length of filename must equal or be larger than SYNO_SMB_PSTRING_LEN.
*/
int __syno_caseless_stat(char __user * filename, bool no_follow,
		struct kstat *stat, unsigned int syno_flags)
{
	struct path path;
	int error;
	int f;
	char *real_filename = NULL;
	int real_filename_len = 0;

	real_filename = kmalloc(SYNO_SMB_PSTRING_LEN, GFP_KERNEL);
	if (!real_filename)
		return -ENOMEM;

	if (no_follow)
		f = LOOKUP_CASELESS_COMPARE;
	else
		f = LOOKUP_FOLLOW|LOOKUP_CASELESS_COMPARE;

	error = syno_user_path_at(AT_FDCWD, filename, f, &path,
				  &real_filename, &real_filename_len);
	if (!error) {
#ifdef MY_ABC_HERE
		error = syno_vfs_getattr(&path, stat, STATX_BASIC_STATS, 0, syno_flags);
#else
		error = vfs_getattr(&path, stat, STATX_BASIC_STATS, 0);
#endif /* MY_ABC_HERE */
		path_put(&path);
		if (real_filename_len) {
			error = copy_to_user(filename,
					     real_filename, real_filename_len) ? -EFAULT : error;
		}
	}

	kfree(real_filename);
	return error;
}
EXPORT_SYMBOL(__syno_caseless_stat);

#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
SYSCALL_DEFINE2(syno_caseless_stat, char __user *, filename, struct stat __user *, statbuf)
{
#ifdef MY_ABC_HERE
	long error = -1;
	struct kstat stat;

	memset(&stat, 0, sizeof(stat));
	error = __syno_caseless_stat(filename, false, &stat, 0);
	if (!error)
		error = cp_new_stat(&stat, statbuf);

	return error;
#else
	return -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
}

SYSCALL_DEFINE2(syno_caseless_lstat, char __user *, filename, struct stat __user *, statbuf)
{
#ifdef MY_ABC_HERE
	long error = -1;
	struct kstat stat;

	memset(&stat, 0, sizeof(stat));
	error = __syno_caseless_stat(filename, true, &stat, 0);
	if (!error)
		error = cp_new_stat(&stat, statbuf);

	return error;
#else
	return -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
static int cp_to_synostat(struct kstat *kst, unsigned int syno_flags,
		struct SYNOSTAT __user *synostat)
{
	int error = -EFAULT;

	if (!synostat) {
		error = -EINVAL;
		goto out;
	}

	if (syno_flags & SYNOST_STAT) {
		error = cp_new_stat(kst, &synostat->st);
		if(error)
			goto out;
	}

	error = __put_user(kst->syno_flags, &synostat->ext.flags);
	if (error)
		goto out;

	if (syno_flags & SYNOST_COMPRESSION) {
		if (copy_to_user(&synostat->ext.compressed,
		                 &kst->syno_compressed,
		                 sizeof(synostat->ext.compressed))) {
			error = -EFAULT;
			goto out;
		}
	}

#ifdef MY_ABC_HERE
	if (syno_flags & SYNOST_ARCHIVE_BIT) {
		error = __put_user(kst->syno_archive_bit,
				&synostat->ext.archive_bit);
		if (error)
			goto out;
	}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
	if (syno_flags & SYNOST_ARCHIVE_VER) {
		error = __put_user(kst->syno_archive_version,
				&synostat->ext.archive_version);
		if (error)
			goto out;
	}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
	if (syno_flags & SYNOST_CREATE_TIME) {
		struct __kernel_old_timespec tmp;

		tmp.tv_sec = kst->syno_create_time.tv_sec;
		tmp.tv_nsec = kst->syno_create_time.tv_nsec;
		if (copy_to_user(&synostat->ext.create_time, &tmp, sizeof(tmp))) {
			error = -EFAULT;
			goto out;
		}
	}
#endif /* MY_ABC_HERE */

	error = 0;
out:
	return error;
}

static int do_syno_stat(char __user *filename, bool no_follow,
		unsigned int syno_flags, struct SYNOSTAT __user *synostat)
{
	long error = -EINVAL;
	struct kstat kst;

	memset(&kst, 0, sizeof(kst));
	if (syno_flags & SYNOST_IS_CASELESS) {
#ifdef MY_ABC_HERE
		error = __syno_caseless_stat(filename, no_follow, &kst, syno_flags);
#else
		error = -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
	} else {
		if (no_follow)
			error = syno_vfs_fstatat(filename, &kst, AT_SYMLINK_NOFOLLOW, syno_flags);
		else
			error = syno_vfs_fstatat(filename, &kst, 0, syno_flags);
	}

	if (error)
		goto out;

	error = cp_to_synostat(&kst, syno_flags, synostat);
out:
	return error;
}
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
SYSCALL_DEFINE3(syno_stat, char __user *, filename, unsigned int, syno_flags, struct SYNOSTAT __user *, synostat)
{
#ifdef MY_ABC_HERE
	return do_syno_stat(filename, false, syno_flags, synostat);
#else
	return -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
}

SYSCALL_DEFINE3(syno_fstat, unsigned int, fd, unsigned int, syno_flags, struct SYNOSTAT __user *, synostat)
{
#ifdef MY_ABC_HERE
	int error;
        struct kstat kst;

        memset(&kst, 0, sizeof(kst));
        error = syno_vfs_fstat(fd, &kst, syno_flags);
        if (error)
                return error;

        return cp_to_synostat(&kst, syno_flags, synostat);
#else
	return -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
}

SYSCALL_DEFINE3(syno_lstat, char __user *, filename, unsigned int, syno_flags, struct SYNOSTAT __user *, synostat)
{
#ifdef MY_ABC_HERE
	return do_syno_stat(filename, true, syno_flags, synostat);
#else
	return -EOPNOTSUPP;
#endif /* MY_ABC_HERE */
}
#endif /* MY_ABC_HERE */

/* Caller is here responsible for sufficient locking (ie. inode->i_lock) */
void __inode_add_bytes(struct inode *inode, loff_t bytes)
{
	inode->i_blocks += bytes >> 9;
	bytes &= 511;
	inode->i_bytes += bytes;
	if (inode->i_bytes >= 512) {
		inode->i_blocks++;
		inode->i_bytes -= 512;
	}
}
EXPORT_SYMBOL(__inode_add_bytes);

void inode_add_bytes(struct inode *inode, loff_t bytes)
{
	spin_lock(&inode->i_lock);
	__inode_add_bytes(inode, bytes);
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(inode_add_bytes);

void __inode_sub_bytes(struct inode *inode, loff_t bytes)
{
	inode->i_blocks -= bytes >> 9;
	bytes &= 511;
	if (inode->i_bytes < bytes) {
		inode->i_blocks--;
		inode->i_bytes += 512;
	}
	inode->i_bytes -= bytes;
}

EXPORT_SYMBOL(__inode_sub_bytes);

void inode_sub_bytes(struct inode *inode, loff_t bytes)
{
	spin_lock(&inode->i_lock);
	__inode_sub_bytes(inode, bytes);
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(inode_sub_bytes);

loff_t inode_get_bytes(struct inode *inode)
{
	loff_t ret;

	spin_lock(&inode->i_lock);
	ret = __inode_get_bytes(inode);
	spin_unlock(&inode->i_lock);
	return ret;
}

EXPORT_SYMBOL(inode_get_bytes);

void inode_set_bytes(struct inode *inode, loff_t bytes)
{
	/* Caller is here responsible for sufficient locking
	 * (ie. inode->i_lock) */
	inode->i_blocks = bytes >> 9;
	inode->i_bytes = bytes & 511;
}

EXPORT_SYMBOL(inode_set_bytes);

#ifdef MY_ABC_HERE
int vfs_quota_query(struct file *file, u64 *used, u64 *reserved, u64 *limit)
{
	if (!file->f_op->quota_query)
		return -EOPNOTSUPP;
	return file->f_op->quota_query(file, used, reserved, limit);
}
EXPORT_SYMBOL(vfs_quota_query);
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
int vfs_syno_space_usage(struct file *file, struct syno_space_usage_info *info)
{
	if (!file->f_op->syno_space_usage)
		return -EOPNOTSUPP;
	return file->f_op->syno_space_usage(file, info);
}
EXPORT_SYMBOL(vfs_syno_space_usage);
#endif /* MY_ABC_HERE */

