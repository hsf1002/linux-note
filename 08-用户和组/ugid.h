#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>


/**
 * uid->username
 */
char *
username_from_id(uid_t uid);

/**
 * username->uid
 */
uid_t
userid_from_name(const char *name);

/**
 *  gid->gname
 */
groupname_from_id(gid_t gid);

/**
 * gname->gid
 */
groupid_from_name(const char *name);
