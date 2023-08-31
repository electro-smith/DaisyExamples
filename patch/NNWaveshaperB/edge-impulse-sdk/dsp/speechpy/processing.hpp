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

#ifndef _EIDSP_SPEECHPY_PROCESSING_H_
#define _EIDSP_SPEECHPY_PROCESSING_H_

#include "../numpy.hpp"

namespace ei {
namespace speechpy {

// one stack frame returned by stack_frames
typedef struct ei_stack_frames_info {
    signal_t *signal;
    ei_vector<uint32_t> frame_ixs;
    int frame_length;
} stack_frames_info_t;

namespace processing {
    /**
     * Lazy Preemphasising on the signal.
     * @param signal: The input signal.
     * @param shift (int): The shift step.
     * @param cof (float): The preemphasising coefficient. 0 equals to no filtering.
     */
    class preemphasis {
public:
        preemphasis(ei_signal_t *signal, int shift, float cof, bool rescale)
            : _signal(signal), _shift(shift), _cof(cof), _rescale(rescale)
        {
            _prev_buffer = (float*)ei_dsp_calloc(shift * sizeof(float), 1);
            _end_of_signal_buffer = (float*)ei_dsp_calloc(shift * sizeof(float), 1);
            _next_offset_should_be = 0;

            if (shift < 0) {
                _shift = signal->total_length + shift;
            }

            if (!_prev_buffer || !_end_of_signal_buffer) return;

            // we need to get the shift bytes from the end of the buffer...
            signal->get_data(signal->total_length - shift, shift, _end_of_signal_buffer);
        }

        /**
         * Get preemphasized data from the underlying audio buffer...
         * This retrieves data from the signal then preemphasizes it.
         * @param offset Offset in the audio signal
         * @param length Length of the audio signal
         */
        int get_data(size_t offset, size_t length, float *out_buffer) {
            if (!_prev_buffer || !_end_of_signal_buffer) {
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }
            if (offset + length > _signal->total_length) {
                EIDSP_ERR(EIDSP_OUT_OF_BOUNDS);
            }

            int ret;
            if (static_cast<int32_t>(offset) - _shift >= 0) {
                ret = _signal->get_data(offset - _shift, _shift, _prev_buffer);
                if (ret != 0) {
                    EIDSP_ERR(ret);
                }
            }
            // else we'll use the end_of_signal_buffer; so no need to check

            ret = _signal->get_data(offset, length, out_buffer);
            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            // it might be that everything is already normalized here...
            bool all_between_min_1_and_1 = true;

            // now we have the signal and we can preemphasize
            for (size_t ix = 0; ix < length; ix++) {
                float now = out_buffer[ix];

                // under shift? read from end
                if (offset + ix < static_cast<uint32_t>(_shift)) {
                    out_buffer[ix] = now - (_cof * _end_of_signal_buffer[offset + ix]);
                }
                // otherwise read from history buffer
                else {
                    out_buffer[ix] = now - (_cof * _prev_buffer[0]);
                }

                if (_rescale && all_between_min_1_and_1) {
                    if (out_buffer[ix] < -1.0f || out_buffer[ix] > 1.0f) {
                        all_between_min_1_and_1 = false;
                    }
                }

                // roll through and overwrite last element
                if (_shift != 1) {
                    numpy::roll(_prev_buffer, _shift, -1);
                }
                _prev_buffer[_shift - 1] = now;
            }

            _next_offset_should_be += length;

            // rescale from [-1 .. 1] ?
            if (_rescale && !all_between_min_1_and_1) {
                matrix_t scale_matrix(length, 1, out_buffer);
                ret = numpy::scale(&scale_matrix, 1.0f / 32768.0f);
                if (ret != 0) {
                    EIDSP_ERR(ret);
                }
            }

            return EIDSP_OK;
        }

