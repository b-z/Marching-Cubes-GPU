#include "Volumedata.h"
#include <cstring>
#include <iostream>
VolumeData::VolumeData() {
    this->data = NULL;
}

VolumeData::~VolumeData() {
    std::cout << "<<<<<<<<<<<<<<<Volume Data Released!<<<<<<<<<<<<<<<<<\n";
}

bool VolumeData::fromNii(Nii* nii) {
    this->nvox = nii->n_img->nvox;
    if (!(nii->n_img->datatype == 4 && nii->n_img->nbyper == 2)) {
        return false;
    }
    this->datalen = sizeof(short) * this->nvox;


    this->data = (short*)malloc(datalen);
    memcpy(this->data, nii->n_img->data, datalen);
    this->datatype = "short";

    this->nx = nii->n_img->nx;
    this->ny = nii->n_img->ny;
    this->nz = nii->n_img->nz;
    this->dx = nii->n_img->dx;
    this->dy = nii->n_img->dy;
    this->dz = nii->n_img->dz;

    return true;
}

int VolumeData::idx(int x, int y, int z) {
#ifdef DEBUG_DIRECTION
    if (x == 1 + nx / 3) return 0; // debug
    if (y == 1 + ny / 3) return 0; // debug
    if (z == 1 + nz / 3) return 0; // debug
#endif
    return x + y * nx + (nz - z - 1) * nx * ny;
}

short VolumeData::at(int x, int y, int z) {
    return this->data[idx(x, y, z)];
}

void VolumeData::set(int x, int y, int z, short new_data) {
    this->data[idx(x, y, z)] = new_data;
}

//
//ImageData* VolumeData::sliceAt(int pos, SliceType slice_type) {
//    ImageData* image;
//    switch (slice_type) {
//    case SLICE_SAGITTAL:
//        image = new ImageData(this->ny, this->nz);
//        for (int i = 0; i < this->ny; i++) {
//            for (int j = 0; j < this->nz; j++) {
//                image->set(i, j, this->at(pos, i, j));
//            }
//        }
//        image->setSpacing(dy, dz);
//        break;
//    case SLICE_CORONAL:
//        image = new ImageData(this->nx, this->nz);
//        for (int i = 0; i < this->nx; i++) {
//            for (int j = 0; j < this->nz; j++) {
//                image->set(i, j, this->at(i, pos, j));
//            }
//        }
//        image->setSpacing(dx, dz);
//        break;
//    case SLICE_AXIAL:
//        image = new ImageData(this->nx, this->ny);
//        for (int i = 0; i < this->nx; i++) {
//            for (int j = 0; j < this->ny; j++) {
//                image->set(i, j, this->at(i, j, pos));
//            }
//        }
//        image->setSpacing(dx, dy);
//        break;
//    }
//    return image;
//}

void VolumeData::release() {
    delete this->data;
    this->data = NULL;
    std::cout << "> delete data" << std::endl;
}