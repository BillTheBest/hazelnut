;*********************************************************************
;******                                                         ******
;******         LN System Calls      (INT n)                    ******
;******                                                         ******
;*********************************************************************
 
   
ipc                    equ 30h

id_nearest             equ 31h

fpage_unmap            equ 32h

thread_switch          equ 33h

thread_schedule        equ 34h

lthread_ex_regs        equ 35h

task_new               equ 36h



ex_regs_update_flag             equ   30
ex_regs_auto_propagating_flag   equ   29


ipc_syscall macro
  
  push  ecx
  push  edx
  push  ebx
  push  eax

  mov   eax,3246h

  push  linear_space_exec
  push  $+5+5+2+2

  mov   edx,$+5+2+2
  mov   ecx,esp

  sysenter

  ;;add   esp,2*4
  add   esp,6*4
  endm