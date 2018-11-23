#include <sys/mman.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sysinfo.h>

//The smallest block i.e. 2^5(MIN) = 32 bytes 
#define MIN 5 
//The maximum amount of levels i.e. 32, 64.. 4 KByte maximum
#define LEVELS 8
//Defines the size of the largest block(2^12 bytes)
#define PAGE 4096

//Flag values for knowing if the block has been taken or not. 
enum flag {Free, Taken};

//Each block consists of a head and a byte sequence. 
//The byte sequence is handed out to the requester(balloc caller)
struct head {
    enum flag status;
    //Determines the size of the block. Index 0 is 32 bytes
    short int level;
    struct head *next;
    struct head *prev;
};

//Creates 4k blocks for the process. Traps to kernel. 
struct head* new() {
    struct head *new = (struct head *) mmap(NULL,
                                            PAGE,
                                            PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE,
                                            -1,
                                            0);
    if(new == MAP_FAILED) {
        return NULL;
    }
    assert(((long int)new & 0xfff) == 0);
    new->status = Free;
    new->level = LEVELS-1;
    return new;
}

//Asterisk belongs to the return type. 
//Given a block we want to find the buddy of it. 
struct head* buddy(struct head* block) {
    int index = block->level;
    //Generates a bitmask for getting the buddy 
    long int mask = 0x1 << (index + MIN);
    // ^ XOR's and toggles the bit that differentiates this block from its buddy
    return (struct head*)((long int)block ^ mask);
}

struct head* merge(struct head* block, struct head* sibling) {
    struct head* primary;
    if(sibling < block) {
        primary = sibling;
    } else {
        primary = block;
    }
}