#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2008 Oracle.  All rights reserved.
 */

#ifndef BTRFS_DELAYED_REF_H
#define BTRFS_DELAYED_REF_H

#include <linux/refcount.h>
#ifdef MY_ABC_HERE
#include "btrfs_inode.h"
#endif /* MY_ABC_HERE */

/* these are the possible values of struct btrfs_delayed_ref_node->action */
#define BTRFS_ADD_DELAYED_REF    1 /* add one backref to the tree */
#define BTRFS_DROP_DELAYED_REF   2 /* delete one backref from the tree */
#define BTRFS_ADD_DELAYED_EXTENT 3 /* record a full extent allocation */
#define BTRFS_UPDATE_DELAYED_HEAD 4 /* not changing ref count on head ref */

struct btrfs_delayed_ref_node {
	struct rb_node ref_node;
	/*
	 * If action is BTRFS_ADD_DELAYED_REF, also link this node to
	 * ref_head->ref_add_list, then we do not need to iterate the
	 * whole ref_head->ref_list to find BTRFS_ADD_DELAYED_REF nodes.
	 */
	struct list_head add_list;

#ifdef MY_ABC_HERE
	struct list_head syno_list;
#endif /* MY_ABC_HERE */

	/* the starting bytenr of the extent */
	u64 bytenr;

	/* the size of the extent */
	u64 num_bytes;

	/* seq number to keep track of insertion order */
	u64 seq;

	/* ref count on this data structure */
	refcount_t refs;

	/*
	 * how many refs is this entry adding or deleting.  For
	 * head refs, this may be a negative number because it is keeping
	 * track of the total mods done to the reference count.
	 * For individual refs, this will always be a positive number
	 *
	 * It may be more than one, since it is possible for a single
	 * parent to have more than one ref on an extent
	 */
	int ref_mod;

	unsigned int action:8;
	unsigned int type:8;
	/* is this node still in the rbtree? */
	unsigned int is_head:1;
	unsigned int in_tree:1;
};

struct btrfs_delayed_extent_op {
	struct btrfs_disk_key key;
	u8 level;
	bool update_key;
	bool update_flags;
	bool is_data;
	u64 flags_to_set;
};

/*
 * the head refs are used to hold a lock on a given extent, which allows us
 * to make sure that only one process is running the delayed refs
 * at a time for a single extent.  They also store the sum of all the
 * reference count modifications we've queued up.
 */
struct btrfs_delayed_ref_head {
	u64 bytenr;
	u64 num_bytes;
	refcount_t refs;
	/*
	 * the mutex is held while running the refs, and it is also
	 * held when checking the sum of reference modifications.
	 */
	struct mutex mutex;

	spinlock_t lock;
	struct rb_root_cached ref_tree;
	/* accumulate add BTRFS_ADD_DELAYED_REF nodes to this ref_add_list. */
	struct list_head ref_add_list;

#ifdef MY_ABC_HERE
	struct list_head ref_syno_list;
#endif /* MY_ABC_HERE */

	struct rb_node href_node;

	struct btrfs_delayed_extent_op *extent_op;

	/*
	 * This is used to track the final ref_mod from all the refs associated
	 * with this head ref, this is not adjusted as delayed refs are run,
	 * this is meant to track if we need to do the csum accounting or not.
	 */
	int total_ref_mod;

	/*
	 * This is the current outstanding mod references for this bytenr.  This
	 * is used with lookup_extent_info to get an accurate reference count
	 * for a bytenr, so it is adjusted as delayed refs are run so that any
	 * on disk reference count + ref_mod is accurate.
	 */
	int ref_mod;

	/*
	 * when a new extent is allocated, it is just reserved in memory
	 * The actual extent isn't inserted into the extent allocation tree
	 * until the delayed ref is processed.  must_insert_reserved is
	 * used to flag a delayed ref so the accounting can be updated
	 * when a full insert is done.
	 *
	 * It is possible the extent will be freed before it is ever
	 * inserted into the extent allocation tree.  In this case
	 * we need to update the in ram accounting to properly reflect
	 * the free has happened.
	 */
	unsigned int must_insert_reserved:1;
	unsigned int is_data:1;
	unsigned int is_system:1;
	unsigned int processing:1;
#ifdef MY_ABC_HERE
	unsigned int syno_usage;
#endif /* MY_ABC_HERE */
};

