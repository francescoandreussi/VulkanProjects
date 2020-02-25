#define cimg_display 0
#define cimg_use_tif

#define WIDTH 1080
#define HEIGHT 1080

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <math.h>
#include <CImg.h>
#include <ImageMagick-7/Magick++.h>

using namespace cimg_library;

class IdentityUV {
private:
    /* data */
public:
    IdentityUV(/* args */);
    ~IdentityUV();

    static void generate();
};

void IdentityUV::generate(){
    CImg<u_short> uvMS(WIDTH, HEIGHT, 1, 3);
    CImg<u_short> uvLS(WIDTH, HEIGHT, 1, 3);
    u_int val = 0;
    float u, v;
    int int_u, int_v;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if ( ((j-540)*(j-540) + (i-540)*(i-540)) < (540*540) ) {
                u = float(j) / float(WIDTH);
                v = float(i) / float(HEIGHT);
                int_u = u * 65535;
                int_v = v * 65535;
                uvMS(j, i, 0, 0) = static_cast<uint8_t>(int_u >> 8); // u
                uvMS(j, i, 0, 1) = static_cast<uint8_t>(int_v >> 8); // v
                uvMS(j, i, 0, 2) = 255; // intensity (= 1)
                uvLS(j, i, 0, 0) = static_cast<uint8_t>((int_u << 8) >> 8);
                uvLS(j, i, 0, 1) = static_cast<uint8_t>((int_v << 8) >> 8);
                uvLS(j, i, 0, 2) = 255;

            } else {
                uvLS(j, i, 0, 0) = 0;
                uvLS(j, i, 0, 1) = 0;
                uvLS(j, i, 0, 2) = 0;
                uvMS(j, i, 0, 0) = 0;
                uvMS(j, i, 0, 1) = 0;
                uvMS(j, i, 0, 2) = 0;
            }
            //std::cout << i << " " << j << " " << colour << std::endl;
        }
    }

    uvLS.save("identityUVLS.png");
    uvMS.save("identityUVMS.png");
}

int main(int argc, char* argv[]){
    IdentityUV::generate();

    return 0;
}
