#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>
#include "lodepng.h"
#include "lodepng.c"

//буду считать компоненты связности с помощью DSU
typedef struct DSU{
    int *parent;
    int *rank;
    int size;
}DSU;

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
  long long unsigned int pngsize;
  int error = lodepng_encode32(&png, &pngsize, image, width, height);
  if(error == 0) {
      lodepng_save_file(png, pngsize, filename);
  } else {
    printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  free(png);
}

void initDSU(DSU *dsu, int size){
    int i;
    dsu->size = size;
    dsu->parent = (int*)malloc(size*sizeof(int));
    dsu->rank = (int*)malloc(size*sizeof(int));
    for (i=0; i<size; i++){
        dsu->rank[i] = 0;
        dsu->parent[i] = i;

    }
}

int rootfunc(DSU *dsu, int a) {
    if (dsu->parent[a]!=a) dsu->parent[a] = rootfunc(dsu, dsu->parent[a]);
    return dsu->parent[a];
}

void unionset(DSU *dsu, int a, int b){
    int aroot, broot;
    aroot = rootfunc(dsu, a);
    broot = rootfunc(dsu, b);
    if (broot==aroot) return;
    if (dsu->rank[aroot] > dsu->rank[broot]) dsu->parent[broot] = aroot;
    else{
        dsu->parent[aroot] = broot;
        if (dsu->rank[aroot] == dsu->rank[broot]) dsu->rank[broot]++;
    }
}

// функция для проверки цветов на схожесть
int isSimilar(unsigned char a, unsigned char b, int cmp){
    return abs(a-b) <= cmp;
}
int black(unsigned char pixel, int cmp){
    return pixel <= cmp;
}


//ищу компоненты связности и раскрашиваю пиксели в random цвета, при этом я оставляю черные пиксели черными
void comp_find(unsigned char *picture, unsigned char *bw_pic, int width, int height, int threshold, int black_threshold) {
    int size = width * height, i, j, cur_ind;
    DSU dsu;
    initDSU(&dsu, width*height);
    for (j= 0; j< height; j++) {
        for (i = 0; i < width;i++) {
            cur_ind = i+j* width;
            if (black(bw_pic[cur_ind], black_threshold)) continue;
            if (i+1 < width){
                int right_idx = j*width +i+1;
                if (!black(bw_pic[right_idx], black_threshold) && isSimilar(bw_pic[cur_ind], bw_pic[right_idx], threshold)){
                    unionset(&dsu, cur_ind, right_idx);
                }
            }
            if (j+1 < height){
                int bottom_idx = (j+ 1)*width + i;
                if (!black(bw_pic[bottom_idx], black_threshold) && isSimilar(bw_pic[cur_ind], bw_pic[bottom_idx], threshold)){
                    unionset(&dsu, cur_ind, bottom_idx);
                }
            }
        }
    }
    int *col_r = (int *)malloc(width*height*sizeof(int));
    int *col_g = (int *)malloc(width*height*sizeof(int));
    int *col_b = (int *)malloc(width*height*sizeof(int));

    srand(time(NULL));

    for (i=0; i<width*height; i++) {
        if (dsu.parent[i] == i && !black(bw_pic[i], black_threshold)){
            col_r[i] = 50+rand()%206;
            col_g[i] = 50 + rand()%206;
            col_b[i] = 50 + rand()%206;
        }
    }
    for (i=0; i<width*height; i++) {
        if (black(bw_pic[i], black_threshold)){
            picture[4*i] = 0;
            picture[4*i+1] = 0;
            picture[4*i+2] = 0;
        }else{
            int root = rootfunc(&dsu, i);
            picture[i*4] = col_r[root];
            picture[i*4+1] = col_g[root];
            picture[i*4+2] = col_b[root];
        }
    }
    free(col_r), free(col_g), free(col_b);
    free(dsu.parent);
    free(dsu.rank);

}


