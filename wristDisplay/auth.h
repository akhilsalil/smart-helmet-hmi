#ifndef AUTH_H
#define AUTH_H

// User roles set at PIN entry.
//   OPERATOR can send commands.
//   VIEWER can browse robot list + detail but no command buttons shown.
enum UserRole {
    ROLE_NONE,
    ROLE_OPERATOR,
    ROLE_VIEWER
};

extern UserRole currentRole;

#endif