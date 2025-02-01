/* 

This C program is used to modulate APRS audio.
It will be used for amateur radio activities with the XuanJing series satellites.

Version: 1.0   Date: February 2, 2025

Developer & Acknowledgments:
    BG7ZDQ - Initial program
    BH1PHL - Provided A reference program.

License: GNU General Public License v3.0

*/

#define SAMPLE_RATE 6000                  // 采样率
#define PI 3.14159265358979323846         // 圆周率
#define MARK_TONE 1200                    // 传号 (1)
#define SPACE_TONE 2200                   // 空号 (0)
#define BAUD_RATE 1200                    // 比特率

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bitmanip.h"

// 声明全局变量
FILE *file;                   // 用于存储音频数据的文件指针
uint32_t total_samples;       // 总采样数
double olderdata;             // 前一个幅度，用于连续相位
double oldercos;              // 前一个COS，用于连续相位
double delta_lenth = 0;       // 采样率精度补偿
uint16_t Crc;                 // CRC校验值
uint8_t Stuff_currbyte;       // 位填充状态跟踪

// WAV 文件头结构体，用于存储 WAV 文件格式的头部信息
typedef struct {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t subchunk2_size;
} WAVHeader;

// 声明函数
void transmit_APRS_frame(const char *ToCall, const char *ToSSID, const char *FmCall, const char *FmSSID, const char *Path, const char *PathSSID, const char *INFO);   // 发送APRS协议帧
void transmit_sync(void);                                // 发送同步标志
void dataframe_start(void);                              // 数据帧初始化
void transmit_byte(uint8_t byte);                        // 发送单字节（含位填充）
void crc_update_byte(uint8_t byte);                      // 按字节更新CRC
void crc_update_bit(uint8_t crc_bit);                    // 按位更新CRC
void stuff_transmit(uint8_t bit);                        // 位填充处理
void nrzi_modulate(uint8_t bit);                         // NRZI编码调制
void modulate(uint8_t bit);                              // FSK调制输出
void write_wav_header(uint32_t data_size);               // 写入WAV文件头
int sign(double num);                                    // 符号函数
void write_tone(double frequency, double duration_ms);   // 写入音频波形
int generate(const char *ToCall, const char *FmCall, const char *Path, const char *INFO, const char *filename);  // 主调制生成函数



int main(int argc, char *argv[]) {

    // 检查参数
    if (argc != 6 || strcmp(argv[1], "--help") == 0) {
        printf("用法: ./aprs <'ToCall-SSID'> <'FmCall-SSID'> <'Path1-SSID、Path2-SSID'> <'INFO'> <'Output Filename'>\n", argv[0]);
        return 1;
    }

    // 获取程序开始时间
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // 调用主调制生成函数
    generate(argv[1], argv[2], argv[3], argv[4], argv[5]);

    // 计算并输出程序运行时间
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("程序运行时间: %lf 秒\n", elapsed_time);

    return 1;
}

