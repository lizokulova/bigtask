#include <stdio.h>
#include <stdlib.h> 
#include <strings.h>
#include <math.h> 
#include "lodepng.h" 

typedef struct DSU{
    int *parent;
    int *rank;
    int size;
}DSU;

// Инициализация DSU
void initDSU(DSU *dsu, int size){
    dsu->parent = (int *)malloc(size*sizeof(int));
    dsu->rank = (int *)malloc(size*sizeof(int));
    dsu->size = size;
    for (int i = 0; i < size; i++) {
        dsu->parent[i] = i;
        dsu->rank[i] = 0;
    }
}
// Найти корень элемента с path compression
int findRoot(DSU *dsu, int x) {
    if (dsu->parent[x] != x) {
        dsu->parent[x] = findRoot(dsu, dsu->parent[x]);
    }
    return dsu->parent[x];
}
// Объединить два множества
void unionSets(DSU *dsu, int x, int y) {
    int rootX = findRoot(dsu, x);
    int rootY = findRoot(dsu, y);
    if (rootX == rootY) return;
    if (dsu->rank[rootX] > dsu->rank[rootY]){
        dsu->parent[rootY] = rootX;
    } else {
        dsu->parent[rootX] = rootY;
        if (dsu->rank[rootX] == dsu->rank[rootY]) {
            dsu->rank[rootY]++;
        }
    }
}

// Проверка, являются ли два цвета "примерно одинаковыми"
int isSimilar(unsigned char a, unsigned char b, int threshold) {
    return abs(a - b) <= threshold;
}

// Найти компоненты связности в изображении
void findConnectedComponents(unsigned char *bw_pic, int width, int height, int threshold) {
    int size = width * height;
    DSU dsu;
    initDSU(&dsu, size);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int current_idx = y * width + x;
            if (x + 1 < width) {
                int right_idx = y * width + (x + 1);
                if (isSimilar(bw_pic[current_idx], bw_pic[right_idx], threshold)) {
                    unionSets(&dsu, current_idx, right_idx);
                }
            }
            if (y + 1 < height) {
                int bottom_idx = (y + 1) * width + x;
                if (isSimilar(bw_pic[current_idx], bw_pic[bottom_idx], threshold)) {
                    unionSets(&dsu, current_idx, bottom_idx);
                }
            }
        }
    }

    int *component_sizes = (int *)calloc(size, sizeof(int));
    for (int i = 0; i < size; i++) {
        int root = findRoot(&dsu, i);
        component_sizes[root]++;
    }
    
    // Вывести информацию о компонентах (для примера)
    int num_components = 0;
    for (int i = 0; i < size; i++) {
        if (component_sizes[i] > 0) {
            num_components++;
        }
    }
    printf("Total connected components: %d\n", num_components);
    
    free(component_sizes);
    free(dsu.parent);
    free(dsu.rank);
}

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
//перевод картинки из черно-белой в RGB
void bw_to_rgba(unsigned char *bw_pic, insigned char *rgb_pic, int width, int height){
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
    int Gx_kernel[2][2] = {{1, 0}, {0, -1}};  // Диагональ \
    int Gy_kernel[2][2] = {{0, 1}, {-1, 0}};  // Диагональ /
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

    rgba_to_bw(bw_pic, picture, width, height);

    // Например, поиграли с  контрастом
    contrast(bw_pic, width, height, 30, 200); 
    
        // посмотрим на промежуточные картинки
    //write_png("contrast.png", finish, width, height);
    
    // поиграли с Гауссом
    Gauss_blur(bw_pic, blr_pic, width, height); 
    color(blr_pic, finish, size);
    // посмотрим на промежуточные картинки
    write_png("gauss.png", finish, width, height);
/*
    // Пример использования
      int width = 5;
      int height = 5;
      unsigned char bw_pic[] = {
          100, 100, 105, 200, 200,
          100, 102, 110, 200, 200,
          100, 100, 105, 200, 200,
          50,  50,  55, 210, 210,
          50,  50,  50, 210, 210
      };
      
      int threshold = 10; // Максимальная разница в цвете
      
      findConnectedComponents(bw_pic, width, height, threshold);
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
    */
    // не забыли почистить память!
    free(bw_pic); 
    free(blr_pic); 
    free(finish); 
    free(picture); 
    
    return 0; 
}
