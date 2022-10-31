// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

#define IPB           (BSIZE / sizeof(struct dinode)) // 每块的inode个数

#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart) // inode所在block

#define BPB           (BSIZE*8) // 每个块包含的bitmap指示位（一字节8位）

#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart) // 指定block的bitmap指示位地址

#define DIRSIZ 14 // 路径名长度,路径是由dirent(dir entry)序列组成的文件

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

