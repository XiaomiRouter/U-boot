#include <common.h>

#if defined(CFG_ENV_IS_IN_NAND) || defined (NAND_FLASH_SUPPORT) /* Environment is in NAND Flash */

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <nand_api.h>
#include "../drivers/ralink_nand.h"


#if 1 //((CONFIG_COMMANDS&(CFG_CMD_ENV|CFG_CMD_FLASH)) == (CFG_CMD_ENV|CFG_CMD_FLASH))
#define CMD_SAVEENV
#elif defined(CFG_ENV_ADDR_REDUND)
#error Cannot use CFG_ENV_ADDR_REDUND without CFG_CMD_ENV & CFG_CMD_FLASH
#endif

char * env_name_spec = "NAND Flash";

#ifdef ENV_IS_EMBEDDED

extern uchar environment[];
env_t *env_ptr = (env_t *)(&environment[0]);
#ifdef CMD_SAVEENV
static env_t *flash_addr = (env_t *)(CFG_ENV_ADDR - CFG_FLASH_BASE);
#endif

#else /* ! ENV_IS_EMBEDDED */

env_t *env_ptr;
#ifdef CMD_SAVEENV
static env_t *flash_addr = (env_t *)(CFG_ENV_ADDR - CFG_FLASH_BASE);
#endif

#endif /* ENV_IS_EMBEDDED */

extern uchar default_environment[];
extern int default_environment_size;
extern int is_nand_page_2048;


uchar env_get_char_spec (int index)
{
	DECLARE_GLOBAL_DATA_PTR;

	return ( *((uchar *)(gd->env_addr + index)) );
}


int env_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 0;
	return (0);
}

int nand_env_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	env_ptr = (env_t *)malloc(CFG_ENV_SIZE);
	if (env_ptr == NULL)
		return -1;

	if (ranand_read((u8 *)env_ptr, (unsigned int)flash_addr, CFG_ENV_SIZE) != CFG_ENV_SIZE)
		return -1;
	else if (crc32(0, env_ptr->data, ENV_SIZE) == env_ptr->crc) {
		gd->env_addr  = (ulong)&(env_ptr->data);
		gd->env_valid = 1;
		return(0);
	}

	gd->env_addr  = (ulong)&default_environment[0];
	gd->env_valid = 0;
	return (0);
}

#ifdef CMD_SAVEENV

int saveenv(void)
{
	int	len, rc;
	ulong	flash_sect_addr;
#if defined(CFG_BLOCKSIZE) && (CFG_BLOCKSIZE > CFG_ENV_SIZE)
	ulong	flash_offset;
	uchar	env_buffer[CFG_BLOCKSIZE];
#else
	uchar *env_buffer = (char *)env_ptr;
#endif	/* CFG_BLOCKSIZE */
	int rcode = 0;

#if defined(CFG_BLOCKSIZE) && (CFG_BLOCKSIZE > CFG_ENV_SIZE)
	flash_offset	= ((ulong)flash_addr) & (CFG_BLOCKSIZE-1);
	flash_sect_addr	= ((ulong)flash_addr) & ~(CFG_BLOCKSIZE-1);
	len	= CFG_BLOCKSIZE;

	/*
	debug ( "copy old content: "
		"sect_addr: %08lX  env_addr: %08lX  offset: %08lX\n",
		flash_sect_addr, (ulong)flash_addr, flash_offset);
	*/

	/* copy old contents to temporary buffer */
	if (ranand_read(env_buffer, flash_sect_addr, len) != len)
		return 1;

	/* copy current environment to temporary buffer */
	memcpy ((uchar *)((unsigned long)env_buffer + flash_offset),
		env_ptr, CFG_ENV_SIZE);
#else
	flash_sect_addr = (ulong)flash_addr;
	len	= CFG_ENV_SIZE;
#endif	/* CFG_BLOCKSIZE */

	puts ("Erasing NAND Flash...\n");
	if (ranand_erase(flash_sect_addr, len))
		return 1;

	puts ("Writing to NAND Flash...\n");
	rc = ranand_write(env_buffer, flash_sect_addr, len);
	if (rc != len) {
		printf ("error %d!\n", rc);
		rcode = 1;
	} else {
		puts ("done\n");
	}

	return rcode;
}

#endif /* CMD_SAVEENV */


void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED)
	ranand_read((u8 *)env_ptr, (unsigned int)flash_addr, CFG_ENV_SIZE);
#endif /* ! ENV_IS_EMBEDDED */
}

#endif /* CFG_ENV_IS_IN_NAND */
