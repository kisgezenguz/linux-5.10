/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (C) 2000-2021 Synology Inc.
 */
#ifndef _UAPI_LINUX_SYNO_ACL_H
#define _UAPI_LINUX_SYNO_ACL_H

#include <linux/types.h>

#define SYNO_ACL_MNT_OPT                        "synoacl"
#define SYNO_ACL_NOT_MNT_OPT                    "no"SYNO_ACL_MNT_OPT

/* Extended attribute names */
#define SYNO_ACL_XATTR_ACCESS                   "system.syno_acl_self"
#define SYNO_ACL_XATTR_INHERIT                  "system.syno_acl_inherit"
#define SYNO_ACL_XATTR_PSEUDO_INHERIT_ONLY      "system.syno_acl_pseudo_inherit_only"

/* MAC EA Exttend attribute names */
#define SYNO_XATTR_EA_PREFIX                    "user.syno."
#define SYNO_XATTR_EA_PREFIX_LEN                (sizeof(SYNO_XATTR_EA_PREFIX) - 1)
#define SYNO_XATTR_NETATALK_PREFIX              "user.netatalk."
#define SYNO_XATTR_NETATALK_PREFIX_LEN          (sizeof(SYNO_XATTR_NETATALK_PREFIX) - 1)

/* Supported ACL a_version fields */
#define SYNO_ACL_XATTR_VERSION                  0x0001

/* An undefined entry e_id value */
#define SYNO_ACL_UNDEFINED_ID                   -1

/* Permissions in the e_perm field
   *Note* It should be consistent with MAY_XXXXX in <linux/fs.h>
   Define it since MAY_XXXX conflict with user space program
*/
#define SYNO_ACL_MAY_EXEC                       0x0001
#define SYNO_ACL_MAY_WRITE                      0x0002
#define SYNO_ACL_MAY_READ                       0x0004
#define SYNO_ACL_MAY_APPEND                     0x0008
#define SYNO_ACL_MAY_ACCESS                     0x0010
#define SYNO_ACL_MAY_OPEN                       0x0020
#define SYNO_ACL_MAY_READ_EXT_ATTR              0x0040
#define SYNO_ACL_MAY_READ_PERMISSION            0x0080
#define SYNO_ACL_MAY_READ_ATTR                  0x0100
#define SYNO_ACL_MAY_WRITE_ATTR                 0x0200
#define SYNO_ACL_MAY_WRITE_EXT_ATTR             0x0400
#define SYNO_ACL_MAY_WRITE_PERMISSION           0x0800
#define SYNO_ACL_MAY_DEL                        0x1000
#define SYNO_ACL_MAY_DEL_CHILD                  0x2000
#define SYNO_ACL_MAY_GET_OWNER_SHIP             0x4000

/* Classify permission type */
#define SYNO_PERM_READABLE                                                     \
	(SYNO_ACL_MAY_READ | SYNO_ACL_MAY_READ_ATTR |                          \
	 SYNO_ACL_MAY_READ_PERMISSION | SYNO_ACL_MAY_READ_EXT_ATTR)

#define SYNO_PERM_WRITE_DATA                                                   \
	(SYNO_ACL_MAY_WRITE | SYNO_ACL_MAY_APPEND | SYNO_ACL_MAY_DEL_CHILD)

#define SYNO_PERM_WRITABLE                                                     \
	(SYNO_PERM_WRITE_DATA | SYNO_ACL_MAY_WRITE_ATTR |                      \
	 SYNO_ACL_MAY_WRITE_EXT_ATTR)

#define SYNO_PERM_OWNER                                                        \
	(SYNO_ACL_MAY_READ_PERMISSION | SYNO_ACL_MAY_WRITE_PERMISSION |        \
	 SYNO_ACL_MAY_GET_OWNER_SHIP)

#define SYNO_PERM_FULL_CONTROL                                                 \
	(SYNO_ACL_MAY_EXEC | SYNO_PERM_READABLE | SYNO_PERM_WRITABLE |         \
	 SYNO_ACL_MAY_WRITE_PERMISSION | SYNO_ACL_MAY_GET_OWNER_SHIP |         \
	 SYNO_ACL_MAY_DEL)

/* inherit mode in the e_inherit field */
#define SYNO_ACL_INHERIT_ONLY                   0x0001
#define SYNO_ACL_INHERIT_FILE                   0x0002
#define SYNO_ACL_INHERIT_DIR                    0x0004
#define SYNO_ACL_INHERIT_NO_PROPOGATE           0x0008

