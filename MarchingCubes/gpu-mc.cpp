﻿#include <vtkAutoInit.h> 
VTK_MODULE_INIT(vtkRenderingOpenGL2)
#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkOutputWindow.h"
#include "vtkFileOutputWindow.h"
#include "Windows.h"
#include "Wingdi.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkPolyData.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkCallbackCommand.h"

#include "gpu-mc.hpp"
#include "VolumeData.h"
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif



#include <sstream>

template <class T>
inline std::string to_string(const T& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}


// Some functions missing in windows
inline double log2(double x) {
    return log(x) / log(2.0);
}

inline double round(double d) {
    return floor(d + 0.5);
}

MarchingCubes::MarchingCubes(VolumeData* v, int isolevel_) {
    voxels = v->data;
    isolevel = isolevel_;
    VBO_ID = 0;
    test_handle = 0;
    camZ = 4.0f;
    speed = 0.1f;
    frame = 0;
    timebase = 0;
    previousTime = 0;
    totalSum = 0;
    xrot = 0;
    yrot = 0;
    test_buffer = NULL;
    buffer_size = 0;
    m_pcoords = VSP<vtkFloatArray>::New();
    m_points = VSP<vtkPoints>::New();
    m_polydata = VSP<vtkPolyData>::New();
    m_mapper = VSP<vtkPolyDataMapper>::New();


    // size: 2的n次方，将数据变成一个正方体，那个正方体的边长
    int size = prepareDataset(&voxels, v->nx, v->ny, v->nz);

    setupVTK();

    setupOpenGL(size, v->nx, v->ny, v->nz, v->dx, v->dy, v->dz);

    setupOpenCL(voxels, size);
}

MarchingCubes::~MarchingCubes() {
    //!!!!!!
    m_polys->Initialize();
}


MarchingCubes* currentInstance;

void renderSceneCallback() {
    currentInstance->renderScene();
}
void idleCallback() {
    currentInstance->idle();
}
void reshapeCallback(int width, int height) {
    currentInstance->reshape(width, height);
}
void keyboardCallback(unsigned char key, int x, int y) {
    currentInstance->keyboard(key, x, y);
}
void mouseMovementCallback(int x, int y) {
    currentInstance->mouseMovement(x, y);
}


void callbackKeydown(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata) {
    MarchingCubes* mc = (MarchingCubes*)clientdata;
    mc->isolevel += 50;
    std::cout << mc->isolevel << std::endl;
    mc->extractSurface = true;
    //glutPostRedisplay();
    mc->renderScene();
}



void MarchingCubes::setupVTK() {
    VSP<vtkFileOutputWindow> w = VSP<vtkFileOutputWindow>::New();
    w->SetFileName("vtk_errors.txt");
    vtkOutputWindow::SetInstance(w);

    m_render_window = VSP<vtkRenderWindow>::New();
    m_renderer = VSP<vtkRenderer>::New();

    m_render_window->AddRenderer(m_renderer);
    m_iren = VSP<vtkRenderWindowInteractor>::New();
    m_iren->SetRenderWindow(m_render_window);
    VSP<vtkInteractorStyleTrackballCamera> t = VSP<vtkInteractorStyleTrackballCamera>::New();
    VSP<vtkCallbackCommand> kd = VSP<vtkCallbackCommand>::New();
    kd->SetClientData(this);
    kd->SetCallback(callbackKeydown);
    t->AddObserver(vtkCommand::KeyPressEvent, kd);
    m_iren->SetInteractorStyle(t);



    m_isoactor = VSP<vtkActor>::New();
    m_renderer->AddActor(m_isoactor);


    m_renderer->SetBackground(0.1, 0.2, 0.4);
    m_render_window->SetSize(600, 600);

    m_polys = VSP<vtkCellArray>::New();


}

void MarchingCubes::mouseMovement(int x, int y) {
    int cx = windowWidth / 2;
    int cy = windowHeight / 2;

    if (x == cx && y == cy) { //The if cursor is in the middle
        return;
    }

    int diffx = x - cx; //check the difference between the current x and the last x position
    int diffy = y - cy; //check the difference between the current y and the last y position
    xrot += (float)diffy / 2; //set the xrot to xrot with the addition of the difference in the y position
    yrot += (float)diffx / 2;// set the xrot to yrot with the addition of the difference in the x position
    glutWarpPointer(cx, cy); //Bring the cursor to the middle
}

void MarchingCubes::renderBitmapString(float x, float y, void *font, char *string) {
    char *c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}


