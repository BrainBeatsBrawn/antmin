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
            if (this->mono_l.size() != this->sz1) {
                this->mono_l.resize (this->sz1);
                this->mono_r.resize (this->sz1);
            }
            if (this->v_ptr && this->v_ptr->rdr.eye.get_eye_side() == oces::eye_side::left) {

                // ommatidia_datas[0] is std::vector<std::array<float, 3>> (vector of RGB)
                for (std::uint32_t i = 0u; i < this->sz1; ++i) {
                    // Some studies use the blue channel as a monochrome luminance:
                    this->mono_l[i] = this->v_ptr->ommatidia_datas[0][i][2];
                    this->mono_r[i] = this->v_ptr->ommatidia_datas[0][this->sz1 + i][2];
                    // Or the mean luminance:
                    // this->mono_l[i] = 0.33f * (this->v_ptr->ommatidia_datas[0][i][0] + this->v_ptr->ommatidia_datas[0][i][1] + this->v_ptr->ommatidia_datas[0][i][2]);
                    // this->mono_r[i] = 0.33f * (this->v_ptr->ommatidia_datas[0][this->sz1 + i][0] + this->v_ptr->ommatidia_datas[0][this->sz1 + i][1] + this->v_ptr->ommatidia_datas[0][this->sz1 + i][2]);
                }
            } else {
                for (std::uint32_t i = 0u; i < this->sz1; ++i) {
                    this->mono_r[i] = this->v_ptr->ommatidia_datas[0][i][2];
                    this->mono_l[i] = this->v_ptr->ommatidia_datas[0][this->sz1 + i][2];
                }
            }
        }

        // Apply lateral inhibition. Demonstrates the use of left_eye and right_eye sm::hexgrids (accessible via v_ptr)
        void lateral_inhibition()
        {
            if (this->inhib_l.size() != this->sz1) {
                this->inhib_l.resize (this->sz1);
                this->inhib_r.resize (this->sz1);
            }

            if (this->v_ptr && this->v_ptr->rdr.read_success && this->v_ptr->rdr.eye_is_hex) {

                sm::hexgrid<float>* hg_ptr = &this->v_ptr->rdr.heye.hg;
                for (std::uint32_t i = 0; i < this->sz1; ++i) {

                    // access: this->v_ptr->rdr.heye.hg ;
                    float nb_value_l = 0.0f; // sum of neighbours
                    float nb_value_r = 0.0f; // sum of neighbours
                    std::uint32_t nbsz = 0u; // number of neighbours
                    // d_n0[i] is a vector of the indices of the 'neighbours in direction 0' wrt the hex at vector index i.
                    nb_value_l += hg_ptr->d_n0[i] == -1 ? 0 : this->mono_l[hg_ptr->d_n0[i]], ++nbsz;
                    nb_value_l += hg_ptr->d_n1[i] == -1 ? 0 : this->mono_l[hg_ptr->d_n1[i]], ++nbsz;
                    nb_value_l += hg_ptr->d_n2[i] == -1 ? 0 : this->mono_l[hg_ptr->d_n2[i]], ++nbsz;
                    nb_value_l += hg_ptr->d_n3[i] == -1 ? 0 : this->mono_l[hg_ptr->d_n3[i]], ++nbsz;
                    nb_value_l += hg_ptr->d_n4[i] == -1 ? 0 : this->mono_l[hg_ptr->d_n4[i]], ++nbsz;
                    nb_value_l += hg_ptr->d_n5[i] == -1 ? 0 : this->mono_l[hg_ptr->d_n5[i]], ++nbsz;
                    nb_value_l /= nbsz;

                    // Right and left eye have same hexgrid, so nbsz is same
                    nb_value_r += hg_ptr->d_n0[i] == -1 ? 0 : this->mono_r[hg_ptr->d_n0[i]];
                    nb_value_r += hg_ptr->d_n1[i] == -1 ? 0 : this->mono_r[hg_ptr->d_n1[i]];
                    nb_value_r += hg_ptr->d_n2[i] == -1 ? 0 : this->mono_r[hg_ptr->d_n2[i]];
                    nb_value_r += hg_ptr->d_n3[i] == -1 ? 0 : this->mono_r[hg_ptr->d_n3[i]];
                    nb_value_r += hg_ptr->d_n4[i] == -1 ? 0 : this->mono_r[hg_ptr->d_n4[i]];
                    nb_value_r += hg_ptr->d_n5[i] == -1 ? 0 : this->mono_r[hg_ptr->d_n5[i]];
                    nb_value_r /= nbsz;

                    const float on_off_l = mono_l[i] - nb_value_l;
                    const float on_off_r = mono_r[i] - nb_value_r;
                    this->inhib_l[i] = on_off_l < 0 ? 0 : on_off_l;
                    this->inhib_r[i] = on_off_r < 0 ? 0 : on_off_r;
                }
            }
        }

        // Process the raw data in v.ommatiida_datas[0], separating out channels etc and applying any filtering that might fit here
        void process()
        {
            if (this->v_ptr == nullptr) { return; } // must have v_ptr to process

            if (this->two_eyes == false) {
                std::cerr << "FIXME: Implement two_eyes == false code\n";
                return;
            }

            this->sz2 = this->v_ptr->ommatidia_datas[0].size();
            if (this->sz2 % 2u != 0u) {
                std::cerr << "sz2 is not even\n";
                return;
            }
            this->sz1 = this->sz2 / 2u;

            this->process_mono();

            if (lat_inhib) { this->lateral_inhibition(); }
        }

        // Number of ommatidia in two eyes
        std::uint32_t sz2 = 0u;
        // Number of ommatidia in one eye
        std::uint32_t sz1 = 0u;

        // Non-owning pointer to the craysim::visual that owns ommatidia_datas. Provides access to OCES hexgrid?
        craysim::visual<glver>* v_ptr = nullptr;

        // A monochrome luminance for left and right eyes
        sm::vvec<float> mono_l = {};
        sm::vvec<float> mono_r = {};

        // Laterally inhibited signal
        sm::vvec<float> inhib_l = {};
        sm::vvec<float> inhib_r = {};

        // We assume there are two eyes in v_ptr->ommatidia_datas[0], but other systems could have one.
        bool two_eyes = true;

        // Set true to apply lateral inhibition
        bool lat_inhib = false;
    };

    // A visualiser class to show eye views.
    template<std::int32_t glver>
    struct hexvision_visualizer
    {
        static constexpr bool twodee = true;

        sm::vec<float> grp_offset = {};

        // Make and maintain copies of the eye_hexgrid
        sm::hexgrid<float> left_eye;
        sm::hexgrid<float> right_eye;

        // Pointer to eyes.
        craysim::compoundray::ommatidia_datamodel<glver>* ep = nullptr;

        hexvision_visualizer (craysim::visual<glver>* _v_craysim,
                              mplot::Visual<glver>* _veye,
                              hexvision<glver>* _hv,
                              sm::vec<float> _grp_offset = {0.0f})
        {
            this->hv = _hv;

            this->grp_offset = _grp_offset;
            if (_v_craysim->sim_opts.test (craysim::options::eye_is_hex) == true) {
                this->grp_offset += sm::vec<float>{0, 0.6f, 0}; // doublehexgrid plots lower than EyeVisual.
            }

            sm::mat<float, 4> mflip (sm::quaternion<float>{0, 0, 1, 0});
            // We apply grp_offset to the mflip matrix, as it will override any offset in the
            // EyeVisual/doublehexgrid constructor
            mflip.pretranslate (this->grp_offset);

            // The eyes are special - they are 'craysim::visual::other_eyes' and will get rendered by craysim::visual::render_and_poll
            if (_v_craysim->sim_opts.test (craysim::options::eye_is_hex) == false) {
                // EyeVisual can provide a 2D eye representation (on its own, with show_3d = false)
                auto eyevm2 = std::make_unique<craysim::compoundray::EyeVisual<glver>> (sm::vec<>{},
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
                if (_v_craysim->rdr.read_success && _v_craysim->rdr.eye_is_hex) {
                    auto dhg = std::make_unique<craysim::doublehexgrid<glver>> (&_v_craysim->rdr.heye.hg, sm::vec<>{});
                    dhg->set_parent (_veye->get_id());
                    dhg->ommData = &_v_craysim->ommatidia_datas[0];
                    dhg->ommatidia = _v_craysim->get_ommatidia_ptr(0); // gets repeatedly reset in craysim_visual
                    dhg->show_flat = true;
                    if (_v_craysim->rdr.eye.get_eye_side() == oces::eye_side::right) {
                        dhg->second_grid_flip_lr = true;       // flip the second grid left-right
                    }
                    if (_v_craysim->rdr.eye.get_eye_side() == oces::eye_side::left) {
                        dhg->first_grid_flip_lr = true;
                    }
                    dhg->grid_offset = {0.00075f, 0};     // grid offset is applied in opposite senses to each grid
                    dhg->setGamma (0.45f);
                    dhg->twodimensional (twodee);
                    dhg->setViewMatrix (mflip);
                    dhg->finalize();
                    this->ep = _veye->addVisualModel (dhg);
                    this->ep->scaleViewMatrix (1000);
                } else {
                    std::cerr << "eye_is_hex is true, but craysim's oces reader is not set up as a hex-equivalent eye\n";
                }
            }

            // Rest of the code here *requires* that we have a hex-equivalent eye
            if (!_v_craysim->rdr.read_success || !_v_craysim->rdr.eye_is_hex) { return; }

            // Copy eye_hexgrids (maybe copy pointers)
            if (_v_craysim->rdr.eye.get_eye_side() == oces::eye_side::right) {
                std::cout << "main hex hexgrid is for the right eye\n";
                this->right_eye = _v_craysim->rdr.heye.hg;
                this->left_eye = _v_craysim->rdr.heye.hg_mirr;
            } else if (_v_craysim->rdr.eye.get_eye_side() == oces::eye_side::left) {
                std::cout << "main hex hexgrid is for the left eye\n";
                this->left_eye = _v_craysim->rdr.heye.hg;
                this->right_eye = _v_craysim->rdr.heye.hg_mirr;
            } else {
                std::cout << "Uh oh\n";
            }

            // Show monochrome data for each eye (in separate HexGrids now). The user will have to manually render these, I think.
            auto hg_mono_left = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, glver>> (&this->left_eye, this->grp_offset + sm::vec<float>{-0.375f, -0.5f, 0});
            hg_mono_left->set_parent (_veye->get_id());
            hg_mono_left->twodimensional (twodee);
            hg_mono_left->cm.setType (mplot::ColourMapType::Ice);
            hg_mono_left->setScalarData (&this->hv->mono_l);
            hg_mono_left->hexVisMode = mplot::HexVisMode::HexInterp;
            hg_mono_left->reflect_lr(); // so we are looking at the inside of the eye - the ant's view
            hg_mono_left->zScale.null_scaling();
            hg_mono_left->addLabel ("mono, left", sm::vec<>{ -0.3f, -0.2f }, mplot::TextFeatures(0.02f));
            hg_mono_left->finalize();
            this->mono_left_hgv = _veye->addVisualModel (hg_mono_left);
            this->mono_left_hgv->scaleViewMatrix (1000);

            auto hg_mono_right = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, glver>> (&this->right_eye, this->grp_offset + sm::vec<float>{0.375f, -0.5f, 0});
            hg_mono_right->set_parent (_veye->get_id());
            hg_mono_right->twodimensional (twodee);
            hg_mono_right->cm.setType (mplot::ColourMapType::Ice);
            hg_mono_right->setScalarData (&this->hv->mono_r);
            hg_mono_right->hexVisMode = mplot::HexVisMode::HexInterp;
            hg_mono_right->reflect_lr();
            hg_mono_right->zScale.null_scaling();
            hg_mono_right->addLabel ("mono, right", sm::vec<>{ 0.2f, -0.2f }, mplot::TextFeatures(0.02f));
            hg_mono_right->finalize();
            this->mono_right_hgv = _veye->addVisualModel (hg_mono_right);
            this->mono_right_hgv->scaleViewMatrix (1000);

            if (this->hv->lat_inhib == true) {
                auto hg_left = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, glver>> (&this->left_eye, this->grp_offset + sm::vec<float>{-0.375f, -1.0f, 0});
                hg_left->set_parent (_veye->get_id());
                hg_left->twodimensional (twodee);
                hg_left->cm.setType (mplot::ColourMapType::Plasma);
                hg_left->setScalarData (&this->hv->inhib_l);
                hg_left->hexVisMode = mplot::HexVisMode::HexInterp;
                hg_left->reflect_lr(); // so we are looking at the inside of the eye - the ant's view
                hg_left->zScale.null_scaling();
                hg_left->addLabel ("lat. inhib.", sm::vec<>{ -0.3f, -0.2f }, mplot::TextFeatures(0.02f));
                hg_left->finalize();
                this->inhib_left_hgv = _veye->addVisualModel (hg_left);
                this->inhib_left_hgv->scaleViewMatrix (1000);

                auto hg_right = std::make_unique<mplot::HexGridVisual<float, sm::hexalign::point_up, glver>> (&this->right_eye, this->grp_offset + sm::vec<float>{0.375f, -1.0f, 0});
                hg_right->set_parent (_veye->get_id());
                hg_right->twodimensional (twodee);
                hg_right->cm.setType (mplot::ColourMapType::Plasma);
                hg_right->setScalarData (&this->hv->inhib_r);
                hg_right->hexVisMode = mplot::HexVisMode::HexInterp;
                hg_right->reflect_lr();
                hg_right->zScale.null_scaling();
                hg_right->finalize();
                this->inhib_right_hgv = _veye->addVisualModel (hg_right);
                this->inhib_right_hgv->scaleViewMatrix (1000);
            }
        }

        // Note that when this goes out of scope, the VisualModels will persist in v
        ~hexvision_visualizer() {}

        void render()
        {
            if (this->mono_left_hgv != nullptr) {
                this->mono_left_hgv->reinit(); // reinitColours no good for now.
            }
            if (this->mono_right_hgv != nullptr) {
                this->mono_right_hgv->reinit();
            }
            if (this->inhib_left_hgv != nullptr) {
                this->inhib_left_hgv->reinit(); // reinitColours no good for now.
            }
            if (this->inhib_right_hgv != nullptr) {
                this->inhib_right_hgv->reinit();
            }
        }

        // The hexvision object that holds eye data - externally managed memory
        hexvision<glver>* hv = nullptr;

        // These HexGridVisuals are initialized with the hexgrid from the OCES eye. They are added to (and thus owned by) veye.
        mplot::HexGridVisual<float, sm::hexalign::point_up, glver>* mono_left_hgv = nullptr;
        mplot::HexGridVisual<float, sm::hexalign::point_up, glver>* mono_right_hgv = nullptr;

        mplot::HexGridVisual<float, sm::hexalign::point_up, glver>* inhib_left_hgv = nullptr;
        mplot::HexGridVisual<float, sm::hexalign::point_up, glver>* inhib_right_hgv = nullptr;
    };
}
