#!/bin/bash

make -j8

#./darknet detector demo cfg/coco.data cfg/yolov3.cfg weights/yolov3.weights test.MTS -thresh 0.3

./darknet detector demo cfg/nctu_delta.data cfg/yolov3.cfg weights/nctu_delta_200.weights test.MTS -thresh 0.3
