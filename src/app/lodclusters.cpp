#include "lodclusters.hpp"

namespace lodclusters {

// LodClusters is implemented by responsibility-specific translation units:
// - lodclusters_config.cpp: constructor, command-line parameters, and defaults.
// - lodclusters_scene.cpp: scene/cache loading, render-scene ownership, camera placement.
// - lodclusters_lifecycle.cpp: app lifecycle, framebuffer image binding, renderer lifetime.
// - lodclusters_runtime.cpp: runtime change handling and per-frame render dispatch.
// - lodclusters_ui.cpp: menus, settings, statistics, and viewport UI.

}  // namespace lodclusters
