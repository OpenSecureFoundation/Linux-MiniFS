/*
 * mount.minifs - MiniFS Mount Helper
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage(const char *prog)
{
    printf("Usage: %s <device> <mountpoint> [options]\n", prog);
    printf("Options:\n");
    printf("  -o options     Mount options\n");
    printf("  -h            Show help\n");
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *device = argv[1];
    const char *mountpoint = argv[2];
    
    /* Construire la commande pour lancer minifs_fuse */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "minifs_fuse %s %s", device, mountpoint);
    
    /* Ajouter les options */
    for (int i = 3; i < argc; i++) {
        strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
        strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 1);
    }
    
    /* Exécuter */
    return system(cmd);
}
