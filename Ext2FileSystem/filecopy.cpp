#include <iostream>
#include <string>
#include <stdexcept>
#include <deque>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ext21.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define BASE_OFFSET 1024
#define EXT2_BLOCK_SIZE 1024
#define IMAGE "image.img"
#define SOURCE "testSourceFile.txt"

typedef unsigned char bmap;
#define __NBITS (8 * (int)sizeof(bmap))
#define __BMELT(d) ((d) / __NBITS)
#define __BMMASK(d) ((bmap)1 << ((d) % __NBITS))
#define BM_SET(d, set) ((set[__BMELT(d)] |= __BMMASK(d)))
#define BM_CLR(d, set) ((set[__BMELT(d)] &= ~__BMMASK(d)))
#define BM_ISSET(d, set) ((set[__BMELT(d)] & __BMMASK(d)) != 0)

unsigned int block_size = 1024;
#define BLOCK_OFFSET(block) (block * block_size)
/* ----------------------------------------------------------------- */
char *imageFilePath;
char *sourceFilePath;
char *targetPath;

unsigned int readFileSize;
unsigned int indirectBlockStart = -1;
unsigned int doubleIndirectBlockStart = -1;
unsigned int tripleIndirectBlockStart = -1;

unsigned int nBlockGroups;
unsigned int blockGroupSize;
unsigned int blockSize = 1024;
unsigned int neededBlockCount;
unsigned int nbInodeTable;

unsigned int nBlockDirect;
unsigned int nBlockIndirect;
unsigned int nBlockDouble;
unsigned long long nBlockTriple;

struct ext2_group_desc groupDescription;
struct ext2_super_block superBlock;
struct ext2_group_desc *grDesc;

std::vector<struct ext2_group_desc> fullGroupDesc;
std::deque<std::pair<int, int>> allocatedInodes; // <groupBlockIndex, respectiveInodeIndex>
std::deque<std::pair<int, int>> allocatedBlocks; // <groupBlockIndex, respectiveIndexInThatGroup>
std::deque<int> allocatedInodeIds;
std::deque<int> allocatedBlockPositions; // Block ids of the allocated blocks
/* ----------------------------------------------------------------- */

// Insert Entries Into Inode
void InsertBlocks(int &fd, std::pair<int, int> inodeInfo, ext2_inode &soughtInode)
{
    int sz = allocatedBlockPositions.size() < 12 ? allocatedBlockPositions.size() : 12;
    for (int i = 0; i < sz; ++i)
        soughtInode.i_block[i] = allocatedBlockPositions[i];
    if (indirectBlockStart != -1)
        soughtInode.i_block[12] = indirectBlockStart;
    if (doubleIndirectBlockStart != -1)
        soughtInode.i_block[13] = doubleIndirectBlockStart;
    if (tripleIndirectBlockStart != -1)
        soughtInode.i_block[14] = tripleIndirectBlockStart;
    lseek(fd, BLOCK_OFFSET(fullGroupDesc[inodeInfo.first].bg_inode_table) + (inodeInfo.second - 1) * sizeof(struct ext2_inode), SEEK_SET);
    write(fd, &soughtInode, sizeof(soughtInode));
}

// Pick one block and allocate it
int PickAllocate(int &fd, int &src, int &currBlockGroup, int &lastBlockIndex)
{
    std::pair<int, int> pTemp = allocatedBlocks.front();
    currBlockGroup = pTemp.first;
    lseek(fd, BLOCK_OFFSET(currBlockGroup * superBlock.s_blocks_per_group + pTemp.second), SEEK_SET);

    allocatedBlockPositions.push_back(currBlockGroup * superBlock.s_blocks_per_group + pTemp.second);
    allocatedBlocks.pop_front();
    lastBlockIndex = pTemp.second;

    return currBlockGroup * superBlock.s_blocks_per_group + pTemp.second;
}

