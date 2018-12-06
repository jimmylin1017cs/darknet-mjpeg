#include "DAI_push.h"

#define CHECK_PYTHON_NULL(p) \
    if (NULL == (p)) {\
        PyErr_Print();\
        exit(EXIT_FAILURE);\
    }


IplImage *image_to_ipl(image im)
{
    int x,y,c;
    IplImage *disp = cvCreateImage(cvSize(im.w,im.h), IPL_DEPTH_8U, im.c);
    int step = disp->widthStep;
    for(y = 0; y < im.h; ++y){
        for(x = 0; x < im.w; ++x){
            for(c= 0; c < im.c; ++c){
                float val = im.data[c*im.h*im.w + y*im.w + x];
                disp->imageData[y*step + x*im.c + c] = (unsigned char)(val*255);
            }
        }
    }
    return disp;
}

cv::Mat image_to_mat(image im)
{
    image copy = copy_image(im);
    constrain_image(copy);
    if(im.c == 3) rgbgr_image(copy);

    IplImage *ipl = image_to_ipl(copy);
    cv::Mat m = cv::cvarrToMat(ipl, true);
    cvReleaseImage(&ipl);
    free_image(copy);
    return m;
}

#ifdef __cplusplus
extern "C" {
#endif

static int first_called = 1;
static cv::Mat m, frame;
static PyObject *pModule, *pDict, *pFunc, *pInstance;

void iot_talk_start(image &im, std::vector<person_sort_det> &person_sort_dets, int &person_num)
{
    std::vector<person_box> boxes;
    //boxes.clear();

    for(int i = 0; i < person_num; ++i)
    {
        char labelstr[4096] = {0};

        sprintf(labelstr, "%d", person_sort_dets[i].id);
        //printf("person: %d\n", person_sort_dets[i].id);

        int left = person_sort_dets[i].x1;
        int top = person_sort_dets[i].y1;
        int right = person_sort_dets[i].x2;
        int bot = person_sort_dets[i].y2;

        person_box tmp_pb = {labelstr, person_sort_dets[i].id, left, top, right, bot};

        boxes.push_back(tmp_pb);
    }

    std::vector<unsigned char> outbuf;
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY); // default quality value is 95
    compression_params.push_back(30);

    frame = image_to_mat(im);
    //m = image_to_mat(im);
    //cv::resize(m, frame, cv::Size(800, 600), 0, 0);

    cv::imencode(".jpg", frame, outbuf, compression_params); // encodes an image into a memory buffer

    if(first_called)
    {
        first_called = 0;
        iot_init();
    }
    
    iot_send(outbuf, boxes);

}


void iot_init()
{
    if(!Py_IsInitialized())
    {
        Py_Initialize();
    }

    //PySys_SetPath(PYTHON_IMPORT_PATH);

    //setenv("PYTHONPATH", PYTHON_IMPORT_PATH, 1);

    pModule = PyImport_ImportModule("DAI_push");
    CHECK_PYTHON_NULL(pModule)

    pDict = PyModule_GetDict(pModule);
    CHECK_PYTHON_NULL(pDict)

    pFunc = PyDict_GetItemString(pDict, "send_frame_to_iottalk");
    CHECK_PYTHON_NULL(pFunc)
}

void iot_send(std::vector<unsigned char> &outbuf, std::vector<person_box> &boxes)
{
    PyObject *pFrameList  = PyList_New(outbuf.size()); // store frame undigned char int

    int boxes_size = boxes.size();
    PyObject *pBoxDict[boxes_size]; // store all boxes information
    for(int i = 0; i < boxes_size; i++) pBoxDict[i] = PyDict_New();
    PyObject *pBoxList  = PyList_New(boxes_size); // store all box dictionaries
    
    PyObject *pArgList = PyTuple_New(2); // arguments for function

    //std::cout<<"pFrameList"<<std::endl;

    // create python list with frame data
    for(int i = 0; i < PyList_Size(pFrameList); i++)
    {
        PyList_SetItem(pFrameList, i, PyInt_FromLong((int)outbuf[i]));
    }

    PyTuple_SetItem(pArgList, 0, pFrameList); // put in argument

    //std::cout<<"pBoxDict"<<std::endl;

    // create python dictionary with box information
    for(int i = 0; i < boxes_size; i++)
    {
        PyDict_SetItem(pBoxDict[i], PyString_FromString("name"), PyString_FromString(boxes[i].name.c_str()));
        PyDict_SetItem(pBoxDict[i], PyString_FromString("id"), PyInt_FromLong(boxes[i].id));
        PyDict_SetItem(pBoxDict[i], PyString_FromString("x1"), PyInt_FromLong(boxes[i].x1));
        PyDict_SetItem(pBoxDict[i], PyString_FromString("y1"), PyInt_FromLong(boxes[i].y1));
        PyDict_SetItem(pBoxDict[i], PyString_FromString("x2"), PyInt_FromLong(boxes[i].x2));
        PyDict_SetItem(pBoxDict[i], PyString_FromString("y2"), PyInt_FromLong(boxes[i].y2));
    }

    //std::cout<<"pBoxList"<<std::endl;

    // put all dictionaries into list
    for(int i = 0; i < boxes_size; i++)
    {
        PyList_SetItem(pBoxList, i, pBoxDict[i]);
    }

    PyTuple_SetItem(pArgList, 1, pBoxList); // put in argument

    if(PyCallable_Check(pFunc))
    {
        PyObject_CallObject(pFunc, pArgList);
    }
    else
    {
        PyErr_Print();
    }
}

#ifdef __cplusplus
}
#endif