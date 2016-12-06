#ifndef VOLUME_DATA_H
#define VOLUME_DATA_H

#include "Nii.h"
//#include "Imagedata.h"
#include <string>

//class ImageData;

enum SliceType {
    SLICE_SAGITTAL = 0,
    SLICE_CORONAL = 1,
    SLICE_AXIAL = 2
};

class VolumeData {
public:
    VolumeData();
    ~VolumeData();
    bool fromNii(Nii* nii);

    int idx(int x, int y, int z);
    short at(int x, int y, int z);
    void set(int x, int y, int z, short new_data);
    //float at_f(int x, int y, int z);

    //ImageData* sliceAt(int pos, SliceType slice_type);
    void release();
public:
    short* data;
    //float* data_f;
    int nx;
    int ny;
    int nz;
    float dx;
    float dy;
    float dz;
    int nvox;
    int datalen;
    std::string datatype;
};


#endif