#define SYNO_ACL_INHERIT_TYPE                                                  \
	(SYNO_ACL_INHERIT_ONLY | SYNO_ACL_INHERIT_FILE | SYNO_ACL_INHERIT_DIR)

#define SYNO_ACL_INHERIT_ALL                                                   \
	(SYNO_ACL_INHERIT_ONLY | SYNO_ACL_INHERIT_FILE |                       \
	 SYNO_ACL_INHERIT_DIR | SYNO_ACL_INHERIT_NO_PROPOGATE)

#define IS_INHERIT_ONE_LEVEL(x)                 ((SYNO_ACL_INHERIT_NO_PROPOGATE & (x)) && ((SYNO_ACL_INHERIT_FILE|SYNO_ACL_INHERIT_DIR) & (x)))
#define IS_INHERIT_ONLY(x)                      (SYNO_ACL_INHERIT_ONLY & (x))
#define IS_MATCH_FILE_TYPE(isdir, x)            ((isdir) ? (SYNO_ACL_INHERIT_DIR & (x)):(SYNO_ACL_INHERIT_FILE & (x)))

#define SYNO_ACL_XATTR_TAG_IS_DENY              0X0001
#define SYNO_ACL_XATTR_TAG_IS_ALLOW             0X0002
#define SYNO_ACL_XATTR_TAG_ALLOW_ALL                                           \
	(SYNO_ACL_XATTR_TAG_IS_DENY | SYNO_ACL_XATTR_TAG_IS_ALLOW)

#define SYNO_ACL_XATTR_TAG_ID_USER              0X0004
#define SYNO_ACL_XATTR_TAG_ID_GROUP             0X0008
#define SYNO_ACL_XATTR_TAG_ID_EVERYONE          0X0010
#define SYNO_ACL_XATTR_TAG_ID_OWNER             0X0020
#define SYNO_ACL_XATTR_TAG_ID_AUTHENTICATEDUSER 0X0040
#define SYNO_ACL_XATTR_TAG_ID_SYSTEM            0X0080
#define SYNO_ACL_XATTR_TAG_ID_ALL                                              \
	(SYNO_ACL_XATTR_TAG_ID_USER | SYNO_ACL_XATTR_TAG_ID_GROUP |            \
	 SYNO_ACL_XATTR_TAG_ID_EVERYONE | SYNO_ACL_XATTR_TAG_ID_OWNER |        \
	 SYNO_ACL_XATTR_TAG_ID_AUTHENTICATEDUSER |                             \
	 SYNO_ACL_XATTR_TAG_ID_SYSTEM)

enum {
	SYNO_KERNEL_IS_FS_SUPPORT = 1,      // File System
	SYNO_KERNEL_IS_FILE_SUPPORT,        // File or Dir
};

enum {
	SYNO_ACL_INHERITED = 1,             // Includes self-defined and inherited ACL.
	SYNO_ACL_PSEUDO_INHERIT_ONLY,       // Includes only inherited ACE, even entry has no inherited attribute.
};


/*
 * these two structs should be packed because btrfs-send will transfer raw xattr
 * data including `syno_acl_xattr_{entry,header}` via syno_acl_to_xattr().
 */
typedef struct {
	__le16                  e_tag;
	__le16                  e_unused1;
	__le32                  e_perm;
	__le16                  e_inherit;
	__le16                  e_unused2;
	__le32                  e_id;
	__le32                  e_level;
} __attribute__((__packed__)) syno_acl_xattr_entry;

typedef struct {
	__le16                  a_version;
	__le16                  a_unused;
	syno_acl_xattr_entry    a_entries[0];
} __attribute__((__packed__)) syno_acl_xattr_header;

static inline size_t syno_acl_xattr_size(int count)
{
	return (sizeof(syno_acl_xattr_header) +
		(count * sizeof(syno_acl_xattr_entry)));
}

static inline int syno_acl_xattr_count(size_t size)
{
	if (size < sizeof(syno_acl_xattr_header))
		return -1;

	size -= sizeof(syno_acl_xattr_header);
	if (size % sizeof(syno_acl_xattr_entry))
		return -1;

	return size / sizeof(syno_acl_xattr_entry);
}

#endif /* _UAPI_LINUX_SYNO_ACL_H */
