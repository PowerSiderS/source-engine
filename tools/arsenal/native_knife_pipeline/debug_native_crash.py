import os
"""Attach only to an explicitly supplied private test PID; capture crash symbols."""
import ctypes as C
from ctypes import wintypes as W
import json,msvcrt,struct,threading
from pathlib import Path
def monitor(pid,folder):
 def run():
  k=C.WinDLL('kernel32',use_last_error=True)
  dbg=C.WinDLL(r'C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\dbghelp.dll')
  k.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenProcess.restype=W.HANDLE
  k.OpenThread.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenThread.restype=W.HANDLE
  k.ReadProcessMemory.argtypes=[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
  k.GetThreadContext.argtypes=[W.HANDLE,C.c_void_p]
  dbg.SymInitializeW.argtypes=[W.HANDLE,W.LPCWSTR,W.BOOL];dbg.SymInitializeW.restype=W.BOOL
  dbg.SymFromAddr.argtypes=[W.HANDLE,C.c_ulonglong,C.POINTER(C.c_ulonglong),C.c_void_p];dbg.SymFromAddr.restype=W.BOOL
  dbg.MiniDumpWriteDump.argtypes=[W.HANDLE,W.DWORD,W.HANDLE,W.DWORD,C.c_void_p,C.c_void_p,C.c_void_p]
  class Record(C.Structure):
   _fields_=[('code',W.DWORD),('flags',W.DWORD),('record',C.c_void_p),('address',C.c_void_p),('count',W.DWORD),('parameters',C.c_ulonglong*15)]
  class ExceptionInfo(C.Structure):_fields_=[('record',Record),('first',W.DWORD)]
  class EventData(C.Union):_fields_=[('exception',ExceptionInfo),('raw',C.c_byte*160)]
  class Event(C.Structure):_fields_=[('code',W.DWORD),('pid',W.DWORD),('tid',W.DWORD),('data',EventData)]
  class Symbol(C.Structure):
   _fields_=[('SizeOfStruct',W.ULONG),('TypeIndex',W.ULONG),('Reserved',C.c_ulonglong*2),('Index',W.ULONG),('Size',W.ULONG),('ModBase',C.c_ulonglong),('Flags',W.ULONG),('Value',C.c_ulonglong),('Address',C.c_ulonglong),('Register',W.ULONG),('Scope',W.ULONG),('Tag',W.ULONG),('NameLen',W.ULONG),('MaxNameLen',W.ULONG),('Name',C.c_char*1)]
  process=k.OpenProcess(0x1fffff,False,pid)
  if not k.DebugActiveProcess(pid):return
  k.DebugSetProcessKillOnExit(False)
  while True:
   event=Event()
   if not k.WaitForDebugEvent(C.byref(event),1000):continue
   status=0x10002
   if event.code==1:
    exception=event.data.exception
    if exception.record.code!=0x80000003:status=0x80010001
    if exception.record.code==0xc0000005 and not exception.first:
     crash_folder=Path(folder);crash_folder.mkdir(exist_ok=True)
     context=C.create_string_buffer(1248);aligned=(C.addressof(context)+15)&~15
     C.c_ulong.from_address(aligned+48).value=0x10001f
     thread=k.OpenThread(0x1fffff,False,event.tid);k.GetThreadContext(thread,aligned)
     rip=C.c_ulonglong.from_address(aligned+248).value;rsp=C.c_ulonglong.from_address(aligned+152).value
     search=r'C:\Users\SnyX\Desktop\projeto clone\source-engine\output\bin;C:\Users\SnyX\Desktop\projeto clone\source-engine\output\cstrike\bin'
     dbg.SymSetOptions(0x14);dbg.SymInitializeW(process,search,True)
     def symbol(address):
      buffer=C.create_string_buffer(C.sizeof(Symbol)+1024);info=C.cast(buffer,C.POINTER(Symbol));info.contents.SizeOfStruct=C.sizeof(Symbol);info.contents.MaxNameLen=1024
      displacement=C.c_ulonglong()
      if dbg.SymFromAddr(process,address,C.byref(displacement),buffer):return C.string_at(C.addressof(buffer)+Symbol.Name.offset).decode(errors='replace')+'+'+hex(displacement.value)
      return hex(address)
     stack=C.create_string_buffer(1024);read=C.c_size_t();k.ReadProcessMemory(process,rsp,stack,1024,C.byref(read))
     addresses=[rip]+[value[0] for value in struct.iter_unpack('<Q',stack.raw[:read.value//8*8])]
     registers={r:hex(C.c_ulonglong.from_address(aligned+o).value) for r,o in dict(rax=120,rcx=128,rdx=136,rbx=144,rsp=152,rbp=160,rsi=168,rdi=176,r8=184,r9=192,r10=200,r11=208,r12=216,r13=224,r14=232,r15=240,rip=248).items()}
     instruction=C.create_string_buffer(160);k.ReadProcessMemory(process,rip-80,instruction,160,C.byref(read))
     result=dict(exception=hex(exception.record.code),address=hex(exception.record.address),rip=hex(rip),registers=registers,parameters=[hex(exception.record.parameters[i]) for i in range(exception.record.count)],instructions=instruction.raw[:read.value].hex(),stack=[symbol(a) for a in addresses if a>0x10000])
     (crash_folder/'crash.json').write_text(json.dumps(result,indent=2))
     class Pointers(C.Structure):_fields_=[('record',C.c_void_p),('context',C.c_void_p)]
     class DumpInfo(C.Structure):_fields_=[('tid',W.DWORD),('pointers',C.c_void_p),('client',W.BOOL)]
     pointers=Pointers(C.addressof(exception.record),aligned);info=DumpInfo(event.tid,C.addressof(pointers),False)
     with (crash_folder/'crash.dmp').open('wb') as file:dbg.MiniDumpWriteDump(process,pid,msvcrt.get_osfhandle(file.fileno()),0,C.byref(info),None,None)
     k.CloseHandle(thread)
   elif event.code in (3,6):
    handle=C.c_void_p.from_buffer(event.data).value
    if handle:k.CloseHandle(W.HANDLE(handle))
   k.ContinueDebugEvent(event.pid,event.tid,status)
   if event.code==5:break
  k.CloseHandle(process)
 thread=threading.Thread(target=run,daemon=True);thread.start();return thread
