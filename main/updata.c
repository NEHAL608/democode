#include "dgus_lcd.h"

#include "update.h"
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"




#define  MAX_FILE_NUM 128
#define  MAX_PATH 128

typedef struct {
    int fileID[MAX_FILE_NUM];
    char fileName[MAX_FILE_NUM][MAX_PATH];
    int fileNumbers;
} FileStruct;

__MFILESTRUCT fileTmp;//�����������������ļ�


/*esp_err_t getFileSize(const char* path, size_t* fileSize) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fclose(file);
    return ESP_OK;
}*/

unsigned int getFileSize(FILE *file)
{
    unsigned int fileSize;

    fseek(file,0,SEEK_END);
    fileSize = ftell(file);//�ж��ļ�����
    fseek(file,0,SEEK_SET);//�ж��곤�Ⱥ󣬽��ļ�ָ��ָ��ͷ��
    return fileSize;
}
int getFileID(char *fileName)
{
    int id,i;

    id = -1;
    for(i=0;i<MAX_PATH;i++)
    {
        if(fileName[i]>=0x30 && fileName[i]<=0x39)
        {
            if(0==i)
                id = 0;
            id *= 10;
            id += (fileName[i]-0x30);
        }
        else
        {
            break;
        }
    }
    return id;
}



int listDirectory(const char* path, int recursive, FileStruct* pFile) {
    char fileName[MAX_PATH];
    int ret = -1;
    int j = 0;

    // Open directory
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("Invalid path: %s\n", path);
        return ret;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
     //  snprintf(fileName, sizeof(fileName), "%s/%s", path, entry->d_name);
   if (strlen(path) + 1 + strlen(entry->d_name) >= MAX_PATH) {
            printf("Path too long\n");
            break;
        }

        strncpy(fileName, path, sizeof(fileName));
        strncat(fileName, "/", sizeof(fileName) - strlen(fileName) - 1);
        strncat(fileName, entry->d_name, sizeof(fileName) - strlen(fileName) - 1);

        if (entry->d_type == DT_DIR) {
            printf("%s is a folder\n", fileName);
        } else if (entry->d_type == DT_REG) {
            int idnumbers = getFileID(entry->d_name);
            if (idnumbers != -1) {
                pFile->fileID[j] = idnumbers;
                strcpy(pFile->fileName[j], fileName);
                j++;
                printf("Found file %d: %s\n", j, fileName);
            } else {
                printf("%s is not named after a number\n", fileName);
            }
        }

        if (recursive && entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char subdir[MAX_PATH];
                strncpy(subdir, path, sizeof(subdir));
                strncat(subdir, "/", sizeof(subdir) - strlen(subdir) - 1);
                strncat(subdir, entry->d_name, sizeof(subdir) - strlen(subdir) - 1);
        
                listDirectory(subdir, recursive, pFile);
            }
        }
    }

    closedir(dir);
    pFile->fileNumbers = j;
    printf("Found file number is %d\n", j);
    printf("\n");
    ret = 0;
    return ret;
}

/*


int listDirectory(char* Path, int Recursive, __MFILESTRUCT *pFile)
{
    HANDLE hFind;
    WIN32_FIND_DATA FindFileData;
    char FileName[MAX_PATH] = { 0 };
    int Ret = -1;
    int i,j;
    int idnumbers;

    strcpy(FileName, Path);
    strcat(FileName, "\\");
    strcat(FileName, "*.*");

    // ���ҵ�һ���ļ�
    hFind = FindFirstFile(FileName, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("Invalid path:%s\n", Path);
        return Ret;
    }

    j = 0;
    for(i=0;i<MAX_FILE_NUM;i++)
    {
        strcpy(FileName, Path);
        strcat(FileName, "\\");
        strcat(FileName, FindFileData.cFileName);

        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            printf("%s is a folder\n", FileName);
        }
        else
        {
            idnumbers = getFileID(FindFileData.cFileName);
            if(-1 != idnumbers)
            {
                pFile->fileID[j] = idnumbers;
                strcpy(pFile->fileName[j],FileName);
                j++;
                //printf("��������%d���ļ�%s\n",j,FileName);
            }
            else
            {
                printf("%s is not named after a number\n", FileName);
            }
        }
        if (FindNextFile(hFind, &FindFileData) == FALSE)
        {
            // ERROR_NO_MORE_FILES ��ʾ�Ѿ�ȫ���������
            if (GetLastError() != ERROR_NO_MORE_FILES)
            {
                //printf("������һ���ļ�����\n");
                printf("Search error!\n");
            }
            else
            {
                Ret = 0;
            }
            break;
        }
    }
    pFile->fileNumbers = j;
    printf("find file number is %d\n",j);
    printf("\n");
    // �رվ��
    FindClose(hFind);
    return Ret;
}
*/
u16 getT5Lcrc(u32 block, u32 len)
{
    int tmp;
    u8  startSave[12] = {0};
    u16 crcValue,i;

    startSave[0] = 0x5a;
    startSave[1] = 0x00;
    startSave[2] = block;
    startSave[3] = len / 4096;

    tmp = sendToT5L(startSave,0xe0,2,0x82);
    if(-1 == tmp)
    {
        return -1;
    }
    i = 0;
    while(1)
    {
                   //vTaskDelay(2000 / portTICK_PERIOD_MS);

        tmp = sendToT5L(startSave,0xe0,2,0x83);
        if(-1 != tmp)
        {
            if(0 == startSave[0])
            {
                crcValue = startSave[3];
                crcValue <<= 8;
                crcValue |= startSave[2];
                return crcValue;
            }
        }
        i++;
        if(i>= 500)
            return -1;
    }
}


