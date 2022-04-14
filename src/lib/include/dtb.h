#ifndef __DTB__H__
#define __DTB__H__

#include "string.h"
#include "utils.h"
#include "mini_uart.h"
// #include "cpio.h"

uint32_t CPIO_BASE;

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009
#define FDT_MAGIC_BIG   0xD00DFEED

#define SWAP_ENDIANNESS(input)  (((input) >> 24) | (((input) & 0x00FF0000) >> 8) | (((input) & 0x0000FF00) << 8) | ((input) << 24))

typedef struct
{
    /* data */
    uint32_t magic;                    // It shall contain the value 0xd00dfeed for big-endian.
    uint32_t totalsize;                // total size in bytes of the devicetree data structure.
    uint32_t off_dt_struct;            // the offset in bytes of the structure block from the beginning of the header.
    uint32_t off_dt_strings;           // the offset in bytes of the string block from the beginning of the header.
    uint32_t off_mem_rsvmap;           // the offset in bytes of the memory reservation block from the beginning of the header.
    uint32_t version;                  // the version of the devicetree.
    uint32_t last_comp_version;        // the lowest version of the devicetree data structure with which the version used is backwards compatible.
    uint32_t boot_cpuid_phys;          // physical ID of the system’s boot CPU.
    uint32_t size_dt_strings;          // This field shall contain the length in bytes of the strings block section of the devicetree blob.
    uint32_t size_dt_struct;           // This field shall contain the length in bytes of the structure block section of the devicetree blob.
}fdt_header;

typedef struct 
{
    /* data */
    uint32_t len;
    uint32_t nameoff;
}fdt_prop;

void fdt_parser(fdt_header *header, void (*callback)(fdt_prop*, char*, char *));
void initramfs_callback(fdt_prop *prop, char *node_name, char *prop_name);

#endif