// Allocate an indirect currBlockGroup
void AllocateIndirect(int &fd, int &src, int &currBlockGroup, int &lastBlockIndex)
{
    int oldPos = allocatedBlockPositions.back();
    if (indirectBlockStart == -1)
        indirectBlockStart = oldPos;

    unsigned int inodeBuffer[blockSize / 4];
    for (int i = 0; i < blockSize / 4; ++i)
        inodeBuffer[i] = 0;
    write(fd, inodeBuffer, blockSize);
    char buffer[blockSize];
    int count = 0;
    while (!allocatedBlocks.empty() && count++ < nBlockIndirect)
    {
        PickAllocate(fd, src, currBlockGroup, lastBlockIndex);
        inodeBuffer[count - 1] = allocatedBlockPositions.back();
        unsigned int toBeRead = readFileSize >= blockSize ? blockSize : readFileSize;
        read(src, buffer, toBeRead);
        for (int i = toBeRead; i < blockSize; ++i)
            buffer[toBeRead] = '\0';
        write(fd, buffer, blockSize);
        readFileSize -= toBeRead;
    }
    lseek(fd, BLOCK_OFFSET(oldPos), SEEK_SET);
    write(fd, inodeBuffer, blockSize);
}

void AllocateDoubleIndirect(int &fd, int &src, int &currBlockGroup, int &lastBlockIndex)
{
    int oldPos = allocatedBlockPositions.back();

    if (doubleIndirectBlockStart == -1)
        doubleIndirectBlockStart = oldPos;

    int inodeBuffer[blockSize / 4];
    for (int i = 0; i < blockSize / 4; ++i)
        inodeBuffer[i] = 0;
    write(fd, inodeBuffer, blockSize);
    char buffer[blockSize];
    int count = 0;
    while (!allocatedBlocks.empty() && count++ < nBlockIndirect)
    {
        PickAllocate(fd, src, currBlockGroup, lastBlockIndex);
        inodeBuffer[count - 1] = allocatedBlockPositions.back();
        AllocateIndirect(fd, src, currBlockGroup, lastBlockIndex);
    }
    lseek(fd, BLOCK_OFFSET(oldPos), SEEK_SET);
    write(fd, inodeBuffer, blockSize);
}

void AllocateTripleIndirect(int &fd, int &src, int &currBlockGroup, int &lastBlockIndex)
{
    int oldPos = allocatedBlockPositions.back();

    if (tripleIndirectBlockStart == -1)
        tripleIndirectBlockStart = oldPos;

    int inodeBuffer[blockSize / 4];
    for (int i = 0; i < blockSize / 4; ++i)
        inodeBuffer[i] = 0;
    write(fd, inodeBuffer, blockSize);
    char buffer[blockSize];
    int count = 0;
    while (!allocatedBlocks.empty() && count++ < nBlockIndirect)
    {
        PickAllocate(fd, src, currBlockGroup, lastBlockIndex);
        inodeBuffer[count - 1] = allocatedBlockPositions.back();
        AllocateDoubleIndirect(fd, src, currBlockGroup, lastBlockIndex);
    }
    lseek(fd, BLOCK_OFFSET(oldPos), SEEK_SET);
    write(fd, inodeBuffer, blockSize);
}

// Allocate blocks
void AllocateBlocks(int &fd, int &src)
{
    int count = 0;
    lseek(src, 0, SEEK_SET);
    int currBlockGroup = -1;
    int lastBlockIndex = -1;
    bmap buffer[blockSize];
    struct ext2_group_desc temp;
    while (!allocatedBlocks.empty())
    {

        PickAllocate(fd, src, currBlockGroup, lastBlockIndex);
        ++count;
        if (count == 13)
            AllocateIndirect(fd, src, currBlockGroup, lastBlockIndex);
        else if (count == 14)
            AllocateDoubleIndirect(fd, src, currBlockGroup, lastBlockIndex);
        else if (count == 15)
            AllocateTripleIndirect(fd, src, currBlockGroup, lastBlockIndex);
        else
        {
            unsigned int toBeRead = readFileSize >= blockSize ? blockSize : readFileSize;
            read(src, buffer, toBeRead);
            for (int i = toBeRead; i < blockSize; ++i)
                buffer[toBeRead] = '\0';

            write(fd, buffer, blockSize);
            readFileSize -= toBeRead;
        }
    }
}

