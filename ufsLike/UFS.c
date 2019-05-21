#include "UFS.h"

#include "disque.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <setjmp.h>
#include <stdbool.h>



#define TRY do{ jmp_buf ex_buf__; if( !setjmp(ex_buf__) ){
#define CATCH } else {
#define ETRY } }while(0)
#define THROW longjmp(ex_buf__, 1)


int findFirstFreeInod(){
    char BlockFreeInod[BLOCK_SIZE];
    ReadBlock(FREE_INODE_BITMAP, BlockFreeInod);
    int i = 0;
    for(i; i < 20; i++){
        if (BlockFreeInod[i] == 1){
            return i+4;
        }
    }
    return -1;
}

int findFirstFreeDataBlock(){
    char BlockFreeData[BLOCK_SIZE];
    ReadBlock(FREE_BLOCK_BITMAP, BlockFreeData);
    int i = 24;
    for(i; i < 44; i++){
        if (BlockFreeData[i] == 1){
            return i;
        }
    }
    return -1;
}

int ReleaseBlock(UINT16 BlockNum) {
    if(BlockNum<44 && BlockNum>23){
        char BlockFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
        BlockFreeBitmap[BlockNum] = 1;
        WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
        return 1;
    }else{return 0;}
}

int HoldBlock(UINT16 BlockNum) {
    if(BlockNum<44 && BlockNum>23){
        char BlockHoldBitmap[BLOCK_SIZE];
        ReadBlock(FREE_BLOCK_BITMAP, BlockHoldBitmap);
        BlockHoldBitmap[BlockNum] = 0;
        WriteBlock(FREE_BLOCK_BITMAP, BlockHoldBitmap);
        return 1;
    }else{return 0;}
}

int ReleaseInod(UINT16 BlockNum) {
    if(BlockNum<24 && BlockNum>3){
        char BlockFreeBitmap[BLOCK_SIZE];
        ReadBlock(FREE_INODE_BITMAP, BlockFreeBitmap);
        BlockFreeBitmap[BlockNum-4] = 1;
        WriteBlock(FREE_INODE_BITMAP, BlockFreeBitmap);
        return 1;
    }else{return 0;}
}

int HoldInod(UINT16 BlockNum) {
    if(BlockNum<24 && BlockNum>3){
        char BlockHoldBitmap[BLOCK_SIZE];
        ReadBlock(FREE_INODE_BITMAP, BlockHoldBitmap);
        BlockHoldBitmap[BlockNum-4] = 0;
        WriteBlock(FREE_INODE_BITMAP, BlockHoldBitmap);
        return 1;
    }else{return 0;}
}

//////////////////////////////////////////////////////////////////////////
iNodeEntry* returniNodeEntryByNum(UINT16 BlockInodNum){
    iNodeEntry *pinodEntry = NULL;
    if(BlockInodNum > 3 && BlockInodNum < 24){
        char blockInod[BLOCK_SIZE];
        ReadBlock(BlockInodNum, blockInod);
        pinodEntry = (iNodeEntry *)blockInod;
    }
    return pinodEntry;
}

DirEntry* returnDirEntryByNum(UINT16 BlockDataNum){
    char blockData[BLOCK_SIZE];
    DirEntry *pDirEntry = NULL;
    if(BlockDataNum > 23 && BlockDataNum < 44){
        ReadBlock(BlockDataNum, blockData);
        pDirEntry = (DirEntry *)blockData;
    }
    return pDirEntry;
}

