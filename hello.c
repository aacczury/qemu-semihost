#include <stdio.h>
#include <inttypes.h>

#define READ_COUNTER_ADDR 0x40050000

#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */ 
#define	O_APPEND	0x0008		/* set append mode */
#define	O_CREAT		0x0200		/* create if nonexistant */
#define	O_TRUNC		0x0400		/* truncate to zero length */

enum Semihost_SYS_CALL {
    Semihost_SYS_OPEN = 0x01,
    Semihost_SYS_CLOSE = 0x02,
    Semihost_SYS_WRITEC = 0x03,
    Semihost_SYS_WRITE0 = 0x04,
    Semihost_SYS_WRITE = 0x05,
    Semihost_SYS_READ = 0x06,
    Semihost_SYS_READC = 0x07,
    Semihost_SYS_ISERROR = 0x08,
    Semihost_SYS_ISTTY = 0x09,
    Semihost_SYS_SEEK = 0x0A,
    Semihost_SYS_FLEN = 0x0C,
    Semihost_SYS_REMOVE = 0x0E,
    Semihost_SYS_TMPNAM = 0x0D,
    Semihost_SYS_RENAME = 0x0F,
    Semihost_SYS_CLOCK = 0x10,
    Semihost_SYS_TIME = 0x11,
    Semihost_SYS_HOSTTEM = 0x12,
    Semihost_SYS_ERRNO = 0x13,
    Semihost_SYS_GET_CMDLINE = 0x15,
    Semihost_SYS_HEAPINFO = 0x16,
    Semihost_SYS_ELAPSED = 0x30,
    Semihost_SYS_TICKFREQ = 0x31,
};

int SemihostCall(enum Semihost_SYS_CALL action, void *arg) __attribute__ ((naked));
int SemihostCall(enum Semihost_SYS_CALL action, void *arg)
{
    __asm__( \
      "bkpt 0xAB\n"\
      "nop\n" \
      "bx lr\n"\
        :::\
    );
}

////zzz00072
// my struct is wrong, so see the zzz00072's struct
// so this is "word" is arm sys call param?
union param_t
{
    int pdInt;
    void *pdPtr;
    char *pdChrPtr;
};

typedef union param_t param;

static int semihost_open(const char *filename, int flags){
	int opening_mode = 0;
	if( flags & O_RDWR )
		opening_mode |= 2;
		if ((flags & O_CREAT) || (flags & O_TRUNC) || (flags & O_WRONLY))
		opening_mode |= 4;
	if( flags & O_APPEND ){
		opening_mode &= ~4;
		opening_mode |= 8;
	}

    param semi_param[3] = {0};

    semi_param[0].pdChrPtr = filename;
    semi_param[1].pdInt = opening_mode;
    semi_param[2].pdPtr = strlen(filename);

	return SemihostCall( Semihost_SYS_OPEN, semi_param );
}

static int semihost_close(int fd){
	return SemihostCall( Semihost_SYS_CLOSE, (void *)fd );
}

static int semihost_write(int fd, void *ptr, int numbytes){
    param semi_param[3] = {0};

    semi_param[0].pdInt = fd;
    semi_param[1].pdPtr = ptr;
    semi_param[2].pdInt = numbytes;

    return SemihostCall( Semihost_SYS_WRITE, semi_param );
}

static int semihost_read(int fd, void *ptr, int numbytes){
    param semi_param[3] = {0};

    semi_param[0].pdInt = fd;
    semi_param[1].pdPtr = ptr;
    semi_param[2].pdInt = numbytes;

	return SemihostCall( Semihost_SYS_READ, semi_param );
}

int32_t *read_counter = (int32_t *) READ_COUNTER_ADDR;
int main(void)
{
	printf("This is a test program for QEMU counter device\n");
	printf("See http://github.com/krasin/qemu-counter for more details\n\n");
	printf("Let's check if the Read Counter device presented\n");
	for (int i = 0; i < 10; i++) {
		printf("The device has been accessed for %"PRId32" times!\n", *read_counter);
	}

    char buffer[65] = {0};
    int gotten;
	int fh;
	char happy[6] = "happy\0";
	fh = semihost_open("test", O_RDWR );
	printf("file handle %d\n", fh);
    if(gotten = semihost_read(fh, buffer, 64)){
		buffer[64-gotten] = '\0';
		printf("%s\n",buffer);
	}
    if (semihost_write(fh, happy, 5) == -1) 
        printf("WRITE ERROR!");
    if( semihost_close( fh ) == -1 )
    	return -1;

	int32_t now = *read_counter;
	if (now == 0) {
		printf("ERROR - No Read Counter detected\n");
	}
	else if (now == 11) {
		printf("OK - Read Counter works as intended\n");
	}
	else {
		printf("ERROR - Something is wrong with Read Counter\n");
	}
	return 0;
}
