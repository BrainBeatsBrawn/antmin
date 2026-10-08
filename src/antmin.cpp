#include <iostream>
#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <format>
#include <filesystem>
#include <fstream>

import sm.flags;
import sm.vvec;
import sm.grid;
import sm.hexgrid;
import sm.hexgrid.hdf;

import mplot.gl.version;
import craysim.compoundray.interop; // mathplot <--> compoundray interoperability
import craysim.compoundray.eyevisual;
import mplot.tools;
import mplot.gridvisual;

import craysim.visual;
import craysim.antbody;
import craysim.doublehexgrid;

import cater.helpers;

// OpenGL 4.3 for Instanced VisualModels
constexpr std::int32_t glver = mplot::gl::version_4_3;

std::int32_t main (std::int32_t argc, char* argv[])
{
    craysim::parsed_inputs prog_opts = craysim::parse_inputs (argc, argv);
    if (prog_opts.opts.test (craysim::options::can_exit)) { return 1; }

    std::int32_t _w = prog_opts.w > 0 ? prog_opts.w : 1920;
    std::int32_t _h = prog_opts.h > 0 ? prog_opts.h : 1080;
    // Create a craysim main window to render the eye/sensor. This loads in the models from gltf file at path

    // Default/initial samples per second for the compound-ray eye
    constexpr std::int32_t default_samples = 512;
    // We need gamma correction of colours in our EyeVisual
    constexpr float gamma_val = 0.45f;
    craysim::visual<glver> v (_w, _h, "AntPOV", prog_opts, default_samples, gamma_val);
    // Set the agent hoverheight from our inputs if necessary
    v.set_hoverheight (prog_opts.hovh, 0.002f); // 2 mm is good for C. velox model
    // Find the model from the glTF that you want to be the landscape
    v.find_landscape ("Landscape.003,ground_inner_high_res");

    v.frame_tau = 0.017;

    // Match the approximate field of view of the original camera
    v.set_horizontal_fov (26.0f);
    v.ambient_intensity = 0.6f; // override the 0.4/0.6 ambient/diffuse intensity

    // Set light source position suitable for Seville
    v.diffuse_position = { 5, 5, -15 };

    // Label options
    v.sim_opts.set (craysim::options::show_fps, true);
    v.sim_opts.set (craysim::options::show_movenum, false);
    v.fps_label_update_period = 1u;

    // Snip csv path reading
    v.setup_breadcrumbs (6000);
    v.bc_mult = 2.0f; // 1 is default
    v.breadcrumb_every = 10;
    // Once CSV has been read (if you are using that feature) do some setup on the landscape
    v.setup_landscape();

    // From cmd line output (after ctrl-z) set the initial view
    v.setSceneTrans (sm::vec<float,3>{ float{0.682335}, float{0.47893}, float{-8.38334} });
    v.setSceneRotation (sm::quaternion<float>{ float{0.73946}, float{0.6732}, float{0.00036425}, float{0.000331613} });

    // Enable random walking. n_steps, a_tau, kappa are the params
    v.setup_random_walk (1500, 150, 100.0f, 0.05f);

    // A window for the 2D eye view projection
    mplot::Visual<glver> veye (920, 512, "Eye view");
    veye.setSceneTrans (sm::vec<float,3>{ float{-0.00859182}, float{-0.616208}, float{-1.18557} });
    veye.setSceneRotation (sm::quaternion<float>{ float{1}, float{0}, float{0}, float{0} });

    // A window for the Ant body view
    mplot::Visual<glver> vant (920, 920, "Ant view");
    vant.setSceneTrans (sm::vec<float,3>{ float{0.113123}, float{0.0217872}, float{-3.7961} });
    vant.setSceneRotation (sm::quaternion<float>{ float{0.937372}, float{0.106131}, float{0.330499}, float{0.0289824} });

    // Load the eye hexgrid, if it is needed
    sm::hexgrid<float> eye_hexgrid;
    if (v.sim_opts.test (craysim::options::eye_is_hex)) {
        // read eye_hexgrid from eyefilenamebase.h5... then:
        std::string eye_hexgrid_path = {};
        if (v.efpaths.empty()) {
            std::cout << "No efpaths yet...\n";
        } else {
            std::cout << "v.efpaths[0] = " << v.efpaths[0] << "\n";
            eye_hexgrid_path = v.efpaths[0];
            mplot::tools::stripFileSuffix (eye_hexgrid_path);
            eye_hexgrid_path += ".h5";
            std::cout << "eye_hexgrid_path = " << eye_hexgrid_path << "\n";
        }
        sm::hexgrid_load (eye_hexgrid, eye_hexgrid_path);
        std::cout << "eye_hexgrid has " << eye_hexgrid.num() << " hexes\n";
    }

    constexpr bool twodee = true;

    craysim::compoundray::ommatidia_datamodel<glver>* ep1 = nullptr;
    if (v.sim_opts.test (craysim::options::eye_is_hex)) {
        auto dhg = std::make_unique<craysim::doublehexgrid<glver>> (&eye_hexgrid, sm::vec<>{0,0,0});
        dhg->set_parent (vant.get_id());
        dhg->ommData = &v.ommatidia_datas[0];
        dhg->ommatidia = v.get_ommatidia_ptr(0); // gets repeatedly reset in craysim_visual
        dhg->show_flat = false;
        dhg->setGamma (gamma_val);
        dhg->twodimensional (false);
        dhg->addMeshgroup (*v.get_head_mesh(0)); // Adds the ant head model
        dhg->finalize();
        ep1 = vant.addVisualModel (dhg);
        ep1->scaleViewMatrix (1000);
    } else {
        // Ant body, plotted in its own window; first the eyes for the body
        auto eyevm1 = std::make_unique<craysim::compoundray::EyeVisual<glver>> (sm::vec<>{}, &v.ommatidia_datas[0], v.get_ommatidia_ptr(0));
        eyevm1->set_parent (vant.get_id());
        eyevm1->name = "Ant Eyes";
        eyevm1->show_3d = true;
        eyevm1->setGamma (gamma_val);
        eyevm1->addMeshgroup (*v.get_head_mesh(0));
        eyevm1->finalize();
        ep1 = vant.addVisualModel (eyevm1);
        // Scale this model up, so it's not tiny like the one in the main scene
        ep1->scaleViewMatrix (1000);
    }
    // The ant body for the separate window
    auto av1 = std::make_unique<craysim::AntBodyVisual<glver>>();
    av1->set_parent (vant.get_id());
    av1->draw_antennae = true;
    av1->draw_body = true;
    av1->finalize();
    mplot::VisualModel<glver>* ant_ptr1 = vant.addVisualModel (av1);
    ant_ptr1->name = "ant";
    ant_ptr1->scaleViewMatrix (1000);

    mplot::GridVisual<float, std::uint32_t, float, glver>* gv1p = nullptr;
    craysim::compoundray::ommatidia_datamodel<glver>* ep2 = nullptr;

    // 2D eye representation
    auto eyevm2 = std::make_unique<craysim::compoundray::EyeVisual<glver>> (sm::vec<>{}, &v.ommatidia_datas[0], v.get_ommatidia_ptr(0));
    eyevm2->set_parent (veye.get_id());
    eyevm2->name = "2D Ant Eyes";
    craysim::add_ant_eye_spherical_projection<glver> (v, eyevm2.get(), 0);
    eyevm2->show_3d = false;
    eyevm2->setGamma (gamma_val);
    eyevm2->twodimensional (twodee);
    eyevm2->show_sphere = false;
    eyevm2->show_rays = false;
    sm::mat<float, 4> mflip (sm::quaternion<float>{0, 0, 1, 0});
    eyevm2->setViewMatrix (mflip);
    eyevm2->finalize();
    ep2 = veye.addVisualModel (eyevm2);
    ep2->scaleViewMatrix (1000);

    craysim::doublehexgrid<glver>* dhp = nullptr; // optional extra double hex of flat eyes
    if (v.sim_opts.test (craysim::options::eye_is_hex)) {
        auto dhg = std::make_unique<craysim::doublehexgrid<glver>> (&eye_hexgrid, sm::vec<>{});
        dhg->set_parent (veye.get_id());
        dhg->ommData = &v.ommatidia_datas[0];
        dhg->ommatidia = v.get_ommatidia_ptr(0); // gets repeatedly reset in craysim_visual
        dhg->show_flat = true; // couple of issues to solve here
        dhg->second_grid_flip_lr = true;   // flip the second grid left-right
        dhg->grid_offset = {0.00075f, 0}; // grid offset is applied in opposide senses to each grid
        dhg->setGamma (gamma_val);
        dhg->twodimensional (twodee);
        dhg->setViewMatrix (mflip);
        dhg->finalize();
        dhp = veye.addVisualModel (dhg);
        dhp->scaleViewMatrix (1000);
    }

    // An ant body to go in the scene
    auto av = std::make_unique<craysim::AntBodyVisual<glver>>();
    av->set_parent (v.get_id());
    av->draw_ring = true;
    av->finalize();
    v.agent_body = v.addVisualModel (av);
    v.agent_body->name = "ant";
    v.agent_body->setViewMatrix (v.initial_camera_space);

    // how we replay a crashed movement for debug
    if (v.sim_opts.test (craysim::options::debug_mv)) {
        try {
            v.do_crashed_movement ();
        } catch (const std::exception& e) {
            std::cout << "Exception moving: " << e.what() << std::endl;
            throw e;
        }
    }

    // Put all our 'other windows' in a container, which we pass in to render_and_poll()
    v.other_windows = { &vant, &veye };
    // Similar for our other eyes
    if (ep2 == nullptr) {
        v.other_eyes[0] = std::vector<craysim::compoundray::ommatidia_datamodel<glver>*>{ ep1 };
    } else {
        if (dhp == nullptr) {
            v.other_eyes[0] = std::vector<craysim::compoundray::ommatidia_datamodel<glver>*>{ ep1, ep2 };
        } else {
            std::cout << "Adding ep1, ep2 and dhp!\n";
            v.other_eyes[0] = std::vector<craysim::compoundray::ommatidia_datamodel<glver>*>{ ep1, ep2, dhp };
        }
    }

    // The main program loop
    while (!(v.readyToFinish() || vant.readyToFinish() || veye.readyToFinish())) {
        v.start_loop_timer(); // It's important to call this line at the start of the loop

        v.render_and_poll(); // Does all the render computations

        if (gv1p != nullptr) {
            //gv1p->reinitColours();
            gv1p->setVectorData (reinterpret_cast<std::vector<sm::vec<float>>*>(&v.ommatidia_datas[1]));
            gv1p->reinit();
            gv1p->render();
        }

        // Here is where you would work on the data for the last view in v.ommatidia_data;

        v.end_loop_timer(); // Mark that we got to the end of the loop
    }

    v.complete_recording();
}
