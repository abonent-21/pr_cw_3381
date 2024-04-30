#include <stdio.h>

#pragma pack(1) // выравнивание в 1 байт для структур

typedef struct BMPHeader{
    char ID[2];
    int file_size;
    char unused[4];
    int pixel_offset;
}BMPHeader;

typedef struct DIBHeader{
    int header_size;
    int width;
    int height;
    int color_planes;
    int bit_per_pixel;
    int BI_RGB; // признак наличия сжатия
    int data_size;
    int pwidth;
    int pheight;
    int colors_palette;
    int imp_color_count;
}DIBHeader;

typedef struct BMPFile{
    BMPHeader bheader;
    DIBHeader dheader;
    char* data;
}BMPFile;
#pragma pop

BMPFile* loadBMPfile(char* file_name){
    FILE* fp = fopen(file_name, "r");
    if (!fp){
        return 1;
    }

    BMPFile* bmp_file = (BMPFile*)malloc(sizeof(BMPFile));
    fread(bmp_file, sizeof(bmp_file), 1, fp);
    fclose(fp);
    return bmp_file;
}
void freeBMPfile(BMPFile* bmp_file){
    if (bmp_file){
        free(bmp_file);
    }
}

int main(int argc, char* argv[]){
    
    return 0;
}