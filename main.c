#include <stdio.h>
#include <stdlib.h>

#define BLOCK 512

typedef struct {
  unsigned char *head;
  size_t len;
} bytes_t;

#define IFDIR 040000
#define ILARG 010000

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

bytes_t read_blocks(FILE *fp, size_t blocks) {
  bytes_t bytes;
  bytes.len = BLOCK * blocks;
  bytes.head = malloc(sizeof(char) * bytes.len);
  for (size_t i = 0; i < bytes.len; i++) {
    bytes.head[i] = fgetc(fp);
  }
  printf("read size %ld\n", bytes.len / BLOCK);
  return bytes;
}

disk_t load(const char *filename) {
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

int main(int argc, char const *argv[]) {
  disk_t disk = load("./v6root");

  inode_t *inodes = disk.inodes;
  inode_t *root_inode = &inodes[0];
  printf("ifdir %o\n", root_inode->i_mode & IFDIR);
  printf("ilarg %o\n", root_inode->i_mode & ILARG);

  return 0;
}