//перевод картинки из RGB в черно-белую
void rgba_to_bw(unsigned char *bw_pic, unsigned char *rgb_pic, int width, int height){
  int i;
  for(i=0; i<width*height; i++){
    bw_pic[i] = 0.3*rgb_pic[4*i] + 0.59*rgb_pic[4*i+1] + 0.11*rgb_pic[4*i+2];
  }
  printf("Made rgb to bw\n");
  return ;
}
//перевод картинки из черно-белой в RGB
void bw_to_rgba(unsigned char *bw_pic, unsigned char *rgb_pic, int width, int height){
    int i;
    int size=4*width*height;
    for(i=0; i<size-4; i+=4){
		rgb_pic[i]=bw_pic[i/4];
		rgb_pic[i+1]=bw_pic[i/4];
		rgb_pic[i+2]=bw_pic[i/4];
		rgb_pic[i+3]=255;
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
            if (bw_pic[i] > 0 && bw_pic[i] < 255) bw_pic[i] = (unsigned char)((bw_pic[i]-min_col)*scale);
        }
    }
    printf("all contrasts done\n");
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
    printf("blur is done\n");
   return;
}
//фильтр Робертса
void roberts_alg(unsigned char *bw_pic, int width, int height) {
    unsigned char *edges = (unsigned char *)malloc(width * height);
    int Gx_kernel[2][2] = {{1, 0}, {0, -1}};  // Диагональ 1
    int Gy_kernel[2][2] = {{0, 1}, {-1, 0}};  // Диагональ 2
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
            float magnitude = sqrtf(gx*gx + gy*gy);
            int edge_val = (int)(magnitude);
            if (edge_val > 255) edge_val = 255;
            edges[y*width + x] = (unsigned char)edge_val;
        }
    }
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (y < height - 1 && x < width - 1) bw_pic[y * width + x] = edges[y * width + x];
        }
    }
    free(edges);
}

void color(unsigned char *blr_pic, unsigned char *res, int size)
{
  int i;
    for(i=0;i<size;i++)
    {
        res[i*4]=10+blr_pic[i]+0.35*blr_pic[i-1];
        res[i*4+1]=(65+blr_pic[i])*0.85;
        res[i*4+2]=(170+blr_pic[i])*0.5;
        res[i*4+3]=255;
    }
    printf("colored\n");
    return;
}
//посмотрим как выглядела бы картика, если бы мы раскрашивали ее без компонент связности
void simple_color(unsigned char *rgba_pic, int width, int height){
	int size=4*width*height;
	int i, r, g, b, a, bw;
	for(i=0; i<size-4; i+=4){
		bw = rgba_pic[i];
		a=255;
		if (bw >= 0 && bw < 70){
            r = 3*bw;
            g = 3*bw;
            b = 3*bw;
        }else if (bw >= 70 && bw < 160){
            r = 150;
			g =3*(bw-70);
			b = 90;
		}else{
		    r = 255;
            g = 200;
            b = 3*(bw-160);
		}
		rgba_pic[i] = r;
		rgba_pic[i+1] = g;
		rgba_pic[i+2] = b;
		rgba_pic[i+3] = a;
	}
}
int main()
{
    const char* filename = "skull.png";
    unsigned int width, height;
    int size, bw_size;

    // Прочитали картинку
    unsigned char* picture = load_png("skull.png", &width, &height);
    if (picture == NULL)
    {
        printf("Problem reading picture from the file %s. Error.\n", filename);
        return -1;
    }

    size = 4*width*height;
    bw_size = width*height;

    unsigned char* bw_pic = (unsigned char*)malloc(bw_size*sizeof(unsigned char));
    unsigned char* blr_pic = (unsigned char*)malloc(bw_size*sizeof(unsigned char));
    unsigned char* finish = (unsigned char*)malloc(size*sizeof(unsigned char));

    rgba_to_bw(bw_pic, picture, width, height);

    contrast(bw_pic, width, height, 40, 200);
    roberts_alg(bw_pic, width, height);
    Gauss_blur(bw_pic, blr_pic, width, height);
    //roberts_alg(blr_pic, width, height);
    bw_to_rgba(blr_pic, finish, width, height);
        // посмотрим на промежуточные картинки
    write_png("contrast.png", finish, width, height);
    simple_color(finish, width, height);
    write_png("simple_color.png", finish, width, height);
    // поиграли с Гауссом
    //roberts_alg(bw_pic, width, height);
    //Gauss_blur(bw_pic, blr_pic, width, height);
    //roberts_alg(blr_pic, width, height);
    //bw_to_rgba(blr_pic, finish, width, height);
    // посмотрим на промежуточные картинки
    //write_png("gauss.png", finish, width, height);
    printf("Done 1 step\n");

    comp_find(finish, blr_pic, width, height, 35, 15);

    //write_png("intermediate_result.png", finish, width, height);

    write_png("final_picture.png", finish, width, height);
    printf("Done 2 step\n");

    free(bw_pic);
    free(blr_pic);
    free(finish);
    free(picture);

    return 0;
}
