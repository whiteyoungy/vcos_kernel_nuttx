/**********************************************************************************************************************
 *  EXAMPLE CODE ONLY
 *  -------------------------------------------------------------------------------------------------------------------
 *  This Example Code is only intended for illustrating an example of a possible BSW integration and BSW configuration.
 *  The Example Code has not passed any quality control measures and may be incomplete. The Example Code is neither
 *  intended nor qualified for use in series production. The Example Code as well as any of its modifications and/or
 *  implementations must be tested with diligent care and must comply with all quality requirements which are necessary
 *  according to the state of the art before their use.
 *********************************************************************************************************************/


/**********************************************************************************************************************
 * CAUTION - DO NOT EDIT
 * -------------------------------------------------------------------------------------------------------------------
 * Never manually edit the following memory definitions in this file.
 *********************************************************************************************************************/
MEMORY
{
  HOST_reserved : ORIGIN = 0x00400000 , LENGTH = 0x00340000 /* 3 MiB */
  BCODE0 : ORIGIN = 0x00740000 , LENGTH = 0x00000020 /* 32 Byte */
  StartupCode_FirstExecInst : ORIGIN = 0x00740020 , LENGTH = 0x00000400 /* 1024 Byte */
  CoreExceptions_FirstExecInst : ORIGIN = 0x00740420 , LENGTH = 0x00000200 /* 512 Byte */
  ApplicationEntry : ORIGIN = 0x00740620 , LENGTH = 0x00000100 /* 256 Byte */
  SECURE_ICU_CODE_FLASH : ORIGIN = 0x00740720 , LENGTH = 0x000AF3E0 /* 701 KiB */
  vHsm_version : ORIGIN = 0x007EFB00 , LENGTH = 0x00000010 /* 16 Byte */
  vHsm_vreserved : ORIGIN = 0x007EFB10 , LENGTH = 0x000104F0 /* 65 KiB */
  RAM : ORIGIN = 0xFEDF0000 , LENGTH = 0x0000FBF0 /* 63 KiB */
  Variables_Shared : ORIGIN = 0xFEDFFBF0 , LENGTH = 0x00000010 /* 16 Byte */
  StartupStack_Shared : ORIGIN = 0xFEDFFC00 , LENGTH = 0x00000400 /* 1024 Byte */
  DATA_FLASH : ORIGIN = 0xFF287C00 , LENGTH = 0x00000400 /* 1024 Byte */
}

