# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_BTRFS_FS) := btrfs.o

btrfs-y += super.o ctree.o extent-tree.o print-tree.o root-tree.o dir-item.o \
	   file-item.o inode-item.o inode-map.o disk-io.o \
	   transaction.o inode.o file.o tree-defrag.o \
	   extent_map.o sysfs.o struct-funcs.o xattr.o ordered-data.o \
	   extent_io.o volumes.o async-thread.o ioctl.o locking.o orphan.o \
	   export.o tree-log.o free-space-cache.o zlib.o lzo.o zstd.o \
	   compression.o delayed-ref.o relocation.o delayed-inode.o scrub.o \
	   reada.o backref.o ulist.o qgroup.o send.o dev-replace.o raid56.o \
	   uuid-tree.o props.o free-space-tree.o tree-checker.o space-info.o \
	   block-rsv.o delalloc-space.o block-group.o discard.o reflink.o

btrfs-$(CONFIG_BTRFS_FS_POSIX_ACL) += acl.o
btrfs-$(CONFIG_SYNO_BTRFS_WINACL) += syno_acl.o
btrfs-$(CONFIG_BTRFS_FS_CHECK_INTEGRITY) += check-integrity.o
btrfs-$(CONFIG_BTRFS_FS_REF_VERIFY) += ref-verify.o

btrfs-$(CONFIG_SYNO_BTRFS_FREE_SPACE_ANALYZE) += free-space-analyze.o
btrfs-$(CONFIG_SYNO_BTRFS_SNAPSHOT_SIZE_CALCULATION) += snapshot-size-query.o
btrfs-$(CONFIG_SYNO_BTRFS_FEATURE_SPACE_USAGE) += syno-extent-usage.o
btrfs-$(CONFIG_SYNO_BTRFS_SYNO_QUOTA) += usrquota.o
btrfs-$(CONFIG_SYNO_BTRFS_LOCKER) += syno-locker.o

btrfs-$(CONFIG_BTRFS_FS_RUN_SANITY_TESTS) += tests/free-space-tests.o \
	tests/extent-buffer-tests.o tests/btrfs-tests.o \
	tests/extent-io-tests.o tests/inode-tests.o tests/qgroup-tests.o \
	tests/free-space-tree-tests.o tests/extent-map-tests.o

btrfs-$(CONFIG_SYNO_BTRFS_FEATURE_TREE) += syno-feat-tree.o
btrfs-$(CONFIG_SYNO_BTRFS_RBD_META) += syno-rbd-meta.o