struct btrfs_delayed_tree_ref {
	struct btrfs_delayed_ref_node node;
	u64 root;
	u64 parent;
	int level;
};

struct btrfs_delayed_data_ref {
	struct btrfs_delayed_ref_node node;
	u64 root;
	u64 parent;
	u64 objectid;
	u64 offset;
#ifdef MY_ABC_HERE
	unsigned int syno_usage;
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
	// Used by quota v1.
	u64 reserved;
	u64 ram_bytes;
	u64 uid; // Only used in I_FREEING or I_WILL_FREE case.
	struct inode *inode;
	bool skip_qgroup;
#endif /* MY_ABC_HERE */
};

struct btrfs_delayed_ref_root {
	/* head ref rbtree */
	struct rb_root_cached href_root;

	/* dirty extent records */
	struct rb_root dirty_extent_root;

	/* this spin lock protects the rbtree and the entries inside */
	spinlock_t lock;

	/* how many delayed ref updates we've queued, used by the
	 * throttling code
	 */
	atomic_t num_entries;

	/* total number of head nodes in tree */
	unsigned long num_heads;

	/* total number of head nodes ready for processing */
	unsigned long num_heads_ready;

#ifdef MY_ABC_HERE
	/* how many delayed DATA ref with SYNO_USAGE we've queued
	 * The calculation site should be consistent with num_entries but
	 * only count DATA refs with SYNO_USAGE.
	 */
	atomic_t num_syno_usage_entries;

	/* total number of head nodes that might have delayed refs with syno_usage
	 * The calculation site should be consistent with num_heads_ready but
	 * only count DATA refs with SYNO_USAGE.
	 */
	unsigned long num_syno_usage_heads_ready;

	/* total number of delayed refs block rsv account */
	u64 total_syno_usage_accounting;
#endif /* MY_ABC_HERE */

	u64 pending_csums;

	/*
	 * set when the tree is flushing before a transaction commit,
	 * used by the throttling code to decide if new updates need
	 * to be run right away
	 */
	int flushing;

	u64 run_delayed_start;

	/*
	 * To make qgroup to skip given root.
	 * This is for snapshot, as btrfs_qgroup_inherit() will manually
	 * modify counters for snapshot and its source, so we should skip
	 * the snapshot in new_root/old_roots or it will get calculated twice
	 */
	u64 qgroup_to_skip;
};

enum btrfs_ref_type {
	BTRFS_REF_NOT_SET,
	BTRFS_REF_DATA,
	BTRFS_REF_METADATA,
	BTRFS_REF_LAST,
};

struct btrfs_data_ref {
	/* For EXTENT_DATA_REF */

	/* Root which refers to this data extent */
	u64 ref_root;

	/* Inode which refers to this data extent */
	u64 ino;

	/*
	 * file_offset - extent_offset
	 *
	 * file_offset is the key.offset of the EXTENT_DATA key.
	 * extent_offset is btrfs_file_extent_offset() of the EXTENT_DATA data.
	 */
	u64 offset;
};

struct btrfs_tree_ref {
	/*
	 * Level of this tree block
	 *
	 * Shared for skinny (TREE_BLOCK_REF) and normal tree ref.
	 */
	int level;

	/*
	 * Root which refers to this tree block.
	 *
	 * For TREE_BLOCK_REF (skinny metadata, either inline or keyed)
	 */
	u64 root;

	/* For non-skinny metadata, no special member needed */
};

struct btrfs_ref {
	enum btrfs_ref_type type;
	int action;

	/*
	 * Whether this extent should go through qgroup record.
	 *
	 * Normally false, but for certain cases like delayed subtree scan,
	 * setting this flag can hugely reduce qgroup overhead.
	 */
	bool skip_qgroup;

