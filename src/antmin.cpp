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

import craysim.visual;
import craysim.antbody;
import craysim.doublehexgrid;

import cater.helpers;

import antbrain.vision;
import antbody.vis;

namespace antmin
{
    // OpenGL 4.3 for Instanced VisualModels
    constexpr std::int32_t glver = mplot::gl::version_4_3;
}

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
    craysim::visual<antmin::glver> v (_w, _h, "AntPOV", prog_opts, default_samples, gamma_val);
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

    // Add an ant body to go in the scene
    auto av = std::make_unique<craysim::AntBodyVisual<antmin::glver>>();
    av->set_parent (v.get_id());
    av->draw_ring = true;
    av->finalize();
    v.agent_body = v.addVisualModel (av);
    v.agent_body->name = "ant";
    v.agent_body->setViewMatrix (v.initial_camera_space);

    // Make a second window for a fixed ant body view
    mplot::Visual<antmin::glver> vant (920, 920, "Ant view");
    vant.setSceneTrans (sm::vec<float,3>{ float{0.113123}, float{0.0217872}, float{-3.7961} });
    vant.setSceneRotation (sm::quaternion<float>{ float{0.937372}, float{0.106131}, float{0.330499}, float{0.0289824} });
    // Use a visualizer class to bundle up the work of drawing the ant
    antbody::visualizer<antmin::glver> antbody_vis (&v, &vant);

    // Make a third window for views of the eyes on their own
    mplot::Visual<antmin::glver> veye (920, 512, "Eye view");
    veye.setSceneTrans (sm::vec<float,3>{ float{-0.00859182}, float{-0.616208}, float{-1.18557} });
    veye.setSceneRotation (sm::quaternion<float>{ float{1}, float{0}, float{0}, float{0} });
    // This class (from module antbrain.vision): computes monochrome channels and applys any
    // filtering to make signals ready for a brain model
    antbrain::hexvision vision_data (&v);

    // Along with (also from module antbrain.vision):
    antbrain::hexvision_visualizer<antmin::glver> eyes_vis (&v, &veye, &vision_data);

    // This is a debug mechanism to replay and study a crashed movement on the NavMesh
    if (v.sim_opts.test (craysim::options::debug_mv)) {
        try {
            v.do_crashed_movement();
        } catch (const std::exception& e) {
            std::cout << "Exception moving: " << e.what() << std::endl;
            throw e;
        }
    }

    // Put all our 'other windows' into the 'other_windows' container in craysim::visual. This
    // ensures they see a render() call for each call to render_and_poll().
    v.other_windows = { &vant, &veye };
    // Put our 'other eyes' (these are separate representations of the same eye-view data that is
    // shown as the agent within the scene) in the 'other_eyes' container.
    v.other_eyes[0] = std::vector<craysim::compoundray::ommatidia_datamodel<antmin::glver>*>{ antbody_vis.ep1 };
    if (eyes_vis.ep != nullptr) { v.other_eyes[0].push_back (eyes_vis.ep); }

    // The main program loop
    while (!(v.readyToFinish() || vant.readyToFinish() || veye.readyToFinish())) {
        v.start_loop_timer(); // It's important to call this line at the start of the loop

        v.render_and_poll(); // Does all the render computations

        vision_data.process();
        eyes_vis.render();

        // Here is where you would work on the data for the last view in v.ommatidia_data;

        v.end_loop_timer(); // Mark that we got to the end of the loop
    }

    v.complete_recording();
}