// 主调制生成函数
int generate(const char *ToCallArg, const char *FmCallArg, const char *PathArg, const char *INFO, const char *filename) {
    
    // 文件写入初始化
    total_samples = 0;// 重置样本计数
    file = fopen(filename, "wb");
    if (!file) { perror("无法打开文件"); return 1; }
    write_wav_header(0);// 写入空的 WAV 头


    /* -----处理传入的参数----- */

    // 提前存储各类变量
    char toCall[8] = {0};
    char toSSID[4] = "0";
    char fmCall[8] = {0};
    char fmSSID[4] = "0";
    char callPart[8] = {0};
    char ssidPart[4] = "0";
    char pathCall[64] = {0};  
    char pathSSID[32] = {0};
    char paths_copy[128];
    
    // 处理目标呼号及SSID
    char *dash_ptr = strchr(ToCallArg, '-');
    if(dash_ptr) {
        size_t len = dash_ptr - ToCallArg;
        if (len > 6) { printf("ToCall 呼号过长！\n"); return 1;}
        strncpy(toCall, ToCallArg, len);
        toCall[len] = '\0';
        strcpy(toSSID, dash_ptr + 1);
        if (atoi(toSSID) > 15) { printf("ToCall SSID 不合法！\n"); return 1;}
    } else {
        strcpy(toCall, ToCallArg);
        if (strlen(ToCallArg) > 6) { printf("ToCall 呼号过长！\n"); return 1;}
    }

    // 处理源站呼号及SSID
    dash_ptr = strchr(FmCallArg, '-');
    if(dash_ptr) {
        size_t len = dash_ptr - FmCallArg;
        if (len > 6) { printf("FmCall 呼号过长！\n"); return 1;}
        strncpy(fmCall, FmCallArg, len);
        fmCall[len] = '\0';
        strcpy(fmSSID, dash_ptr + 1);
        if (atoi(fmSSID) > 15) { printf("FmCall SSID 不合法！\n"); return 1;}
    } else {
        strcpy(fmCall, FmCallArg);
        if (strlen(FmCallArg) > 6) { printf("ToCall 呼号过长！\n"); return 1;}
    }

    // 处理路径参数
    strncpy(paths_copy, PathArg, sizeof(paths_copy) - 1);
    paths_copy[sizeof(paths_copy) - 1] = '\0';
    int tokenCount = 0;
    int first = 1;
    char *token = strtok(paths_copy, ",");

    while (token != NULL && tokenCount < 8) {
        char *dash = strchr(token, '-');
        if (dash) {
            size_t len = dash - token;
            if (len > 6) { printf("路径呼号 '%.*s' 过长！\n", (int)len, token); return 1;}
            strncpy(callPart, token, len);
            callPart[len] = '\0';
            strcpy(ssidPart, dash + 1);
            if (atoi(ssidPart) > 15) { printf("路径 SSID '%s' 不合法！\n", ssidPart); return 1;}
        } else {
            if (strlen(token) > 6) { printf("路径呼号 '%s' 过长！\n", token); return 1;}
            strcpy(callPart, token);
        }
        // 如果不是第一个，就添加逗号分隔符
        if (!first) {
            strcat(pathCall, ",");
            strcat(pathSSID, ",");
        }
        strcat(pathCall, callPart);
        strcat(pathSSID, ssidPart);
        first = 0;
        tokenCount++;
        token = strtok(NULL, ",");
    }
    
    /* -----参数处理结束----- */

    // 调试信息
    printf("目标呼号: %s-%s\n", toCall, toSSID);
    printf("发送方呼号: %s-%s\n", fmCall, fmSSID);
    printf("路径呼号: %s-%s\n", pathCall, pathSSID);
    printf("INFO: %s\n\n", INFO);

    // 发送APRS协议帧
    write_tone(0, 150);
    transmit_APRS_frame(toCall, toSSID, fmCall, fmSSID, pathCall, pathSSID, INFO);
    write_tone(0, 150);

    // 计算数据大小并写入文件头
    uint32_t data_size = total_samples * sizeof(short);
    write_wav_header(data_size);
    fclose(file);
    printf("End.\n");
    return 0;
}

/*
每段源站、目标、中继地址各占有7个地址段，每个地址段长8bit。
其中第7地址段划为控制字段与SSID，结构如下：

位数 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
作用 | C | R | R |      SSID     | X |

其中C为控制字段，R为保留位（填充1即可），X为终止指示符。
针对源站与目标地址，地址所属的C为0时为源站地址，C为1时为目标地址。
针对中继地址，C为0时表示尚未在本中继局完成中继，C为1时表示已经完成了中继。
当X为1时，表示地址到此结束。
*/