	/*
	 * Optional. For which root is this modification.
	 * Mostly used for qgroup optimization.
	 *
	 * When unset, data/tree ref init code will populate it.
	 * In certain cases, we're modifying reference for a different root.
	 * E.g. COW fs tree blocks for balance.
	 * In that case, tree_ref::root will be fs tree, but we're doing this
	 * for reloc tree, then we should set @real_root to reloc tree.
	 */
	u64 real_root;
	u64 bytenr;
	u64 len;

	/* Bytenr of the parent tree block */
	u64 parent;
	union {
		struct btrfs_data_ref data_ref;
		struct btrfs_tree_ref tree_ref;
	};

#ifdef MY_ABC_HERE
	int syno_usage;
#endif /* MY_ABC_HERE */

#ifdef MY_ABC_HERE
	// Used by quota v1.
	u64 ram_bytes;
	struct inode *inode;
#endif /* MY_ABC_HERE */
};

extern struct kmem_cache *btrfs_delayed_ref_head_cachep;
extern struct kmem_cache *btrfs_delayed_tree_ref_cachep;
extern struct kmem_cache *btrfs_delayed_data_ref_cachep;
extern struct kmem_cache *btrfs_delayed_extent_op_cachep;

int __init btrfs_delayed_ref_init(void);
void __cold btrfs_delayed_ref_exit(void);

static inline void btrfs_init_generic_ref(struct btrfs_ref *generic_ref,
				int action, u64 bytenr, u64 len, u64 parent)
{
	generic_ref->action = action;
	generic_ref->bytenr = bytenr;
	generic_ref->len = len;
	generic_ref->parent = parent;
}

static inline void btrfs_init_tree_ref(struct btrfs_ref *generic_ref,
				int level, u64 root)
{
	/* If @real_root not set, use @root as fallback */
	if (!generic_ref->real_root)
		generic_ref->real_root = root;
	generic_ref->tree_ref.level = level;
	generic_ref->tree_ref.root = root;
	generic_ref->type = BTRFS_REF_METADATA;
}

static inline void btrfs_init_data_ref(struct btrfs_ref *generic_ref,
				u64 ref_root, u64 ino, u64 offset
#ifdef MY_ABC_HERE
				, int syno_usage
#endif /* MY_ABC_HERE */
				)
{
	/* If @real_root not set, use @root as fallback */
	if (!generic_ref->real_root)
		generic_ref->real_root = ref_root;
	generic_ref->data_ref.ref_root = ref_root;
	generic_ref->data_ref.ino = ino;
	generic_ref->data_ref.offset = offset;
	generic_ref->type = BTRFS_REF_DATA;
#ifdef MY_ABC_HERE
	generic_ref->syno_usage = syno_usage;
#endif /* MY_ABC_HERE */
}

static inline struct btrfs_delayed_extent_op *
btrfs_alloc_delayed_extent_op(void)
{
	return kmem_cache_alloc(btrfs_delayed_extent_op_cachep, GFP_NOFS);
}

static inline void
btrfs_free_delayed_extent_op(struct btrfs_delayed_extent_op *op)
{
	if (op)
		kmem_cache_free(btrfs_delayed_extent_op_cachep, op);
}

#ifdef MY_ABC_HERE
static inline struct btrfs_delayed_data_ref *
btrfs_delayed_node_to_data_ref(struct btrfs_delayed_ref_node *node)
{
	return container_of(node, struct btrfs_delayed_data_ref, node);
}
#endif /* MY_ABC_HERE */

static inline void btrfs_put_delayed_ref(struct btrfs_delayed_ref_node *ref)
{
#ifdef MY_ABC_HERE
	struct btrfs_delayed_data_ref *data_ref;
#endif /* MY_ABC_HERE */

	WARN_ON(refcount_read(&ref->refs) == 0);
	if (refcount_dec_and_test(&ref->refs)) {
		WARN_ON(ref->in_tree);
		switch (ref->type) {
		case BTRFS_TREE_BLOCK_REF_KEY:
		case BTRFS_SHARED_BLOCK_REF_KEY:
			kmem_cache_free(btrfs_delayed_tree_ref_cachep, ref);
			break;
		case BTRFS_EXTENT_DATA_REF_KEY:
		case BTRFS_SHARED_DATA_REF_KEY:
#ifdef MY_ABC_HERE
			data_ref = btrfs_delayed_node_to_data_ref(ref);
			syno_usrquota_inode_put(data_ref->inode);
#endif /* MY_ABC_HERE */
			kmem_cache_free(btrfs_delayed_data_ref_cachep, ref);
			break;
		default:
			BUG();
		}
	}
}

