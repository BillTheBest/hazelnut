/***********************************************************************
 * pe-loader (c) 2000 University of Karlsruhe
 * author: Volkmar Uhlig <volkmar@ira.uka.de>
 *
 * $Log: pe.c,v $
 * Revision 1.1  2000/05/16 14:56:55  uhlig
 * added coff/pe-file loading support to rmgr.
 * currently only tested on pe-files generated by MSDEV-Studio but should work with "normal" coff files too.
 *
 */
#include <alloca.h>
#include <flux/machine/types.h>
#include <flux/exec/exec.h>
#include "coff-i386.h"
#include <flux/exec/elf.h>

int exec_load_pe(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
		 void *handle, exec_info_t *out_info)
{
	vm_size_t actual;
	FILHDR x;
	DOSHDR dhdr;
	AOUTHDR ohdr;
	SCNHDR * phdr;
	__u32 foffset = 0;
	__u32 image_offset = 0;

	vm_size_t phsize;
	int i;
	int result;

	/* read the header of the file - could have DOS header */
	if ((result = (*read)(handle, 0, &dhdr, sizeof(dhdr), &actual)) != 0)
	    return result;
	if (actual < sizeof(dhdr))
	    return EX_NOT_EXECUTABLE;

	//printf("dos header: %x, %x\n", dhdr.f_magic, dhdr.f_lfanew);

	if (dhdr.f_magic == DOSMAGIC) {
	    //printf("found dosmagic\n");
	    if ((unsigned int)dhdr.f_lfanew == 0) 
		return EX_NOT_EXECUTABLE;
	    foffset = dhdr.f_lfanew; 
	    foffset += 4; // hack - skip pe00 magic of NT
	    //printf("file offset: %x\n", foffset);
	}
	/* Read the coff header.  */
	if ((result = (*read)(handle, foffset, &x, sizeof(x), &actual)) != 0)
	    return result;
	if (actual < sizeof(x))
	    return EX_NOT_EXECUTABLE;

	//printf("magic: %x\n", x.f_magic);
	/* check magic */
	if (I386BADMAG(x))
		return EX_NOT_EXECUTABLE;

	/* read optional header */
	if ((result = (*read)(handle, foffset + FILHSZ, &ohdr, x.f_opthdr, &actual)) != 0)
	    return result;

	if (x.f_opthdr == NT32_OHDRSIZE) {
	    image_offset = ohdr.u.nt.image_base;
	}
	else 
	    image_offset = 0;

	out_info->entry = (vm_offset_t) ohdr.entry + image_offset;

	phsize = x.f_nscns * SCNHSZ;
	phdr = (SCNHDR*)alloca(phsize);

	result = (*read)(handle, x.f_opthdr + foffset + FILHSZ, phdr, phsize, &actual);
	if (result)
		return result;
	if (actual < phsize)
		return EX_CORRUPT;
	

	for (i = 0; i < x.f_nscns; i++)
	{
		SCNHDR *ph = (SCNHDR*)((vm_offset_t)phdr + i * SCNHSZ);
		ph->s_name[7] = 0;
		//printf("section: %s\n", ph->s_name);
		if (!(ph->s_flags & SECT_DISCARDABLE))
		{
			exec_sectype_t type = EXEC_SECTYPE_ALLOC |
					      EXEC_SECTYPE_LOAD;
			if (ph->s_flags & SECT_READ) type |= EXEC_SECTYPE_READ;
			if (ph->s_flags & SECT_WRITE) type |= EXEC_SECTYPE_WRITE;
			if (ph->s_flags & SECT_EXECUTE) type |= EXEC_SECTYPE_EXECUTE;
#if 1
			result = (*read_exec)(handle,
					      ph->s_scnptr, ph->misc.s_vsize,
					      ph->s_vaddr + image_offset, 
					      ph->s_size, type);
#else
			printf("load %x -> %x, size: %d/%d\n", ph->s_scnptr, ph->s_vaddr + image_offset, ph->misc.s_vsize, ph->s_size);
#endif
		}
	}

	return 0;
}
