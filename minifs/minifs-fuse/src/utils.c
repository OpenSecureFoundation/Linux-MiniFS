/*
 * MiniFS - Utility Functions
 * Fonctions utilitaires pour les bitmaps et autres opérations
 */

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include "../include/minifs.h"

void minifs_set_bit(uint8_t *bitmap, uint32_t bit)
{
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void minifs_clear_bit(uint8_t *bitmap, uint32_t bit)
{
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int minifs_test_bit(const uint8_t *bitmap, uint32_t bit)
{
    return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

uint32_t minifs_find_first_zero_bit(const uint8_t *bitmap, uint32_t size)
{
    uint32_t bytes = (size + 7) / 8;
    
    for (uint32_t i = 0; i < bytes; i++) {
        if (bitmap[i] != 0xFF) {
            for (int bit = 0; bit < 8; bit++) {
                uint32_t pos = i * 8 + bit;
                if (pos >= size) return size;
                
                if (!minifs_test_bit(bitmap, pos)) {
                    return pos;
                }
            }
        }
    }
    
    return size;
}

uint64_t minifs_current_time(void)
{
    return (uint64_t)time(NULL);
}

void minifs_inode_set_mode(minifs_inode_t *inode, mode_t mode)
{
    if (inode) {
        inode->mode = mode;
    }
}