// 函数APRS帧发送主函数
void transmit_APRS_frame(const char *ToCall, const char *ToSSID, const char *FmCall, const char *FmSSID, const char *Path, const char *PathSSID, const char *INFO){

    unsigned char ssid, binary_ssid, assembled_ssid;
    uint8_t i, crcl, crch;
    const char* pi;

    // 发送帧头，并初始化CRC和位填充状态
    transmit_sync();
    printf(": Frame start.\n\n");
    dataframe_start();


    // 发送目标地址和SSID(ToCall)
    printf("ToCall:\n");
    for (int i = 0; i < 6; i++) {
        // 如果字符串长度不足6位，补充空格（0x20）
        char c = (i < strlen(ToCall)) ? ToCall[i] : 0x20;
        transmit_byte(c << 1);
        printf(": %c\n", c);
    } 
    // 拼装并发送ToSSID
    ssid = atoi(ToSSID);   // 将ToSSID转换为0到15之间的整数值
    binary_ssid = (ssid << 1);
    // 该地址属于目标地址且路径尚未结束，故C=1、X=0，应拼接111????0
    assembled_ssid = 0b11100000 | binary_ssid;
    transmit_byte(assembled_ssid);
    printf(": ToSSID\n\n");


    // 发送源站地址和SSID(FmCall)
    printf("FmCall:\n");
    for (int i = 0; i < 6; i++) {
        char c = (i < strlen(FmCall)) ? FmCall[i] : 0x20;
        transmit_byte(c << 1);
        printf(": %c\n", c);
    } 
    // 拼装并发送FmSSID
    ssid = atoi(FmSSID);
    binary_ssid = (ssid << 1);
    // 该地址属于源站地址且路径尚未结束，故C=0、X=0，应拼接011????0
    assembled_ssid = 0b01100000 | binary_ssid;
    transmit_byte(assembled_ssid);
    printf(": FmSSID\n\n");

    // 发送路径信息
    printf("Path:\n");
    for (int i = 0; i < 6; i++) {
        char c = (i < strlen(Path)) ? Path[i] : 0x20;
        transmit_byte(c << 1);
        printf(": %c\n", c);
    } 
    // 拼装并发送PathSSID
    ssid = atoi(PathSSID);
    binary_ssid = (ssid << 1);
    // 该中继地址未命中，路径已经结束，故C=0、X=1，应拼接011????1
    assembled_ssid = 0b01100001 | binary_ssid;
    transmit_byte(assembled_ssid);
    printf(": PathSSID\n\n");

    // 发送帧信息及数据类型
    transmit_byte(0x03);   // 标记该帧为UI帧
    printf("\n");
    transmit_byte(0xf0);   // 数据类型
    printf("\n\n");

    // 发送信息
    for(pi=INFO; *pi!=0; pi++){
        transmit_byte(*pi);
        printf(":%c\n", *pi);
    }
    
    // 计算并发送CRC校验值（取反后拆分高低位）
    Crc^=0xffff;
    crcl=Crc&0xff;
    crch=(Crc>>8)&0xff;
    transmit_byte(crcl);
    printf(": CRC low\n");
    transmit_byte(crch);
    printf(": CRC high\n\n");

    // 帧尾
    transmit_sync();
    printf(": Frame end.\n");
    transmit_sync();
    printf(": Frame end.\n\n");
}

// 发送同步字0x7E（01111110 b）
void transmit_sync(void){
    uint8_t sync_word = 0x7E;
    for (int i = 7; i >= 0; i--) {
        printf("%d", (sync_word >> i) & 1);
        nrzi_modulate((sync_word >> i) & 1);
    }
}

// 初始化帧的CRC和位填充状态
void dataframe_start(void){
    Crc=0xffff;          // CRC初始值
    Stuff_currbyte=0;    // 位填充状态清零
}

// 发送一个字节（含位填充和CRC更新）
void transmit_byte(uint8_t byte){
    uint8_t i;
    for (int i = 7; i >= 0; i--) {
        // 使用位操作判断当前位
        printf("%d", (byte >> i) & 1);
    }
    for (i=0;i<8;i++){
        // 从低位到高位发送每一位
        stuff_transmit(CHB(byte,BIT(i))?1:0);
    }
    crc_update_byte(byte); // 更新CRC
}

