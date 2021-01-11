typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
//new code............
//type lock_t to declare a lock
typedef struct __lock_t {
    uint ticket; //current ticket number is being served 
} lock_t;


