/*
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EIDSP_IMAGE_PROCESSING__H__
#define __EIDSP_IMAGE_PROCESSING__H__

#include "edge-impulse-sdk/dsp/ei_utils.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/dsp/returntypes.hpp"
#include "edge-impulse-sdk/dsp/image/processing.hpp"

namespace ei { namespace image { namespace processing {

enum YUV_OPTIONS
{
    BIG_ENDIAN_ORDER = 1, //RGB reading from low to high memory.  Otherwise, uses native encoding
    PAD_4B = 2, // pad 0x00 on the high B. ie 0x00RRGGBB
};

/**
 * @brief Convert YUV to RGB
 *
 * @param rgb_out Output buffer (can be the same as yuv_in if big enough)
 * @param yuv_in Input buffer
 * @param in_size_B Size of input image in B
 * @param opts Note, only BIG_ENDIAN_ORDER supported presently
 */
int yuv422_to_rgb888(
    unsigned char *rgb_out,
    unsigned const char *yuv_in,
    unsigned int in_size_B,
    YUV_OPTIONS opts)
{

    // Clamp out of range values
    #define EI_CLAMP(t) (((t) > 255) ? 255 : (((t) < 0) ? 0 : (t)))

    // Color space conversion for RGB
    #define EI_GET_R_FROM_YUV(y, u, v) ((298 * y + 409 * v + 128) >> 8)
    #define EI_GET_G_FROM_YUV(y, u, v) ((298 * y - 100 * u - 208 * v + 128) >> 8)
    #define EI_GET_B_FROM_YUV(y, u, v) ((298 * y + 516 * u + 128) >> 8)

    unsigned int in_size_pixels = in_size_B / 4;
    yuv_in += in_size_B - 1;

    int rgb_end = TEST_BIT_MASK(opts, PAD_4B) ? 2 * in_size_B : (6 * in_size_B) / 4;
    rgb_out += rgb_end - 1;

    // Going backwards probably looks strange, but
    // This allows us to do the algorithm in place!
    // User needs to put the YUV image into a larger buffer than necessary
    // But going backwards means we don't overwrite the YUV bytes
    //  until we don't need them anymore
    for (unsigned int i = 0; i < in_size_pixels; ++i) {
        int y2 = *yuv_in-- - 16;
        int v = *yuv_in-- - 128;
        int y0 = *yuv_in-- - 16;
        int u0 = *yuv_in-- - 128;

        if (TEST_BIT_MASK(opts, BIG_ENDIAN_ORDER)) {
            *rgb_out-- = EI_CLAMP(EI_GET_B_FROM_YUV(y2, u0, v));
            *rgb_out-- = EI_CLAMP(EI_GET_G_FROM_YUV(y2, u0, v));
            *rgb_out-- = EI_CLAMP(EI_GET_R_FROM_YUV(y2, u0, v));
            if (TEST_BIT_MASK(opts, PAD_4B)) {
                *rgb_out-- = 0;
            }

            *rgb_out-- = EI_CLAMP(EI_GET_B_FROM_YUV(y0, u0, v));
            *rgb_out-- = EI_CLAMP(EI_GET_G_FROM_YUV(y0, u0, v));
            *rgb_out-- = EI_CLAMP(EI_GET_R_FROM_YUV(y0, u0, v));
            if (TEST_BIT_MASK(opts, PAD_4B)) {
                *rgb_out-- = 0;
            }
        }
        else {
            // not yet supported
            return EIDSP_NOT_SUPPORTED;
        }
    }
    return EIDSP_OK;
}

/**
 * @brief Crops an image. Can be in-place. 4B alignment for best performance
 * (Alignment is tested, will fall back to B by B movement)
 *
 * @param srcWidth X dimension in pixels
 * @param srcHeight Y dimension in pixels
 * @param srcImage Input buffer
 * @param startX X coord of first pixel to keep
 * @param startY Y coord of the first pixel to keep
 * @param dstWidth Desired X dimension in pixels (should be smaller than srcWidth)
 * @param dstHeight Desired Y dimension in pixels (should be smaller than srcHeight)
 * @param dstImage Output buffer, can be the same as srcImage
 * @param iBpp 8 or 16 for bits per pixel
 */
