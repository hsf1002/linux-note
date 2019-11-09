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
username_from_id(uid_t uid)
{
    struct passwd *pwd;

    pwd = getpwuid(uid);
    return (pwd == NULL) ? NULL : pwd->pw_name;
}

/**
 * username->uid
 */
uid_t
userid_from_name(const char *name)
{
    struct passwd *pwd;
    uid_t u;
    char *endptr;

    if (NULL == name || *name == '\0')
        return -1;

    u = strtol(name, &endptr, 10);

    if (*endptr == '\0')
        return u;

    pwd = getpwnam(name);
    return (pwd == NULL) ? -1 : pwd->pw_uid;
}

/**
 *  gid->gname
 */
groupname_from_id(gid_t gid)
{
    struct group *grp;

    grp = getgrgid(gid);
    return (grp == NULL) ? NULL : grp->gr_name;
}

/**
 * gname->gid
 */
groupid_from_name(const char *name)
{
    struct group *grp;
    gid_t g;
    char *endptr;

    if (NULL == name || *name == '\0')
        return -1;

    g = strtol(name, &endptr, 10);

    if (*endptr == '\0')
        return g;
        
    grp = getgrnam(name);
    return (grp == NULL) ? -1 : grp->gr_gid;
}

