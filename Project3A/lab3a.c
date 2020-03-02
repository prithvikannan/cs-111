#define _XOPEN_SOURCE 500 // to get rid of compiler warnings

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ext2_fs.h"
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

int mountFd;

const int SB_OFFSET = 1024;
const int BYTE_ALIGN = 4;
const int SIZE_OF_DATESTRING = 32;

struct ext2_super_block sb;
struct ext2_inode inode;
struct ext2_group_desc group;
struct ext2_dir_entry dir;

uint32_t blockSize;
uint32_t inodeSize;

char *timeString(uint32_t time)
{
    time_t rawtime = time;
    struct tm *formattedTime = gmtime(&rawtime);
    char *formattedDate = malloc(sizeof(char) * SIZE_OF_DATESTRING);
    strftime(formattedDate, SIZE_OF_DATESTRING, "%m/%d/%y %H:%M:%S", formattedTime);
    return formattedDate;
}

void printDirectoryEntries(uint32_t start, uint32_t parentNum)
{
    uint32_t curr = start;
    while (curr < start + blockSize)
    {
        if (pread(mountFd, &dir, sizeof(struct ext2_dir_entry), curr) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        uint32_t logicalOffset = curr - start;
        uint32_t inodeNumber = dir.inode;
        uint32_t entryLength = dir.rec_len;
        curr += entryLength;
        uint32_t nameLength = dir.name_len;
        if (!inodeNumber)
            continue;
        fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'", parentNum, logicalOffset, inodeNumber, entryLength, nameLength);
        fprintf(stdout,"%s",  dir.name);
        fprintf(stdout, "'\n");
    }
}

void printInodeDirectorySummary(uint32_t offset, int num)
{

    // dL0

    int ib;
    int indir1;
    int indir2;
    int indir3;

    uint32_t j = 0;
    uint32_t k = 0;
    uint32_t m = 0;

    while (j < 12)
    {
        if (pread(mountFd, &ib, BYTE_ALIGN, offset + 40 + j * BYTE_ALIGN) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        if (ib)
        {
            int curOffset = blockSize * ib;
            printDirectoryEntries(curOffset, num);
        }
        j++;
    }

    // dL1

    if (pread(mountFd, &indir1, BYTE_ALIGN, offset + 40 + 48) < 0)
    {
        fprintf(stderr, "Error: unable to pread\n");
        exit(1);
    }
    if (indir1)
    {
        j = 0;
        while (j < blockSize / BYTE_ALIGN)
        {
            if (pread(mountFd, &ib, BYTE_ALIGN, blockSize * indir1 + j * BYTE_ALIGN) < 0)
            {
                fprintf(stderr, "Error: unable to pread\n");
                exit(1);
            }
            int curOffset = blockSize * ib;
            if (ib)
            {
                printDirectoryEntries(curOffset, num);
            }
            j++;
        }
    }

    // dL2

    if (pread(mountFd, &indir2, BYTE_ALIGN, offset + 40 + 52) < 0)
    {
        fprintf(stderr, "Error: unable to pread\n");
        exit(1);
    }
    if (indir2)
    {
        k = 0;
        while (k < blockSize / BYTE_ALIGN)
        {
            if (pread(mountFd, &indir1, BYTE_ALIGN, indir2 * blockSize + k * BYTE_ALIGN) < 0)
            {
                fprintf(stderr, "Error: unable to pread\n");
                exit(1);
            }
            if (indir1)
            {
                j = 0;
                while (j < blockSize / BYTE_ALIGN)
                {
                    if (pread(mountFd, &ib, BYTE_ALIGN, blockSize * indir1 + j * BYTE_ALIGN) < 0)
                    {
                        fprintf(stderr, "Error: unable to pread\n");
                        exit(1);
                    }
                    int curOffset = blockSize * ib;
                    if (ib)
                        printDirectoryEntries(curOffset, num);
                    j++;
                }
            }
            k++;
        }
    }

    // dL3
    if (pread(mountFd, &indir3, BYTE_ALIGN, offset + 40 + 56) < 0)
    {
        fprintf(stderr, "Error: unable to pread\n");
        exit(1);
    }
    if (indir3)
    {
        m = 0;
        while (m < blockSize / BYTE_ALIGN)
        {
            if (pread(mountFd, &indir2, BYTE_ALIGN, indir3 * blockSize + m * BYTE_ALIGN) < 0)
            {
                fprintf(stderr, "Error: unable to pread\n");
                exit(1);
            }
            if (indir2)
            {
                k = 0;
                while (k < blockSize / BYTE_ALIGN)
                {
                    if (pread(mountFd, &indir1, BYTE_ALIGN, indir2 * blockSize + k * BYTE_ALIGN) < 0)
                    {
                        fprintf(stderr, "Error: unable to pread\n");
                        exit(1);
                    }
                    if (indir1 != 0)
                    {
                        j = 0;
                        while (j < blockSize / BYTE_ALIGN)
                        {
                            if (pread(mountFd, &ib, BYTE_ALIGN, blockSize * indir1 + j * BYTE_ALIGN) < 0)
                            {
                                fprintf(stderr, "Error: unable to pread\n");
                                exit(1);
                            }
                            int curOffset = blockSize * ib;
                            if (ib)
                                printDirectoryEntries(curOffset, num);
                            j++;
                        }
                    }
                    k++;
                }
            }
            m++;
        }
    }
}

void indirectSingle(uint32_t inodeNumber, uint32_t blockNumber)
{
    uint32_t blockOffset = blockNumber * blockSize;
    uint32_t blockValue;
    uint32_t i = 0;
    while (i < blockSize / BYTE_ALIGN)
    {
        if (pread(mountFd, &blockValue, sizeof(blockValue), blockOffset + i * BYTE_ALIGN) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        if (blockValue != 0)
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 1, 12 + i, blockNumber, blockValue);
        i++;
    }
}
void indirectDouble(uint32_t inodeNumber, uint32_t blockNumberOfindirectDouble)
{
    uint32_t blockOffsetL1Indirect = blockNumberOfindirectDouble * blockSize, blockOffset;
    uint32_t blockValueL1Indirect, blockValue;
    uint32_t i = 0, j = 0;
    while (i < blockSize / BYTE_ALIGN)
    {
        if (pread(mountFd, &blockValueL1Indirect, sizeof(blockValue), blockOffsetL1Indirect + i * BYTE_ALIGN) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        blockOffset = blockValueL1Indirect * blockSize;
        if (blockValueL1Indirect != 0)
        {
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 2, 12 + blockSize / BYTE_ALIGN + i * blockSize / BYTE_ALIGN, blockNumberOfindirectDouble, blockValueL1Indirect);
            while (j < blockSize / BYTE_ALIGN)
            {
                if (pread(mountFd, &blockValue, sizeof(blockValue), blockOffset + j * BYTE_ALIGN) < 0)
                {
                    fprintf(stderr, "Error: unable to pread\n");
                    exit(1);
                }
                if (blockValue)
                    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 1, 12 + blockSize / BYTE_ALIGN + i * blockSize / BYTE_ALIGN + j, blockValueL1Indirect, blockValue);
                j++;
            }
        }
        i++;
    }
}
void indirectTriple(uint32_t inodeNumber, uint32_t blockNumberOfindirectTriple)
{
    uint32_t blockOffsetL3 = blockNumberOfindirectTriple * blockSize;
    uint32_t blocksSeenThusFar = 12 + blockSize / BYTE_ALIGN + blockSize / BYTE_ALIGN * blockSize / BYTE_ALIGN;
    uint32_t i = 0, j = 0, k = 0;
    while (i < blockSize / BYTE_ALIGN)
    {
        uint32_t blockOffsetL2, blockNumberindirectDouble;
        if (pread(mountFd, &blockNumberindirectDouble, sizeof(blockNumberindirectDouble), blockOffsetL3 + i * BYTE_ALIGN) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        blockOffsetL2 = blockNumberindirectDouble * blockSize;
        if (blockNumberindirectDouble != 0)
        {
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 3, blocksSeenThusFar + i * (blockSize * blockSize / 8), blockNumberOfindirectTriple, blockNumberindirectDouble);
            while (j < blockSize / BYTE_ALIGN)
            {
                uint32_t blockOffsetL1, blockNumberindirectSingle;
                if (pread(mountFd, &blockNumberindirectSingle, sizeof(blockNumberindirectSingle), blockOffsetL2 + j * BYTE_ALIGN) < 0)
                {
                    fprintf(stderr, "Error: unable to pread\n");
                    exit(1);
                }
                blockOffsetL1 = blockNumberindirectSingle * blockSize;
                if (blockNumberindirectSingle != 0)
                {
                    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 2, blocksSeenThusFar + i * (blockSize * blockSize / 8) + j * (blockSize / BYTE_ALIGN), blockNumberindirectDouble, blockNumberindirectSingle);
                    while (k < blockSize / BYTE_ALIGN)
                    {
                        uint32_t dataBlockNumber;
                        if (pread(mountFd, &dataBlockNumber, sizeof(dataBlockNumber), blockOffsetL1 + k * BYTE_ALIGN) < 0)
                        {
                            fprintf(stderr, "Error: unable to pread\n");
                            exit(1);
                        }
                        if (dataBlockNumber != 0)
                            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 1, blocksSeenThusFar + i * (blockSize * blockSize / 8) + j * (blockSize / BYTE_ALIGN) + k, blockNumberindirectSingle, dataBlockNumber);
                        k++;
                    }
                }
                j++;
            }
        }
        i++;
    }
}

