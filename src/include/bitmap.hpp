#pragma once
#include <fstream>
#include <string>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#pragma pack(1)
typedef struct {
    WORD Type;
    DWORD Size;
    WORD Reserved1;
    WORD Reserved2;
    DWORD OffBits;
} BitMapFileHeader_t;

typedef struct {
    DWORD Size;
    DWORD Width;
    DWORD Height;
    WORD Planes;
    WORD BitCount;
    DWORD Compression;
    DWORD SizeImage;
    DWORD XPixPerMeter;
    DWORD YPixPerMeter;
    DWORD ClrUsed;
    DWORD ClrImportant;
} BitMapInfoHeader_t;

typedef struct {
    BitMapFileHeader_t File;
    BitMapInfoHeader_t Info;
} BitMap_t;
#pragma pack()

void save_bitmap(unsigned char *img, std::string filename, int width, int height);
