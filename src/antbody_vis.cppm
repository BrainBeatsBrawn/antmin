module;

#include <memory>
#include <iostream>
#include <cstdint>
#include <vector>

// 'pre-brain' data management on Compound-ray input; selection of colour channels, etc. This should
// be code that is common to multiple brain model systems
export module antbody.vis;

import sm.vec;
import sm.vvec;

import mplot.hexgridvisual;
import mplot.visual;
import craysim.visual;
import craysim.antbody;
import craysim.doublehexgrid;
import craysim.compoundray.eyevisual;

export namespace antbody
{
    // Visualizer for an ant body, head and (dynamic) eyes
    template <std::int32_t glver>
    struct visualizer
    {
        // Pointer to the eye visualmodel
        craysim::compoundray::ommatidia_datamodel<glver>* ep1 = nullptr;

        visualizer (craysim::visual<glver>* v_craysim, mplot::Visual<glver>* vant, sm::hexgrid<float>* eye_hexgrid)
        {
            // Could get eye_hexgrid from v_craysim

            if (v_craysim->sim_opts.test (craysim::options::eye_is_hex)) {
                auto dhg = std::make_unique<craysim::doublehexgrid<glver>> (eye_hexgrid, sm::vec<>{});
                dhg->set_parent (vant->get_id());
                dhg->ommData = &v_craysim->ommatidia_datas[0];
                dhg->ommatidia = v_craysim->get_ommatidia_ptr(0); // gets repeatedly reset in craysim_visual
                dhg->show_flat = false;
                dhg->setGamma (0.45f);
                dhg->twodimensional (false);
                dhg->addMeshgroup (*v_craysim->get_head_mesh(0)); // Adds the ant head model
                dhg->finalize();
                this->ep1 = vant->addVisualModel (dhg);
                this->ep1->scaleViewMatrix (1000);
            } else {
                // Ant body, plotted in its own window; first the eyes for the body (3D representation)
                auto eyevm1 = std::make_unique<craysim::compoundray::EyeVisual<glver>> (sm::vec<>{}, &v_craysim->ommatidia_datas[0], v_craysim->get_ommatidia_ptr(0));
                eyevm1->set_parent (vant->get_id());
                eyevm1->name = "Ant Eyes";
                eyevm1->show_3d = true;
                eyevm1->setGamma (0.45f);
                eyevm1->addMeshgroup (*v_craysim->get_head_mesh(0));
                eyevm1->finalize();
                this->ep1 = vant->addVisualModel (eyevm1);
                // Scale this model up, so it's not tiny like the one in the main scene
                this->ep1->scaleViewMatrix (1000);
            }
            // The ant body for the separate window
            auto av1 = std::make_unique<craysim::AntBodyVisual<glver>>();
            av1->set_parent (vant->get_id());
            av1->draw_antennae = true;
            av1->draw_body = true;
            av1->finalize();
            mplot::VisualModel<glver>* ant_ptr1 = vant->addVisualModel (av1);
            ant_ptr1->name = "ant";
            ant_ptr1->scaleViewMatrix (1000);
        }
    };
}