        ~preemphasis() {
            if (_prev_buffer) {
                ei_dsp_free(_prev_buffer, _shift * sizeof(float));
            }
            if (_end_of_signal_buffer) {
                ei_dsp_free(_end_of_signal_buffer, _shift * sizeof(float));
            }
        }

private:
        ei_signal_t *_signal;
        int _shift;
        float _cof;
        float *_prev_buffer;
        float *_end_of_signal_buffer;
        size_t _next_offset_should_be;
        bool _rescale;
    };
}

namespace processing {
    /**
     * Preemphasising on the signal. This modifies the signal in place!
     * For memory consumption reasons you **probably** want the preemphasis class,
     * which lazily loads the signal in.
     * @param signal (array): The input signal.
     * @param shift (int): The shift step.
     * @param cof (float): The preemphasising coefficient. 0 equals to no filtering.
     * @returns 0 when successful
     */
    __attribute__((unused)) static int preemphasis(float *signal, size_t signal_size, int shift = 1, float cof = 0.98f)
    {
        if (shift < 0) {
            shift = signal_size + shift;
        }

        // so we need to keep some history
        float *prev_buffer = (float*)ei_dsp_calloc(shift * sizeof(float), 1);

        // signal - cof * xt::roll(signal, shift)
        for (size_t ix = 0; ix < signal_size; ix++) {
            float now = signal[ix];

            // under shift? read from end
            if (ix < static_cast<uint32_t>(shift)) {
                signal[ix] = now - (cof * signal[signal_size - shift + ix]);
            }
            // otherwise read from history buffer
            else {
                signal[ix] = now - (cof * prev_buffer[0]);
            }

            // roll through and overwrite last element
            numpy::roll(prev_buffer, shift, -1);
            prev_buffer[shift - 1] = now;
        }

        ei_dsp_free(prev_buffer, shift * sizeof(float));

        return EIDSP_OK;
    }

    /**
     * frame_length is a float and can thus be off by a little bit, e.g.
     * frame_length = 0.018f actually can yield 0.018000011f
     * thus screwing up our frame calculations here...
     */
    static float ceil_unless_very_close_to_floor(float v) {
        if (v > floor(v) && v - floor(v) < 0.001f) {
            v = (floor(v));
        }
        else {
            v = (ceil(v));
        }
        return v;
    }

    /**
     * Calculate the length of a signal that will be sused for the settings provided.
     * @param signal_size: The number of frames in the signal
     * @param sampling_frequency (int): The sampling frequency of the signal.
     * @param frame_length (float): The length of the frame in second.
     * @param frame_stride (float): The stride between frames.
     * @returns Number of frames required, or a negative number if an error occured
     */
    static int calculate_signal_used(
        size_t signal_size,
        uint32_t sampling_frequency,
        float frame_length,
        float frame_stride,
        bool zero_padding,
        uint16_t version)
    {
        int frame_sample_length;
        int length;
        if (version == 1) {
            frame_sample_length = static_cast<int>(round(static_cast<float>(sampling_frequency) * frame_length));
            frame_stride = round(static_cast<float>(sampling_frequency) * frame_stride);
            length = frame_sample_length;
        }
        else {
            frame_sample_length = static_cast<int>(ceil_unless_very_close_to_floor(static_cast<float>(sampling_frequency) * frame_length));
            float frame_stride_arg = frame_stride;
            frame_stride = ceil_unless_very_close_to_floor(static_cast<float>(sampling_frequency) * frame_stride_arg);
            length = (frame_sample_length - (int)frame_stride);
        }

        volatile int numframes;
        volatile int len_sig;

        if (zero_padding) {
            // Calculation of number of frames
            numframes = static_cast<int>(
                ceil(static_cast<float>(signal_size - length) / frame_stride));

            // Zero padding
            len_sig = static_cast<int>(static_cast<float>(numframes) * frame_stride) + frame_sample_length;
        }
        else {
            numframes = static_cast<int>(
                floor(static_cast<float>(signal_size - length) / frame_stride));
            len_sig = static_cast<int>(
                (static_cast<float>(numframes - 1) * frame_stride + frame_sample_length));
        }

        return len_sig;
    }

