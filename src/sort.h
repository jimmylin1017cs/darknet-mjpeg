#ifndef SORT_H
#define SORT_H

#include "box.h" // I do not know why it can not put in the extern c

typedef struct{
    int id;
    int x1, y1, x2, y2;
    float prob;
} person_sort_det;

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "utils.h"
#include "blas.h"
#include "cuda.h"
#include "image.h"

void draw_detections_with_sort_id(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes);

void draw_detections_with_sort(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes);
void sort_init();
int sort_update(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes);
void sort_cleanUp();

#ifdef __cplusplus
}
#endif

#endif // SORT_H

/*
typedef struct {
    int w;
    int h;
    float scale;
    float rad;
    float dx;
    float dy;
    float aspect;
} augment_args;

typedef struct {
    int w;
    int h;
    int c;
    float *data;
} image;

typedef struct{
    float x, y, w, h;
} box;

typedef struct detection{
    box bbox;
    int classes;
    float *prob;
    float *mask;
    float objectness;
    int sort_class;
} detection;

*/