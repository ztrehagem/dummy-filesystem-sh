#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "src/hexdump.h"

#define BLOCK 512

typedef struct {
  unsigned char *head;
  size_t len;
} bytes_t;

#define IFDIR 040000
#define ILARG 010000
#define ROOTINO 1

typedef struct {
  unsigned short s_isize;
  unsigned short s_fsize;
  unsigned short s_nfree;
  unsigned short s_free[100];
  unsigned short s_ninode;
  unsigned short s_inode[100];
  unsigned char s_flock;
  unsigned char s_ilock;
  unsigned char s_fmod;
  unsigned char s_ronly;
  unsigned short s_time[2];
  unsigned short pad[50];
} filsys_t;

typedef struct {
  unsigned short i_mode;
  unsigned char i_nlink;
  unsigned char i_uid;
  unsigned char i_gid;
  unsigned char i_size0;
  unsigned short i_size1;
  unsigned short i_addr[8];
  unsigned short i_attime[2];
  unsigned short i_mttime[2];
} inode_t;

typedef struct {
  bytes_t boot_area, super_area, inode_area, storage_area;
  filsys_t *filsys;
  inode_t *inodes;
} disk_t;

typedef struct {
  unsigned short ino;
  char name[14];
} entry_t;

bytes_t read_blocks(FILE *fp, size_t blocks) {
  bytes_t bytes;
  bytes.len = BLOCK * blocks;
  bytes.head = malloc(sizeof(char) * bytes.len);
  for (size_t i = 0; i < bytes.len; i++) {
    bytes.head[i] = fgetc(fp);
  }
  // printf("read %ld block\n", bytes.len / BLOCK);
  return bytes;
}

disk_t load_disk(const char *filename) {
  disk_t disk;
  FILE *fp = fopen(filename, "r");
  disk.boot_area = read_blocks(fp, 1);
  disk.super_area = read_blocks(fp, 1);
  disk.filsys = (filsys_t *)disk.super_area.head;
  disk.inode_area = read_blocks(fp, disk.filsys->s_isize);
  disk.inodes = (inode_t *)disk.inode_area.head;
  disk.storage_area = read_blocks(fp, disk.filsys->s_fsize);
  fclose(fp);
  return disk;
}

int iaddr_to_saddr(disk_t *disk, unsigned short addr) {
  return addr - 2 - disk->filsys->s_isize;
}

inode_t *get_inode(disk_t *disk, size_t ino) {
  return &disk->inodes[ino - 1];
}

size_t get_filesize(inode_t *inode) {
  return (inode->i_size0 << 8) + (inode->i_size1);
}



bytes_t load_file(disk_t *disk, inode_t *inode) {
  bytes_t bytes;
  bytes.len = get_filesize(inode);
  bytes.head = malloc(sizeof(char) * bytes.len);
  if (inode->i_mode & ILARG) {
    // 間接参照
    printf("間接参照は実装してません\n");
    exit(1);
  } else {
    // 直接参照
    for (size_t i = 0; i < bytes.len; i += BLOCK) {
      size_t len = (size_t)fmin(bytes.len - i, i + BLOCK);
      size_t saddr = iaddr_to_saddr(disk, inode->i_addr[i / BLOCK]) * BLOCK;
      memcpy(&bytes.head[i], &disk->storage_area.head[saddr], len);
    }
  }
  return bytes;
}

void ls(disk_t *disk, inode_t *wd) {
  char input[100] = {};
  fgets(input, sizeof(input), stdin);

  char *opts = strtok(input, " \n");
  char opt_l = 0;

  if (opts != NULL && opts[0] == '-') {
    for (char *c = &opts[0]; c < &opts[strlen(opts)]; c++) {
      opt_l |= *c == 'l';
    }
  }

  bytes_t dir = load_file(disk, wd);
  entry_t *entries = (entry_t *)dir.head; // TODO sort
  size_t entries_num = dir.len / sizeof(entry_t);

  for (entry_t *entry = &entries[0]; entry < &entries[entries_num]; entry++) {
    if (opt_l) {
      inode_t *inode = get_inode(disk, entry->ino);
      printf("%c%c%c%c%c%c%c%c%c%c  %8ld ",
        inode->i_mode & IFDIR ? 'd' : '-',
        inode->i_mode & 0400 ? 'r' : '-',
        inode->i_mode & 0200 ? 'w' : '-',
        inode->i_mode & 0100 ? 'x' : '-',
        inode->i_mode & 040 ? 'r' : '-',
        inode->i_mode & 020 ? 'w' : '-',
        inode->i_mode & 010 ? 'x' : '-',
        inode->i_mode & 04 ? 'r' : '-',
        inode->i_mode & 02 ? 'w' : '-',
        inode->i_mode & 01 ? 'x' : '-',
        get_filesize(inode)
      );
    }
    printf("%s\n", entry->name);
  }
}

inode_t *cd(disk_t *disk, inode_t *wd) {
  char input[100] = {};
  fgets(input, sizeof(input) - 1, stdin);
  char *name = strtok(input, " \n");

  bytes_t dir = load_file(disk, wd);
  entry_t *entries = (entry_t *)dir.head; // TODO sort
  size_t entries_num = dir.len / sizeof(entry_t);
  inode_t *target = NULL;
  for (entry_t *entry = &entries[0]; entry < &entries[entries_num]; entry++) {
    if (strncmp(name, entry->name, sizeof(entry->name)) == 0) {
      target = get_inode(disk, entry->ino);
      break;
    }
  }
  if (target && target->i_mode & IFDIR) {
    printf("moved.\n");
    return target;
  } else {
    printf("no such directory\n");
    return wd;
  }
}

int main(int argc, char const *argv[]) {
  disk_t disk = load_disk("./v6root");

  inode_t *wd = get_inode(&disk, ROOTINO);

  char cmd[10];
  while (1) {
    fseek(stdin, 0, SEEK_END);
    printf("-------->> ");
    scanf("%s", cmd);
    if (strcmp(cmd, "exit") == 0) {
      break;
    } else if(strcmp(cmd, "ls") == 0) {
      ls(&disk, wd);
    } else if(strcmp(cmd, "cd") == 0) {
      wd = cd(&disk, wd);
    } else {
      printf("unknown command\n");
    }
  }

  return 0;
}
