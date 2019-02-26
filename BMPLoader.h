#ifndef CVTOOLBOX_BMPLOADER_H
#define CVTOOLBOX_BMPLOADER_H

#include <stdint.h>

typedef struct tagBMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
}__attribute__((packed)) BMPFileHeader_t;

typedef struct tagBMPInfoHeader {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
}__attribute__((packed)) BMPInfoHeader_t;

typedef struct tagPixelBGRA {
    uint8_t peBlue;
    uint8_t peGreen;
    uint8_t peRed;
    uint8_t peAlpha;
} PixelBGRA_t;

typedef struct tagBMPFile {
    BMPFileHeader_t bmpHeader;
    BMPInfoHeader_t bmpInfoHeader;
    PixelBGRA_t *paletteEntry;
    uint8_t *buffer;
    uint8_t rowInvert;
    uint8_t colInvert;
} BMPFile_t;

typedef struct structImage {
    uint32_t rowSize;
    uint32_t colSize;
    uint32_t size;
    PixelBGRA_t *pixel;
} Image_t;

BMPFile_t* loadBMPFile(const char *fileName, BMPFile_t *pBMPFile);
void printBMPInfo(const BMPFile_t* pBMPFile);
void closeBMPFile(BMPFile_t *pBMPFile);
void unpackBMPToImage(const BMPFile_t *pBMPFile, Image_t *image);
void packImageTo24BitBMP(BMPFile_t *pBMPFile, const Image_t *image);
void writeBMPFile(BMPFile_t *pBMPFile, const char *fileName);

#endif //CVTOOLBOX_BMPLOADER_H