void printInodeSummary(uint32_t firstBlockInode, uint32_t numInodes, uint32_t freeInodeBitmap)
{
    uint32_t i = 0, j = 0;
    char fileType = 0;
    while (i < numInodes)
    {
        int bit = i & 7;
        unsigned char read;
        if (pread(mountFd, &read, sizeof(uint8_t), freeInodeBitmap * blockSize + (uint8_t)(i >> 3)) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        if (!((read >> bit) & 1))
        {
            i++;
            continue;
        }

        if (pread(mountFd, &inode, inodeSize, 1024 + (firstBlockInode - 1) * (blockSize) + i * sizeof(struct ext2_inode)) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }

        if (inode.i_mode == 0 || inode.i_links_count == 0)
        {
            i++;
            continue;
        }

        switch ((inode.i_mode & 0xF000))
        {
        case 0xA000:
            fileType = 's';
            break;
        case 0x8000:
            fileType = 'f';
            break;
        case 0x4000:
            fileType = 'd';
            printInodeDirectorySummary(1024 + (firstBlockInode - 1) * (blockSize) + i * sizeof(struct ext2_inode), i + 1);
            break;
        }

        uint32_t fileSize = inode.i_size;

        fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d", i + 1, fileType, (inode.i_mode & 0xFFF), inode.i_uid, inode.i_gid, inode.i_links_count, timeString(inode.i_ctime), timeString(inode.i_mtime), timeString(inode.i_atime), fileSize, inode.i_blocks);
        if (!(fileType == 's' && fileSize < 60))
        {
            j = 0;
            while (j < 15)
            {
                fprintf(stdout, ",%u", inode.i_block[j]);
                j++;
            }
        }
        else
        {
            fprintf(stdout, ",%u", inode.i_block[0]);
        }
        fprintf(stdout, "\n");

        uint32_t inodeNum = i + 1;

        if (!(fileType == 's' && fileSize < 60))
        {
            if (inode.i_block[12] != 0)
                indirectSingle(inodeNum, inode.i_block[12]);
            if (inode.i_block[13] != 0)
                indirectDouble(inodeNum, inode.i_block[13]);
            if (inode.i_block[14] != 0)
                indirectTriple(inodeNum, inode.i_block[14]);
        }
        i++;
    }
}

void scanFreeEntries(bool isBitmap, uint32_t num, uint32_t freeMap)
{
    uint32_t i = 0;
    while (i < num)
    {
        int bit = i & 7;
        uint8_t byte = i >> 3;
        unsigned char read;
        if (pread(mountFd, &read, sizeof(uint8_t), freeMap * blockSize + byte) < 0)
        {
            fprintf(stderr, "Error: unable to pread\n");
            exit(1);
        }
        if (!((read >> bit) & 1))
        {
            if (isBitmap)
            {
                fprintf(stdout, "BFREE,%d\n", i + 1);
            }
            else
            {
                fprintf(stdout, "IFREE,%d\n", i + 1);
            }
        }
        i++;
    }
}

int main(int argc, char **argv)
{
    if ((argc != 2) || (access(argv[1], F_OK) != 0))
    {
        fprintf(stderr, "Error: invalid argument\n");
        exit(1);
    }

    if ((mountFd = open(argv[1], O_RDONLY)) < 0)
    {
        fprintf(stderr, "Error: could not mount\n");
        exit(2);
    }

    if (pread(mountFd, &sb, sizeof(struct ext2_super_block), SB_OFFSET) < 0)
    {
        fprintf(stderr, "Error: unable to pread\n");
        exit(1);
    }

    // superblock summary

    blockSize = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;
    inodeSize = sb.s_inode_size;
    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", sb.s_blocks_count, sb.s_inodes_count, blockSize, inodeSize, sb.s_blocks_per_group, sb.s_inodes_per_group, sb.s_first_ino);

    // group summary

    if (pread(mountFd, &group, sizeof(struct ext2_group_desc), SB_OFFSET + sizeof(struct ext2_super_block)) < 0)
    {
        fprintf(stderr, "Error: unable to pread\n");
        exit(1);
    }
    fprintf(stdout, "GROUP,%d,%u,%u,%u,%u,%u,%u,%u\n", 0, sb.s_blocks_count, sb.s_inodes_count, group.bg_free_blocks_count, group.bg_free_inodes_count, group.bg_block_bitmap, group.bg_inode_bitmap, group.bg_inode_table);

    // free block entries

    scanFreeEntries(1, sb.s_blocks_count, group.bg_block_bitmap);

    // free I-node entries

    scanFreeEntries(0, sb.s_inodes_count, group.bg_inode_bitmap);

    // I-node summary

    printInodeSummary(group.bg_inode_table, sb.s_inodes_count, group.bg_inode_bitmap);
}
