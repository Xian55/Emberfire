#ifndef _DMYST_MINIMAL_ZIP_H
#define _DMYST_MINIMAL_ZIP_H
// Minimal libzip public API — only the subset ContentMgr.cpp uses. Links against zip.lib (import lib
// generated from the shipped 32-bit zip.dll, so the ABI matches exactly). Struct layout = standard libzip 1.x.
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long long zip_uint64_t;
typedef long long          zip_int64_t;
typedef unsigned int       zip_uint32_t;
typedef unsigned short     zip_uint16_t;
typedef unsigned int       zip_flags_t;

struct zip;
struct zip_file;
typedef struct zip      zip_t;
typedef struct zip_file zip_file_t;

struct zip_stat {
    zip_uint64_t valid;
    const char  *name;
    zip_uint64_t index;
    zip_uint64_t size;
    zip_uint64_t comp_size;
    time_t       mtime;
    zip_uint32_t crc;
    zip_uint16_t comp_method;
    zip_uint16_t encryption_method;
    zip_uint32_t flags;
};
typedef struct zip_stat zip_stat_t;

#define ZIP_RDONLY 16
#define ZIP_CREATE 1

struct zip       *zip_open(const char *path, int flags, int *errorp);
int               zip_close(struct zip *archive);
zip_int64_t       zip_get_num_entries(struct zip *archive, zip_flags_t flags);
const char       *zip_get_name(struct zip *archive, zip_uint64_t index, zip_flags_t flags);
struct zip_file  *zip_fopen(struct zip *archive, const char *fname, zip_flags_t flags);
struct zip_file  *zip_fopen_index(struct zip *archive, zip_uint64_t index, zip_flags_t flags);
int               zip_fclose(struct zip_file *file);
zip_int64_t       zip_fread(struct zip_file *file, void *buf, zip_uint64_t nbytes);
void              zip_stat_init(zip_stat_t *sb);
int               zip_stat(struct zip *archive, const char *fname, zip_flags_t flags, zip_stat_t *sb);
int               zip_stat_index(struct zip *archive, zip_uint64_t index, zip_flags_t flags, zip_stat_t *sb);

#ifdef __cplusplus
}
#endif
#endif
