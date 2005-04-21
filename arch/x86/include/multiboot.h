//----------------------------------------------------------------------
//   $Id: multiboot.h,v 1.1 2005/04/21 21:31:24 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
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
  unsigned char signature[4];
  unsigned short version;
  unsigned long oem_string;
  unsigned long capabilities;
  unsigned long video_mode;
  unsigned short total_memory;
  unsigned short oem_software_rev;
  unsigned long oem_vendor_name;
  unsigned long oem_product_name;
  unsigned long oem_product_rev;
  unsigned char reserved[222];
  unsigned char oem_data[256];
} __attribute__ ((packed));

/* VBE mode information.  */
struct vbe_mode
{
  unsigned short mode_attributes;
  unsigned char win_a_attributes;
  unsigned char win_b_attributes;
  unsigned short win_granularity;
  unsigned short win_size;
  unsigned short win_a_segment;
  unsigned short win_b_segment;
  unsigned long win_func;
  unsigned short bytes_per_scanline;

  /* >=1.2 */
  unsigned short x_resolution;
  unsigned short y_resolution;
  unsigned char x_char_size;
  unsigned char y_char_size;
  unsigned char number_of_planes;
  unsigned char bits_per_pixel;
  unsigned char number_of_banks;
  unsigned char memory_model;
  unsigned char bank_size;
  unsigned char number_of_image_pages;
  unsigned char reserved0;

  /* direct color */
  unsigned char red_mask_size;
  unsigned char red_field_position;
  unsigned char green_mask_size;
  unsigned char green_field_position;
  unsigned char blue_mask_size;
  unsigned char blue_field_position;
  unsigned char reserved_mask_size;
  unsigned char reserved_field_position;
  unsigned char direct_color_mode_info;

  /* >=2.0 */
  unsigned long phys_base;
  unsigned long reserved1;
  unsigned short reversed2;

  /* >=3.0 */
  unsigned short linear_bytes_per_scanline;
  unsigned char banked_number_of_image_pages;
  unsigned char linear_number_of_image_pages;
  unsigned char linear_red_mask_size;
  unsigned char linear_red_field_position;
  unsigned char linear_green_mask_size;
  unsigned char linear_green_field_position;
  unsigned char linear_blue_mask_size;
  unsigned char linear_blue_field_position;
  unsigned char linear_reserved_mask_size;
  unsigned char linear_reserved_field_position;
  unsigned long max_pixel_clock;

  unsigned char reserved3[189];
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