iNodeEntry* returniNodeDirEntryByChemin(const char *pDirName){
    if(pDirName[0] != '/' || strcmp(pDirName,"/")==0){
        return NULL;
    }
    int len = strlen(pDirName);
	char line[len];
	strcpy(line, pDirName);
	const char s[2] = "/";
	char *token;
	token = strtok(line, s);


    char blockInod[BLOCK_SIZE];
    ReadBlock(4, blockInod);
    iNodeEntry* inodeStruct = (iNodeEntry *)blockInod;


	char blockData[BLOCK_SIZE];
    ReadBlock(inodeStruct->Block[0], blockData);
    DirEntry *pDirEntry = (DirEntry *)blockData;

	int exist = 0;
	while( token != NULL ){
        int k = 0;
        int nbFiles = inodeStruct->iNodeStat.st_size / 16;
        for (k; k<nbFiles; k++){
            ReadBlock(pDirEntry[k+2].iNode + 4, blockInod);
            inodeStruct = (iNodeEntry *)blockInod;
            if(strcmp(pDirEntry[k+2].Filename, token)==0){
                ReadBlock(inodeStruct->Block[0], blockData);
                pDirEntry = (DirEntry *)blockData;
                exist = 1;
                break;
            }else{
                exist = 0;
            }
        }
        token = strtok(NULL, s);
	}
	if (exist == 1){
        return inodeStruct;
	}else{
        return NULL;
	}
}

iNodeEntry* returniNodeDirParentEntryByChemin(const char *pDirName){
    if(pDirName[0] != '/' || strcmp(pDirName,"/")==0){
        return NULL;
    }
    int len = strlen(pDirName);
	char line[len];
	strcpy(line, pDirName);
	const char s[2] = "/";
	char *token;
	token = strtok(line, s);

	int j = 0;
    for (j; pDirName[j]; pDirName[j]=='/' ? j++ : *pDirName++);
    int nbDir = j-1; //nombre de / sauf le premier

	char blockInod[BLOCK_SIZE];
    ReadBlock(4, blockInod);
    iNodeEntry* inodeStruct = (iNodeEntry *)blockInod;
    iNodeEntry* inodeStructParent = inodeStruct;


	char blockData[BLOCK_SIZE];
    ReadBlock(inodeStruct->Block[0], blockData);
    DirEntry *pDirEntry = (DirEntry *)blockData;

	int exist = 0;
	while( token != NULL ){
        int k = 0;
        int nbFiles = inodeStruct->iNodeStat.st_size / 16;
        for (k; k<nbFiles; k++){
            inodeStructParent = inodeStruct;
            ReadBlock(pDirEntry[k+2].iNode + 4, blockInod);
            inodeStruct = (iNodeEntry *)blockInod;
            if(strcmp(pDirEntry[k+2].Filename, token)==0){
                ReadBlock(inodeStruct->Block[0], blockData);
                pDirEntry = (DirEntry *)blockData;
                exist = 1;
                break;
            }else{
                exist = 0;
            }
        }
        if (exist){
            j++;
            token = strtok(NULL, s);
        }else{
            break;
        }

	}
	if (exist == 1){
        return inodeStructParent;
	}else if (exist == 0 && j == nbDir+1){
        return inodeStructParent;
	}else{
        return NULL;
	}
}

bool verifieExistDir(const char *pDirName){
	iNodeEntry* iNodeEntry = returniNodeDirEntryByChemin(pDirName);
	if(iNodeEntry != NULL){
        return true;
	}
	return false;
}

void creerDirDefault(const char *pDirName){
    char blockData[BLOCK_SIZE];
    char blockInode[BLOCK_SIZE];
    char blockInodeParent[BLOCK_SIZE];

    DirEntry *pDirEntry = (DirEntry *)blockData;
    iNodeEntry* inodeStruct = (iNodeEntry *)blockInode;
    iNodeEntry* inodeStructParent = (iNodeEntry *)blockInodeParent;

    inodeStruct = returniNodeDirEntryByChemin(pDirName);
    inodeStructParent = returniNodeDirParentEntryByChemin(pDirName);
    //DirEntry* pDirEntry = returnDirEntryByNum(inodeStruct->Block[0]);
    if(inodeStruct != NULL){
        strcpy(pDirEntry[1].Filename,"..");
        strcpy(pDirEntry[0].Filename,".");

        pDirEntry[1].iNode = inodeStructParent->iNodeStat.st_ino;
        pDirEntry[0].iNode = inodeStruct->iNodeStat.st_ino;

        ++(inodeStructParent->iNodeStat.st_nlink);
        ++(inodeStruct->iNodeStat.st_nlink);
        inodeStruct->iNodeStat.st_size = 0;

        WriteBlock(inodeStruct->Block[0], blockData);
        WriteBlock(inodeStruct->iNodeStat.st_ino+4, blockInode);
        WriteBlock(inodeStructParent->iNodeStat.st_ino+4, blockInodeParent);
    }
}

