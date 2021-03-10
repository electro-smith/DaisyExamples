/*
	Copyright 2017-2018 Emilie Gillet, Fred's Lab

	Author: Frederic Meslin (fred@fredslab.net)
	Author: Oliver Rockstedt (info@sourcebox.de)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	 
	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
	 
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
	See http://creativecommons.org/licenses/MIT/ for more information.
*/

ENTRY(Reset_Handler)

_estack = 0x20020000;

_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x400;

MEMORY
{
	FLASH (rx)			: ORIGIN = 0x08000000, LENGTH = 2M			/** Flash Sector 1 - application */
	ITCM (xrw)			: ORIGIN = 0x00000000, LENGTH = 64K				/** RAM ITCM - Instruction TCM */
	DTCM (xrw)			: ORIGIN = 0x20000000, LENGTH = 128K			/** RAM DTCM - Data TCM */
	RAM_D1 (xrw)    	: ORIGIN = 0x24000000, LENGTH = 512K			/** SRAM D1 - AXI SRAM */
	RAM_D2 (xrw)    	: ORIGIN = 0x30000000, LENGTH = 256K			/** SRAM D2 - SRAM1 & SRAM2 */
	RAM_D2_DMA (xrw)	: ORIGIN = 0x30040000, LENGTH = 32K				/** SRAM D2 - SRAM3 (uncached) */
	RAM_D3 (xrw)    	: ORIGIN = 0x38000000, LENGTH = 64K				/** SRAM D3 - Static ram Domain 3 */
}

SECTIONS
{
/*****************************************************************************/
/* Flash sections 													         */
/*****************************************************************************/
	.isr_vector :
	{
		. = ALIGN(4);
		KEEP(*(.isr_vector))	/* Startup code */
		. = ALIGN(4);
	} >FLASH

	.text :
	{
		. = ALIGN(4);
		_stext = .;				/* Application start marker */
		*(.text)				/* Remaining code */
		*(.text.*)				/* Remaining code */
		*(.rodata)				/* Read-only data (constants) */
		*(.rodata*)
		*(.glue_7)
		*(.glue_7t)
		KEEP (*(.init))
		KEEP (*(.fini))
		. = ALIGN(4);
		_etext = .;				/* Application end marker */
	} >FLASH

/*****************************************************************************/
/* RAM sections 													         */
/*****************************************************************************/
	_sidata = LOADADDR(.data);
	.data :
	{
		. = ALIGN(4);
		_sdata = . ;			/* Data start marker */
		*(.data*)
		*(.data.*)
		. = ALIGN(4);
		_edata = .;				/* Data end marker */
	} >DTCM AT> FLASH

	.bss :
	{
		. = ALIGN(4);
		_sbss = .;				/* BSS start marker */
		*(.bss)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;				/* BSS end marker */
	} >DTCM

	.dma (NOLOAD):
	{
		. = ALIGN(4);
		_ramd2dma = .;			/* RAMD2DMA start marker */
		*(.dma)
		*(.dma*)
		*(COMMON)
		. = ALIGN(4);
		_eramd2dma = .;			/* RAMD2DMA end marker */
	} >RAM_D2_DMA
	
	.ramd1 (NOLOAD):
	{
		. = ALIGN(4);
		_ramd1 = .;				/* RAMD1 start marker */
		*(.ramd1)
		*(.ramd1*)
		*(COMMON)
		. = ALIGN(4);
		_eramd1 = .;			/* RAMD1 end marker */
	} >RAM_D1

	.ramd2 (NOLOAD):
	{
		. = ALIGN(4);
		_ramd2 = .;				/* RAMD2 start marker */
		*(.ramd2)
		*(.ramd2*)
		*(COMMON)
		. = ALIGN(4);
		_eramd2 = .;			/* RAMD2 end marker */
	} >RAM_D2

	.ramd3 (NOLOAD):
	{
		. = ALIGN(4);
		_ramd3 = .;				/* RAMD2 start marker */
		*(.ramd3)
		*(.ramd3*)
		*(COMMON)
		. = ALIGN(4);
		_eramd3 = .;			/* RAMD2 end marker */
	} >RAM_D3
	
	.dtcm (NOLOAD):
	{
		. = ALIGN(4);
		_sdtcm = .;				/* DTCM start marker */
		*(.dtcm)
		*(.dtcm*)
		. = ALIGN(4);
		_edtcm = .;				/* DTCM end marker */
	} >DTCM

	.itcm :
	{
		. = ALIGN(4);
		_sitcm = .;				/* ITCM start marker */
		*(.ramtext*)
		*(.itcm)
		*(.itcm*)
		. = ALIGN(4);
		_eitcm = .;				/* ITCM end marker */
	} >ITCM AT> FLASH
	
	PROVIDE ( end = _ebss );
	PROVIDE ( _end = _ebss );

/*****************************************************************************/
/* Stack and heap 													         */
/*****************************************************************************/
	._user_heap_stack :
	{
		. = ALIGN(4);
		PROVIDE ( end = . );
		PROVIDE ( _end = . );
		. = . + _Min_Heap_Size;
		. = . + _Min_Stack_Size;
		. = ALIGN(4);
	} >DTCM

	DISCARD :
	{
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
	}
}