int upDataBlock(u8* buf, u32 blockID)
{
    u32 cycNmu = 32 * 1024 / 128; // Calculate cycNmu inline
    u32 VPaddress = 0xc000;
  
   u8* linTmp = malloc(32 * 1024);
   memcpy(linTmp, buf, 32 * 1024); // Use memcpy for buffer copying

  /* for(i=0;i<32*1024;i++)
    {
        linTmp[i] = buf[i];//�����ļ����������ᱻ�޸�
    }*/
    
    for(u32 i=0;i<cycNmu;i++)
    {

     int tmp=sendToT5L(&linTmp[i*128],VPaddress,64,0x82);
     if(-1 != tmp)
        {
            VPaddress += (128>>1);
        }
        else
            return -1;
    }
            free(linTmp);  // Release the allocated memory

    u8 startSave[6] = { 0x5a, 0x02, blockID >> 8, blockID, 0xc0, 0x00 };

    int tmp = sendToT5L(startSave,0xaa,6,0x82);
    if(-1 == tmp)
    {
        return -1;
    }
  u32 i = 0;
   while(1)
    {
        tmp = sendToT5L(startSave,0xaa,2,0x83);
        if(-1 != tmp)
        {
            if(0 == startSave[0])
                return 0 ;
        }
        i++;
        if(i>= 500)
            return -1;
    }
}

int copyFile(u8 targetID, u32 len)
{
    int tmp;
    u8  startSave[12] = {0};
    u32 blockNum,i;

    blockNum = len / (256*1024);
    if(len % (256*1024))
        blockNum++;
    startSave[0] = 0x5a;
    startSave[1] = 0x03;
    startSave[2] = 0;
    startSave[3] = 17;
    startSave[4] = 0;
    startSave[5] = targetID;
    startSave[6] = 0;
    startSave[7] = blockNum;

    tmp = sendToT5L(startSave,0xaa,6,0x82);
    if(-1 == tmp)
    {
        return -1;
    }
    i = 0;
    while(1)
    {
            //vTaskDelay(2000 / portTICK_PERIOD_MS);
        tmp = sendToT5L(startSave,0xaa,2,0x83);
        if(-1 != tmp)
        {
            if(0 == startSave[0])
            {
                return 0;
            }
        }
        i++;
        if(i>= 500000)
            return -1;
    }
}

