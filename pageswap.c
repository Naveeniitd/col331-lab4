#include "types.h"
#include "param.h"
#include "fs.h"
#include "mmu.h"
#include "proc.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"

#include "defs.h"
#include "buf.h"


#define NSWAP 128
extern struct superblock sb;

struct swap_slot {
    int is_free;  // 0 if the slot is used, 1 if it's free
    uint va;      // Virtual address
    int page_perm; // Permissions
} swap_table[NSWAP];

void swapinit(void) {
    for(int i = 0; i < NSWAP; i++) {
        swap_table[i].is_free = 1;
        swap_table[i].va = 0;
        swap_table[i].page_perm = 0;
    }
}

int find_free_swap_slot(void) {
    for(int i = 0; i < NSWAP; i++) {
        if(swap_table[i].is_free) {
            return i;
        }
    }
    return -1; // No free slot found
}

void swap_out(uint va, int perm) {
    struct buf *b;
    int slot = find_free_swap_slot();
    if(slot == -1) panic("No free swap slots available");

    for (int i = 0; i < 8; i++) {
        uint swap_block = sb.swapstart + (slot * 8) + i;
        b = bread(ROOTDEV, swap_block);
        memmove(b->data, (char*)P2V(va) + (i * BSIZE), BSIZE);
        bwrite(b);
        brelse(b);
    }

    swap_table[slot].is_free = 0;
    swap_table[slot].va = va;
    swap_table[slot].page_perm = perm;
}

void swap_in(uint va) {
    struct buf *b;
    int slot;
    for(slot = 0; slot < NSWAP; slot++) {
        if(!swap_table[slot].is_free && swap_table[slot].va == va) {
            break;
        }
    }
    if(slot == NSWAP) panic("swap in failed: no slot found");

    for (int i = 0; i < 8; i++) {
        uint swap_block = sb.swapstart + (slot * 8) + i;
        b = bread(ROOTDEV, swap_block);
        memmove((char*)P2V(va) + (i * BSIZE), b->data, BSIZE);
        brelse(b);
    }

    swap_table[slot].is_free = 1;
}
