#include <stdio.h>
//#include "mmath.h"
 #include "driver/uart.h"
#include "dgus_lcd.h"
#define BUF_SIZE (1024)

//mode��82 �� 83
//recLen��ʾ���ճ���



typedef  unsigned char u8;
typedef  char s8;
typedef  unsigned short int u16;
typedef  short int s16;
typedef  unsigned int u32;
typedef  int s32;

uint8_t rbuf[16 * 1024] = {0};
int rptrHead = 0, rptrTail = 0;



void recFromCom()
{
    uint8_t rxbuftmp[2048];
    int len = uart_read_bytes(UART_NUM_2, rxbuftmp, sizeof(rxbuftmp), pdMS_TO_TICKS(100));

    if (len > 0) {

        for (int i = 0; i < len; i++) {
            rbuf[rptrHead] = rxbuftmp[i];
            rptrHead++;
            rptrHead &= 0x00003FFF;
        }
    }
}
int pro8283Analyse(u16 address,u8 mode, u8 recLen, u8 *buf)
{
    int lenTmp;
    u8 tmpData[512];
    u16 addresstmp;

    if(rptrHead < rptrTail)
        lenTmp = rptrHead + 0x00004000 - rptrTail;
    else
        lenTmp = rptrHead - rptrTail;

    if(lenTmp >= recLen)
    {
        if(rbuf[rptrTail]==0x5a && rbuf[(rptrTail+1)&0x00003fff]==0xa5 \
        && rbuf[(rptrTail+2)&0x00003fff]==(recLen-3) && rbuf[(rptrTail+3)&0x00003fff]==mode )
        {
            rptrTail += 3;
            rptrTail &= 0x00003ffff;
            for(lenTmp=0;lenTmp<(recLen-3);lenTmp++)
            {
                tmpData[lenTmp] = rbuf[rptrTail];
                rptrTail++;
                rptrTail &= 0x00003ffff;
            }
            if(0==Calculate_CRC16(tmpData,recLen-3,0))
            {
                addresstmp = tmpData[1];
                addresstmp <<= 8;
                addresstmp |= tmpData[2];
                if(addresstmp != address)
                    return -1;
                if(mode == 0x82)
                {
                    for(lenTmp=0;lenTmp<(recLen-6);lenTmp++)
                    {
                        buf[lenTmp] = tmpData[lenTmp+1];
                    }
                }
                else
                {
                    for(lenTmp=0;lenTmp<(recLen-9);lenTmp++)
                    {
                        buf[lenTmp] = tmpData[lenTmp+4];
                    }
                }
                return 0;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            rptrTail++;
            rptrTail &= 0x00003ffff;
            return -1;
        }
    }
    return -1;
}

//address ��ʼVP
//buf�Ƕ�д��ŵ�����
//len�Ƕ�дVP�ĳ���
//mode��82 �� 83
#define waitTime 100
int sendToT5L(u8 *buf, u16 address, u8 len, u8 mode)
{
    u8 sendTmp[512];
    int tmp;
    u32 i;

    if(len > 120)
        len = 120;
    sendTmp[0] = 0x5a;
    sendTmp[1] = 0xa5;
    rptrHead = rptrTail = 0;//��ʼ������
   switch (mode)
    {
    case 0x82:
        sendTmp[2] = len << 1;
        sendTmp[2] += 5;
        sendTmp[3] = 0x82;
        sendTmp[4] = address>>8;
        sendTmp[5] = address;
        memcpy(&sendTmp[6], buf, len << 1);

        Calculate_CRC16(&sendTmp[3],sendTmp[2]-2,1);
        break;
    case 0x83:
        sendTmp[2] = 0x06;
        sendTmp[3] = 0x83;
        sendTmp[4] = address>>8;
        sendTmp[5] = address;
        sendTmp[6] = len;
        Calculate_CRC16(&sendTmp[3],sendTmp[2]-2,1);
        break;
    default:
        printf("Invalid instruction\n");
        break;
    }

    for (i = 0; i < 3; i++) {
    tmp = serial_send_data(sendTmp, sendTmp[2] + 3);
    if (tmp == -1) {
        printf("Queue is full\n");
        return -1;
    }

    tmp = 0;
    while (tmp < waitTime) {
        recFromCom();
        if (mode == 0x82 && pro8283Analyse(0x4F4B, 0x82, 8, buf) == 0)
            return 0;
        else if (mode == 0x83 && pro8283Analyse(address, 0x83, len * 2 + 1 + 2 + 1 + 5, buf) == 0)
            return 0;
        tmp++;
    }

    }
    return -1;
}
/*int sendToT5L(u8 *buf, u16 address, u8 len, u8 mode)
{
    u8 sendTmp[512];
    int tmp;
    u32 i;

    if(len > 120)
        len = 120;
    sendTmp[0] = 0x5a;
    sendTmp[1] = 0xa5;
    rptrHead = rptrTail = 0;//��ʼ������
    switch (mode)
    {
    case 0x82:
        sendTmp[2] = len << 1;
        sendTmp[2] += 5;
        sendTmp[3] = 0x82;
        sendTmp[4] = address>>8;
        sendTmp[5] = address;
        for(tmp=0;tmp<(len<<1);tmp++)
            sendTmp[6+tmp] = buf[tmp];
        Calculate_CRC16(&sendTmp[3],sendTmp[2]-2,1);
        break;
    case 0x83:
        sendTmp[2] = 0x06;
        sendTmp[3] = 0x83;
        sendTmp[4] = address>>8;
        sendTmp[5] = address;
        sendTmp[6] = len;
        Calculate_CRC16(&sendTmp[3],sendTmp[2]-2,1);
        break;
    default:
        //printf("Υ��ָ���ʽ\n");
        printf("Invalid instruction\n");
        break;
    }
    if(mode==0x82)
    {
        for(i=0;i<3;i++)
        {
            tmp = sendToCom(sendTmp, sendTmp[2]+3);
            if(tmp == -1)
            {
                printf("Queue is full\n");
                return -1;
            }
            tmp = 0;
            while(1)
            {
               // Sleep(2);
                recFromCom();
                if(0==pro8283Analyse(0x4F4B,0x82,8,buf))
                    return 0;
                tmp++;
                if(tmp>=waitTime)
                {
                    printf("receive timeout\n");
                    break;
                }
            }
        }
        return -1;
    }
    else if(mode==0x83)
    {
        for(i=0;i<3;i++)
        {
            tmp = sendToCom(sendTmp, sendTmp[2]+3);
            if(tmp == -1)
            {
                printf("Queue is full\n");
                return -1;
            }
            tmp = 0;
            while(1)
            {
             //   Sleep(2);
                recFromCom();
                if(0==pro8283Analyse(address,0x83,len*2+1+2+1+5,buf))
                    return 0;
                tmp++;
                if(tmp>=waitTime)
                {
                    printf("receive timeout\n");
                    break;
                }
            }
        }
        return -1;
    }
    else
    {
        return -1;
    }
    return -1;
}

*/