#define BLOCKSIZE 0x8000
int writeFileToT5L(FILE *fileTmpdata, u32 fileNum)
{
    u32 fileSize,i,j,k;
    u8 *fileDataPtr;
    u32 block256NUM,blockID;
    u32 tmpcalc;

    fileSize = getFileSize(fileTmpdata);
    u32 fileSizeTmp = (fileSize + BLOCKSIZE - 1) / BLOCKSIZE; // Calculate fileSizeTmp inline

    fileDataPtr = (u8*)malloc(fileSizeTmp * BLOCKSIZE);
    memset(fileDataPtr,0,fileSizeTmp * BLOCKSIZE);
    fread(fileDataPtr,1,fileSize,fileTmpdata);

    block256NUM = fileSizeTmp / 8;//һ���ж��ٸ�256K�飬������Ȳ���
    blockID = 17;
    u8 percent[2];
   printf("File size: %d bytes\n", fileSize);
    for( i=0;i<block256NUM;i++)
    {
        u16 crcTmp = Calculate_CRC16(&fileDataPtr[i*256*1024],256*1024,0);
        for( k=0;k<3;k++)
         {                     
            for( j=0;j<8;j++)
            {
                if(-1 != upDataBlock(&fileDataPtr[i*256*1024+j*32*1024],blockID*8+j))
                {
                    tmpcalc = 8 * i + j;
                    tmpcalc *= 100;
                    tmpcalc /= fileSizeTmp;
                    percent[0] = tmpcalc >> 8;
                    percent[1] = tmpcalc;
                    sendToT5L(percent,0xBFFF,1,0x82);
                 }
                else
                {
                    break;//��0��ʼ��ͷ����
                }
            }
          /*  for (j = 0, k = 0; j < 8 && k < 3; j++) {
            if (-1 != upDataBlock(&fileDataPtr[i * 256 * 1024 + j * 32 * 1024], blockID * 8 + j)) {
                u32 tmpcalc = 8 * i + j;
                tmpcalc *= 100;
                tmpcalc /= fileSizeTmp;
                u8 percent[2];
                percent[0] = tmpcalc >> 8;
                percent[1] = tmpcalc;
                sendToT5L(percent, 0xBFFF, 1, 0x82);
                k++;
            } else {
                break;
            }*/
        
            if(8==j)
            {
              u16  crcCalc = getT5Lcrc(blockID,256*1024);
                if(crcTmp != crcCalc)
                {
                    printf("save error!\n");
                }
                else
                {
                    blockID++;
                    break;
                }
            }
         }
        if(k==3)
        {
            free(fileDataPtr);
            return -1;
        }
    }
   u32 reservedLen = fileSizeTmp % 8;//���в���8����32K��
    if(reservedLen)//�еĲ���256K���룬����Ҫ��������,���ǿ϶���32K����
    {
       u16 crcTmp = Calculate_CRC16(&fileDataPtr[i*256*1024],reservedLen*32*1024,0);
        /*for( k=0;k<3;k++)
        {                 

            for( j=0;j<reservedLen;j++)
            {
                if(-1 != upDataBlock(&fileDataPtr[i*256*1024+j*32*1024],blockID*8+j))
                {
                   // printf("%d 32KB write successfully\n",j);
                    tmpcalc = 8 * i + j;
                    tmpcalc *= 100;
                    tmpcalc /= fileSizeTmp;
                    percent[0] = tmpcalc >> 8;
                    percent[1] = tmpcalc;
                    sendToT5L(percent,0xBFFF,1,0x82);
                }
                else
                {
                    break;//��0��ʼ��ͷ����
                }
            }*/

        for (j = 0, k = 0; j < reservedLen && k < 3; j++) {
            if (-1 != upDataBlock(&fileDataPtr[block256NUM * 256 * 1024 + j * 32 * 1024], blockID * 8 + j)) {
                u32 tmpcalc = 8 * block256NUM + j;
                tmpcalc *= 100;
                tmpcalc /= fileSizeTmp;
                percent[0] = tmpcalc >> 8;
                percent[1] = tmpcalc;
                sendToT5L(percent, 0xBFFF, 1, 0x82);
                k++;
            } else {
                break;
            }
        
            if(reservedLen==j)
            {
              u16  crcCalc = getT5Lcrc(blockID,256*1024);
                crcCalc = getT5Lcrc(blockID,reservedLen*32*1024);
                if(crcTmp != crcCalc)
                {
                    printf("save error\n");
                }
                else
                {
                    percent[0] = 0;
                    percent[1] = 100;
                    sendToT5L(percent,0xBFFF,1,0x82);
                    blockID++;
                    break;
                }
             }
        }
        if(k==3)
        {
            free(fileDataPtr);
            return -1;
        }

    }
    if(-1 != copyFile(fileNum,fileSizeTmp*BLOCKSIZE))
    {
        free(fileDataPtr);
        return 0;
    }
    free(fileDataPtr);
    return -1;
}
void updateFile()
{
    int tmp;
  
    FILE *updateFileBuf;
      //  updateFileBuf = fopen("/sdcard/32.icl","rb");
        updateFileBuf = fopen("/sdcard/48.icl","rb");

   //  updateFileBuf = fopen("/data/58.icl","rb");
            //  updateFileBuf = fopen("/data/14ShowFile.bin","rb");
            // updateFileBuf = fopen("/data/13TouchFile.bin","rb");
            // updateFileBuf = fopen("/data/22_Config.bin","rb");
    uint64_t start_time = esp_timer_get_time();

    if (!updateFileBuf) {
        printf( "Failed to read existing file ");
    }
        if(updateFileBuf != NULL)
        {
            tmp = writeFileToT5L(updateFileBuf, 48);
            if(-1 == tmp)
            {
                printf("error!\n");
            }
            fclose(updateFileBuf);
        }
        else
        {
            printf("Fail to open file\n");
        }
         printf("update sucess\n");
            uint64_t end_time = esp_timer_get_time();
            uint64_t elapsed_time = end_time - start_time;

    // Print the elapsed time in microseconds
        printf("Elapsed time: %lld us\n", elapsed_time);
 //   }
}
