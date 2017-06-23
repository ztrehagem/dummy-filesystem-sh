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
  unsigned char name[14];
} entry_t;

bytes_t read_blocks(FILE *fp, size_t blocks) {
  bytes_t bytes;
  bytes.len = BLOCK * blocks;
  bytes.head = malloc(sizeof(char) * bytes.len);
  for (size_t i = 0; i < bytes.len; i++) {
    bytes.head[i] = fgetc(fp);
  }
  printf("read %ld block\n", bytes.len / BLOCK);
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

bytes_t load_file(disk_t *disk, inode_t *inode) {
  bytes_t bytes;
  bytes.len = (inode->i_size0 << 8) + (inode->i_size1);
  bytes.head = malloc(sizeof(char) * bytes.len);
  if (inode->i_mode & ILARG) {
    // 間接参照
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

int main(int argc, char const *argv[]) {
  disk_t disk = load_disk("./v6root");

  inode_t *inode = get_inode(&disk, ROOTINO);
  bytes_t dir = load_file(&disk, inode);
  entry_t *entries = (entry_t *)dir.head; // TODO sort
  size_t entries_num = dir.len / sizeof(entry_t);
  for (entry_t *entry = &entries[0]; entry < &entries[entries_num]; entry++) {
    printf("%4d\t%s\n", entry->ino, entry->name);
  }

  return 0;
}
