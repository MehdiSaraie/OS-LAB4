#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork(ticks);
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// added
int
sys_calculateBPS(void)
{
  int num = myproc()->tf->ebx;
  cprintf("(k-mode) value in register edx = %d\n", num);

  int i = 0;
  while(i*i < num)
  {
    i++;
  }
  return i-1;
}

// added
int
sys_processStartTime(void)
{
  return myproc()->startTime;
}

// added
int sys_getAncestors(void)
{
  int pid;
  if (argint(0, &pid) < 0)
    return -1;

  if(pid < 0)
    return -1;

  int curr = pid;
  int prev = pid;
  while(curr > 0)
  {
    curr = giveParent(curr);
    if(curr == -1)
      break;
    cprintf("my id: %d, ", prev);

    cprintf("my parent id: %d\n", curr);
    prev = curr;
  }

  return 1;
}

// added
int sys_setSleep(void)
{
  struct rtcdate r1;
  cmostime(&r1);
  uint ticks0 = ticks;
  // cprintf("start time(sec) = %d s\n", r1.second);
  // cprintf("-start time(ticks) = %d ticks\n", ticks0);

  // uint ticks0;
  int time;
  if (argint(0, &time) < 0)
    return -1;
  time *= 100;

  acquire(&tickslock);
  while (ticks - ticks0 < time)
  {
    if (myproc()->killed)
      return -1;

    release(&tickslock);
    sti();
    acquire(&tickslock);
  }
  release(&tickslock);

  uint ticks1 = ticks;
  struct rtcdate r2;
  cmostime(&r2);
  // cprintf("end time(sec) = %d s\n", r2.second);
  // cprintf("-end time(ticks) = %d ticks\n", ticks1);
  cprintf("-duration(sec) = %d s\n", r2.second - r1.second);
  cprintf("-duration(ticks) = %d ticks\n", ticks1 - ticks0);

  return 1;
}

//added
int sys_getDescendants(void)
{
  int pid;
  if (argint(0, &pid) < 0)
    return -1;

  if(pid < 0)
    return -1;

  int curr = pid;
  int prev = pid;
  while(curr > 0)
  {
    curr = giveYoungestChild(curr);
    if(curr == -1)
      break;
    cprintf("my id: %d, ", prev);

    cprintf("my child id: %d\n", curr);
    prev = curr;
  }

  return 1;
}

//added
int sys_spinlockTest(void)
{
  int i;
  if (argint(0, &i) < 0)
    return -1;
  spinlockTest(i);
  return 0;
}




// added
int sys_rwinit(void) {
  return rwinit();
}


//added
int sys_rwtest(void)
{
  int role, priority;
  if (argint(0, &role) < 0 || argint(1, &priority) < 0)
    return -1;
  
  if (priority == 0)
  	return rwtest0(role);
  else if (priority == 1)
  	return rwtest1(role);
  return -1;
}