// 按字节更新CRC
void crc_update_byte(uint8_t byte){
    uint8_t i;
    for(i=0;i<8;i++){
       // 逐位处理：检查每一位是否为1
       crc_update_bit(CHB(byte,BIT(i))?1:0);
    }
}

// 按位更新CRC（CRC-16-CCITT算法，多项式0x8408）
void crc_update_bit(uint8_t crc_bit){
    uint8_t shiftbit;
    shiftbit=0x0001&(uint8_t)Crc; // 获取当前最低位
    Crc>>=1;                      // 右移一位
    if(shiftbit != crc_bit){      // 如果输入位与最低位不同
        Crc ^= 0x8408;            // 异或多项式
    }
}

// 位填充：连续5个1后插入0
void stuff_transmit(uint8_t bit){
    Stuff_currbyte=(Stuff_currbyte<<1) | bit; // 记录当前位
    nrzi_modulate(bit);                       // 调制发送
    if ((Stuff_currbyte & 0x1f)==0x1f){       // 检查是否连续5个1
        Stuff_currbyte=(Stuff_currbyte<<1);   // 插入0
        nrzi_modulate(0);                     // 发送填充的0
    } 
}

// NRZI编码：0翻转信号，1保持信号
void nrzi_modulate(uint8_t bit){
    static uint8_t oldstat=0; // 静态变量记录当前电平状态
    if (bit==0){
        oldstat=oldstat?0:1;  // 输入0时翻转电平
    }
    modulate(oldstat);        // 根据电平状态调制频率
}

// 调制输出：1200Hz或2200Hz（每个符号输出8个采样点）
void modulate(uint8_t bit){
    uint8_t i; 
    if(bit==1){
        write_tone(MARK_TONE, 1000.0 / BAUD_RATE);
    }else{
        write_tone(SPACE_TONE, 1000.0 / BAUD_RATE);
    }
}

// 生成 WAV 文件头
void write_wav_header(uint32_t data_size) {
    WAVHeader header = {
        .riff = "RIFF",
        .chunk_size = 36 + data_size,
        .wave = "WAVE",
        .fmt = "fmt ",
        .subchunk1_size = 16,
        .audio_format = 1,
        .num_channels = 1,
        .sample_rate = SAMPLE_RATE,
        .byte_rate = SAMPLE_RATE * 1 * 16 / 8,
        .block_align = 1 * 16 / 8,
        .bits_per_sample = 16,
        .data = "data",
        .subchunk2_size = data_size
    };
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(WAVHeader), 1, file);
}

// 函数：判断正负号
int sign(double num) {
    if (num >= 0) {
        return 1;
    } else if (num < 0) {
        return -1;
    } else {
        return 0;
    }
}

// 函数：生成并写入指定频率和持续时间和初始相位的正弦波音频
void write_tone(double frequency, double duration_ms) {
    uint32_t num_samples = SAMPLE_RATE * duration_ms / 1000;
    delta_lenth += SAMPLE_RATE * duration_ms / 1000 - num_samples;
    if (delta_lenth >= 1) {
        num_samples += (int)delta_lenth;
        delta_lenth -= (int)delta_lenth;
    }
    double phi_samples = SAMPLE_RATE * (sign(oldercos) * asin(olderdata) + abs(sign(oldercos) - 1) / 2 * PI);
    short buffer[num_samples];
    for (uint32_t i = 0; i < num_samples; ++i) {
        buffer[i] = (short)(32767 * sin((2 * PI * frequency * i + phi_samples) / SAMPLE_RATE));
    }
    fwrite(buffer, sizeof(short), num_samples, file);
    total_samples += num_samples;
    olderdata = sin((2 * PI * frequency * num_samples + phi_samples) / SAMPLE_RATE);
    oldercos = cos((2 * PI * frequency * num_samples + phi_samples) / SAMPLE_RATE);
}