static inline u64 btrfs_ref_head_to_space_flags(
				struct btrfs_delayed_ref_head *head_ref)
{
	if (head_ref->is_data)
		return BTRFS_BLOCK_GROUP_DATA;
	else if (head_ref->is_system)
		return BTRFS_BLOCK_GROUP_SYSTEM;
	return BTRFS_BLOCK_GROUP_METADATA;
}

static inline void btrfs_put_delayed_ref_head(struct btrfs_delayed_ref_head *head)
{
	if (refcount_dec_and_test(&head->refs))
		kmem_cache_free(btrfs_delayed_ref_head_cachep, head);
}

int btrfs_add_delayed_tree_ref(struct btrfs_trans_handle *trans,
			       struct btrfs_ref *generic_ref,
			       struct btrfs_delayed_extent_op *extent_op);
int btrfs_add_delayed_data_ref(struct btrfs_trans_handle *trans,
			       struct btrfs_ref *generic_ref,
			       u64 reserved);
int btrfs_add_delayed_extent_op(struct btrfs_trans_handle *trans,
				u64 bytenr, u64 num_bytes,
				struct btrfs_delayed_extent_op *extent_op);
void btrfs_merge_delayed_refs(struct btrfs_trans_handle *trans,
			      struct btrfs_delayed_ref_root *delayed_refs,
			      struct btrfs_delayed_ref_head *head);

struct btrfs_delayed_ref_head *
btrfs_find_delayed_ref_head(struct btrfs_delayed_ref_root *delayed_refs,
			    u64 bytenr);
int btrfs_delayed_ref_lock(struct btrfs_delayed_ref_root *delayed_refs,
			   struct btrfs_delayed_ref_head *head);
static inline void btrfs_delayed_ref_unlock(struct btrfs_delayed_ref_head *head)
{
	mutex_unlock(&head->mutex);
}
void btrfs_delete_ref_head(struct btrfs_delayed_ref_root *delayed_refs,
			   struct btrfs_delayed_ref_head *head);

struct btrfs_delayed_ref_head *btrfs_select_ref_head(
		struct btrfs_delayed_ref_root *delayed_refs);
#ifdef MY_ABC_HERE
struct btrfs_delayed_ref_head *btrfs_select_data_ref_head(
		struct btrfs_delayed_ref_root *delayed_refs);
#endif /* MY_ABC_HERE */

int btrfs_check_delayed_seq(struct btrfs_fs_info *fs_info, u64 seq);

void btrfs_delayed_refs_rsv_release(struct btrfs_fs_info *fs_info, int nr);
void btrfs_update_delayed_refs_rsv(struct btrfs_trans_handle *trans);
int btrfs_delayed_refs_rsv_refill(struct btrfs_fs_info *fs_info,
				  enum btrfs_reserve_flush_enum flush);
void btrfs_migrate_to_delayed_refs_rsv(struct btrfs_fs_info *fs_info,
				       struct btrfs_block_rsv *src,
				       u64 num_bytes);
int btrfs_should_throttle_delayed_refs(struct btrfs_trans_handle *trans);
bool btrfs_check_space_for_delayed_refs(struct btrfs_fs_info *fs_info);

/*
 * helper functions to cast a node into its container
 */
static inline struct btrfs_delayed_tree_ref *
btrfs_delayed_node_to_tree_ref(struct btrfs_delayed_ref_node *node)
{
	return container_of(node, struct btrfs_delayed_tree_ref, node);
}

#ifdef MY_ABC_HERE
#else
static inline struct btrfs_delayed_data_ref *
btrfs_delayed_node_to_data_ref(struct btrfs_delayed_ref_node *node)
{
	return container_of(node, struct btrfs_delayed_data_ref, node);
}
#endif /* MY_ABC_HERE */

#endif
