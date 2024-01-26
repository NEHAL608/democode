#ifndef UPDATE_H_INCLUDED
#define UPDATE_H_INCLUDED

typedef  unsigned char u8;
typedef  char s8;
typedef  unsigned short int u16;
typedef  short int s16;
typedef  unsigned int u32;
typedef  int s32;

#define  MAX_FILE_NUM 128
#define  MAX_PATH 128

typedef struct __mfilestruct
{
    u32 fileNumbers;
    u32 fileID[MAX_FILE_NUM];
    char fileName[MAX_FILE_NUM][MAX_PATH];
}__MFILESTRUCT;

u32 getFileSize(FILE *file);
int getFileID(char *fileName);
//int listDirectory(char* Path, int Recursive, __MFILESTRUCT *pFile);


void updateFile();
#endif // UPDATE_H_INCLUDED