// Setup inode structure with given source file stats
void SetupInode(int &fd, int inodeIndex, ext2_inode &soughtInode, std::string file)
{
    struct timespec ts;

    lseek(fd, BLOCK_OFFSET(groupDescription.bg_inode_table) + (inodeIndex - 1) * sizeof(struct ext2_inode), SEEK_SET);
    read(fd, &soughtInode, sizeof(soughtInode));
    for (int i = 0; i < 15; ++i)
        soughtInode.i_block[i] = 0;
    struct stat st;
    stat(file.c_str(), &st);
    clock_gettime(CLOCK_REALTIME, &ts);
    
    // Set stats of sourceFile to Inode then write back that into file
    soughtInode.i_links_count = 1;
    soughtInode.i_ctime = ts.tv_sec;
    soughtInode.i_size = st.st_size;
    soughtInode.i_uid = st.st_uid;
    soughtInode.i_gid = st.st_gid;
    soughtInode.i_mode |= EXT2_FT_REG_FILE;
    soughtInode.i_mode |= EXT2_S_IFREG;
    ts = st.st_atim;
    soughtInode.i_atime = ts.tv_sec; 
    ts = st.st_ctim;
    soughtInode.i_ctime = ts.tv_sec;
    ts = st.st_mtim;
    soughtInode.i_mtime = ts.tv_sec;
    soughtInode.i_flags = 0;
    // User
    if (st.st_mode & S_IRUSR)
        soughtInode.i_mode |= EXT2_S_IRUSR;
    if (st.st_mode & S_IWUSR)
        soughtInode.i_mode |= EXT2_S_IWUSR;
    if (st.st_mode & S_IXUSR)
        soughtInode.i_mode |= EXT2_S_IXUSR;

    // Group
    if (st.st_mode & S_IRGRP)
        soughtInode.i_mode |= EXT2_S_IRGRP;
    if (st.st_mode & S_IWGRP)
        soughtInode.i_mode |= EXT2_S_IWGRP;
    if (st.st_mode & S_IXGRP)
        soughtInode.i_mode |= EXT2_S_IXGRP;

    // Other
    if (st.st_mode & S_IROTH)
        soughtInode.i_mode |= EXT2_S_IROTH;
    if (st.st_mode & S_IWOTH)
        soughtInode.i_mode |= EXT2_S_IWOTH;
    if (st.st_mode & S_IXOTH)
        soughtInode.i_mode |= EXT2_S_IXOTH;

    // Calculate the total number of needed block count ( indirect, double, triple blocks are included )
    neededBlockCount = std::ceil(st.st_size / static_cast<float>(blockSize));

    if (neededBlockCount > 12 + nBlockIndirect + nBlockDouble)
    {
        int tripleNeededBlock = std::ceil((st.st_size - 1ll * (12 + nBlockIndirect + nBlockDouble) * blockSize) / static_cast<float>(blockSize));
        int neededDouble = std::ceil(tripleNeededBlock / static_cast<float>(nBlockDouble));
        int neededIndirect = std::ceil((tripleNeededBlock - (neededDouble - 1) * nBlockDouble) / static_cast<float>(nBlockIndirect));

        neededBlockCount += 3 + nBlockIndirect + neededDouble + (neededDouble - 1) * nBlockIndirect + neededIndirect;
    }
    else if (neededBlockCount > 12 + nBlockIndirect)
    {
        int doubleNeededBlock = std::ceil((st.st_size - 1ll * (12 + nBlockIndirect) * blockSize) / static_cast<float>(blockSize));
        int neededIndirect = std::ceil(doubleNeededBlock / static_cast<float>(nBlockIndirect));

        neededBlockCount += 2 + neededIndirect;
    }
    else if (neededBlockCount > 12)
        neededBlockCount++;

    soughtInode.i_blocks = neededBlockCount;
    readFileSize = soughtInode.i_size;
}

// Find and store blocks for allocation
void IdentifyBlocksForAllocation(int &fd)
{
    int count = 0;
    for (int i = 0; i < nBlockGroups; ++i)
    {
        if (fullGroupDesc[i].bg_free_blocks_count > 0)
        {
            int start = fullGroupDesc[i].bg_inode_table + nbInodeTable;
            bmap mp[blockSize];
            lseek(fd, BLOCK_OFFSET(fullGroupDesc[i].bg_block_bitmap), SEEK_SET);
            read(fd, mp, blockSize);
            for (int j = 0; j < blockSize; ++j)
            {
                int k = 0;
                unsigned char ch = mp[j];
                for (; k < 8; ++k)
                {
                    if (!(ch & (1 << k)))
                    {
                        superBlock.s_free_blocks_count--;
                        mp[j] += std::pow(2, k);
                        fullGroupDesc[i].bg_free_blocks_count--;
                        allocatedBlocks.push_back(std::make_pair(i, j * 8 + k + 1));
                        if (++count == neededBlockCount)
                        {
                            lseek(fd, BLOCK_OFFSET(fullGroupDesc[i].bg_block_bitmap), SEEK_SET);
                            write(fd, mp, blockSize);
                            return;
                        }
                    }
                }
            }
            lseek(fd, BLOCK_OFFSET(fullGroupDesc[i].bg_block_bitmap), SEEK_SET);
            write(fd, mp, blockSize);
        }
    }
}

