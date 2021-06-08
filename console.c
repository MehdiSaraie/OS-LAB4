// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

//4D
#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void consputc(int);

static int panicked = 0;

static struct
{
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{

  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do
  {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...)
{
  //
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint *)(void *)(&fmt + 1);
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
  {
    if (c != '%')
    {
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch (c)
    {
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if ((s = (char *)*argp++) == 0)
        s = "(null)";
      for (; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if (locking)
    release(&cons.lock);
}

void panic(char *s)
{

  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *)P2V(0xb8000); // CGA memory

#define INPUT_BUF 128
struct
{
  char buf[INPUT_BUF];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} input;
static int movedLeft = 0;
static int delDist = 0;

#define C(x) ((x) - '@') // Control-x

void clearFurther(int n, int *pos)
{
  (*pos) += n;
  for (int i = 0; i < n; i++)
  {
    crt[(*pos)--] = (' ' & 0xff) | 0x0700;
  }
}

static void
cgaputc(int c)
{

  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);

  if (c == '\n')
  {
    if (movedLeft > 0)
    { // remove excess blank
      for (int i = 0; i < movedLeft - 1; i++)
      {
        c = input.buf[input.e - movedLeft + i];
        crt[pos++] = (c & 0xff) | 0x0700;
      }
      crt[pos] = (' ' & 0xff) | 0x0700;
    }
    pos += 80 - pos % 80;
  }
  else if (c == BACKSPACE)
  {
    if (pos > 0)
    {
      --pos;
      if (movedLeft > 0)
      {
        clearFurther(movedLeft + 1, &pos);
        crt[pos++] = (' ' & 0xff) | 0x0700;
        for (int i = 0; i < movedLeft; i++)
        {
          c = input.buf[input.e - movedLeft + i];
          crt[pos++] = (c & 0xff) | 0x0700;
        }
        pos -= (movedLeft + 1);
      }
    }
  }
  else
  {
    if (c == 0xE4)
    { // moving the cursor
      // add a blank...print the followings...move the cursor back to the blank
      pos--;
      crt[pos++] = (' ' & 0xff) | 0x0700;
      for (int i = 0; i < movedLeft; i++)
      {
        c = input.buf[input.e - movedLeft + i];
        crt[pos++] = (c & 0xff) | 0x0700;
      }
      pos -= (movedLeft + 1);
    }
    else if (c == 0xE5)
    { // moving the cursor
      // switch the blank with it's right neighbour...print the followings...move the cursor back to the blank
      c = input.buf[input.e - movedLeft - 1];
      crt[pos++] = (c & 0xff) | 0x0700;
      crt[pos++] = (' ' & 0xff) | 0x0700;
      for (int i = 0; i < movedLeft; i++)
      {
        c = input.buf[input.e - movedLeft + i];
        crt[pos++] = (c & 0xff) | 0x0700;
      }
      pos -= (movedLeft + 1);
    }
    else if (c == C('K'))
    {
      pos -= delDist;
      crt[pos++] = (' ' & 0xff) | 0x0700;
      for (int i = 0; i < movedLeft; i++)
      {
        c = input.buf[input.e - movedLeft + i];
        crt[pos++] = (c & 0xff) | 0x0700;
      }
      pos -= (movedLeft + 1);
    }
    else if (c == C('L'))
    {
      // pos += delDist;
      for (int i = 0; i < delDist; i++)
      {
        c = input.buf[input.e - movedLeft - delDist + i];
        crt[pos++] = (c & 0xff) | 0x0700;
      }
      crt[pos++] = (' ' & 0xff) | 0x0700;
      for (int i = 0; i < movedLeft; i++)
      {
        c = input.buf[input.e - movedLeft + i];
        crt[pos++] = (c & 0xff) | 0x0700;
      }
      pos -= (movedLeft + 1);
    }
    else
    {
      if (movedLeft > 0)
      { // printing at cursors position
        crt[pos++] = (c & 0xff) | 0x0700;
        crt[pos++] = (' ' & 0xff) | 0x0700;
        for (int i = 0; i < movedLeft; i++)
        {
          c = input.buf[input.e - movedLeft + i];
          crt[pos++] = (c & 0xff) | 0x0700;
        }
        pos -= (movedLeft + 1);
      }
      else
        crt[pos++] = (c & 0xff) | 0x0700; // black on white
    }
  }

  if (pos < 0 || pos > 25 * 80)
    panic("pos under/overflow");

  if ((pos / 80) >= 24)
  { // Scroll up.
    memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
    pos -= 80;
    memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
  crt[pos] = ' ' | 0x0700;
}

void consputc(int c)
{

  if (panicked)
  {
    cli();
    for (;;)
      ;
  }

  if (c == BACKSPACE)
  {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  }
  else if (c == 0xE5 || c == 0xE4 || c == C('K') || c == C('L'))
  {
    cgaputc(c);
    return;
  }
  else
    uartputc(c);
  cgaputc(c);
}

int isDelimiter(char in)
{
  switch (in)
  {
  case '.':
    return 1;
  case ',':
    return 1;
  case ';':
    return 1;
  case ':':
    return 1;
  case ' ':
    return 1;
  default:
    return 0;
  }
}

void extendedDelete(int currentPos, int n)
{
  for (int i = 0; i < n; i++)
  {
    if (input.e - currentPos != input.w)
    {
      for (int j = 0; j < currentPos; j++)
      {
        input.buf[(input.e - currentPos - 1 + j) % INPUT_BUF] = input.buf[(input.e - currentPos + j) % INPUT_BUF];
      }
      input.e--;
      consputc(BACKSPACE);
    }
  }
}

void consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while ((c = getc()) >= 0)
  {
    switch (c)
    {
    case C('P'): // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'): // Kill line.
      if(movedLeft)
      {
        delDist = movedLeft;
        movedLeft = 0;
        consputc(C('L'));
      }
      while (input.e != input.w &&
             input.buf[(input.e - 1) % INPUT_BUF] != '\n')
      {
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'):
    case '\x7f': // Backspace
      extendedDelete(movedLeft, 1);
      break;
    case 0xE4: // left arrow key
      if (input.e - movedLeft > input.w)
      {
        movedLeft++;
        consputc(c);
      }
      break;
    case 0xE5: // right arrow key
      if (movedLeft > 0)
      {
        movedLeft--;
        consputc(c);
      }
      break;
    case C('K'): // ctrl + <-
      for (int i = 0; i < input.e - movedLeft - input.w; i++)
      {
        if (isDelimiter(input.buf[input.e - movedLeft - i - 2]))
        {
          delDist = i + 1;
          movedLeft += delDist;
          consputc(c);
          break;
        }
        else if (i == input.e - movedLeft - input.w - 1)
        {
          delDist = i + 1;
          movedLeft += delDist;
          consputc(c);
          break;
        }
      }
      break;
    case C('L'): // ctrl + ->
      for (int i = 0; i < movedLeft - 1; i++)
      {
        if (isDelimiter(input.buf[input.e - movedLeft + i + 1]))
        {
          delDist = i + 1;
          movedLeft -= delDist;
          consputc(c);
          break;
        }
        else if (i == movedLeft - 2)
        {
          delDist = i + 2;
          movedLeft -= delDist;
          consputc(c);
          break;
        }
      }
      break;
    case C('I'):
      extendedDelete(movedLeft, input.e - input.w - movedLeft);
      break;
    default:
      if (c != 0 && input.e - input.r < INPUT_BUF)
      {
        c = (c == '\r') ? '\n' : c;
        if (c == '\n' || c == C('D'))
        {
          input.buf[input.e++ % INPUT_BUF] = c;
          movedLeft = (movedLeft > 0) ? movedLeft + 1 : 0;
        }
        else
        {
          for (int i = 0; i < movedLeft + 1; i++) // shift right
            input.buf[(input.e + 1 - i) % INPUT_BUF] = input.buf[(input.e - i) % INPUT_BUF];
          input.buf[(input.e - movedLeft) % INPUT_BUF] = c;
          input.e++;
        }

        consputc(c);
        if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF)
        {
          if (movedLeft > 0)
            movedLeft = 0;

          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if (doprocdump)
  {
    procdump(); // now call procdump() wo. cons.lock held
  }
}

int consoleread(struct inode *ip, char *dst, int n)
{

  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while (n > 0)
  {
    while (input.r == input.w)
    {

      if (myproc()->killed)
      {
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D'))
    { // EOF
      if (n < target)
      {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n)
{

  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for (i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void consoleinit(void)
{

  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}