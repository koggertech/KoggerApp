#ifndef DSP_H
#define DSP_H

#include "stdint.h"
#include "math.h"

typedef struct __attribute__((packed)) Complex16 {
    Complex16() {
        real = 0;
        imag = 0;
    }
    Complex16(int16_t r, int16_t i) {
        real = r;
        imag = i;
    }

    float logPow() {
        return 10.0f * log10(real*real + imag*imag);
    }

    float arg() {
        return std::atan2(imag, real);
    }

    Complex16 mulConj(Complex16 second) {
        Complex16 res;
        res.real = real*second.real + imag*second.imag;
        res.imag = -real*second.imag + imag*second.real;
        return res;
    }

    Complex16 getBitOffsetedRight(uint32_t offset) {
        Complex16 res;
        res.real = real >> offset;
        res.imag = imag >> offset;
        return res;
    }

    int16_t real = 0;
    int16_t imag = 0;
} Complex16;

#endif // DSP_H
