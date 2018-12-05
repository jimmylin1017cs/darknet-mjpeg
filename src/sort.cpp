#include "sort.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>



#include <iostream>
#include <vector>

#include <Python.h>

#define PERSON_ONLY 1
#define SORT_FREQ 1 // sort update frequency
#define MAX_SORT_NUM 500
#define ENABLE_SORT_TRACKER 1

#define CHECK_PYTHON_NULL(p) \
    if (NULL == (p)) {\
        PyErr_Print();\
        exit(EXIT_FAILURE);\
    }


#ifdef __cplusplus
extern "C" {
#endif

static float **last_probs;
static box *last_boxes;
static int *last_sort_ids;
static int last_sort_ct = 0;
static std::vector<person_sort_det> person_sort_dets;

static int first_called = 1;

PyObject *pName, *pModule, *pDict, *pClass, *pTracker, *pTrackerRet, *pInstance;

void draw_detections_with_sort_id(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes)
{
    int person_num = sort_update(im, dets, num, thresh, names, alphabet, classes);
    printf("person_sort_dets.size(): %d\n", person_num);

    for(int i = 0; i < person_num; ++i)
    {
        char labelstr[4096] = {0};

        sprintf(labelstr, "%d", person_sort_dets[i].id);
        printf("person: %d\n", person_sort_dets[i].id);

        int left = person_sort_dets[i].x1;
        int top = person_sort_dets[i].y1;
        int right = person_sort_dets[i].x2;
        int bot = person_sort_dets[i].y2;

        float red = 0;
        float green = 255;
        float blue = 0;

        float rgb[3];
        rgb[0] = red;
        rgb[1] = green;
        rgb[2] = blue;

        int width = im.h * .006;
        draw_box_width(im, left, top, right, bot, width, red, green, blue);
        if (alphabet)
        {
            image label = get_label(alphabet, labelstr, (im.h*.03));
            draw_label(im, top + width, left, label, rgb);
            free_image(label);
        }
    }
}

void draw_detections_with_sort(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes)
{
    int i,j;

    sort_update(im, dets, num, thresh, names, alphabet, classes);

    // edit nctu_delta.data classes = 1
    // data/coco.names person is index 0
    for(i = 0; i < num; ++i){
        char labelstr[4096] = {0};
        int class_index = -1;
        for(j = 0; j < classes; ++j){
            if (dets[i].prob[j] > thresh){
                if (class_index < 0) {
                    strcat(labelstr, names[j]);
                    class_index = j;
                } else {
                    strcat(labelstr, ", ");
                    strcat(labelstr, names[j]);
                }
                printf("%s: %.0f%%\n", names[j], dets[i].prob[j]*100);
            }
        }
        if(class_index >= 0){
            int width = im.h * .006;

            /*
               if(0){
               width = pow(prob, 1./2.)*10+1;
               alphabet = 0;
               }
             */

            //printf("%d %s: %.0f%%\n", i, names[class_index], prob*100);
            int offset = class_index*123457 % classes;
            float red = get_color(2,offset,classes);
            float green = get_color(1,offset,classes);
            float blue = get_color(0,offset,classes);
            float rgb[3];

            //width = prob*20+2;

            rgb[0] = red;
            rgb[1] = green;
            rgb[2] = blue;
            box b = dets[i].bbox;
            //printf("%f %f %f %f\n", b.x, b.y, b.w, b.h);

            int left  = (b.x-b.w/2.)*im.w;
            int right = (b.x+b.w/2.)*im.w;
            int top   = (b.y-b.h/2.)*im.h;
            int bot   = (b.y+b.h/2.)*im.h;

            if(left < 0) left = 0;
            if(right > im.w-1) right = im.w-1;
            if(top < 0) top = 0;
            if(bot > im.h-1) bot = im.h-1;

            draw_box_width(im, left, top, right, bot, width, red, green, blue);
            if (alphabet) {
                image label = get_label(alphabet, labelstr, (im.h*.03));
                draw_label(im, top + width, left, label, rgb);
                free_image(label);
            }

            // nctu_delta.cfg [region] classes=80 coords=4, so no go into the scope
            if (dets[i].mask){
                image mask = float_to_image(14, 14, 1, dets[i].mask);
                image resized_mask = resize_image(mask, b.w*im.w, b.h*im.h);
                image tmask = threshold_image(resized_mask, .5);
                embed_image(tmask, im, left, top);
                free_image(mask);
                free_image(resized_mask);
                free_image(tmask);
            }
        }
    }
}

void sort_init()
{
    Py_Initialize();

    pModule = PyImport_ImportModule("sort");
    CHECK_PYTHON_NULL(pModule)

    pDict = PyModule_GetDict(pModule);
    pClass = PyDict_GetItemString(pDict, "Sort"); // get Sort class from sort.py

    if (PyCallable_Check(pClass))
    {
        pTracker = PyObject_CallObject(pClass, NULL);
    }
}

//int sort_update(image im, box* detections, float** probs, int num, int classes, float thresh, int *sort_ids, int sort_freq, int filter_small_scale){
int sort_update(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes)
{
    //std::vector<person_sort_det> person_sort_dets;
    person_sort_dets.clear();

    if(first_called)
    {
        first_called = 0;
        sort_init();
    }

    int person_num = 0;
    // store all boxes(detections) information
    PyObject *pDetections = PyList_New(0);

    for(int i = 0; i < num; ++i)
    {
        int class_index = -1;
        //printf("%d\n", classes);
        for(int j = 0; j < classes; ++j)
        {
            if (dets[i].prob[j] > thresh)
            {
                if (class_index < 0)
                {
                    class_index = j;
                }
                printf("%s: %.0f%%\n", names[j], dets[i].prob[j]*100);
            }
        }

        //printf("%d\n", class_index);

        if(class_index >= 0)
        {
            person_num++;

            /*
               if(0){
               width = pow(prob, 1./2.)*10+1;
               alphabet = 0;
               }
            */

            box b = dets[i].bbox;
            int left  = (b.x-b.w/2.)*im.w;
            int right = (b.x+b.w/2.)*im.w;
            int top   = (b.y-b.h/2.)*im.h;
            int bot   = (b.y+b.h/2.)*im.h;
            float prob = dets[i].prob[class_index];

            if(left < 0) left = 0;
            if(right > im.w-1) right = im.w-1;
            if(top < 0) top = 0;
            if(bot > im.h-1) bot = im.h-1;

            // skip too small box
            float filter_small_scale = 10;
            if(filter_small_scale!=0 && ((right-left)<im.h/filter_small_scale || (bot-top)<im.h/filter_small_scale))
                continue;

            // store person detection information
            person_sort_det tmp_psd = {-1, left, top, right, bot, prob};
            person_sort_dets.push_back(tmp_psd);

            // store left, top, right, bot, prob
            PyObject *pDetection = PyList_New(5);
            PyList_SetItem(pDetection, 0, PyInt_FromLong(left));
            PyList_SetItem(pDetection, 1, PyInt_FromLong(top));
            PyList_SetItem(pDetection, 2, PyInt_FromLong(right));
            PyList_SetItem(pDetection, 3, PyInt_FromLong(bot));
            PyList_SetItem(pDetection, 4, PyFloat_FromDouble(prob));

            if(PyList_Append(pDetections, pDetection))
            {
                printf("ERROR: failed to append pDetection into pDetections.\n");
            }

            // free pDetection
            Py_DECREF(pDetection);
        }
    }

    // no any person
    if(person_num == 0)
    {
        printf("Do not detect any person!\n");
        Py_DECREF(pDetections);
        return 0;
    }

    // get sort.update return
    pTrackerRet = PyObject_CallMethod(pTracker, "update", "O", pDetections);

    int pTrackerRetSize = PyList_Size(pTrackerRet);

    PyObject *pSortDets;

    int x1, y1, x2, y2, score;
    
    printf("pTrackerRetSize: %d\n", pTrackerRetSize);

    for(int i = 0; i < pTrackerRetSize; i++)
    {
        pSortDets = PyList_GetItem(pTrackerRet, i);
        x1 = PyInt_AsLong(PyList_GetItem(pSortDets, 0));
        y1 = PyInt_AsLong(PyList_GetItem(pSortDets, 1));
        x2 = PyInt_AsLong(PyList_GetItem(pSortDets, 2));
        y2 = PyInt_AsLong(PyList_GetItem(pSortDets, 3));
        score = PyInt_AsLong(PyList_GetItem(pSortDets, 4)) % MAX_SORT_NUM;

        if(i < person_num)
        {
            if(person_sort_dets[i].id == -1)
            {
                person_sort_dets[i].id = score;
            }
        }
    }

    // free pDetections
   	Py_DECREF(pDetections);

    return pTrackerRetSize;
}

void sort_cleanUp()
{
	Py_DECREF(pModule);
    Py_DECREF(pName);
    Py_Finalize();
}

#ifdef __cplusplus
}
#endif