#ifndef DAI_PUSH
#define DAI_PUSH

#include <Python.h>
#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "sort.h"

typedef struct{
    std::string name;
    int id;
    int x1, y1, x2, y2;
} person_box;

#ifdef __cplusplus
extern "C" {
#endif

#include "image.h"

void iot_talk_start(image &im, std::vector<person_sort_det> &person_sort_dets, int &person_num);
void iot_init();
void iot_send(std::vector<unsigned char> &outbuf, std::vector<person_box> &boxes);

#ifdef __cplusplus
}
#endif

#endif

