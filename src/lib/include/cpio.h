#ifndef __CPIO__H__
#define __CPIO__H__

#include "mini_uart.h"
#include "string.h"
#include "utils.h"
#include "dtb.h"
#include "mm.h"

/* The cpio archive format collects any number of files, directories, and
 * other file	system objects (symbolic links,	device nodes, etc.) into a
 * single stream of bytes.
 */

typedef struct {
    char c_magic[6];      // determine whethter the archive is written in big-endian or little-endian.
    char c_ino[8];        // inode numbers from disk.
    char c_mode[8];       // The mode specifies	both the regular permissions and the file type.
    char c_uid[8];        // numeric user id of the owner.
    char c_gid[8];        // numeric group id of the owner.
    char c_nlink[8];      // The number	of links to this file.
    char c_mtime[8];      // Modification time of the file, indicated as the number of seconds since the start of the epoch, 00:00:00 UTC January 1, 1970.
    char c_filesize[8];   // The size of the file. limitation of this archive format: 4GB.
    char c_devmajor[8];   // major device number.
    char c_devminor[8];   // minor device number.
    char c_rdevmajor[8];  // For block special and character special entries, this field contains the associated device number.
    char c_rdevminor[8];
    char c_namesize[8];   // The number	of bytes in the	pathname that follows the header. This count includes the trailing NUL byte.
    char c_check[8];      // This field	is always set to zero by writers and ignored by	readers.
} cpio_new_header;

#define CPIO_MAGIC_NUM "070701"
#define HEADER_SIZE    (sizeof(cpio_new_header))
#define END_OF_CPIO    "TRAILER!!!"
// #define CPIO_BASE      0x20000000
extern uint32_t CPIO_BASE;

int cpio_header_parser(cpio_new_header *header, char** file_name, unsigned long* file_size, char** data, cpio_new_header **next_header);
void cpio_ls(cpio_new_header *header);
void cpio_cat(cpio_new_header *header, char *input);
void *cpio_load(cpio_new_header *header, const char *target_file_name);

#endif