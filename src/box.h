#ifndef BOX_H
#define BOX_H
#include "darknet.h"

// ##### jimmy add for json person box
typedef struct{
    int id;
    int x1, y1, x2, y2;
    float prob;
} person_sort_det;
// ##### jimmy end

typedef struct{
    float dx, dy, dw, dh;
} dbox;

float box_rmse(box a, box b);
dbox diou(box a, box b);
box decode_box(box b, box anchor);
box encode_box(box b, box anchor);

#endif
