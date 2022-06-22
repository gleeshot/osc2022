#include "dtb.h"

void fdt_parser(fdt_header *header, void (*callback)(fdt_prop*, char*, char *)) {
    // uart_puts("into parser");
    //uart_hex(SWAP_ENDIANNESS(FDT_MAGIC_BIG));
    //uart_hex(header->magic);
    if (header->magic != SWAP_ENDIANNESS(FDT_MAGIC_BIG)) {
        //uart_puts("go wrong");
        return;
    }
    //uart_puts("pass magic");

    uint32_t *struct_addr = (uint32_t *)((char *)header + SWAP_ENDIANNESS(header->off_dt_struct));
    uint32_t *string_addr = (uint32_t *)((char *)header + SWAP_ENDIANNESS(header->off_dt_strings));
    char *node_name;

    // uart_puts("find node");
    while (1) {
        uint32_t token = SWAP_ENDIANNESS(*struct_addr);
        // uart_hex(token);
        if (token == FDT_END) {
            // uart_puts("hihihihihhihihihhi");
            break;
        }
        else if (token == FDT_BEGIN_NODE) {
            node_name = (char *)(struct_addr + 1);
            unsigned int node_name_size = strlen(node_name) + 1; // length of the string includes '\0'.
            struct_addr += align_by_4(node_name_size) / 4;
        }
        else if (token == FDT_PROP) {
            fdt_prop *property = (fdt_prop *)(struct_addr + 1);
            
            struct_addr += (sizeof(fdt_prop) + align_by_4(SWAP_ENDIANNESS(property->len))) / 4;
            char *property_name = (char *)(string_addr) + SWAP_ENDIANNESS(property->nameoff);
            callback(property, node_name, property_name);
        }

        struct_addr++;
    }
    
    // uart_puts("finish");
}

void initramfs_callback(fdt_prop *prop, char *node_name, char *prop_name) {
    // uart_puts(prop_name);
    if (strcmp(prop_name, "linux,initrd-start") == 0) {
        // uart_send('b');
        uint32_t *prop_addr = (uint32_t *)(prop + 1);
        CPIO_BASE = SWAP_ENDIANNESS(*prop_addr);
        // CPIO_BASE += KERNEL_VIRT_BASE;
        uart_puts("[initramfs] CPIO_BASE: ");
        uart_hex(CPIO_BASE);
        uart_puts("\r\n");
    }
}