// Find first block group that has a free inode
std::pair<int, int> GetFirstFreeBlockGroupInode(int &fd) // Returns <blockGroupId, respectiveIndexOfInode>
{
    std::pair<int, int> ret = std::make_pair(-1, -1);
    bool flag = false;
    for (int i = 0; i < nBlockGroups; ++i)
    {
        if (fullGroupDesc[i].bg_free_inodes_count > 0)
        {
            superBlock.s_free_inodes_count--;
            fullGroupDesc[i].bg_free_inodes_count--;
            groupDescription = fullGroupDesc[i];
            bmap mp[blockSize];
            lseek(fd, BLOCK_OFFSET(fullGroupDesc[i].bg_inode_bitmap), SEEK_SET);
            read(fd, mp, blockSize);
            for (int j = 0; j < blockSize; ++j)
            {
                unsigned char ch = mp[j];
                for (int k = 0; k < 8; ++k)
                {
                    if (!(ch & (1 << k)))
                    {
                        mp[j] += std::pow(2, k);

                        lseek(fd, BLOCK_OFFSET(fullGroupDesc[i].bg_inode_bitmap), SEEK_SET);
                        write(fd, mp, blockSize);

                        allocatedInodes.push_back(std::make_pair(i, j * 8 + k + 1));
                        return ret = std::make_pair(i, j * 8 + k + 1);
                    }
                }
            }
            break;
        }
    }
    return ret;
}

// Initialize variables
void Init(int &fd)
{

    lseek(fd, 1024, SEEK_SET);
    read(fd, &superBlock, sizeof(superBlock));

    blockSize = 1024 << superBlock.s_log_block_size;
    block_size = blockSize;

    nBlockGroups = std::ceil(superBlock.s_inodes_count / static_cast<float>(superBlock.s_inodes_per_group));
    nbInodeTable = std::ceil((superBlock.s_inodes_per_group * sizeof(struct ext2_inode)) / static_cast<float>(blockSize));
    fullGroupDesc.resize(nBlockGroups);

    nBlockDirect = 12;
    nBlockIndirect = blockSize / sizeof(unsigned int);
    nBlockDouble = nBlockIndirect * nBlockIndirect;
    nBlockTriple = nBlockDouble * nBlockIndirect;

    lseek(fd, blockSize * std::ceil((sizeof(superBlock) + 1024)/ static_cast<float>(blockSize)), SEEK_SET);
    for (int i = 0; i < nBlockGroups; ++i)
        read(fd, &(fullGroupDesc[i]), sizeof(struct ext2_group_desc));
}

int CheckBlock(ext2_inode *root, std::string path, bmap *buffer)
{
    ext2_dir_entry *entry;
    entry = reinterpret_cast<ext2_dir_entry *>(buffer);
    unsigned int readBytes = 0;
    while (readBytes < root->i_size && entry->inode)
    {
        std::string name = "";

        for (int i = 0; i < entry->name_len; ++i)
            name.push_back(entry->name[i]);
        if (name == path)
            return entry->inode;
        entry = static_cast<ext2_dir_entry *>(static_cast<void *>(entry) + entry->rec_len);
        readBytes += entry->rec_len;
    }

    return -1;
}

int CheckIndirect(int &fd, int blockNumber, ext2_inode *root, std::string path, bmap *buffer)
{
    unsigned int ids[blockSize / 4];
    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, ids, blockSize);

    for (int i = 0; i < blockSize / 4; ++i)
    {
        lseek(fd, BLOCK_OFFSET(ids[i]), SEEK_SET);
        read(fd, buffer, blockSize);
        int res = CheckBlock(root, path, buffer);

        if (res)
            return res;
    }

    return -1;
}

