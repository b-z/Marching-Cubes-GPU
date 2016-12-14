#ifndef HPMC_H
#define HPMC_H

////#define __NO_STD_VECTOR // Use cl::vector instead of STL version
#define __CL_ENABLE_EXCEPTIONS
#define __USE_GL_INTEROP

#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <gl/glew.h>
//#include <GL/glut.h>
#include <glut.h>
#include "openCLGLUtilities.hpp"
#include <math.h>

#pragma comment(lib, "Z:/zhoubowei/Documents/VolumeSeg/lib/glew/lib/x64/GLUT32.lib")

//using namespace cl;

//typedef unsigned int uint;
//typedef unsigned char uchar;

#ifndef VSP
#define VSP vtkSmartPointer
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


class VolumeData;

typedef struct {
    int x, y, z;
} MCSize;

typedef struct {
    float x, y, z;
} MCSizef;

//#include "vtkOpenGLPolyDataMapper.h"
//#include "vtkOpenGLVertexBufferObject.h"
//class vtkOpenGLPolyDataMapper_ 
//    :public vtkOpenGLPolyDataMapper
//{
//public:
//    vtkOpenGLPolyDataMapper_()
//        :vtkOpenGLPolyDataMapper()
//    {
//
//    }
//    ~vtkOpenGLPolyDataMapper_(){}
//    vtkOpenGLVertexBufferObject* GetVBO() {
//        return this->VBO;
//    }
//};
#include "vtkSmartPointer.h"
class vtkRenderWindowInteractor;
class vtkActor;
class vtkRenderWindow;
class vtkCellArray;
class vtkRenderer;
class vtkFloatArray;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;

class MarchingCubes {
public:
    MarchingCubes(VolumeData* v, int isolevel_);
    ~MarchingCubes();
    void setupOpenGL(int size, int sizeX, int sizeY, int sizeZ, float spacingX, float spacingY, float spacingZ);
    void setupOpenCL(short * voxels, int size);
    void run();
    void renderScene();
    void idle();
    void reshape(int width, int height);
    void keyboard(unsigned char key, int x, int y);
    void mouseMovement(int x, int y);
    void drawFPSCounter(int sum);

    int prepareDataset(short ** voxels, int sizeX, int sizeY, int sizeZ);
    void renderBitmapString(float x, float y, void *font, char *string);

    void updateScalarField();
    void histoPyramidConstruction();
    void histoPyramidTraversal(int sum);

    //char * getCLErrorString(cl_int error);

    void test();
    //void printError(std::string text = "");

public:
    // Define some globals
    GLuint VBO_ID;
    cl::Program program;
    cl::CommandQueue queue;
    cl::Context context;
    // bool writingTo3DTextures;
    bool extractSurfaceOnEveryFrame;
    bool extractSurface;

    int SIZE;
    int windowWidth, windowHeight;
    int windowID;

    cl::Image3D rawData;
    cl::Image3D cubeIndexesImage;
    cl::Buffer cubeIndexesBuffer;
    cl::Kernel constructHPLevelKernel;
    cl::Kernel constructHPLevelCharCharKernel;
    cl::Kernel constructHPLevelCharShortKernel;
    cl::Kernel constructHPLevelShortShortKernel;
    cl::Kernel constructHPLevelShortIntKernel;
    cl::Kernel classifyCubesKernel;
    cl::Kernel traverseHPKernel;
    std::vector<cl::Image3D> images;
    std::vector<cl::Buffer> buffers;

    MCSizef scalingFactor;
    MCSizef translation;

    float camX, camY, camZ;// = 4.0f; //X, Y, and Z
    float lastx, lasty, xrot, yrot, xrotrad, yrotrad; //Last pos and rotation
    float speed;// = 0.1f; //Movement speed

    int frame;// = 0;
    int timebase;// = 0;
    char s[100];
    int previousTime;// = 0;
    
    cl::size_t<3> origin;
    cl::size_t<3> region;
    int totalSum;
    cl::BufferGL *VBOBuffer;
public:
    short* voxels;
    int isolevel;
    GLuint test_handle;
    VSP<vtkActor> m_isoactor;
    VSP<vtkRenderWindow> m_render_window;
    VSP<vtkCellArray> m_polys;
    VSP<vtkRenderer> m_renderer;
    VSP<vtkRenderWindowInteractor> m_iren;

    VSP<vtkFloatArray> m_pcoords;
    VSP<vtkPoints> m_points;
    VSP<vtkPolyData> m_polydata;
    VSP<vtkPolyDataMapper> m_mapper;

    cl_float* test_buffer;
    int buffer_size;

public:
    void setupVTK();
};



#endif