    /**
     * Frame a signal into overlapping frames.
     * @param info This is both the base object and where we'll store our results.
     * @param sampling_frequency (int): The sampling frequency of the signal.
     * @param frame_length (float): The length of the frame in second.
     * @param frame_stride (float): The stride between frames.
     * @param zero_padding (bool): If the samples is not a multiple of
     *        frame_length(number of frames sample), zero padding will
     *        be done for generating last frame.
     * @returns EIDSP_OK if OK
     */
    static int stack_frames(stack_frames_info_t *info,
                            float sampling_frequency,
                            float frame_length,
                            float frame_stride,
                            bool zero_padding,
                            uint16_t version)
    {
        if (!info->signal || !info->signal->get_data || info->signal->total_length == 0) {
            EIDSP_ERR(EIDSP_SIGNAL_SIZE_MISMATCH);
        }

        size_t length_signal = info->signal->total_length;
        int frame_sample_length;
        int length;
        if (version == 1) {
            frame_sample_length = static_cast<int>(round(static_cast<float>(sampling_frequency) * frame_length));
            frame_stride = round(static_cast<float>(sampling_frequency) * frame_stride);
            length = frame_sample_length;
        }
        else {
            frame_sample_length = static_cast<int>(ceil_unless_very_close_to_floor(static_cast<float>(sampling_frequency) * frame_length));
            float frame_stride_arg = frame_stride;
            frame_stride = ceil_unless_very_close_to_floor(static_cast<float>(sampling_frequency) * frame_stride_arg);
            length = (frame_sample_length - (int)frame_stride);
        }

        volatile int numframes;
        volatile int len_sig;

        if (zero_padding) {
            // Calculation of number of frames
            numframes = static_cast<int>(
                ceil(static_cast<float>(length_signal - length) / frame_stride));

            // Zero padding
            len_sig = static_cast<int>(static_cast<float>(numframes) * frame_stride) + frame_sample_length;

            info->signal->total_length = static_cast<size_t>(len_sig);
        }
        else {
            numframes = static_cast<int>(
                floor(static_cast<float>(length_signal - length) / frame_stride));
            len_sig = static_cast<int>(
                (static_cast<float>(numframes - 1) * frame_stride + frame_sample_length));

            info->signal->total_length = static_cast<size_t>(len_sig);
        }

        info->frame_ixs.clear();

        int frame_count = 0;

        for (size_t ix = 0; ix < static_cast<uint32_t>(len_sig); ix += static_cast<size_t>(frame_stride)) {
            if (++frame_count > numframes) break;

            info->frame_ixs.push_back(ix);
        }

        info->frame_length = frame_sample_length;

        return EIDSP_OK;
    }

    /**
     * Calculate the number of stack frames for the settings provided.
     * This is needed to allocate the right buffer size for the output of f.e. the MFE
     * blocks.
     * @param signal_size: The number of frames in the signal
     * @param sampling_frequency (int): The sampling frequency of the signal.
     * @param frame_length (float): The length of the frame in second.
     * @param frame_stride (float): The stride between frames.
     * @param zero_padding (bool): If the samples is not a multiple of
     *        frame_length(number of frames sample), zero padding will
     *        be done for generating last frame.
     * @returns Number of frames required, or a negative number if an error occured
     */
    static int32_t calculate_no_of_stack_frames(
        size_t signal_size,
        uint32_t sampling_frequency,
        float frame_length,
        float frame_stride,
        bool zero_padding,
        uint16_t version)
    {
        int frame_sample_length;
        int length;
        if (version == 1) {
            frame_sample_length = static_cast<int>(round(static_cast<float>(sampling_frequency) * frame_length));
            frame_stride = round(static_cast<float>(sampling_frequency) * frame_stride);
            length = frame_sample_length;
        }
        else {
            frame_sample_length = static_cast<int>(ceil_unless_very_close_to_floor(static_cast<float>(sampling_frequency) * frame_length));
            float frame_stride_arg = frame_stride;
            frame_stride = ceil_unless_very_close_to_floor(static_cast<float>(sampling_frequency) * frame_stride_arg);
            length = (frame_sample_length - (int)frame_stride);
        }

        volatile int numframes;

        if (zero_padding) {
            // Calculation of number of frames
            numframes = static_cast<int>(
                ceil(static_cast<float>(signal_size - length) / frame_stride));
        }
        else {
            numframes = static_cast<int>(
                floor(static_cast<float>(signal_size - length) / frame_stride));
        }

        return numframes;
    }