int CheckDoubleIndirect(int &fd, int blockNumber, ext2_inode *root, std::string path, bmap *buffer)
{
    unsigned int ids[blockSize / 4];
    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, ids, blockSize);

    for (int i = 0; i < blockSize / 4; ++i)
    {
        int res = CheckIndirect(fd, ids[i], root, path, buffer);
        if (res)
            return res;
    }

    return -1;
}

int CheckTripleIndirect(int &fd, int blockNumber, ext2_inode *root, std::string path, bmap *buffer)
{
    unsigned int ids[blockSize / 4];
    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, ids, blockSize);

    for (int i = 0; i < blockSize / 4; ++i)
    {
        int res = CheckDoubleIndirect(fd, ids[i], root, path, buffer);
        if (res)
            return res;
    }

    return -1;
}

unsigned int TraverseDirectory(int &fd, ext2_inode *root, std::string path)
{
    ext2_dir_entry *entry;
    bmap *buffer = new bmap[blockSize];
    int res;
    for (int i = 0; i < 12; ++i)
    {
        lseek(fd, BLOCK_OFFSET(root->i_block[i]), SEEK_SET);
        read(fd, buffer, blockSize);

        entry = reinterpret_cast<ext2_dir_entry *>(buffer);
        unsigned int readBytes = 0;
        res = CheckBlock(root, path, buffer);
        if (res)
        {
            delete[] buffer;
            return res;
        }
    }

    res = CheckIndirect(fd, root->i_block[12], root, path, buffer);
    if (res)
    {
        delete[] buffer;
        return res;
    }
    res = CheckDoubleIndirect(fd, root->i_block[13], root, path, buffer);
    if (res)
    {
        delete[] buffer;
        return res;
    }

    res = CheckTripleIndirect(fd, root->i_block[14], root, path, buffer);

    delete[] buffer;
    return 0;
}

std::pair<int, int> FindDirectory(int &fd, std::string path)
{
    int range = 1;
    int inodeNumber = 2;
    ext2_group_desc firstGroup;
    lseek(fd, 1024 + blockSize, SEEK_SET);
    read(fd, &firstGroup, sizeof(firstGroup));

    int group = 0;
    while (1)
    {
        if (range == -1)
            break;

        int pos = path.find('/');
        range = pos;
        std::string smallPath = path;
        if (pos != -1)
            smallPath = path.substr(0, pos);

        ext2_inode root;
        lseek(fd, BLOCK_OFFSET(fullGroupDesc[group].bg_inode_table) + (inodeNumber - 1) * sizeof(ext2_inode), SEEK_SET);
        read(fd, &root, sizeof(root));
        inodeNumber = TraverseDirectory(fd, &root, smallPath);
        group = std::floor(inodeNumber / static_cast<float>(superBlock.s_inodes_per_group));
        inodeNumber -= group * superBlock.s_inodes_per_group;
        path.erase(path.begin(), path.begin() + range + 1);
    }
    return std::make_pair(group, inodeNumber);
}

int BlockHelper(int &fd, int blockNumber, int inodeNumber, ext2_inode &node, short type, std::string fileName, bmap *buffer)
{
    int base = 8;
    ext2_dir_entry *entry;

    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, buffer, blockSize);
    entry = reinterpret_cast<ext2_dir_entry *>(buffer);
    unsigned int readBytes = 0;
    if (blockNumber == 0)
    {
        neededBlockCount = 1;
        IdentifyBlocksForAllocation(fd);
        int a, b, c;
        a = b = c = 0;
        PickAllocate(fd, a, b, c);
        blockNumber = allocatedBlockPositions.back();
        allocatedBlockPositions.pop_back();
        lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
        read(fd, buffer, blockSize);

        entry->inode = inodeNumber;
        entry->file_type = type;

        int offset = 4 * std::ceil((fileName.size()) / 4.0f) - fileName.size();
        for (int i = 0; i < fileName.size(); ++i)
            entry->name[i] = fileName[i];
        for (int i = 0; i < offset; ++i)
            entry->name[fileName.size() + i] = '\0';

        entry->name_len = fileName.size();
        entry->rec_len = 1012;
        lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
        write(fd, buffer, blockSize);
        return blockNumber;
    }

    while (readBytes < node.i_size && entry->inode)
    {
        ext2_dir_entry *temp = static_cast<ext2_dir_entry *>(static_cast<void *>(entry) + entry->rec_len);
        if (readBytes + entry->rec_len > node.i_size || !temp->inode)
        {
            int oldLen = entry->rec_len;

            int used = base + std::ceil((entry->name_len) / 4.0f) * 4;

            if(used + 8 + fileName.size() + 4 * std::ceil((fileName.size()) / 4.0f) - fileName.size() > oldLen)
                break;
            entry->rec_len = used;

            entry = static_cast<ext2_dir_entry *>(static_cast<void *>(entry) + used);
            entry->inode = inodeNumber;
            entry->file_type = type;

            int offset = 4 * std::ceil((fileName.size()) / 4.0f) - fileName.size();
            for (int i = 0; i < fileName.size(); ++i)
                entry->name[i] = fileName[i];
            for (int i = 0; i < offset; ++i)
                entry->name[fileName.size() + i] = '\0';

            entry->name_len = fileName.size();
            entry->rec_len = oldLen - used;
            lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
            write(fd, buffer, blockSize);

            return 1;
        }
        std::string name(entry->name);
        entry = static_cast<ext2_dir_entry *>(static_cast<void *>(entry) + entry->rec_len);
        readBytes += entry->rec_len;
    }

    return -1;
}

