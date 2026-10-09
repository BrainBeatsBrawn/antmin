module;

#include <memory>
#include <iostream>
#include <cstdint>
#include <vector>

// 'pre-brain' data management on Compound-ray input; selection of colour channels, etc. This should
// be code that is common to multiple brain model systems
export module antbrain.vision;

import sm.quaternion;
import sm.mat;
import sm.vec;
import sm.vvec;
import mplot.hexgridvisual;
import craysim.visual;
import craysim.doublehexgrid;
import craysim.compoundray.eyevisual;

export namespace antbrain
{
    template <std::int32_t glver> struct hexvision;

    // A visualiser class to show eye views.
    template<std::int32_t glver>
    struct hexvision_visualizer
    {
        static constexpr bool twodee = true;

        sm::vec<float> grp_offset = {};

        // Pointer to eyes.
        craysim::compoundray::ommatidia_datamodel<glver>* ep = nullptr;

        hexvision_visualizer (craysim::visual<glver>* _v_craysim,
                              mplot::Visual<glver>* _veye,
                              sm::hexgrid<float>* eye_hexgrid,
                              //hexvision<glver>* _hv,
                              sm::vec<float> _grp_offset = {0.0f})
        {
            this->grp_offset = _grp_offset;

            sm::mat<float, 4> mflip (sm::quaternion<float>{0, 0, 1, 0});
            if (_v_craysim->sim_opts.test (craysim::options::eye_is_hex) == false) {
                // EyeVisual can provide a 2D eye representation (on its own, with show_3d = false)
                auto eyevm2 = std::make_unique<craysim::compoundray::EyeVisual<glver>> (this->grp_offset,
                                                                                        &(_v_craysim->ommatidia_datas[0]),
                                                                                        _v_craysim->get_ommatidia_ptr(0));
                eyevm2->set_parent (_veye->get_id());
                eyevm2->name = "2D Ant Eyes";
                craysim::add_ant_eye_spherical_projection<glver> (*_v_craysim, eyevm2.get(), 0);
                eyevm2->show_3d = false;
                eyevm2->setGamma (0.45f);
                eyevm2->twodimensional (twodee); // Determines if the model should rotate when the mouse is dragged
                eyevm2->show_sphere = false;
                eyevm2->show_rays = false;
                eyevm2->setViewMatrix (mflip);
                eyevm2->finalize();
                this->ep = _veye->addVisualModel (eyevm2);
                this->ep->scaleViewMatrix (1000);
            } else {
                // There can be an optional extra double hex of flat eyes if 'eye_is_hex'
                auto dhg = std::make_unique<craysim::doublehexgrid<glver>> (eye_hexgrid, this->grp_offset);
                dhg->set_parent (_veye->get_id());
                dhg->ommData = &_v_craysim->ommatidia_datas[0];
                dhg->ommatidia = _v_craysim->get_ommatidia_ptr(0); // gets repeatedly reset in craysim_visual
                dhg->show_flat = true;                  // couple of issues to solve here
                dhg->second_grid_flip_lr = true;       // flip the second grid left-right
                dhg->grid_offset = {0.00075f, 0};     // grid offset is applied in opposide senses to each grid
                dhg->setGamma (0.45f);
                dhg->twodimensional (twodee);
                dhg->setViewMatrix (mflip);
                dhg->finalize();
                this->ep = _veye->addVisualModel (dhg);
                this->ep->scaleViewMatrix (1000);
            }
        }

        // Note that when this goes out of scope, the VisualModels will persist in v
        ~hexvision_visualizer() {}

        // The hexvision object that holds eye data - externally managed memory
        hexvision<glver>* hv = nullptr;

        // These HexGridVisuals are initialized with the hexgrid from the OCES eye. They are added to (and thus owned by) veye.
        mplot::HexGridVisual<float, sm::hexalign::point_up, glver>* eye_left_hgv = nullptr;
        mplot::HexGridVisual<float, sm::hexalign::point_up, glver>* eye_right_hgv = nullptr;
    };

#if 0

    /*!
     * Visualizer and preprocessor for eyes. This code is written on the assumption that you are
     * using hex-equivalent compound-ray eyes. It takes the raw data in v_ptr->ommatidia_datas and
     * processes it. Adapt to your application. You may wish to collapse into a mono channel by some
     * method or apply lateral inhibition or edge detection or some other convolution that is
     * appropriate for the data. Effectively, this is the first stage of a visual brain model,
     * processing and visualizing the input signal while it has the retinotopic layout of the eyes.
     */
    template <std::int32_t glver>
    struct hexvision
    {
        hexvision (craysim::visual<glver>* vp)
        {
            this->v_ptr = vp;
        }

        void process_mono()
        {
        }

        void lateral_inhibition()
        {
        }

        // Process the raw data in v.ommatiida_datas[0], separating out channels etc and applying any filtering that might fit here
        void process()
        {
            // ommatidia_datas[0] is std::vector<std::array<float, 3>> (vector of RGB)
            std::vector<std::array<float, 3>>& vod = v.ommatidia_datas[0];

            if (st->mono_l.size() != this->n_half_ommatidia) {
                st->mono_l.resize (this->n_half_ommatidia);
                st->mono_r.resize (this->n_half_ommatidia);
            }

            this->process_mono();

            if (lat_inhib) { this->lateral_inhibition(); }

            if (st->gv2p_left != nullptr && v.ommatidia_datas[0].size() > 0) {
                // Doesn't appear to be necessary to reinit the RGB grid here. What about the scalar (lat inhib) Grid?
                // Possibly reinitColours calls here, for member visualizers
            }
        }

        // Non-owning pointer to the craysim::visual that owns ommatidia_datas. Provides access to OCES hexgrid?
        craysim::visual<glver>* v_ptr;

        // Left eye values. Do I need a copy? left is v_ptr->ommatidia_datas[0][firsthalf] and could/should be a span
        std::vector<sm::vec<float, 3>> eye_left_data = {};
        std::vector<sm::vec<float, 3>> eye_right_data = {};

        // A monochrome luminance for left and right eyes
        sm::vvec<float> mono_l = {};
        sm::vvec<float> mono_r = {};
    };
#endif
}
