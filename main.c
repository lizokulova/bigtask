#include <stdio.h>
#include <stdlib.h> 
#include <strings.h>
#include <math.h> 
#include "lodepng.h" 


unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height) 
{
  unsigned char* image = NULL; 
  int error = lodepng_decode32_file(&image, width, height, filename);
  if(error != 0) {
    printf("error %u: %s\n", error, lodepng_error_text(error)); 
  }
  return (image);
}

void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
  unsigned char* png;
  long unsigned int pngsize;
  int error = lodepng_encode32(&png, &pngsize, image, width, height);
  if(error == 0) {
      lodepng_save_file(png, pngsize, filename);
  } else { 
    printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  free(png);
}
//перевод картинки из RGB в черно-белую
void rgba_to_bw(unsigned char *bw_pic, insigned char *rgb_pic, int width, int height){
  int i;
  for(i=0; i<width*height; i++){
    bw_pic[i] = 0.3*rgb_pic[4*i] + 0.59*rgb_pic[4*i+1] + 0.11*rgb_pic[4*i+2];
  }
  return ;
}

// вариант огрубления серого цвета в ЧБ с заданными верхним и нижним диапазоном огрубления в main
void contrast(unsigned char *bw_pic, int width, int height, int bottom, int top){ 
    int i; 
    for(i=0; i < width*height; i++){
        if(bw_pic[i] < bottom)
        bw_pic[i] = 0; 
        if(bw_pic[i] > top)
        bw_pic[i] = 255;
    } 
    // далее использую метод линейного контрастирования
    int min_col = 255, max_col = 0;
    for (i=0; i<width*height; i++){
        if (bw_pic[i] > 0 && bw_pic[i] < 255){
            if (bw_pic[i] < min_col) min_col = bw_pic[i];
            if (bw_pic[i] > max_col) max_col = bw_pic[i];
        }
    }
    if (max_col>min_col){
        float scale = 255.0f / (max_col - min_col);
        for (i = 0; i < width*height; i++){
            if (bw_pic[i] > 0 && bw_pic[i] < 255) bw_pic[i] = (unsigned char)((bw_pic[i]-min_val)*scale);
        }
    }
    return; 
} 

// Гауссово размыттие
void Gauss_blur(unsigned char *col, unsigned char *blr_pic, int width, int height)
{ 
    int i, j; 
    for(i=1; i < height-1; i++) 
        for(j=1; j < width-1; j++)
        { 
            blr_pic[width*i+j] = 0.084*col[width*i+j] + 0.084*col[width*(i+1)+j] + 0.084*col[width*(i-1)+j]; 
            blr_pic[width*i+j] = blr_pic[width*i+j] + 0.084*col[width*i+(j+1)] + 0.084*col[width*i+(j-1)]; 
            blr_pic[width*i+j] = blr_pic[width*i+j] + 0.063*col[width*(i+1)+(j+1)] + 0.063*col[width*(i+1)+(j-1)]; 
            blr_pic[width*i+j] = blr_pic[width*i+j] + 0.063*col[width*(i-1)+(j+1)] + 0.063*col[width*(i-1)+(j-1)]; 
        } 
   return; 
} 
//оператор Робертса
void roberts_alg(unsigned char *bw_pic, int width, int height) {
    unsigned char *edges = (unsigned char *)malloc(width * height);
    const int Gx_kernel[2][2] = {{1, 0}, {0, -1}};  // Диагональ \
    const int Gy_kernel[2][2] = {{0, 1}, {-1, 0}};  // Диагональ /
    int x, y;
    for (y = 0; y < height-1; y++) {
        for (x = 0; x < width-1; x++) {
            int gx = 0, gy = 0;
            for (int ky = 0; ky < 2; ky++) {
                for (int kx = 0; kx < 2; kx++) {
                    int idx = (y + ky) * width + (x + kx);
                    gx += bw_pic[idx] * Gx_kernel[ky][kx];
                    gy += bw_pic[idx] * Gy_kernel[ky][kx];
                }
            }
            float magnitude = sqrtf(gx * gx + gy * gy);
            int edge_val = (int)(magnitude);
            if (edge_val > 255) edge_val = 255;
            edges[y * width + x] = (unsigned char)edge_val;
        }
    }
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (y < height - 1 && x < width - 1) bw_pic[y * width + x] = edges[y * width + x];
        }
    }
    free(edges);
}

//  Место для экспериментов
void color(unsigned char *blr_pic, unsigned char *res, int size)
{ 
  int i;
    for(i=1;i<size;i++) 
    { 
        res[i*4]=40+blr_pic[i]+0.35*blr_pic[i-1]; 
        res[i*4+1]=65+blr_pic[i]; 
        res[i*4+2]=170+blr_pic[i]; 
        res[i*4+3]=255; 
    } 
    return; 
} 
  
int main() 
{ 
    const char* filename = "skull.png"; 
    unsigned int width, height;
    int size;
    int bw_size;
    
    // Прочитали картинку
    unsigned char* picture = load_png("skull.png", &width, &height); 
    if (picture == NULL)
    { 
        printf("Problem reading picture from the file %s. Error.\n", filename); 
        return -1; 
    } 

    size = width * height * 4;
    bw_size = width * height;
    
    
    unsigned char* bw_pic = (unsigned char*)malloc(bw_size*sizeof(unsigned char)); 
    unsigned char* blr_pic = (unsigned char*)malloc(bw_size*sizeof(unsigned char)); 
    unsigned char* finish = (unsigned char*)malloc(size*sizeof(unsigned char)); 
 
    // Например, поиграли с  контрастом
    contrast(bw_pic, bw_size); 
        // посмотрим на промежуточные картинки
    write_png("contrast.png", finish, width, height);
    
    // поиграли с Гауссом
    Gauss_blur(bw_pic, blr_pic, width, height); 
    // посмотрим на промежуточные картинки
    write_png("gauss.png", finish, width, height);
    
    // сделали еще что-нибудь
    // .....
    // ....
    // ....
    // ....
    // ....
    // ....
    // ....
    //
    
    write_png("intermediate_result.png", finish, width, height);
    color(blr_pic, finish, bw_size); 
    
    // выписали результат
    write_png("picture_out.png", finish, width, height); 
    
    // не забыли почистить память!
    free(bw_pic); 
    free(blr_pic); 
    free(finish); 
    free(picture); 
    
    return 0; 
}
