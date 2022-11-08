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

#define NDIRECT 11
#define NINDIRECT (BSIZE / sizeof(uint))
#define NDOUBLEINDIRECT (NINDIRECT * NINDIRECT)
#define MAXFILE (NDIRECT + NINDIRECT+ NDOUBLEINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+2];   // Data block addresses
};
// 每块的inode个数
#define IPB           (BSIZE / sizeof(struct dinode)) 
// inode所在block
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)
// 每个块包含的bitmap指示位（一字节8位）
#define BPB           (BSIZE*8) 
// 指定block的bitmap指示位地址
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart) 
// 路径名长度,路径是由dirent(dir entry)序列组成的文件
#define DIRSIZ 14 

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

