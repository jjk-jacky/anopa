
#ifndef AA_ERR_H
#define AA_ERR_H

enum
{
    ERR_INVALID_NAME = 1,
    ERR_UNKNOWN,
    ERR_DEPEND,
    ERR_IO,
    ERR_WRITE_STATUS,
    ERR_CHDIR,
    ERR_EXEC,
    ERR_PIPES,
    ERR_S6,
    ERR_FAILED,
    ERR_TIMEDOUT,
    ERR_IO_REPODIR,
    ERR_IO_SCANDIR,
    ERR_FAILED_ENABLE,
    /* not actual service error, see aa_ensure_service_loaded() */
    ERR_ALREADY_UP,
    ERR_NOT_UP,
    _NB_ERR
};

extern const char const *errmsg[_NB_ERR];

#endif /* AA_ERR_H */