int cropImage(
    const uint8_t *srcImage,
    int srcWidth,
    int srcHeight,
    int startX,
    int startY,
    uint8_t *dstImage,
    int dstWidth,
    int dstHeight,
    int iBpp)
{
    uint32_t *s32, *d32;
    int x, y;

    if (startX < 0 || startX >= srcWidth || startY < 0 || startY >= srcHeight ||
        (startX + dstWidth) > srcWidth || (startY + dstHeight) > srcHeight) {
        return EIDSP_PARAMETER_INVALID; // invalid parameters
    }
    if (iBpp != 8 && iBpp != 16) {
        return EIDSP_PARAMETER_INVALID;
    }

    if (iBpp == 8) {
        const uint8_t *s;
        uint8_t *d;
        for (y = 0; y < dstHeight; y++) {
            s = &srcImage[srcWidth * (y + startY) + startX];
            d = &dstImage[(dstWidth * y)];
            x = 0;
            if ((intptr_t)s & 3 || (intptr_t)d & 3) { // either src or dst pointer is not aligned
                for (; x < dstWidth; x++) {
                    *d++ = *s++; // have to do it byte-by-byte
                }
            }
            else {
                // move 4 bytes at a time if aligned or alignment not enforced
                s32 = (uint32_t *)s;
                d32 = (uint32_t *)d;
                for (; x < dstWidth - 3; x += 4) {
                    *d32++ = *s32++;
                }
                // any remaining stragglers?
                s = (uint8_t *)s32;
                d = (uint8_t *)d32;
                for (; x < dstWidth; x++) {
                    *d++ = *s++;
                }
            }
        } // for y
    } // 8-bpp
    else {
        uint16_t *s, *d;
        for (y = 0; y < dstHeight; y++) {
            s = (uint16_t *)&srcImage[2 * srcWidth * (y + startY) + startX * 2];
            d = (uint16_t *)&dstImage[(dstWidth * y * 2)];
            x = 0;
            if ((intptr_t)s & 2 || (intptr_t)d & 2) { // either src or dst pointer is not aligned
                for (; x < dstWidth; x++) {
                    *d++ = *s++; // have to do it 16-bits at a time
                }
            }
            else {
                // move 4 bytes at a time if aligned or alignment no enforced
                s32 = (uint32_t *)s;
                d32 = (uint32_t *)d;
                for (; x < dstWidth - 1; x += 2) { // we can move 2 pixels at a time
                    *d32++ = *s32++;
                }
                // any remaining stragglers?
                s = (uint16_t *)s32;
                d = (uint16_t *)d32;
                for (; x < dstWidth; x++) {
                    *d++ = *s++;
                }
            }
        } // for y
    } // 16-bpp case

    return EIDSP_OK;
} /* cropImage() */

/**
 * @copydoc cropImage(
    int srcWidth,
    int srcHeight,
    const uint8_t *srcImage,
    int startX,
    int startY,
    int dstWidth,
    int dstHeight,
    uint8_t *dstImage,
    int iBpp)
 */
int crop_image_rgb888_packed(
    const uint8_t *srcImage,
    int srcWidth,
    int srcHeight,
    int startX,
    int startY,
    uint8_t *dstImage,
    int dstWidth,
    int dstHeight)
{
    // use 8 bpp mode, but do everything *3 for RGB
    return cropImage(
        srcImage,
        srcWidth * 3,
        srcHeight,
        startX * 3,
        startY,
        dstImage,
        dstWidth * 3,
        dstHeight,
        8);
}

/**
 * @brief Resize an image using interpolation
 * Can be used to resize the image smaller or larger
 * If resizing much smaller than 1/3 size, then a more rubust algorithm should average all of the pixels
 * This algorithm uses bilinear interpolation - averages a 2x2 region to generate each new pixel
 *
 * @param srcWidth Input image width in pixels
 * @param srcHeight Input image height in pixels
 * @param srcImage Input buffer
 * @param dstWidth Output image width in pixels
 * @param dstHeight Output image height in pixels
 * @param dstImage Output buffer, can be same as input buffer
 * @param pixel_size_B Size of pixels in Bytes.  3 for RGB, 1 for mono
 */
