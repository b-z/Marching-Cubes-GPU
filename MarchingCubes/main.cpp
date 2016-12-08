#include "gpu-mc.hpp"
//#include "rawUtilities.hpp"
#include <iostream>
#include <string>
#include "Nii.h"
#include "VolumeData.h"
//using namespace std;
//#include <cstdlib>
#define GLX_DIRECT_RENDERING 1
int main(int argc, char ** argv) {
#if 1
    Nii* nii = new Nii;
    nii->niiReadImage("Z:/zhoubowei/Documents/VolumeSeg/VolumeSeg/data/test.nii");
    //nii->niiReadImage("Z:/zhoubowei/Documents/VolumeSeg/VolumeSeg/data/101345a.nii.gz");
    nii->niiPrintInfo();
    VolumeData* v = new VolumeData;
    v->fromNii(nii);

    //int sizeX = v->nx;
    //int sizeY = v->ny;
    //int sizeZ = v->nz;

    //int stepSizeX = 1;
    //int stepSizeY = 1;
    //int stepSizeZ = 1;

    //float scaleX = v->dx;
    //float scaleY = v->dy;
    //float scaleZ = v->dz;

    //int size_tmp = v->nvox;
    //short* voxels = v->data;

    MarchingCubes mc(v, 1000);

    
    mc.run();

    return 0;
#else
    // Process arguments
    if (argc == 5 || argc == 8 || argc == 11) {
        char * filename = argv[1];
        int sizeX = atoi(argv[2]);
        int sizeY = atoi(argv[3]);
        int sizeZ = atoi(argv[4]);
        int stepSizeX = 1;
        int stepSizeY = 1;
        int stepSizeZ = 1;
        float scaleX = 1;
        float scaleY = 1;
        float scaleZ = 1;
        if (argc > 5) {
            stepSizeX = atoi(argv[5]);
            stepSizeY = atoi(argv[6]);
            stepSizeZ = atoi(argv[7]);
        }
        if (argc > 8) {
            scaleX = atof(argv[8]);
            scaleY = atof(argv[9]);
            scaleZ = atof(argv[10]);
        }
        unsigned char * voxels = readRawFile(filename, sizeX, sizeY, sizeZ, stepSizeX, stepSizeY, stepSizeZ);
        if (voxels == NULL) {
            cout << "File '" << filename << "' not found!" << endl;
            return EXIT_FAILURE;
        }
        int size = prepareDataset(&voxels, sizeX / stepSizeX, sizeY / stepSizeY, sizeZ / stepSizeZ);
        setupOpenGL(&argc, argv, size, sizeX / stepSizeX, sizeY / stepSizeY, sizeZ / stepSizeZ, scaleX, scaleY, scaleZ);
        setupOpenCL(voxels, size);
        run();
    }
    else {
        cout << "usage: filename.raw sizeX sizeY sizeZ [stepSizeX stepSizeY stepSizeZ] [spacingX spacingY spacingZ]" << endl;
    }

    return 0;
#endif
}