int bd_FormatDisk(){
    TRY
    {
        //hold tous les block de 0 a 24 dans le bitmap des blocks
        int i = 0;
        for(i = 0; i < 24; i++){
            HoldBlock(i);
        }

        for(i = 0; i < 20; i++){
            char INodeBlock[BLOCK_SIZE];
            iNodeEntry *pIE = (iNodeEntry *)INodeBlock;
            pIE->iNodeStat.st_ino = i;//numero inode
            pIE->iNodeStat.st_nlink = 0;
            pIE->iNodeStat.st_size = 0;
            pIE->iNodeStat.st_blocks = 1;
            WriteBlock(i+4, INodeBlock);
        }

        //hold le inode 0 dans le bitmap des inodes
        HoldInod(4);
        printf("UFS: Saisie i-node %d \n", 1);

        char INodeBlock[BLOCK_SIZE];
        ReadBlock(4, INodeBlock);
        iNodeEntry *pIE = (iNodeEntry *)INodeBlock;

        pIE->iNodeStat.st_mode = S_IFDIR;
        ++(pIE->iNodeStat.st_nlink);
        pIE->iNodeStat.st_size = 0;

        pIE->Block[0] = 24;

        HoldBlock(24);
        printf("UFS: Saisie block %d \n", 24);
        char BlockData[BLOCK_SIZE];
        DirEntry *pDirEntry = (DirEntry *)BlockData;
        strcpy(pDirEntry[0].Filename,".");
        strcpy(pDirEntry[1].Filename,"..");
        pDirEntry[0].iNode = 0;

        WriteBlock(24, BlockData);
        WriteBlock(4, INodeBlock);

        //release les inodes 1 a 19 dans le bitmap des inodes
        for(i = 1; i < 20; i++){
            ReleaseInod(i+4);
        }
        for(i = 1; i < 20; i++){
            ReleaseBlock(i+24);
        }
    }
    CATCH
    {
        return 0;
    }
    ETRY;
    return 1;
}

int bd_create(const char *pDirName){


    int len = strlen(pDirName);
	char line[len];
	char* nameDir;
	strcpy(line, pDirName);
	const char s[2] = "/";
	char *token;
	token = strtok(line, s);

	while( token != NULL ){
	    strcpy(nameDir, token);
        token = strtok(NULL, s);
	}


    iNodeEntry* inodeStruct = returniNodeDirEntryByChemin(pDirName);

    iNodeEntry* inodeStructParent = returniNodeDirParentEntryByChemin(pDirName);

    DirEntry* pDirEntry = returnDirEntryByNum(inodeStructParent->Block[0]);

    if(inodeStruct == NULL && inodeStructParent != NULL){
        int inode = findFirstFreeInod();
        int intBlockData = findFirstFreeDataBlock();
        HoldInod(inode);
        printf("UFS: Saisie i-node %d \n", inode-4+1);
        HoldBlock(intBlockData);
        printf("UFS: Saisie block %d \n", intBlockData);

        char INodeBlock[BLOCK_SIZE];
        ReadBlock(inode, INodeBlock);
        iNodeEntry *pIE = (iNodeEntry *)INodeBlock;

        pIE->Block[0] = intBlockData;
        pIE->iNodeStat.st_ino = inode-4;
        pIE->iNodeStat.st_mode = S_IFREG;
        pIE->iNodeStat.st_nlink = 0;
        creerDirDefault(pDirName);

        WriteBlock(inode, INodeBlock);

        char iNodeBlock[BLOCK_SIZE];
        inodeStructParent = returniNodeDirParentEntryByChemin(pDirName);
        UINT16 i = inodeStructParent->iNodeStat.st_ino+4;
        ReadBlock(inodeStructParent->iNodeStat.st_ino+4, iNodeBlock);
        iNodeEntry* inodeStructParent = (iNodeEntry *)iNodeBlock;
        inodeStructParent->iNodeStat.st_size += 16;
        WriteBlock(i, iNodeBlock);

        int nbFiles = inodeStructParent->iNodeStat.st_size / 16;
        char blockData[BLOCK_SIZE];
        ReadBlock(inodeStructParent->Block[0], blockData);

        DirEntry* ppDirEntry = (DirEntry *)blockData;
        strcpy(ppDirEntry[nbFiles+2].Filename, nameDir);
        ppDirEntry[nbFiles+2].iNode = inode-4;

        WriteBlock(inodeStructParent->Block[0], blockData);

    }else{
        printf("Dossier existe déjà");
    }
    return 1;
}

