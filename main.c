#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include<math.h>

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
        BMPFile* empty = NULL;
        return empty;
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

RGB rgb(int r, int g, int b){
    RGB tmp;
    tmp.r = r;
    tmp.g = g;
    tmp.b = b;
    return tmp;
}

void set_color(RGB* cell, RGB color){
    cell->r = color.r;
    cell->g = color.g;
    cell->b = color.b;
}

void fill_circle(BMPFile* bmp_file, RGB color, int x0, int y0, int radius){ 
    // в функцию подаются уже исправленные координаты (x, y) -> (x, h - y)
    // (0, 0) -> (0, h)
    int h = bmp_file->dheader.height;
    int w = bmp_file->dheader.width;
    for (int i = -radius; i <= radius; i++){
        for (int j = -radius; j <= radius; j++){
            int x_r = x0 + j;
            int y_r = y0 - i;
            if (i * i + j * j <= radius * radius &&
            x_r >= 0 && x_r < w && y_r >= 0 && y_r < h){
                set_color(&((bmp_file->data)[y_r][x_r]), color);
            }
        }
    }

}

void fill_rectagle(BMPFile* bmp_file, RGB color, int x0, int y0, int radius){
    // в функцию подаются уже исправленные координаты (x, y) -> (x, h - y)
    // (0, 0) -> (0, h)
    int h = bmp_file->dheader.height;
    int w = bmp_file->dheader.width;
    for (int i = -radius; i < radius; i++){
        for (int j = -radius; j < radius; j++){
            int x_r = x0 + j;
            int y_r = y0 - i;
            if (x_r >= 0 && x_r < w && y_r >= 0 && y_r < h){
                set_color(&((bmp_file->data)[y_r][x_r]), color);
            }
        }
    }
}




void draw_line(BMPFile* bmp_file, RGB color, 
            int x1, int y1, int x2, int y2, int thickness, char type){
    int h = bmp_file->dheader.height;

    thickness = ceil(thickness / 2);
    if (thickness <= 0){
        printf("Error sign of thickness");
        return;
    }

    const int deltaX = abs(x2 - x1);
    const int deltaY = abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;
    set_color(&((bmp_file->data)[y2][x2]), color);
    if (type == 'r'){
        fill_rectagle(bmp_file, color, x2, y2, thickness);
    }else{
        fill_circle(bmp_file, color, x2, y2, thickness);
    }
    while(x1 != x2 || y1 != y2) 
    {
        set_color(&((bmp_file->data)[y1][x1]), color);
        if (type == 'r'){
            fill_rectagle(bmp_file, color, x1, y1, thickness);
        }else{
            fill_circle(bmp_file, color, x1, y1, thickness);
        }
        int error2 = error * 2;
        if(error2 > -deltaY) 
        {
            error -= deltaY;
            x1 += signX;
        }
        if(error2 < deltaX) 
        {
            error += deltaX;
            y1 += signY;
        }
    }

}

void draw_rectangle(BMPFile* bmp_file, RGB color, int x0, int y0, 
                    int x1, int y1, int th, char type, int is_fill, RGB color_fill){
    int dx = x1 - x0;
    int dy = y1 - y0;
    draw_line(bmp_file, color, x0, y0, x0 + dx, y0, th, type);
    draw_line(bmp_file, color, x0, y0, x0, y0 + dy, th, type);
    draw_line(bmp_file, color, x0, y0 + dy, x0 + dx, y0 + dy, th, type);
    draw_line(bmp_file, color, x0 + dx, y0, x0 + dx, y0 + dy, th, type);
    int t = (int)(th / 2) + 1;
    if (is_fill == 1){
        for (int y = y0 - t; y > (y1 + t); y--){
            for (int x = x0 + t; x < (x1 - t); x++){
                set_color(&((bmp_file->data)[y][x]), color_fill);
            }
        }
    }
    else if (is_fill == 2){
        int h = bmp_file->dheader.height;
        int w = bmp_file->dheader.width;
        for (int y = 0; y < h; y++){
            for (int x = 0; x < w; x++){
                if (!(x > (x0 - t) && y < (y0 + t) 
                        && x < (x1 + t) && y > (y1 - t))){
                    set_color(&((bmp_file->data)[y][x]), color_fill);
                }
            }
        }
    }
}