int IndirectHelper(int &fd, int blockNumber, int inodeNumber, ext2_inode &node, short type, std::string fileName, bmap *buffer)
{
    unsigned int ids[blockSize / 4];

    if (blockNumber == 0)
    {
        neededBlockCount = 1;
        IdentifyBlocksForAllocation(fd);
        int a, b, c;
        a = b = c = 0;
        PickAllocate(fd, a, b, c);
        blockNumber = allocatedBlockPositions.back();
        allocatedBlockPositions.pop_back();
    }
    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, ids, blockSize);

    for (int i = 0; i < blockSize / 4; ++i)
    {
        int old = ids[i];
        lseek(fd, BLOCK_OFFSET(ids[i]), SEEK_SET);
        read(fd, buffer, blockSize);
        int res = BlockHelper(fd, ids[i], inodeNumber, node, type, fileName, buffer);

        if (old == 0)
            ids[i] = res;
        if (res)
        {
            lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
            write(fd, ids, blockSize);
            return blockNumber;
        }
    }

    return -1;
}

int DoubleIndirectHelper(int &fd, int blockNumber, int inodeNumber, ext2_inode &node, short type, std::string fileName, bmap *buffer)
{
    unsigned int ids[blockSize / 4];

    if (blockNumber == 0)
    {
        neededBlockCount = 1;
        IdentifyBlocksForAllocation(fd);
        int a, b, c;
        a = b = c = 0;
        PickAllocate(fd, a, b, c);
        blockNumber = allocatedBlockPositions.back();
        allocatedBlockPositions.pop_back();
    }
    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, ids, blockSize);

    for (int i = 0; i < blockSize / 4; ++i)
    {
        int old = ids[i];
        lseek(fd, BLOCK_OFFSET(ids[i]), SEEK_SET);
        read(fd, buffer, blockSize);
        int res = IndirectHelper(fd, ids[i], inodeNumber, node, type, fileName, buffer);

        if (old == 0)
            ids[i] = res;
        if (res)
        {
            lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
            write(fd, ids, blockSize);
            return blockNumber;
        }
    }

    return -1;
}
int TripleIndirectHelper(int &fd, int blockNumber, int inodeNumber, ext2_inode &node, short type, std::string fileName, bmap *buffer)
{
    unsigned int ids[blockSize / 4];

    if (blockNumber == 0)
    {
        neededBlockCount = 1;
        IdentifyBlocksForAllocation(fd);
        int a, b, c;
        a = b = c = 0;
        PickAllocate(fd, a, b, c);
        blockNumber = allocatedBlockPositions.back();
        allocatedBlockPositions.pop_back();
    }
    lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
    read(fd, ids, blockSize);

    for (int i = 0; i < blockSize / 4; ++i)
    {
        int old = ids[i];
        lseek(fd, BLOCK_OFFSET(ids[i]), SEEK_SET);
        read(fd, buffer, blockSize);
        int res = DoubleIndirectHelper(fd, ids[i], inodeNumber, node, type, fileName, buffer);

        if (old == 0)
            ids[i] = res;
        if (res)
        {
            lseek(fd, BLOCK_OFFSET(blockNumber), SEEK_SET);
            write(fd, ids, blockSize);
            return blockNumber;
        }
    }

    return -1;
}

