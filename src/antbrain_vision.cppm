module;

#include <memory>
#include <iostream>
#include <cstdint>
#include <vector>

// 'pre-brain' data management on Compound-ray input; selection of colour channels, etc. This should
// be code that is common to multiple brain model systems
export module antbrain.vision;

import sm.vec;
import sm.vvec;

import mplot.gl.version;

export namespace antbrain
{
    // OpenGL 4.3 for Instanced VisualModels
    constexpr std::int32_t glver = mplot::gl::version_4_3;
}
export namespace antbrain::vision
{
    // State for our eye visualizations
    template <int glver>
    struct state
    {
        state()
        {
            //this->blue_l.resize (this->g2->n());
            //this->blue_r.resize (this->g2->n());
        }

        std::uint32_t n_half_ommatidia = 0u;
        std::uint32_t n_all_ommatidia = 0u;

        // Left eye values. Do I need a copy?
        std::vector<sm::vec<float, 3>> eye_left = {};
        // Right eye values
        std::vector<sm::vec<float, 3>> eye_right = {};

        // Also need a half-grid for 'left panorama' and 'right panorama' portions that are fed into the mushroom body projection neurons
        sm::vvec<float> blue_l = {};
        sm::vvec<float> blue_r = {};
    };
}