void draw_8pixels(BMPFile* bmp_file, int64_t x0, int64_t y0,
                  int64_t x, int64_t y, int32_t thickness, RGB color)
{
    fill_circle(bmp_file, color, x + x0, y + y0, thickness);
    fill_circle(bmp_file, color ,x + x0, -y + y0, thickness);
    fill_circle(bmp_file, color, -x + x0, -y + y0, thickness);
    fill_circle(bmp_file, color, -x + x0, y + y0, thickness);
    fill_circle(bmp_file, color, y + x0, x + y0, thickness);
    fill_circle(bmp_file, color, y + x0, -x + y0, thickness);
    fill_circle(bmp_file, color, -y + x0, -x + y0, thickness);
    fill_circle(bmp_file, color, -y + x0, x + y0, thickness);
}

void draw_circle(BMPFile* bmp_file, int64_t x0, int64_t y0, int32_t radius,
                 int32_t thickness, RGB color,
                 uint8_t is_fill, RGB fill_color)
{
    y0 =  bmp_file->dheader.height - y0;
    int64_t hor_dist;
    int64_t diag_dist;
    int64_t dist;

    int64_t x = 0;
    int64_t y = radius;
    

    thickness = (int)((thickness + 1) / 2);

    if (thickness <= 0){
        printf("Error sign of thickness");
        return;
    }


    if (is_fill == 1) {
        fill_circle(bmp_file, fill_color, x0, y0, radius);
    }
    else if (is_fill == 2){
        int h = bmp_file->dheader.height;
        int w = bmp_file->dheader.width;
        for (int i = 0; i < w; i++){
            for (int j = 0; j < h; j++){
                if (!((i - x0) * (i - x0)  + (j - y0) * (j - y0) <= radius * radius)){
                set_color(&((bmp_file->data)[j][i]), fill_color);
                }
            }
        }
    }
   
    dist = 3 - 2 * y;
    while (x <= y) {
        draw_8pixels(bmp_file, x0, y0, x, y, thickness, color);
        if (dist < 0)
            dist = dist + 4 * x + 6;
        else {
            dist = dist + 4 * (x - y) + 10;
            y--;
        }
        x++; 
    }
}

void draw_circle_ornament(BMPFile* bmp_file, RGB color){
    int h = bmp_file->dheader.height;
    int w = bmp_file->dheader.width;
    int x0 = (int)(w / 2);
    int y0 = (int)(h / 2);
    int radius = (int)(h / 2);
    draw_circle(bmp_file, x0, y0, radius, 1, color, 2, color);
}

void draw_rectangle_ornament(BMPFile* bmp_file, int thickness, RGB color){

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
    char* name_of_input_file = "file4.bmp";

    int s_color_replace = 0;
    int s_ornament = 0;
    int s_filled_rects = 0;
    RGB old_color;
    RGB new_color;
    char* pattern;
    RGB color;
    int thickness;
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
    int coords[4] = {60, 60, 120, 120};
    int h = bmp_file->dheader.height;
    coords[1] = h - coords[1];
    coords[3] = h - coords[3];
    // draw_line(bmp_file, rgb(255, 0, 0), coords[0], coords[1], coords[2], coords[3], 3, 'r');
    if (coords[1] < 0 || coords[3] < 0){printf("Error coord in fill rect"); return;}
    draw_rectangle(bmp_file, rgb(0, 255, 0), coords[0], coords[1], coords[2], coords[3], 20, 'r', 0, rgb(255, 0, 0));
    // draw_circle(bmp_file, coords[2], coords[3], 20, 1, rgb(0, 255, 0), 2, rgb(0, 255, 0));
    // draw_circle_ornament(bmp_file, rgb(0, 255, 0));
    writeBMPfile(name_output_file, bmp_file);
    return 0;
}