void MarchingCubes::drawFPSCounter(int sum) {
    frame++;

    int time = glutGet(GLUT_ELAPSED_TIME);
    if (time - timebase > 100) { // 1 times per second
        sprintf_s(s, "Triangles: %d FPS: %4.2f. Speed: %d ms. Isovalue: %4.3f", sum, frame*1000.0 / (time - timebase), (int)round(time - previousTime), (float)isolevel);
        timebase = time;
        frame = 0;
    }

    previousTime = time;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    renderBitmapString(-0.99f, 0.95f, GLUT_BITMAP_8_BY_13, s);
    renderBitmapString(-0.99f, 0.9f, GLUT_BITMAP_8_BY_13, "+/-: Change isovalue. W,A,S,D: Move object. Mouse: Rotate object.");
    if (extractSurfaceOnEveryFrame) {
        renderBitmapString(-0.99f, 0.85, GLUT_BITMAP_8_BY_13, "Extracting surfaces on every frame. Press 'e' to change.");
    }
    else {
        renderBitmapString(-0.99f, 0.85f, GLUT_BITMAP_8_BY_13, "Extracting surfaces only on isovalue change. Press 'e' to change.");
    }
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

void MarchingCubes::idle() {
    glutPostRedisplay();
}

void MarchingCubes::reshape(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.5f, 10.0f);
}

void MarchingCubes::renderScene() {
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glutSetWindow(windowID);

    if (extractSurfaceOnEveryFrame || extractSurface) {
        glDeleteBuffers(1, &VBO_ID);

        if (VBOBuffer != NULL && totalSum > 0) {
            delete VBOBuffer;
        }
        histoPyramidConstruction();

        // Read top of histoPyramid an use this size to allocate VBO below
        int sum[8];

        queue.enqueueReadBuffer(buffers[buffers.size() - 1], CL_FALSE, 0, sizeof(int) * 8, sum);

        queue.finish();
        totalSum = sum[0] + sum[1] + sum[2] + sum[3] + sum[4] + sum[5] + sum[6] + sum[7];

        if (totalSum == 0) {
            std::cout << "No triangles were extracted. Check isovalue." << std::endl;
            extractSurface = false;
            return;
        }

        // Create new VBO

        /*
        VBO_ID: opengl的缓存编号
        VBOBuffer: opencl的缓存
        */
        glGenBuffers(1, &VBO_ID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
        glBufferData(GL_ARRAY_BUFFER, totalSum * 18 * sizeof(cl_float), NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Traverse the histoPyramid and fill VBO
        histoPyramidTraversal(totalSum);
        queue.flush();
        buffer_size = totalSum * 18 * sizeof(cl_float);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
        glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
        glGetBufferPointerv(GL_ARRAY_BUFFER, GL_BUFFER_MAP_POINTER, (void**)&test_buffer);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        test();

    }
    /*
    // Render VBO
    reshape(windowWidth, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(-camX, -camY, -camZ);

    glRotatef(xrot, 1.0, 0.0, 0.0);
    glRotatef(yrot, 0.0, 1.0, 0.0);

    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glScalef(scalingFactor.x, scalingFactor.y, scalingFactor.z); // spacing在这里起作用
    glTranslatef(translation.x, translation.y, translation.z);

    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    // Normal Buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    //glBindBuffer(GL_ARRAY_BUFFER, test_handle);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, 24, BUFFER_OFFSET(0));
    glNormalPointer(GL_FLOAT, 24, BUFFER_OFFSET(12));

    if (extractSurfaceOnEveryFrame || extractSurface)
        queue.finish();
    //glWaitSync(traversalSync, 0, GL_TIMEOUT_IGNORED);
    glDrawArrays(GL_TRIANGLES, 0, totalSum * 3);

    // Release buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glPopMatrix();
    */
    //// Render text
    //glPushMatrix();
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //glColor3f(1.0f, 1.0f, 0.0f);
    //drawFPSCounter(totalSum);
    //glPopMatrix();
    //
    //glutSwapBuffers();

    extractSurface = false;
    //renWin->Render();

}

void MarchingCubes::run() {
    glutMainLoop();
}

void MarchingCubes::setupOpenGL(int size, int sizeX, int sizeY, int sizeZ, float spacingX, float spacingY, float spacingZ) {
    currentInstance = this; // see http://stackoverflow.com/questions/3589422/using-opengl-glutdisplayfunc-within-class

    /* Initialize GLUT */
    //glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    //glutInitWindowPosition(0, 0);
    //glutInitWindowSize(800, 800);
    windowID = glutCreateWindow("GPU Marching Cubes");
    //glutDisplayFunc(renderSceneCallback);
    //glutIdleFunc(idleCallback);
    //glutReshapeFunc(reshapeCallback);
    //glutKeyboardFunc(keyboardCallback);
    //glutMotionFunc(mouseMovementCallback);

    glutSetWindow(windowID);

    glewInit();
    //glEnable(GL_NORMALIZE);
    //glEnable(GL_DEPTH_TEST);
    //glShadeModel(GL_SMOOTH);
    //glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHTING);

    //// Set material properties which will be assigned by glColor
    //GLfloat color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    //GLfloat specReflection[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specReflection);
    //GLfloat shininess[] = { 16.0f };
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    //// Create light components
    //GLfloat ambientLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    //GLfloat diffuseLight[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    //GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //GLfloat position[] = { -0.0f, 4.0f, 1.0f, 1.0f };

    //// Assign created components to GL_LIGHT0
    //glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    //glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    //glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    //glLightfv(GL_LIGHT0, GL_POSITION, position);

    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = 2;
    region[1] = 2;
    region[2] = 2;
    scalingFactor.x = spacingX*1.5f / size;
    scalingFactor.y = spacingY*1.5f / size;
    scalingFactor.z = spacingZ*1.5f / size;

    translation.x = (float)sizeX / 2.0f;
    translation.y = -(float)sizeY / 2.0f;
    translation.z = -(float)sizeZ / 2.0f;

    extractSurface = true;
    extractSurfaceOnEveryFrame = false;

}

void MarchingCubes::keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case '+':
        isolevel += 50;
        if (!extractSurfaceOnEveryFrame)
            extractSurface = true;
        break;
    case '-':
        isolevel -= 50;
        if (!extractSurfaceOnEveryFrame)
            extractSurface = true;
        break;
        //WASD movement
    case 'w':
        camZ -= 0.1f;
        break;
    case 's':
        camZ += 0.1f;
        break;
    case 'a':
        camX -= 0.1f;
        break;
    case 'd':
        camX += 0.1f;
        break;
    case 'e':
        extractSurfaceOnEveryFrame = !extractSurfaceOnEveryFrame;
        break;
    case 27:
    case 'q':
        //TODO some clean up
        glutDestroyWindow(windowID);
        break;
    }
}