    /**
     * This function performs local cepstral mean and
     * variance normalization on a sliding window. The code assumes that
     * there is one observation per row.
     * @param features_matrix input feature matrix, will be modified in place
     * @param win_size The size of sliding window for local normalization.
     *   Default=301 which is around 3s if 100 Hz rate is
     *   considered(== 10ms frame stide)
     * @param variance_normalization If the variance normilization should
     *   be performed or not.
     * @param scale Scale output to 0..1
     * @returns 0 if OK
     */
    static int cmvnw(matrix_t *features_matrix, uint16_t win_size = 301, bool variance_normalization = false,
        bool scale = false)
    {
        if (win_size == 0) {
            return EIDSP_OK;
        }

        uint16_t pad_size = (win_size - 1) / 2;

        int ret;
        float *features_buffer_ptr;

        // mean & variance normalization
        EI_DSP_MATRIX(vec_pad, features_matrix->rows + (pad_size * 2), features_matrix->cols);
        if (!vec_pad.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        ret = numpy::pad_1d_symmetric(features_matrix, &vec_pad, pad_size, pad_size);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        EI_DSP_MATRIX(mean_matrix, vec_pad.cols, 1);
        if (!mean_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        EI_DSP_MATRIX(window_variance, vec_pad.cols, 1);
        if (!window_variance.buffer) {
            return EIDSP_OUT_OF_MEM;
        }

        for (size_t ix = 0; ix < features_matrix->rows; ix++) {
            // create a slice on the vec_pad
            EI_DSP_MATRIX_B(window, win_size, vec_pad.cols, vec_pad.buffer + (ix * vec_pad.cols));
            if (!window.buffer) {
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }

            ret = numpy::mean_axis0(&window, &mean_matrix);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }

            // subtract the mean for the features
            for (size_t fm_col = 0; fm_col < features_matrix->cols; fm_col++) {
                features_matrix->buffer[(ix * features_matrix->cols) + fm_col] =
                    features_matrix->buffer[(ix * features_matrix->cols) + fm_col] - mean_matrix.buffer[fm_col];
            }
        }

        ret = numpy::pad_1d_symmetric(features_matrix, &vec_pad, pad_size, pad_size);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        for (size_t ix = 0; ix < features_matrix->rows; ix++) {
            // create a slice on the vec_pad
            EI_DSP_MATRIX_B(window, win_size, vec_pad.cols, vec_pad.buffer + (ix * vec_pad.cols));
            if (!window.buffer) {
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }

            if (variance_normalization == true) {
                ret = numpy::std_axis0(&window, &window_variance);
                if (ret != EIDSP_OK) {
                    EIDSP_ERR(ret);
                }

                features_buffer_ptr = &features_matrix->buffer[ix * vec_pad.cols];
                for (size_t col = 0; col < vec_pad.cols; col++) {
                    *(features_buffer_ptr) = (*(features_buffer_ptr)) /
                                             (window_variance.buffer[col] + 1e-10);
                    features_buffer_ptr++;
                }
            }
        }

        if (scale) {
            ret = numpy::normalize(features_matrix);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }
        }

        return EIDSP_OK;
    }

    /**
     * Perform normalization for MFE frames, this converts the signal to dB,
     * then add a hard filter, and quantize / dequantize the output
     * @param features_matrix input feature matrix, will be modified in place
     */
    static int mfe_normalization(matrix_t *features_matrix, int noise_floor_db) {
        const float noise = static_cast<float>(noise_floor_db * -1);
        const float noise_scale = 1.0f / (static_cast<float>(noise_floor_db * -1) + 12.0f);

        for (size_t ix = 0; ix < features_matrix->rows * features_matrix->cols; ix++) {
            float f = features_matrix->buffer[ix];
            if (f < 1e-30) {
                f = 1e-30;
            }
            f = numpy::log10(f);
            f *= 10.0f; // scale by 10
            f += noise;
            f *= noise_scale;
            // clip again

            /* Here is the python code we're duplicating:
            # Quantize to 8 bits and dequantize back to float32
            mfe = np.uint8(np.around(mfe * 2**8))
            # clip to 2**8
            mfe = np.clip(mfe, 0, 255)
            mfe = np.float32(mfe / 2**8)
            */

            f = roundf(f*256)/256;

            if (f < 0.0f) f = 0.0f;
            else if (f > 1.0f) f = 1.0f;
            features_matrix->buffer[ix] = f;
        }

        return EIDSP_OK;
    }

    /**
     * Perform normalization for spectrogram frames, this converts the signal to dB,
     * then add a hard filter
     * @param features_matrix input feature matrix, will be modified in place
     */
    static int spectrogram_normalization(matrix_t *features_matrix, int noise_floor_db) {
        const float noise = static_cast<float>(noise_floor_db * -1);
        const float noise_scale = 1.0f / (static_cast<float>(noise_floor_db * -1) + 12.0f);

        for (size_t ix = 0; ix < features_matrix->rows * features_matrix->cols; ix++) {
            float f = features_matrix->buffer[ix];
            if (f < 1e-30) {
                f = 1e-30;
            }
            f = numpy::log10(f);
            f *= 10.0f; // scale by 10
            f += noise;
            f *= noise_scale;
            // clip again
            if (f < 0.0f) f = 0.0f;
            else if (f > 1.0f) f = 1.0f;
            features_matrix->buffer[ix] = f;
        }

        return EIDSP_OK;
    }
};

} // namespace speechpy
} // namespace ei

#endif // _EIDSP_SPEECHPY_PROCESSING_H_