int bd_mkdir(const char *pDirName){

    int len = strlen(pDirName);
	char line[len];
	char* nameDir;
	strcpy(line, pDirName);
	const char s[2] = "/";
	char *token;
	token = strtok(line, s);

	while( token != NULL ){
	    strcpy(nameDir, token);
        token = strtok(NULL, s);
	}

    iNodeEntry* inodeStruct = returniNodeDirEntryByChemin(pDirName);

    iNodeEntry* inodeStructParent = returniNodeDirParentEntryByChemin(pDirName);

    DirEntry* pDirEntry = returnDirEntryByNum(inodeStructParent->Block[0]);

    if(inodeStruct == NULL && inodeStructParent != NULL){
        int inode = findFirstFreeInod();
        int intBlockData = findFirstFreeDataBlock();
        HoldInod(inode);
        printf("UFS: Saisie i-node %d \n", inode-4+1);
        HoldBlock(intBlockData);
        printf("UFS: Saisie block %d \n", intBlockData);

        char INodeBlock[BLOCK_SIZE];
        ReadBlock(inode, INodeBlock);
        iNodeEntry *pIE = (iNodeEntry *)INodeBlock;

        pIE->Block[0] = intBlockData;
        pIE->iNodeStat.st_ino = inode-4;
        pIE->iNodeStat.st_mode = S_IFDIR;
        pIE->iNodeStat.st_nlink = 0;
        creerDirDefault(pDirName);

        WriteBlock(inode, INodeBlock);

        char iNodeBlock[BLOCK_SIZE];
        inodeStructParent = returniNodeDirParentEntryByChemin(pDirName);
        UINT16 i = inodeStructParent->iNodeStat.st_ino+4;
        ReadBlock(inodeStructParent->iNodeStat.st_ino+4, iNodeBlock);
        iNodeEntry* inodeStructParent = (iNodeEntry *)iNodeBlock;
        inodeStructParent->iNodeStat.st_size += 16;
        WriteBlock(i, iNodeBlock);

        int nbFiles = inodeStructParent->iNodeStat.st_size / 16;
        char blockData[BLOCK_SIZE];
        ReadBlock(inodeStructParent->Block[0], blockData);

        DirEntry* ppDirEntry = (DirEntry *)blockData;
        strcpy(ppDirEntry[nbFiles+2].Filename, nameDir);
        ppDirEntry[nbFiles+2].iNode = inode-4;

        WriteBlock(inodeStructParent->Block[0], blockData);

    }else{
        printf("Dossier existe déjà");
    }
    return 1;
}

int bd_ls(const char *pDirLocation){
    iNodeEntry* inodeStruct = returniNodeDirEntryByChemin(pDirLocation);
    DirEntry* pDirEntry = returnDirEntryByNum(inodeStruct->Block[0]);
    char type;
    if(inodeStruct != NULL){
        int i = 0;
        printf("ls /");
        int k = 0;
        int nbFiles = inodeStruct->iNodeStat.st_size / 16;
        for (k; k<nbFiles; k++){
            if(inodeStruct->iNodeStat.st_mode == S_IFREG){
                type = 'd';
            }else{type = '-';}
            printf("%-10c %-10s size: %-10d inode: %-5d nlink: %-5d \n", type, pDirEntry[i].Filename, inodeStruct->iNodeStat.st_size, inodeStruct->iNodeStat.st_ino, inodeStruct->iNodeStat.st_nlink);

        }
    }

}

int bd_rm(const char *pFilename){
}