int MarchingCubes::prepareDataset(short ** voxels, int sizeX, int sizeY, int sizeZ) {
    // If all equal and power of two exit
    if (sizeX == sizeY && sizeY == sizeZ && sizeX == pow(2, log2(sizeX)))
        return sizeX;

    // Find largest size and find closest power of two
    int largestSize = max(sizeX, max(sizeY, sizeZ));
    int size = 0;
    int i = 1;
    while (1 << i < largestSize)
        i++;
    size = 1 << i;

    // Make new voxel array of this size and fill it with zeros
    short * newVoxels = new short[size*size*size];
    for (int j = 0; j < size*size*size; j++)
        newVoxels[j] = 0;

    // Fill the voxel array with previous data
    for (int x = 0; x < sizeX; x++) {
        for (int y = 0; y < sizeY; y++) {
            for (int z = 0; z < sizeZ; z++) {
                newVoxels[x + y*size + z*size*size] = voxels[0][x + y*sizeX + z*sizeX*sizeY];
            }
        }
    }
    delete[] voxels[0];
    voxels[0] = newVoxels;
    return size;
}



void MarchingCubes::setupOpenCL(short * voxels, int size) {
    SIZE = size;
    try {
        // Create a context that use a GPU and OpenGL interop.
        context = createCLGLContext(CL_DEVICE_TYPE_GPU, VENDOR_ANY);

        // Get a list of devices on this platform
        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        // Create a command queue and use the first device
        queue = cl::CommandQueue(context, devices[0]);

        // Check if writing to 3D textures are supported
        std::string sourceFilename;

        sourceFilename = "gpu-mc-morton.cl";

        // Read source file
        std::ifstream sourceFile(sourceFilename.c_str());
        if (sourceFile.fail()) {
            std::cout << "Failed to open OpenCL source file" << std::endl;
            exit(-1);
        }
        std::string sourceCode(
            std::istreambuf_iterator<char>(sourceFile),
            (std::istreambuf_iterator<char>()));

        // Insert size
        cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));

        // Make program of the source code in the context
        program = cl::Program(context, source);

        // Build program for these specific devices
        try {
            std::string buildOptions = "-DSIZE=" + to_string(SIZE);
            program.build(devices, buildOptions.c_str());
        }
        catch (cl::Error error) {
            if (error.err() == CL_BUILD_PROGRAM_FAILURE) {
                std::cout << "Build log:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
            }
            throw error;
        }


        int bufferSize = SIZE*SIZE*SIZE;
        buffers.push_back(cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*bufferSize));
        bufferSize /= 8;
        buffers.push_back(cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*bufferSize));
        bufferSize /= 8;
        buffers.push_back(cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(short)*bufferSize));
        bufferSize /= 8;
        buffers.push_back(cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(short)*bufferSize));
        bufferSize /= 8;
        buffers.push_back(cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(short)*bufferSize));
        bufferSize /= 8;
        for (int i = 5; i < log2((float)SIZE); i++) {
            buffers.push_back(cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int)*bufferSize));
            bufferSize /= 8;
        }

        cubeIndexesBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(char)*SIZE*SIZE*SIZE);
        cubeIndexesImage = cl::Image3D(context, CL_MEM_READ_ONLY,
            cl::ImageFormat(CL_R, CL_UNSIGNED_INT8),
            SIZE, SIZE, SIZE);


        // Transfer dataset to device
        rawData = cl::Image3D(
            context,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            cl::ImageFormat(CL_R, CL_SIGNED_INT16),
            SIZE, SIZE, SIZE,
            0, 0, voxels
            );
        delete[] voxels;

        // Make kernels
        constructHPLevelKernel = cl::Kernel(program, "constructHPLevel");
        classifyCubesKernel = cl::Kernel(program, "classifyCubes");
        traverseHPKernel = cl::Kernel(program, "traverseHP");

        constructHPLevelCharCharKernel = cl::Kernel(program, "constructHPLevelCharChar");
        constructHPLevelCharShortKernel = cl::Kernel(program, "constructHPLevelCharShort");
        constructHPLevelShortShortKernel = cl::Kernel(program, "constructHPLevelShortShort");
        constructHPLevelShortIntKernel = cl::Kernel(program, "constructHPLevelShortInt");

    }
    catch (cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        std::cout << getCLErrorString(error.err()) << std::endl;
    }
}