int resize_image(
    const uint8_t *srcImage,
    int srcWidth,
    int srcHeight,
    uint8_t *dstImage,
    int dstWidth,
    int dstHeight,
    int pixel_size_B)
{
// Copied from ei_camera.cpp in firmware-eta-compute
// Modified for RGB888
// This needs to be < 16 or it won't fit. Cortex-M4 only has SIMD for signed multiplies
    constexpr int FRAC_BITS = 14;
    constexpr int FRAC_VAL = (1 << FRAC_BITS);
    constexpr int FRAC_MASK = (FRAC_VAL - 1);

    uint32_t src_x_accum, src_y_accum; // accumulators and fractions for scaling the image
    uint32_t x_frac, nx_frac, y_frac, ny_frac;
    int x, y, ty;

    if (srcHeight < 2) {
        return EIDSP_PARAMETER_INVALID;
    }

    // start at 1/2 pixel in to account for integer downsampling which might miss pixels
    src_y_accum = FRAC_VAL / 2;
    const uint32_t src_x_frac = (srcWidth * FRAC_VAL) / dstWidth;
    const uint32_t src_y_frac = (srcHeight * FRAC_VAL) / dstHeight;

    //from here out, *3 b/c RGB
    srcWidth *= pixel_size_B;
    //srcHeight not used for indexing
    //dstWidth still needed as is
    //dstHeight shouldn't be scaled

    const uint8_t *s;
    uint8_t *d;

    for (y = 0; y < dstHeight; y++) {
        // do indexing computations
        ty = src_y_accum >> FRAC_BITS; // src y
        y_frac = src_y_accum & FRAC_MASK;
        src_y_accum += src_y_frac;
        ny_frac = FRAC_VAL - y_frac; // y fraction and 1.0 - y fraction

        s = &srcImage[ty * srcWidth];
        d = &dstImage[y * dstWidth * pixel_size_B]; //not scaled above
        // start at 1/2 pixel in to account for integer downsampling which might miss pixels
        src_x_accum = FRAC_VAL / 2;
        for (x = 0; x < dstWidth; x++) {
            uint32_t tx, p00, p01, p10, p11;
            // do indexing computations
            tx = (src_x_accum >> FRAC_BITS) * pixel_size_B;
            x_frac = src_x_accum & FRAC_MASK;
            nx_frac = FRAC_VAL - x_frac; // x fraction and 1.0 - x fraction
            src_x_accum += src_x_frac;

            //interpolate and write out
            for (int color = 0; color < pixel_size_B;
                 color++) // do pixel_size_B times for pixel_size_B colors
            {
                p00 = s[tx];
                p10 = s[tx + pixel_size_B];
                p01 = s[tx + srcWidth];
                p11 = s[tx + srcWidth + pixel_size_B];
                p00 = ((p00 * nx_frac) + (p10 * x_frac) + FRAC_VAL / 2) >> FRAC_BITS; // top line
                p01 = ((p01 * nx_frac) + (p11 * x_frac) + FRAC_VAL / 2) >> FRAC_BITS; // bottom line
                p00 = ((p00 * ny_frac) + (p01 * y_frac) + FRAC_VAL / 2) >> FRAC_BITS; //top + bottom
                *d++ = (uint8_t)p00; // store new pixel
                //ready next loop
                tx++;
            }
        } // for x
    } // for y
    return EIDSP_OK;
} // resizeImage()

/**
 * @brief Calculate new dims that match the aspect ratio of destination
 * This prevents a squashed look
 * The smallest axis is held constant
 *
 * @param srcWidth Input width in pixels
 * @param srcHeight Input height in pixels
 * @param dstWidth Ultimate width in pixels
 * @param dstHeight Ultimate height in pixels
 * @param[out] cropWidth Width in pixels that matches the aspect ratio
 * @param[out] cropHeight Height in pixels that matches the aspect ratio
 */
void calculate_crop_dims(
    int srcWidth,
    int srcHeight,
    int dstWidth,
    int dstHeight,
    int &cropWidth,
    int &cropHeight)
{
    //first, trim the largest axis to match destination aspect ratio
    //calculate by fixing the smaller axis
    if (srcWidth > srcHeight) {
        cropWidth = (uint32_t)(dstWidth * srcHeight) / dstHeight; //cast in case int is small
        cropHeight = srcHeight;
    }
    else {
        cropHeight = (uint32_t)(dstHeight * srcWidth) / dstWidth;
        cropWidth = srcWidth;
    }
}

int crop_and_interpolate_rgb888(
    const uint8_t *srcImage,
    int srcWidth,
    int srcHeight,
    uint8_t *dstImage,
    int dstWidth,
    int dstHeight)
{
    int cropWidth, cropHeight;
    // What are dimensions that maintain aspect ratio?
    calculate_crop_dims(srcWidth, srcHeight, dstWidth, dstHeight, cropWidth, cropHeight);
    // Now crop to that dimension
    int res = crop_image_rgb888_packed(
        srcImage,
        srcWidth,
        srcHeight,
        (srcWidth - cropWidth) / 2,
        (srcHeight - cropHeight) / 2,
        dstImage,
        cropWidth,
        cropHeight);

    if( res != EIDSP_OK) { return res; }
    // Finally, interpolate down to desired dimensions, in place
    return resize_image(dstImage, cropWidth, cropHeight, dstImage, dstWidth, dstHeight, 3);
}

int crop_and_interpolate_image(
    const uint8_t *srcImage,
    int srcWidth,
    int srcHeight,
    uint8_t *dstImage,
    int dstWidth,
    int dstHeight,
    int pixel_size_B)
{
    int cropWidth, cropHeight;
    // What are dimensions that maintain aspect ratio?
    calculate_crop_dims(srcWidth, srcHeight, dstWidth, dstHeight, cropWidth, cropHeight);

    // Now crop to that dimension
    int res =  cropImage(
        srcImage,
        srcWidth * pixel_size_B,
        srcHeight,
        ((srcWidth - cropWidth) / 2) * pixel_size_B,
        (srcHeight - cropHeight) / 2,
        dstImage,
        cropWidth * pixel_size_B,
        cropHeight,
        8);

    if( res != EIDSP_OK) { return res; }

    // Finally, interpolate down to desired dimensions, in place
    return resize_image(dstImage, cropWidth, cropHeight, dstImage, dstWidth, dstHeight, pixel_size_B);
}

}}} //namespaces
#endif //!__EI_IMAGE_PROCESSING__H__