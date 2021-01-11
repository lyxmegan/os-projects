#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

// new code starts from here.......................
//build tread library
int 
thread_create(void (*start_routine)(void*, void*), void *arg1, void *arg2)
{
  const int pgSize = 4096;
  lock_t lock;
	lock_init(&lock);
	lock_acquire(&lock);
// we get two pages (in case we get back memory that is not page aligned)
  void *stack = malloc(pgSize*2);
  lock_release(&lock);
  if((uint)stack % pgSize != 0) // not page aligned
  {
    stack += (pgSize - ((uint)stack % pgSize)); // offset to start at next page
  }
  //create the child thread
  int result = clone(start_routine, arg1, arg2, stack);
  return result;
}

int 
thread_join()
{
  void* stack = malloc(sizeof(void*));
  int result = join(&stack);
  lock_t lock;
	lock_init(&lock);
	lock_acquire(&lock);
	free(stack);
	lock_release(&lock);
	return result;
}

//create ticket lock(reference the code from the chapter 28: Locks)
int 
lock_init(lock_t *lock)
{ 
  //0: lock is available, 1: lock is held
  lock->ticket = 0;
  return 0;
}

void 
lock_acquire(lock_t *lock)
{
  while(xchg(&lock->ticket, 1) != 0)
	{}
}

void 
lock_release(lock_t *lock)
{
  xchg(&lock->ticket, 0);
}