void MarchingCubes::histoPyramidConstruction() {

    updateScalarField();

    // Run base to first level
    constructHPLevelCharCharKernel.setArg(0, buffers[0]);
    constructHPLevelCharCharKernel.setArg(1, buffers[1]);

    queue.enqueueNDRangeKernel(
        constructHPLevelCharCharKernel,
        cl::NullRange,
        cl::NDRange(SIZE / 2, SIZE / 2, SIZE / 2),
        cl::NullRange
        );

    int previous = SIZE / 2;

    constructHPLevelCharShortKernel.setArg(0, buffers[1]);
    constructHPLevelCharShortKernel.setArg(1, buffers[2]);

    queue.enqueueNDRangeKernel(
        constructHPLevelCharShortKernel,
        cl::NullRange,
        cl::NDRange(previous / 2, previous / 2, previous / 2),
        cl::NullRange
        );

    previous /= 2;

    constructHPLevelShortShortKernel.setArg(0, buffers[2]);
    constructHPLevelShortShortKernel.setArg(1, buffers[3]);

    queue.enqueueNDRangeKernel(
        constructHPLevelShortShortKernel,
        cl::NullRange,
        cl::NDRange(previous / 2, previous / 2, previous / 2),
        cl::NullRange
        );

    previous /= 2;

    constructHPLevelShortShortKernel.setArg(0, buffers[3]);
    constructHPLevelShortShortKernel.setArg(1, buffers[4]);

    queue.enqueueNDRangeKernel(
        constructHPLevelShortShortKernel,
        cl::NullRange,
        cl::NDRange(previous / 2, previous / 2, previous / 2),
        cl::NullRange
        );

    previous /= 2;

    constructHPLevelShortIntKernel.setArg(0, buffers[4]);
    constructHPLevelShortIntKernel.setArg(1, buffers[5]);

    queue.enqueueNDRangeKernel(
        constructHPLevelShortIntKernel,
        cl::NullRange,
        cl::NDRange(previous / 2, previous / 2, previous / 2),
        cl::NullRange
        );

    previous /= 2;

    // Run level 2 to top level
    for (int i = 5; i < log2(SIZE) - 1; i++) {
        constructHPLevelKernel.setArg(0, buffers[i]);
        constructHPLevelKernel.setArg(1, buffers[i + 1]);
        previous /= 2;
        queue.enqueueNDRangeKernel(
            constructHPLevelKernel,
            cl::NullRange,
            cl::NDRange(previous, previous, previous),
            cl::NullRange
            );
    }

}