void AddInodeIntoDirectory(int &fd, int inodeNumber, ext2_inode &node, short type, std::string fileName)
{
    bmap *buffer = new bmap[blockSize];
    int res;
    for (int i = 0; i < 12; ++i)
    {
        int old = node.i_block[i];
        res = BlockHelper(fd, node.i_block[i], inodeNumber, node, type, fileName, buffer);
        if (old == 0)
            node.i_block[i] = res;
        if (res)
        {
            delete[] buffer;
            return;
        }
    }

    int old = node.i_block[12];
    res = IndirectHelper(fd, node.i_block[12], inodeNumber, node, type, fileName, buffer);
    if (old == 0)
        node.i_block[12] = res;
    if (res)
    {
        delete[] buffer;
        return;
    }
    old = node.i_block[13];
    res = IndirectHelper(fd, node.i_block[13], inodeNumber, node, type, fileName, buffer);
    if (old == 0)
        node.i_block[13] = res;

    if (res)
    {
        delete[] buffer;
        return;
    }

    old = node.i_block[14];
    res = IndirectHelper(fd, node.i_block[14], inodeNumber, node, type, fileName, buffer);
    if (old == 0)
        node.i_block[14] = res;

    delete[] buffer;
}

// Copy given source file
int ReadFile(int &fd, std::string sourcePath)
{
    ext2_inode ino;

    int src = open(sourcePath.c_str(), O_RDONLY);
    std::pair<int, int> inodeInfo = GetFirstFreeBlockGroupInode(fd);
    SetupInode(fd, inodeInfo.second, ino, sourcePath);
    ino.i_mode |= EXT2_S_IFREG;
    IdentifyBlocksForAllocation(fd);
    AllocateBlocks(fd, src);
    InsertBlocks(fd, inodeInfo, ino);

    std::cout << inodeInfo.first * superBlock.s_inodes_per_group + inodeInfo.second << " ";

    while (!allocatedBlockPositions.empty())
    {
        std::cout << allocatedBlockPositions.front();
        if (allocatedBlockPositions.size() != 1)
            std::cout << ",";
        else
            std::cout << "\n";
        allocatedBlockPositions.pop_front();
    }

    indirectBlockStart = doubleIndirectBlockStart = tripleIndirectBlockStart = -1;
    close(src);
    return inodeInfo.first * superBlock.s_inodes_per_group + inodeInfo.second;
}

// Copy given source directory
int ReadDirectory(int &fd, int pid, std::string sourcePath)
{
    ext2_inode in;

    int src = open(sourcePath.c_str(), O_RDONLY);

    std::pair<int, int> inodeInfo = GetFirstFreeBlockGroupInode(fd);
    SetupInode(fd, inodeInfo.second, in, sourcePath);

    neededBlockCount = 1;
    IdentifyBlocksForAllocation(fd);
    int t1 = 1, t2 = -1;
    PickAllocate(fd, src, t1, t2);
    in.i_block[0] = allocatedBlockPositions.back();
    allocatedBlockPositions.pop_back();

    DIR *temp;
    struct dirent *dir;

    temp = opendir(sourcePath.c_str());

    in.i_mode |= EXT2_S_IFDIR;
    in.i_size = blockSize;

    ext2_dir_entry *entry;
    bmap *buffer = new bmap[blockSize];

    lseek(fd, BLOCK_OFFSET(in.i_block[0]), SEEK_SET);
    read(fd, buffer, blockSize);

    // Add .
    entry = reinterpret_cast<ext2_dir_entry *>(buffer);
    entry->rec_len = 12;
    entry->name_len = 1;
    entry->name[0] = '.';
    entry->name[1] = entry->name[2] = entry->name[3] = '\0';
    entry->file_type = 2;
    entry->inode = inodeInfo.first * superBlock.s_inodes_per_group + inodeInfo.second;
    // Add ..
    entry = static_cast<ext2_dir_entry *>(static_cast<void *>(entry) + entry->rec_len);
    entry->rec_len = blockSize - 24;
    entry->name_len = 2;
    entry->name[0] = entry->name[1] = '.';
    entry->name[2] = entry->name[3] = '\0';
    entry->file_type = 2;
    entry->inode = pid;

    lseek(fd, BLOCK_OFFSET(in.i_block[0]), SEEK_SET);
    write(fd, buffer, blockSize);

    delete[] buffer;
    std::vector<std::string> files;
    while (dir = readdir(temp))
    {
        std::string path(sourcePath);
        path += "/";
        path += std::string(dir->d_name);
        files.push_back(path);
    }
    std::sort(files.begin(), files.end(), std::greater<std::string>()); // Sort files
    while (!files.empty())                                              // Take one directory entry and copy it
    {
        std::string path = files.back();
        std::string t;
        unsigned int ps = path.rfind('/');
        if (ps != std::string::npos)
            t = path.substr(ps + 1, path.size());

        if (t == "." || t == "..")
        {
            files.pop_back();
            continue;
        }

        files.pop_back();
        struct stat st;
        stat(path.c_str(), &st);

        short type;
        int resInode;
        if (S_ISDIR(st.st_mode))
        {
            type = 2;
            resInode = ReadDirectory(fd, pid, path);
        }
        else
        {
            type = 1;
            resInode = ReadFile(fd, path);
        }

        ps = path.rfind('/');
        if (ps != std::string::npos)
            path = path.substr(ps + 1, path.size());

        AddInodeIntoDirectory(fd, resInode, in, type, path);
    }

    lseek(fd, BLOCK_OFFSET(fullGroupDesc[inodeInfo.first].bg_inode_table) + (inodeInfo.second - 1) * sizeof(struct ext2_inode), SEEK_SET);
    write(fd, &in, sizeof(in)); // Write allocated dir inode

    close(src);
    return inodeInfo.first * superBlock.s_inodes_per_group + inodeInfo.second;
}

