//----------------------------------------------------------------------
//   $Id: multiboot.h,v 1.2 2005/04/22 19:43:04 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: multiboot.h,v $
//  Revision 1.1  2005/04/21 21:31:24  nomenquis
//  added lfb support, also we now use a different grub version
//  we also now read in the grub multiboot version
//
//----------------------------------------------------------------------

//
// Credit where credit is due, this is shamelessly stolen (borrowed, we'll give it 
// back soon) from the spoon os.
//


#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

/* VBE controller information.  */
struct vbe_controller
{
  uint8 signature[4];
  uint16 version;
  uint32 oem_string;
  uint32 capabilities;
  uint32 video_mode;
  uint16 total_memory;
  uint16 oem_software_rev;
  uint32 oem_vendor_name;
  uint32 oem_product_name;
  uint32 oem_product_rev;
  uint8 reserved[222];
  uint8 oem_data[256];
} __attribute__ ((packed));

/* VBE mode information.  */
struct vbe_mode
{
  uint16 mode_attributes;
  uint8 win_a_attributes;
  uint8 win_b_attributes;
  uint16 win_granularity;
  uint16 win_size;
  uint16 win_a_segment;
  uint16 win_b_segment;
  uint32 win_func;
  uint16 bytes_per_scanline;

  /* >=1.2 */
  uint16 x_resolution;
  uint16 y_resolution;
  uint8 x_char_size;
  uint8 y_char_size;
  uint8 number_of_planes;
  uint8 bits_per_pixel;
  uint8 number_of_banks;
  uint8 memory_model;
  uint8 bank_size;
  uint8 number_of_image_pages;
  uint8 reserved0;

  /* direct color */
  uint8 red_mask_size;
  uint8 red_field_position;
  uint8 green_mask_size;
  uint8 green_field_position;
  uint8 blue_mask_size;
  uint8 blue_field_position;
  uint8 reserved_mask_size;
  uint8 reserved_field_position;
  uint8 direct_color_mode_info;

  /* >=2.0 */
  uint32 phys_base;
  uint32 reserved1;
  uint16 reversed2;

  /* >=3.0 */
  uint16 linear_bytes_per_scanline;
  uint8 banked_number_of_image_pages;
  uint8 linear_number_of_image_pages;
  uint8 linear_red_mask_size;
  uint8 linear_red_field_position;
  uint8 linear_green_mask_size;
  uint8 linear_green_field_position;
  uint8 linear_blue_mask_size;
  uint8 linear_blue_field_position;
  uint8 linear_reserved_mask_size;
  uint8 linear_reserved_field_position;
  uint32 max_pixel_clock;

  uint8 reserved3[189];
} __attribute__ ((packed));



/* The Multiboot header. */
typedef struct multiboot_header
{
   uint32 magic			: 32;
   uint32 flags			: 32;
   uint32 checksum		: 32;
   uint32 header_addr		: 32;
   uint32 load_addr		: 32;
   uint32 load_end_addr		: 32;
   uint32 bss_end_addr		: 32;
   uint32 entry_addr		: 32;
   uint32 mode_type		: 32;
   uint32 width			: 32;
   uint32 height		: 32;
   uint32 depth			: 32;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
  uint32 tabsize;
  uint32 strsize;
  uint32 addr;
  uint32 reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
  uint32 num;
  uint32 size;
  uint32 addr;
  uint32 shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info
{
  uint32 flags			: 32;
  uint32 mem_lower		: 32;
  uint32 mem_upper		: 32;
  uint32 boot_device	: 32;
  uint32 cmdline		: 32;
  uint32 mods_count		: 32;
  uint32 mods_addr		: 32;
  union
  {
     aout_symbol_table_t aout_sym;
     elf_section_header_table_t elf_sec;
  } u;
  uint32 mmap_length		: 32;
  uint32 mmap_addr		: 32;
  uint32 drives_length		: 32;
  uint32 drives_addr		: 32;
  uint32 config_table		: 32;
  uint32 boot_loader_name	: 32;
  uint32 apm_table		: 32;
  uint32 vbe_control_info	: 32;
  uint32 vbe_mode_info		: 32;
  uint32 vbe_mode		: 32;
  uint32 vbe_interface_seg	: 32;
  uint32 vbe_interface_off	: 32;
  uint32 vbe_interface_len	: 32;
} multiboot_info_t;

/* The module structure. */
typedef struct module
{
  uint32 mod_start;
  uint32 mod_end;
  uint32 string;
  uint32 reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
 *    but no size. */
typedef struct memory_map
{
  uint32 size;
  uint32 base_addr_low;
  uint32 base_addr_high;
  uint32 length_low;
  uint32 length_high;
  uint32 type;
} memory_map_t;


#endif
