#pragma once

#include "cli.hpp" // for CommandResult, if needed
#include <memory>
#include "router/router.hpp"
#include <vector>
// Include forward declarations or actual headers for Topology & Router
// if they are not already included in cli.hpp
// e.g.:
// class Topology;
// class Router;

/**
 * An interactive function that lets users navigate through routers
 * and see path validation in real time.
 *
 * @param topology The shared pointer to the Topology to be explored.
 * @param start_router (Optional) A particular router from which to start.
 * @return A CommandResult describing the final outcome of the session.
 */
CommandResult interactiveRouterExplorer(std::shared_ptr<Topology> topology,
                                        std::shared_ptr<Router> start_router = nullptr);
