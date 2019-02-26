#include "BMPLoader.h"
#include <stdio.h>
#include <stdlib.h>

BMPFile_t* loadBMPFile(const char *fileName, BMPFile_t *pBMPFile) {
    FILE* fstream;
    fstream = fopen(fileName, "rb");
    if(fstream == NULL) {
        printf("File \"%s\" open failed.\n", fileName);
        fclose(fstream);
        exit(-1);
    } else {
        printf("File %s open succeed.\n", fileName);
    }

    BMPFileHeader_t *pBmpFileHeader = &pBMPFile->bmpHeader;
    BMPInfoHeader_t *pBmpInfoHeader = &pBMPFile->bmpInfoHeader;
    fread(pBmpFileHeader, sizeof(BMPFileHeader_t), 1, fstream);
    fread(pBmpInfoHeader, sizeof(BMPInfoHeader_t), 1, fstream);
    if(pBmpInfoHeader->biClrUsed == 0) {
        pBmpInfoHeader->biClrUsed = 1U << pBmpInfoHeader->biBitCount;
    }

    if(pBmpInfoHeader->biWidth < 0) {
        pBMPFile->colInvert = 1;
        pBmpInfoHeader->biWidth = ~pBmpInfoHeader->biWidth + 1;
    } else {
        pBMPFile->colInvert = 0;
    }
    if(pBmpInfoHeader->biHeight < 0) {
        pBMPFile->rowInvert = 0;
        pBmpInfoHeader->biHeight = ~pBmpInfoHeader->biHeight + 1;
    } else {
        pBMPFile->rowInvert = 1;
    }

    if(pBmpInfoHeader->biBitCount <= 16) {
        pBMPFile->paletteEntry = (PixelBGRA_t *) malloc((pBmpInfoHeader->biClrUsed) * sizeof(PixelBGRA_t));
        fread((char *) pBMPFile->paletteEntry, sizeof(PixelBGRA_t), pBmpInfoHeader->biClrUsed, fstream);
    } else {
        pBMPFile->paletteEntry = NULL;
    }
    pBMPFile->buffer = (uint8_t *)malloc((pBmpInfoHeader->biSizeImage) * sizeof(uint8_t));
    fseek(fstream, pBmpFileHeader->bfOffBits, SEEK_SET);
    fread((char *)pBMPFile->buffer, sizeof(uint8_t), pBmpInfoHeader->biSizeImage, fstream);
    fclose(fstream);
    return pBMPFile;
}

void closeBMPFile(BMPFile_t *pBMPFile) {
    if(pBMPFile->paletteEntry != NULL)
        free(pBMPFile->paletteEntry);
    if(pBMPFile->buffer != NULL)
        free(pBMPFile->buffer);
}

void printBMPInfo(const BMPFile_t *pBMPFile) {
    const BMPFileHeader_t *pBmpFileHeader = &pBMPFile->bmpHeader;
    const BMPInfoHeader_t *pBmpInfoHeader = &pBMPFile->bmpInfoHeader;
    printf("Type: %c%c\nFile Size: %d Bytes\nStart Address: 0x%x\n",
           pBmpFileHeader->bfType & 0xFF, pBmpFileHeader->bfType >> 8,
           pBmpFileHeader->bfSize, pBmpFileHeader->bfOffBits
    );
    printf("Size: %d\nWidth: %d\nHeight: %d\nPlanes: %d\nBits Per Pixel: %d\n"
                   "Compression: %d\nImage Data Size: %d\nXPixelsPerMeter: %d\n"
                   "YPixelsPerMeter: %d\nPalette Entry Counts: %d\nPalette Important: %d\n",
           pBmpInfoHeader->biSize, pBmpInfoHeader->biWidth, pBmpInfoHeader->biHeight,
           pBmpInfoHeader->biPlanes, pBmpInfoHeader->biBitCount, pBmpInfoHeader->biCompression,
           pBmpInfoHeader->biSizeImage, pBmpInfoHeader->biXPelsPerMeter, pBmpInfoHeader->biYPelsPerMeter,
           pBmpInfoHeader->biClrUsed, pBmpInfoHeader->biClrImportant
    );
    if(pBMPFile->rowInvert) {
        printf("Row Inverted\n");
    }
    if(pBMPFile->colInvert) {
        printf("Column Inverted\n");
    }
}

