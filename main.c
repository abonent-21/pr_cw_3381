#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>

#pragma pack(1) // выравнивание в 1 байт для структур

typedef struct BMPHeader{
    char ID[2];
    uint32_t file_size;
    uint16_t reversed1;
    uint16_t reversed2;
    uint32_t  pixel_offset;
}BMPHeader;

typedef struct RGB{
    uint8_t b;
    uint8_t g;
    uint8_t r;
} RGB;

typedef struct DIBHeader{
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t color_planes;
    uint16_t bit_per_pixel;
    uint32_t comp; // признак наличия сжатия
    uint32_t data_size;
    uint32_t pwidth;
    uint32_t pheight;
    uint32_t colors_palette;
    uint32_t imp_color_count;
}DIBHeader;

typedef struct BMPFile{
    BMPHeader bheader;
    DIBHeader dheader;
    RGB** data;
}BMPFile;
#pragma pop

BMPFile* loadBMPfile(char* file_name){
    FILE* fp = fopen(file_name, "rb");
    if (!fp){
        return 1;
    }

    BMPFile* bmp_file = (BMPFile*)malloc(sizeof(BMPFile)); // выделяем память для основной структуры
    fread(&(bmp_file->bheader), sizeof(BMPHeader), 1, fp); // считываем заголовки
    fread(&(bmp_file->dheader), sizeof(DIBHeader), 1, fp);

    int w = bmp_file->dheader.width;
    int h = bmp_file->dheader.height;
    bmp_file->data = (RGB**)malloc(h * sizeof(RGB*));
    for (int i = 0; i < h; i++){
        int padding = (4 - (w*sizeof(RGB)) % 4) % 4;
        bmp_file->data[i] = (RGB*)malloc(w * sizeof(RGB) + padding);
        fread(bmp_file->data[i], 1, w * sizeof(RGB) + padding, fp);
    }
    fclose(fp);
    return bmp_file;
    
}


void writeBMPfile(char* name_file, BMPFile* input_file){
    FILE* file = fopen(name_file, "wb");
    fwrite(&input_file->bheader, sizeof(BMPHeader), 1, file);
    fwrite(&input_file->dheader, sizeof(DIBHeader), 1, file);
    int width = input_file->dheader.width;
    int height = input_file->dheader.height;
    uint8_t padding = (4 - (width * sizeof(RGB)) % 4) % 4;
    uint8_t paddingBytes[3] = {0};
    for (int i = 0; i < height; i++) {
        fwrite(input_file->data[i], sizeof(RGB), width, file);
        fwrite(paddingBytes, sizeof(uint8_t), padding, file); // Записываем выравнивающие байты
    }
    fclose(file);
}

int cmp_color(RGB color1, RGB color2){
    return color1.r == color2.r && color1.g == color2.g && color1.b == color2.b;
}

void set_color(RGB* cell, RGB color){
    cell->r = color.r;
    cell->g = color.g;
    cell->b = color.b;
}

void change_color(BMPFile* input_file, RGB old_color, RGB new_color){
    for (int i = 0; i < input_file->dheader.height; i++){
        for (int j = 0; j < input_file->dheader.width; j++){
            RGB rgb = (input_file->data)[i][j];
            if (cmp_color(rgb, old_color)){
                set_color(&((input_file->data)[i][j]), new_color);
            } 
        }
    }
}


int main(int argc, char* argv[]){
    struct option long_option[] = {
        {"color_replace", no_argument,  0, 'r'},
        {"old_color",   required_argument, 0, 'd'},
        {"new_color",   required_argument, 0, 'n'},
        {"ornament", no_argument,  0, 'o'},
        {"pattern",   required_argument, 0, 'd'},
        {"color",   required_argument, 0, 'c'},
        {"thickness",   required_argument, 0, 't'},
        {"count",   required_argument, 0, 'u'},
        {"filled_rects",   no_argument, 0, 'f'},
        {"border_color",   required_argument, 0, 'b'},
        {"input",   required_argument, 0, 'i'},
        {"output",   required_argument, 0, 'o'},
        {"help",   required_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int opt;
    int option_index;
    char* name_output_file = "out.bmp";
    char* name_of_input_file = "input.bmp";

    int s_color_replace = 0;
    int s_ornament = 0;
    int s_filled_rects = 0;
    RGB old_color;
    RGB new_color;
    char* pattern;
    RGB color;
    int thikness;
    int count;
    int r1, g1, b1;

    while ((opt = getopt_long(argc, argv, "rofd:n:i:o:h", long_option, &option_index)) != -1){
        switch(opt){
            case 'r':
                s_color_replace = 1;
                break;
            case 'o':
                s_ornament = 1;
                break;
            case 't':
                s_filled_rects = 1;
                break;
            case 'd':
                sscanf(optarg, "%d.%d.%d", &r1, &g1, &b1);
                old_color.r = r1;
                old_color.g = g1;
                old_color.b = b1;
                break;
            case 'n':
                sscanf(optarg, "%d.%d.%d", &r1, &g1, &b1);
                new_color.r = r1;
                new_color.g = g1;
                new_color.b = b1;
                break;
        }

    }
    BMPFile* bmp_file =  loadBMPfile(name_of_input_file);

    if ((s_color_replace + s_ornament + s_filled_rects) > 1){
        printf("Error of keys\n");
    }
    if (s_color_replace){
        change_color(bmp_file, old_color, new_color);
    }
    writeBMPfile(name_output_file, bmp_file);
    return 0;
}