int main(int argc, char *argv[])
{
    imageFilePath = argv[1];
    sourceFilePath = argv[2];
    targetPath = argv[3];
    std::string path(targetPath);

    int fd = open(imageFilePath, O_RDWR);

    lseek(fd, 1024, SEEK_SET);
    read(fd, &superBlock, sizeof(superBlock));
    Init(fd);

    bool flag = false;
    for (int i = 0; i < path.size(); ++i)
        if (path[i] < '0' || path[i] > '9')
            flag = true;

    if(targetPath[0] == '/')
        path.erase(path.begin());
    if(path.back() == '/')
        path.pop_back();

    std::pair<int, int> dirLoc;
    ext2_inode dir;
    if (flag) // If targetdirpath is given
    {
        std::pair<int, int> targetDir = FindDirectory(fd, path);
        dirLoc = targetDir;
        lseek(fd, BLOCK_OFFSET(fullGroupDesc[targetDir.first].bg_inode_table) + (targetDir.second - 1) * sizeof(ext2_inode), SEEK_SET);
        read(fd, &dir, sizeof(ext2_inode));
    }
    else // if targetinode is given
    {
        int inodeNumber = atoi(targetPath);
        int group = std::floor(inodeNumber / static_cast<float>(superBlock.s_inodes_per_group));
        inodeNumber -= group * superBlock.s_inodes_per_group;
        dirLoc = std::make_pair(group, inodeNumber);
        lseek(fd, BLOCK_OFFSET(fullGroupDesc[group].bg_inode_table) + (inodeNumber - 1) * sizeof(ext2_inode), SEEK_SET);
        read(fd, &dir, sizeof(ext2_inode));
    }

    std::string source(sourceFilePath);

    struct stat st;
    stat(sourceFilePath, &st);
    int resInodeNumber;
    short type;
    if (S_ISDIR(st.st_mode)) // Is source file is a directory?
    {
        type = 2;
        resInodeNumber = ReadDirectory(fd, dirLoc.first * superBlock.s_inodes_per_group + dirLoc.second, source);
    }
    else
    {
        type = 1;
        resInodeNumber = ReadFile(fd, source);
    }

    unsigned int ps = source.rfind('/');
    if (ps != std::string::npos)
        source = source.substr(ps + 1, source.size());
    AddInodeIntoDirectory(fd, resInodeNumber, dir, type, source); // Add allocated inode into targetdir

    lseek(fd, 1024, SEEK_SET);
    write(fd, &superBlock, sizeof(superBlock));
    lseek(fd, blockSize * std::ceil((1024 + sizeof(superBlock)) / static_cast<float>(blockSize)), SEEK_SET);

    for (int j = 0; j < nBlockGroups; ++j)
        write(fd, &fullGroupDesc[j], sizeof(groupDescription));


    close(fd);
    return 0;
}