void unpackBMPToImage(const BMPFile_t *pBMPFile, Image_t *image) {
    if(pBMPFile->buffer == NULL) {
        printf("Unpack Failed\nNo image data\n");
        return;
    }
    if(pBMPFile->bmpInfoHeader.biClrUsed && pBMPFile->paletteEntry == NULL) {
        printf("Unpack Falied\nNo palette data\n");
        return;
    }
    image->colSize = (uint32_t)abs(pBMPFile->bmpInfoHeader.biWidth);
    image->rowSize = (uint32_t)abs(pBMPFile->bmpInfoHeader.biHeight);
    image->size = image->colSize * image->rowSize;
    if(image->pixel != NULL) {
        free(image->pixel);
    }
    image->pixel = (PixelBGRA_t *)malloc(sizeof(PixelBGRA_t) * image->size);

    PixelBGRA_t *pPixel;
    int32_t rowMove;
    int8_t colMove;
    if(pBMPFile->rowInvert & pBMPFile->colInvert) {
        pPixel = image->pixel + image->size - 1;
        rowMove = 0;
        colMove = -1;
    } else if(pBMPFile->rowInvert) {
        pPixel = image->pixel + image->size - image->colSize;
        colMove = 1;
        rowMove = -2 * image->colSize;
    } else if(pBMPFile->colInvert) {
        pPixel = image->pixel + image->colSize - 1;
        colMove = -1;
        rowMove = 2 * image->colSize;
    } else {
        pPixel = image->pixel;
        colMove = 1;
        rowMove = 0;
    }
    uint64_t bufferOffset = 0;
    uint32_t bitMask;
    if(pBMPFile->bmpInfoHeader.biBitCount == 32) {
        bitMask = 0xFFFFFFFF;
    } else {
        bitMask = (1U << pBMPFile->bmpInfoHeader.biBitCount) - 1U;
    }
    for(uint32_t row = 0; row < image->rowSize; row++) {
        for(uint32_t col = 0; col < image->colSize; col++) {
            *((uint32_t *)pPixel) = ((*(pBMPFile->buffer + (bufferOffset >> 3))
                    >> (bufferOffset & 0x7 & ~(pBMPFile->bmpInfoHeader.biBitCount - 1))
            )) & bitMask;
            bufferOffset += pBMPFile->bmpInfoHeader.biBitCount;
            pPixel += colMove;
        }
        bufferOffset = (bufferOffset + 0x1F) & ~0x1FU;
        pPixel += rowMove;
    }
    if(pBMPFile->bmpInfoHeader.biBitCount <= 16) {
        pPixel = image->pixel;
        for(uint32_t i = 0; i < image->size; i++) {
            *pPixel = (*(pBMPFile->paletteEntry + (*(uint32_t *)pPixel)));
            pPixel++;
        }
    }
}

void packImageTo24BitBMP(BMPFile_t *pBMPFile, const Image_t *image) {
    if(image == NULL || image->pixel == NULL || pBMPFile == NULL) {
        return;
    }
    BMPFileHeader_t *pFileHeader = &pBMPFile->bmpHeader;
    BMPInfoHeader_t *pInfoHeader = &pBMPFile->bmpInfoHeader;
    if(pBMPFile->buffer != NULL) {
        free(pBMPFile->buffer);
    }
    pInfoHeader->biSize = 40;
    pInfoHeader->biWidth = image->colSize;
    pInfoHeader->biHeight = image->rowSize * -1;
    pInfoHeader->biBitCount = 24;
    pInfoHeader->biClrUsed = 0;
    pInfoHeader->biClrImportant = 0;
    pInfoHeader->biXPelsPerMeter = 0;
    pInfoHeader->biYPelsPerMeter = 0;
    pInfoHeader->biCompression = 0;
    uint32_t bufferRowLength = ((image->colSize * pInfoHeader->biBitCount + 31) >> 5) << 2;
    pInfoHeader->biSizeImage = bufferRowLength * image->rowSize;
    pFileHeader->bfType = (uint16_t)'B' + (((uint16_t)'M') << 8);
    pFileHeader->bfOffBits = sizeof(BMPFileHeader_t) + sizeof(BMPInfoHeader_t);
    pFileHeader->bfSize = pFileHeader->bfOffBits + pInfoHeader->biSizeImage;
    pFileHeader->bfReserved1 = pFileHeader->bfReserved2 = 0;
    pBMPFile->buffer = (uint8_t *)malloc(pInfoHeader->biSizeImage);
    pBMPFile->paletteEntry = NULL;

    PixelBGRA_t *pPixel = image->pixel;
    uint8_t *pBuffer = pBMPFile->buffer;
    for(uint32_t i = 0; i < image->size; i++) {
        *(uint32_t *)pBuffer = *(uint32_t *)pPixel;
        pBuffer += 3;
        pPixel++;
    }
}

void writeBMPFile(BMPFile_t *pBMPFile, const char *fileName) {
    if(pBMPFile == NULL || pBMPFile->buffer == NULL) {
        printf("Broken bmp structure\n");
        return;
    }
    FILE *fstream;
    fstream = fopen(fileName, "wb");
    if(fstream == NULL) {
        printf("File %s open failed.\n", fileName);
        fclose(fstream);
        return;
    }
    fwrite(&pBMPFile->bmpHeader, sizeof(BMPFileHeader_t), 1, fstream);
    fwrite(&pBMPFile->bmpInfoHeader, sizeof(BMPInfoHeader_t), 1, fstream);
    if(pBMPFile->paletteEntry != NULL) {
        fwrite(pBMPFile->paletteEntry, sizeof(PixelBGRA_t), pBMPFile->bmpInfoHeader.biClrUsed, fstream);
    }
    fwrite(pBMPFile->buffer, sizeof(uint8_t), pBMPFile->bmpInfoHeader.biSizeImage, fstream);
    fclose(fstream);
}