void MarchingCubes::updateScalarField() {

    classifyCubesKernel.setArg(0, buffers[0]);
    classifyCubesKernel.setArg(1, cubeIndexesBuffer);
    classifyCubesKernel.setArg(2, rawData);
    classifyCubesKernel.setArg(3, isolevel);
    queue.enqueueNDRangeKernel(
        classifyCubesKernel,
        cl::NullRange,
        cl::NDRange(SIZE, SIZE, SIZE),
        cl::NullRange
        );

    cl::size_t<3> offset;
    offset[0] = 0;
    offset[1] = 0;
    offset[2] = 0;
    cl::size_t<3> region;
    region[0] = SIZE;
    region[1] = SIZE;
    region[2] = SIZE;

    // Copy buffer to image
    queue.enqueueCopyBufferToImage(cubeIndexesBuffer, cubeIndexesImage, 0, offset, region);
}

void MarchingCubes::histoPyramidTraversal(int sum) {
    // Make OpenCL buffer from OpenGL buffer
    unsigned int i = 0;

    traverseHPKernel.setArg(0, rawData);
    traverseHPKernel.setArg(1, cubeIndexesImage);
    for (i = 0; i < buffers.size(); i++) {
        traverseHPKernel.setArg(i + 2, buffers[i]);
    }
    i += 2;

    VBOBuffer = new cl::BufferGL(context, CL_MEM_WRITE_ONLY, VBO_ID); // bug here! ---fixed
    //VBOBuffer = new cl::BufferGL(context, CL_MEM_WRITE_ONLY, test_handle); // bug here! ---fixed
    //VBOBuffer = new cl::BufferGL(*test_buffer);


    traverseHPKernel.setArg(i, *VBOBuffer);
    traverseHPKernel.setArg(i + 1, isolevel);
    traverseHPKernel.setArg(i + 2, sum);
    //cl_event syncEvent = clCreateEventFromGLsyncKHR((cl_context)context(), (cl_GLsync)glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0), 0);
    //glFinish();
    std::vector<cl::Memory> v;
    v.push_back(*VBOBuffer);
    //std::vector<Event> events;
    //Event e;
    //events.push_back(Event(syncEvent));
    queue.enqueueAcquireGLObjects(&v);

    // Increase the global_work_size so that it is divideable by 64
    int global_work_size = sum + 64 - (sum - 64 * (sum / 64));
    // Run a NDRange kernel over this buffer which traverses back to the base level
    queue.enqueueNDRangeKernel(traverseHPKernel, cl::NullRange, cl::NDRange(global_work_size), cl::NDRange(64));

    cl::Event traversalEvent;
    queue.enqueueReleaseGLObjects(&v, 0, &traversalEvent);
    //	traversalSync = glCreateSyncFromCLeventARB((cl_context)context(), (cl_event)traversalEvent(), 0); // Need the GL_ARB_cl_event extension
    queue.flush();
}

bool tmp = true;

void MarchingCubes::test() {
    //VSP<vtkFloatArray> pcoords = VSP<vtkFloatArray>::New();
    vtkFloatArray* pcoords = vtkFloatArray::New();
    pcoords->SetNumberOfComponents(3);
    pcoords->SetArray(test_buffer, totalSum * 18, true);


    //VSP<vtkPoints> points = VSP<vtkPoints>::New();
    vtkPoints* points = vtkPoints::New();
    points->SetData(pcoords);

    //m_polys->Initialize();
    vtkCellArray* polys = vtkCellArray::New();
    for (int i = 0; i < totalSum; i++) {
        polys->InsertNextCell(3);
        polys->InsertCellPoint(i * 6);
        polys->InsertCellPoint(i * 6 + 2);
        polys->InsertCellPoint(i * 6 + 4);
    }

    //VSP<vtkPolyData> polydata = VSP<vtkPolyData>::New();
    m_polydata->SetPoints(points);
    m_polydata->SetPolys(polys);
    points->Delete();
    polys->Delete();
    pcoords->Delete();
    //VSP<vtkPolyDataMapper> mapper = VSP<vtkPolyDataMapper>::New();
    m_mapper->SetInputData(m_polydata);
    m_isoactor->SetMapper(m_mapper);
    m_render_window->Render();

}

