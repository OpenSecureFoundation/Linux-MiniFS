/*
 * mkfs.minifs - MiniFS Formatting Tool
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/minifs.h"

void print_usage(const char *prog)
{
    printf("Usage: %s <device> [options]\n", prog);
    printf("Options:\n");
    printf("  --inodes N       Number of inodes (default: 4096)\n");
    printf("  --block-size N   Block size (default: 4096)\n");
    printf("  --force          Force formatting\n");
    printf("  --version        Show version\n");
    printf("  --help           Show this help\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *device = argv[1];
    uint32_t num_inodes = 4096;
    int force = 0;
    
    /* Parser les options */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--inodes") == 0 && i + 1 < argc) {
            num_inodes = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("mkfs.minifs version 1.0.0\n");
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    /* Obtenir la taille du fichier/périphérique */
    struct stat st;
    if (stat(device, &st) != 0) {
        perror("stat");
        return 1;
    }
    
    uint64_t size = st.st_size;
    
    if (!force) {
        printf("Format %s with MiniFS? This will destroy all data! (y/N): ", device);
        char response;
        scanf(" %c", &response);
        if (response != 'y' && response != 'Y') {
            printf("Aborted.\n");
            return 0;
        }
    }
    
    printf("\n");
    
    int ret = minifs_format(device, size, num_inodes);
    
    return ret == 0 ? 0 : 1;
}