SECTIONS
{  .Startup_Code_Core0 ALIGN(4) :
  {
    *(.bootcode)
  } > BCODE0
  _Startup_Code_Core0_start = ADDR(.Startup_Code_Core0);
  __Startup_Code_Core0_start = ADDR(.Startup_Code_Core0);
  _Startup_Code_Core0_end = ENDADDR(.Startup_Code_Core0) - 1;
  __Startup_Code_Core0_end = ENDADDR(.Startup_Code_Core0) - 1;
  _Startup_Code_Core0_limit = ENDADDR(.Startup_Code_Core0);
  __Startup_Code_Core0_limit = ENDADDR(.Startup_Code_Core0);

  _Startup_Code_Core0_ALL_start = _Startup_Code_Core0_start;
  _Startup_Code_Core0_ALL_end = _Startup_Code_Core0_end;
  _Startup_Code_Core0_ALL_limit = _Startup_Code_Core0_limit;

  .Brs_Startup_Code ALIGN(4) :
  {
    *(.startup)
  } > StartupCode_FirstExecInst
  _Brs_Startup_Code_start = ADDR(.Brs_Startup_Code);
  __Brs_Startup_Code_start = ADDR(.Brs_Startup_Code);
  _Brs_Startup_Code_end = ENDADDR(.Brs_Startup_Code) - 1;
  __Brs_Startup_Code_end = ENDADDR(.Brs_Startup_Code) - 1;
  _Brs_Startup_Code_limit = ENDADDR(.Brs_Startup_Code);
  __Brs_Startup_Code_limit = ENDADDR(.Brs_Startup_Code);

  .BrsMain_Startup_Code ALIGN(4) :
  {
    *(.exceptionhandle)
  } > StartupCode_FirstExecInst
  _BrsMain_Startup_Code_start = ADDR(.BrsMain_Startup_Code);
  __BrsMain_Startup_Code_start = ADDR(.BrsMain_Startup_Code);
  _BrsMain_Startup_Code_end = ENDADDR(.BrsMain_Startup_Code) - 1;
  __BrsMain_Startup_Code_end = ENDADDR(.BrsMain_Startup_Code) - 1;
  _BrsMain_Startup_Code_limit = ENDADDR(.BrsMain_Startup_Code);
  __BrsMain_Startup_Code_limit = ENDADDR(.BrsMain_Startup_Code);

  _Brs_Startup_Code_ALL_start = _Brs_Startup_Code_start;
  _Brs_Startup_Code_ALL_end = _BrsMain_Startup_Code_end;
  _Brs_Startup_Code_ALL_limit = _BrsMain_Startup_Code_limit;

  .STACK_C3 ALIGN(4) :
  {
    . += 1024;
    . = ALIGN(4);
  } > StartupStack_Shared
  _STACK_C3_start = ADDR(.STACK_C3);
  __STACK_C3_start = ADDR(.STACK_C3);
  _STACK_C3_end = ENDADDR(.STACK_C3) - 1;
  __STACK_C3_end = ENDADDR(.STACK_C3) - 1;
  _STACK_C3_limit = ENDADDR(.STACK_C3);
  __STACK_C3_limit = ENDADDR(.STACK_C3);

  _STACK_C3_ALL_start = _STACK_C3_start;
  _STACK_C3_ALL_end = _STACK_C3_end;
  _STACK_C3_ALL_limit = _STACK_C3_limit;
  _startup_stack_limit = _STACK_C3_start;

  .idle_stack ALIGN(4) :
  {
    . += 1024;
    . = ALIGN(4);
  } > RAM
  _idle_stack_start = ADDR(.idle_stack);
  __idle_stack_start = ADDR(.idle_stack);
  _idle_stack_end = ENDADDR(.idle_stack) - 1;
  __idle_stack_end = ENDADDR(.idle_stack) - 1;
  _idle_stack_limit = ENDADDR(.idle_stack);
  __idle_stack_limit = ENDADDR(.idle_stack);


  .int_stack ALIGN(4) :
  {
    . += 1024;
    . = ALIGN(4);
  } > RAM
  _int_stack_start = ADDR(.int_stack);
  __int_stack_start = ADDR(.int_stack);
  _int_stack_end = ENDADDR(.int_stack) - 1;
  __int_stack_end = ENDADDR(.int_stack) - 1;
  _int_stack_limit = ENDADDR(.int_stack);
  __int_stack_limit = ENDADDR(.int_stack);

  .rodata ALIGN(4) :
  {
    *(.rodata)
  } > SECURE_ICU_CODE_FLASH
  _rodata_start = ADDR(.rodata);
  __rodata_start = ADDR(.rodata);
  _rodata_end = ENDADDR(.rodata) - 1;
  __rodata_end = ENDADDR(.rodata) - 1;
  _rodata_limit = ENDADDR(.rodata);
  __rodata_limit = ENDADDR(.rodata);

  .rosdata ALIGN(4) :
  {
    *(.ldata)
    *(.rosdata)
  } > SECURE_ICU_CODE_FLASH
  _rosdata_start = ADDR(.rosdata);
  __rosdata_start = ADDR(.rosdata);
  _rosdata_end = ENDADDR(.rosdata) - 1;
  __rosdata_end = ENDADDR(.rosdata) - 1;
  _rosdata_limit = ENDADDR(.rosdata);
  __rosdata_limit = ENDADDR(.rosdata);

  .rozdata ALIGN(4) :
  {
    *(.rozdata)
    *(.zrodata)
  } > SECURE_ICU_CODE_FLASH
  _rozdata_start = ADDR(.rozdata);
  __rozdata_start = ADDR(.rozdata);
  _rozdata_end = ENDADDR(.rozdata) - 1;
  __rozdata_end = ENDADDR(.rozdata) - 1;
  _rozdata_limit = ENDADDR(.rozdata);
  __rozdata_limit = ENDADDR(.rozdata);

  .sdata2 ALIGN(4) :
  {
    *(.sdata2)
  } > SECURE_ICU_CODE_FLASH
  _sdata2_start = ADDR(.sdata2);
  __sdata2_start = ADDR(.sdata2);
  _sdata2_end = ENDADDR(.sdata2) - 1;
  __sdata2_end = ENDADDR(.sdata2) - 1;
  _sdata2_limit = ENDADDR(.sdata2);
  __sdata2_limit = ENDADDR(.sdata2);

  .secinfo ALIGN(4) :
  {
    *(.secinfo)
  } > SECURE_ICU_CODE_FLASH
  _secinfo_start = ADDR(.secinfo);
  __secinfo_start = ADDR(.secinfo);
  _secinfo_end = ENDADDR(.secinfo) - 1;
  __secinfo_end = ENDADDR(.secinfo) - 1;
  _secinfo_limit = ENDADDR(.secinfo);
  __secinfo_limit = ENDADDR(.secinfo);

  .tdata ALIGN(4) :
  {
    *(.tdata)
  } > SECURE_ICU_CODE_FLASH
  _tdata_start = ADDR(.tdata);
  __tdata_start = ADDR(.tdata);
  _tdata_end = ENDADDR(.tdata) - 1;
  __tdata_end = ENDADDR(.tdata) - 1;
  _tdata_limit = ENDADDR(.tdata);
  __tdata_limit = ENDADDR(.tdata);

  .text ALIGN(4) :
  {
    *(.intvect)
    *(.fixaddr)
    *(.fixtype)
    *(.picbase)
    *(.pidbase)
    *(.syscall)
    *(.text)
    *(.vletext)
  } > SECURE_ICU_CODE_FLASH
  _text_start = ADDR(.text);
  __text_start = ADDR(.text);
  _text_end = ENDADDR(.text) - 1;
  __text_end = ENDADDR(.text) - 1;
  _text_limit = ENDADDR(.text);
  __text_limit = ENDADDR(.text);

  _Const_Default_ALL_start = _rodata_start;
  _Const_Default_ALL_end = _text_end;
  _Const_Default_ALL_limit = _text_limit;

  .bss ALIGN(4) CLEAR :
  {
    *(.bss)
    *(COMMON)
    . = ALIGN(4);
  } > RAM
  _bss_start = ADDR(.bss);
  __bss_start = ADDR(.bss);
  _bss_end = ENDADDR(.bss) - 1;
  __bss_end = ENDADDR(.bss) - 1;
  _bss_limit = ENDADDR(.bss);
  __bss_limit = ENDADDR(.bss);

  .data_ROM ROM_NOCOPY(.data) :> SECURE_ICU_CODE_FLASH
  _data_rom_start = ADDR(.data_ROM);
  __data_rom_start = ADDR(.data_ROM);
  _data_rom_limit = ENDADDR(.data_ROM);
  __data_rom_limit = ENDADDR(.data_ROM);

  .data ALIGN(4) :
  {
    *(.data)
    *(.data.Share.VAR_INIT)
    . = ALIGN(4);
  } > RAM
  _data_start = ADDR(.data);
  __data_start = ADDR(.data);
  _data_end = ENDADDR(.data) - 1;
  __data_end = ENDADDR(.data) - 1;
  _data_limit = ENDADDR(.data);
  __data_limit = ENDADDR(.data);

  .heap ALIGN(16) NOCLEAR :
  {
    . += 8192;
  } > RAM
  _heap_start = ADDR(.heap);
  __heap_start = ADDR(.heap);
  _heap_end = ENDADDR(.heap) - 1;
  __heap_end = ENDADDR(.heap) - 1;
  _heap_limit = ENDADDR(.heap);
  __heap_limit = ENDADDR(.heap);

  .sbss ALIGN(4) CLEAR :
  {
    *(.sbss)
    *(SMALLCOMMON)
    . = ALIGN(4);
  } > RAM
  _sbss_start = ADDR(.sbss);
  __sbss_start = ADDR(.sbss);
  _sbss_end = ENDADDR(.sbss) - 1;
  __sbss_end = ENDADDR(.sbss) - 1;
  _sbss_limit = ENDADDR(.sbss);
  __sbss_limit = ENDADDR(.sbss);

  .sdabase ALIGN(4) :
  {
    *(.sdabase)
  } > RAM
  _sdabase_start = ADDR(.sdabase);
  __sdabase_start = ADDR(.sdabase);
  _sdabase_end = ENDADDR(.sdabase) - 1;
  __sdabase_end = ENDADDR(.sdabase) - 1;
  _sdabase_limit = ENDADDR(.sdabase);
  __sdabase_limit = ENDADDR(.sdabase);

  .sdata_ROM ROM_NOCOPY(.sdata) :> SECURE_ICU_CODE_FLASH
  _sdata_rom_start = ADDR(.sdata_ROM);
  __sdata_rom_start = ADDR(.sdata_ROM);
  _sdata_rom_limit = ENDADDR(.sdata_ROM);
  __sdata_rom_limit = ENDADDR(.sdata_ROM);

  .sdata ALIGN(4) :
  {
    *(.sdata)
    . = ALIGN(4);
  } > RAM
  _sdata_start = ADDR(.sdata);
  __sdata_start = ADDR(.sdata);
  _sdata_end = ENDADDR(.sdata) - 1;
  __sdata_end = ENDADDR(.sdata) - 1;
  _sdata_limit = ENDADDR(.sdata);
  __sdata_limit = ENDADDR(.sdata);

  .stack ALIGN(4) :
  {
    *(.stack)
  } > RAM
  _stack_start = ADDR(.stack);
  __stack_start = ADDR(.stack);
  _stack_end = ENDADDR(.stack) - 1;
  __stack_end = ENDADDR(.stack) - 1;
  _stack_limit = ENDADDR(.stack);
  __stack_limit = ENDADDR(.stack);

  .zbss ALIGN(4) CLEAR :
  {
    *(.PPC.EMB.sbss0)
    *(.zbss)
    *(ZEROCOMMON)
    . = ALIGN(4);
  } > RAM
  _zbss_start = ADDR(.zbss);
  __zbss_start = ADDR(.zbss);
  _zbss_end = ENDADDR(.zbss) - 1;
  __zbss_end = ENDADDR(.zbss) - 1;
  _zbss_limit = ENDADDR(.zbss);
  __zbss_limit = ENDADDR(.zbss);

  .zdata_ROM ROM_NOCOPY(.zdata) :> SECURE_ICU_CODE_FLASH
  _zdata_rom_start = ADDR(.zdata_ROM);
  __zdata_rom_start = ADDR(.zdata_ROM);
  _zdata_rom_limit = ENDADDR(.zdata_ROM);
  __zdata_rom_limit = ENDADDR(.zdata_ROM);

  .zdata ALIGN(4) :
  {
    *(.PPC.EMB.sdata0)
    *(.zdata)
    . = ALIGN(4);
  } > RAM
  _zdata_start = ADDR(.zdata);
  __zdata_start = ADDR(.zdata);
  _zdata_end = ENDADDR(.zdata) - 1;
  __zdata_end = ENDADDR(.zdata) - 1;
  _zdata_limit = ENDADDR(.zdata);
  __zdata_limit = ENDADDR(.zdata);

  _Data_Default_ALL_start = _bss_start;
  _Data_Default_ALL_end = _zdata_end;
  _Data_Default_ALL_limit = _zdata_limit;

}


