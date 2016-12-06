#ifndef NII_H
#define NII_H
#include <typeinfo.h>
#include "nifti1_io.h"
#include <string>
#include <iostream>
#pragma comment(lib, "niftiio.lib")
#pragma comment(lib, "znz.lib")

class Nii {
public:
    Nii() {
        this->n_img = NULL;
    }
    ~Nii() {
        niiFreeImage();
        std::cout << "> nii free" << std::endl;
    }
    void niiReadImage(std::string filepath) {
        this->n_img = nifti_image_read(filepath.c_str(), true);
    }
    /*
    \param   filepath_prefix: path without extensions
    */
    void niiWriteImage(std::string filepath_prefix) {
        nifti_set_filenames(this->n_img, filepath_prefix.c_str(), false, false);
        nifti_image_write(this->n_img);
    }

    /*
    \fn         write an image with same info with current image
    \param      filepath_prefix: path without extensions
    \param      data: data
    */
    /*void niiWriteImage(std::string filepath_prefix, void* data) {
    void* tmp = this->n_img->data;
    this->n_img->data = data;
    niiWriteImage(filepath_prefix);
    this->n_img->data = tmp;
    }*/

    void niiPrintInfo() {
        std::cout << "nx:\t" << this->n_img->nx << "\tdimensions of grid array" << std::endl;
        std::cout << "ny:\t" << this->n_img->ny << std::endl;
        std::cout << "nz:\t" << this->n_img->nz << std::endl;
        std::cout << "ndim:\t" << this->n_img->ndim << std::endl;
        std::cout << "dx:\t" << this->n_img->dx << "\tgrid spacings" << std::endl;
        std::cout << "dy:\t" << this->n_img->dy << std::endl;
        std::cout << "dz:\t" << this->n_img->dz << std::endl;
        std::cout << "nvox:\t" << this->n_img->nvox << "\tnumber of voxels = nx*ny*nz*...*nw" << std::endl;
        std::cout << "nbyper:\t" << this->n_img->nbyper << "\tbytes per voxel, matches datatype" << std::endl;
        std::cout << "datatype:\t" << this->n_img->datatype << ", " << typeid(this->n_img->datatype).name() << "\ttype of data in voxels: DT_* code" << std::endl;
        //std::cout<<"intent_name:\t"<<this->n_img->intent_name<<"\toptional description of intent data"<<std::endl;
        //std::cout<<"descrip:\t"<<this->n_img->descrip<<"\toptional text to describe dataset"<<std::endl;
        //std::cout<<"auxfile:\t"<<this->n_img->aux_file<<"\tauxiliary filename"<<std::endl;
    }

    void niiFreeImage() {
        nifti_image_free(this->n_img);
        //delete this->n_img;
        //this->n_img = NULL;
    }
public:
    nifti_image* n_img;
};


#endif



#ifdef here_are_some_comments
typedef struct {                /*!< Image storage struct **/

    int ndim;                    /*!< last dimension greater than 1 (1..7) */
    int nx;                      /*!< dimensions of grid array             */
    int ny;                      /*!< dimensions of grid array             */
    int nz;                      /*!< dimensions of grid array             */
    int nt;                      /*!< dimensions of grid array             */
    int nu;                      /*!< dimensions of grid array             */
    int nv;                      /*!< dimensions of grid array             */
    int nw;                      /*!< dimensions of grid array             */
    int dim[8];                  /*!< dim[0]=ndim, dim[1]=nx, etc.         */
    size_t nvox;                    /*!< number of voxels = nx*ny*nz*...*nw   */
    int nbyper;                  /*!< bytes per voxel, matches datatype    */
    int datatype;                /*!< type of data in voxels: DT_* code    */

    float dx;                    /*!< grid spacings      */
    float dy;                    /*!< grid spacings      */
    float dz;                    /*!< grid spacings      */
    float dt;                    /*!< grid spacings      */
    float du;                    /*!< grid spacings      */
    float dv;                    /*!< grid spacings      */
    float dw;                    /*!< grid spacings      */
    float pixdim[8];             /*!< pixdim[1]=dx, etc. */

    float scl_slope;             /*!< scaling parameter - slope        */
    float scl_inter;             /*!< scaling parameter - intercept    */

    float cal_min;               /*!< calibration parameter, minimum   */
    float cal_max;               /*!< calibration parameter, maximum   */

    int qform_code;              /*!< codes for (x,y,z) space meaning  */
    int sform_code;              /*!< codes for (x,y,z) space meaning  */

    int freq_dim;               /*!< indexes (1,2,3, or 0) for MRI    */
    int phase_dim;               /*!< directions in dim[]/pixdim[]     */
    int slice_dim;               /*!< directions in dim[]/pixdim[]     */

    int   slice_code;           /*!< code for slice timing pattern    */
    int   slice_start;           /*!< index for start of slices        */
    int   slice_end;           /*!< index for end of slices          */
    float slice_duration;        /*!< time between individual slices   */

                                 /*! quaternion transform parameters
                                 [when writing a dataset, these are used for qform, NOT qto_xyz]   */
    float quatern_b, quatern_c, quatern_d,
        qoffset_x, qoffset_y, qoffset_z,
        qfac;

    mat44 qto_xyz;               /*!< qform: transform (i,j,k) to (x,y,z) */
    mat44 qto_ijk;               /*!< qform: transform (x,y,z) to (i,j,k) */

    mat44 sto_xyz;               /*!< sform: transform (i,j,k) to (x,y,z) */
    mat44 sto_ijk;               /*!< sform: transform (x,y,z) to (i,j,k) */

    float toffset;               /*!< time coordinate offset */

    int xyz_units;              /*!< dx,dy,dz units: NIFTI_UNITS_* code  */
    int time_units;              /*!< dt       units: NIFTI_UNITS_* code  */

    int nifti_type;              /*!< 0==ANALYZE, 1==NIFTI-1 (1 file),
                                 2==NIFTI-1 (2 files),
                                 3==NIFTI-ASCII (1 file) */
    int   intent_code;           /*!< statistic type (or something)       */
    float intent_p1;             /*!< intent parameters                   */
    float intent_p2;             /*!< intent parameters                   */
    float intent_p3;             /*!< intent parameters                   */
    char  intent_name[16];       /*!< optional description of intent data */

    char descrip[80];           /*!< optional text to describe dataset   */
    char aux_file[24];           /*!< auxiliary filename                  */

    char *fname;                 /*!< header filename (.hdr or .nii)         */
    char *iname;                 /*!< image filename  (.img or .nii)         */
    int   iname_offset;          /*!< offset into iname where data starts    */
    int   swapsize;              /*!< swap unit in image data (might be 0)   */
    int   byteorder;             /*!< byte order on disk (MSB_ or LSB_FIRST) */
    void *data;                  /*!< pointer to data: nbyper*nvox bytes     */

    int                num_ext;  /*!< number of extensions in ext_list       */
    nifti1_extension * ext_list; /*!< array of extension structs (with data) */
    analyze_75_orient_code analyze75_orient; /*!< for old analyze files, orient */

} nifti_image;
#endif
