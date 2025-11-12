#pragma once

#define _USE_MATH_DEFINES

#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <string>
#include "../../Utilities/Random.h"
#include "../../Organism/Organism.h"

// TODO: This world currently supports a single resource type. 
// Add support for multiple resource types later.
// How should they be differentiated? Hmm...
// Perhaps by associating resources with logical operators?

// ---------------------------------------------------
// Interface for Organisms to interact with the world 
// ---------------------------------------------------
class Agent {
public:
    std::shared_ptr<Organism> org; // pointer to the actual evolving Organism (contains genome, brain)
    int row, col; // position
    int fitness = 0; // accumulated resources

    // 0 is North, 1 is East, 2 is South, 3 is West
    int facingDir;

    Agent(std::shared_ptr<Organism> o, int r = 0, int c = 0) 
        : org(o), row(r), col(c) {}

    // Links agent to an actual Organism
    void linkOrganism(std::shared_ptr<Organism> o) {
        org = o;
    }
};

// ---------------------------------------------------
// A single cell in the world
// ---------------------------------------------------
struct Cell {
    bool resource = false;   // true if this cell currently has food
    Agent* occupant = nullptr; // pointer to agent (nullptr if empty)
};

// ---------------------------------------------------
// Toroidal world map
// ---------------------------------------------------

static constexpr int dRow[4] = {-1, 0, 1, 0}; // N, E, S, W
static constexpr int dCol[4] = { 0, 1, 0, -1};

enum class VisionMode {
    NEAREST, // vision sensors only display values for nearest resources/agents 
    AVERAGE // vision sensors compute an average for resources/agents seen within the radius
};

class WorldMap {
public:
    int width, height;
    double resDensity; // resource density probability
    double resGrowthRate; // resource regrowth rate
    // int popSize;
    std::vector<std::vector<Cell>> grid; // vector of rows of Cells
    std::vector<Agent> agents;

    VisionMode visionMode;

    WorldMap() : width(0), height(0), resDensity(0.0), resGrowthRate(0.0), 
        visionMode(VisionMode::NEAREST) {}

    WorldMap(int w, int h, double density, double grow, std::string vismode) 
        : width(w), height(h), 
        resDensity(density), resGrowthRate(grow),
        grid(h, std::vector<Cell>(w))
    {   
        // Place sparse resources
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                double p = Random::getDouble(0.0, 1.0);
                grid.at(r).at(c).resource = (p < resDensity);
            }
        }

        if (vismode == "Nearest") visionMode = VisionMode::NEAREST;
        else if (vismode == "Average") visionMode = VisionMode::AVERAGE;
    }

    void growResource() {
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                double p = Random::getDouble(0.0, 1.0);
                if (!grid.at(r).at(c).resource && p < resGrowthRate) {
                    grid.at(r).at(c).resource = true;
                }
            }
        }
    }

    void resetResource() {
        // Place sparse resources
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                double p = Random::getDouble(0.0, 1.0);
                grid.at(r).at(c).resource = (p < resDensity);
            }
        }
    }

    void addAgents(std::vector<std::shared_ptr<Organism>> population) {
        // Add agents randomly and link them to existing Organisms
        // TODO: Prevent too many agents from spawning into the same cell
        for (int i = 0; i < population.size(); ++i) {
            int r = Random::getInt(0, height - 1);
            int c = Random::getInt(0, width - 1);
            agents.emplace_back(population.at(i), r, c);
            grid.at(r).at(c).occupant = &agents.back();

            // Agent gets lucky!
            if (grid.at(r).at(c).resource) {
                grid.at(r).at(c).resource = false;
                agents.back().fitness += 1; // TODO: we can vary this later
            };
        }
    }

    // void linkAgents() {
    //     // I guess this only works if population size is constant
    //     assert(population.size() == agents.size()); 
    //     for (int i = 0; i < population.size(); ++i) {
    //         agents.at(i).linkOrganism(population.at(i));
    //     }
    // }

    // void resetAgents() {
    //     for (int i = 0; i < agents.size(); ++i) { 
    //         // We should maintain the linkage
    //         // agents.at(i).org.reset();

    //         // Reset position
    //         int row = Random::getInt(0, height - 1);
    //         int col = Random::getInt(0, width - 1);
    //         agents.at(i).row = row;
    //         agents.at(i).col = col;

    //         // Reset fitness
    //         agents.at(i).fitness = 0;

    //         // Agent gets lucky!
    //         if (grid.at(row).at(col).resource) {
    //             grid.at(row).at(col).resource = false;
    //             agents.at(i).fitness += 1; // TODO: we can vary this later
    //         };
    //     }
    // }

    void resetMap() {
        resetResource();
        agents.clear();
    }

    void display(std::ostream & os) const {
        /*
        Sample grid:
            0  1  2  3  4 
          +---------------+
        0 | .  R  .  .  . |
        1 | .  .  ^  .  . |
        2 | .  R  .  .  R |
        3 | .  .  .  .  . |
        4 | R  .  .  .  . |
          +---------------+
        */

        // Horizontal indices
        os << "    ";
        for (int c = 0; c < width; ++c) {
            os << std::setw(2) << c << " ";
        }
        os << "\n";

        // Top border
        os << "   +" << std::string(width * 3, '-') << "+\n";

        for (int r = 0; r < height; ++r) {
            // Row index, left border
            os << std::setw(2) << r << " |"; 

            for (int c = 0; c < width; ++c) {
                const Cell & cell = grid.at(r).at(c);

                char symbol = '.'; // default

                if (cell.occupant != nullptr) {
                    const Agent & agent = *(grid.at(r).at(c).occupant);
                    if (agent.facingDir == 0) symbol = '^'; // North
                    else if (agent.facingDir == 1) symbol = '>'; // East
                    else if (agent.facingDir == 2) symbol = 'v'; // South
                    else if (agent.facingDir == 3) symbol = '<'; // West
                }
                else if (cell.resource) symbol = 'R';
                
                os << " " << symbol << " ";
            }
            os << "|\n"; // End of row
        }

        // Bottom border
        os << "   +" << std::string(width * 3, '-') << "+\n";
    }

    friend std::ostream & operator<<(std::ostream & os, const WorldMap & map) {
        map.display(os);
        return os;
    }

    // TODO: Rotation and forward commands are continuous values output from Organism brains?
    // Rotation commands are 1, 0, -1 (right, no turn, left)
    // Forward commands are 0, 1 (stay or move forward one cell)
    void stepAgent(Agent & agent, int rotationCmd, int forwardCmd) {
        // Remove from old position
        grid.at(agent.row).at(agent.col).occupant = nullptr;

        // Rotate agent
        agent.facingDir = (agent.facingDir + rotationCmd + 4) % 4;

        // Move with toroidal wrap (and update the agent's internal position)
        agent.col = (agent.col + dCol[agent.facingDir] * forwardCmd + width) % width;
        agent.row = (agent.row + dRow[agent.facingDir] * forwardCmd + height) % height;

        // Check for resource
        Cell & c = grid.at(agent.row).at(agent.col);
        if (c.resource) {
            c.resource = false;
            agent.fitness += 1; // TODO: we can vary this later
            std::cout << "ATE \n" << std::endl;
        }
        // Occupy new cell
        c.occupant = &agent;
    }

    // Agents can perceive what's ahead and to the sides (via a 180 degrees arc)
    // Their field of view is divided into three cones (6 inputs for Organisms)
    // This function returns the "intensity" of nearby resources and agents in each cone
    std::vector<double> senseWorld(Agent & agent, double visionRadius) {
        std::vector<double> resourceSignal(3, 0.0); 
        std::vector<double> agentSignal(3, 0.0);

        // Record the number of resources/agents in each cone (for average vision)
        std::vector<int> resourceCount(3, 0);
        std::vector<int> agentCount(3, 0);

        // To cover all bases, investigate all nearby cells 
        // within a (visionRadius*2+1) x (visionRadius*2+1) box
        // These indices are relative to the agent's current position
        for (int r = -visionRadius; r <= visionRadius; ++r) {
            for (int c = -visionRadius; c <= visionRadius; ++c) {
                // Skip agent's current position
                if (r == 0 && c == 0) continue;

                // Skip cells not within agent's vision radius
                double dist = std::sqrt(r*r + c*c); // distance from agent
                if (dist > visionRadius) continue;
                
                // Skip if cell is behind you
                if (agent.facingDir == 0 && r > 0) continue; // N
                if (agent.facingDir == 1 && c < 0) continue;  // E
                if (agent.facingDir == 2 && r < 0) continue;  // S
                if (agent.facingDir == 3 && c > 0) continue; // W

                // Global, toroidal wrapped indices for cell
                int rWrapped = (agent.row + r + height) % height;
                int cWrapped = (agent.col + c + width) % width;

                // Compute angle of current cell (relative to facing direction)
                // Also rotate coordinates
                double angle = 0.0; // in radians
                if (agent.facingDir == 0) angle = std::atan2(-c, -r); // N
                if (agent.facingDir == 1) angle = std::atan2(-r, c); // E
                if (agent.facingDir == 2) angle = std::atan2(c, r); // S
                if (agent.facingDir == 3) angle = std::atan2(r, -c); // W

                // Assign current cell to a cone
                int cone = -1;
                if (angle < -M_PI/6.0 && angle >= -M_PI/2.0) cone = 2; // right-front
                else if (angle >= -M_PI/6.0 && angle <= M_PI/6.0) cone = 1; // center-front
                else if (angle > M_PI/6.0 && angle <= M_PI/2.0) cone = 0; // left-front
                if (cone == -1) continue;
                assert(cone > -1 && "WorldMap.hpp: cell couldn't be assigned to a vision cone.");

                // Compute "intensity" score for the current cell 
                double intensity = 1 / (dist + 1);
                Cell & cell = grid.at(rWrapped).at(cWrapped);

                // Retain "intensity" of NEAREST resources and agents in each cone
                // NOTE: If a resource or agent is 1 square away, intensity is 0.5
                if (visionMode == VisionMode::NEAREST) {
                    if (cell.resource) {
                        // Replace with nearest (largest) signals
                        resourceSignal.at(cone) = std::max(resourceSignal.at(cone), intensity);
                    }     
                    if (cell.occupant) {
                        agentSignal.at(cone) = std::max(agentSignal.at(cone), intensity);
                    }
                }

                // Average over all resources and agents seen in each cone
                else if (visionMode == VisionMode::AVERAGE) {
                    if (cell.resource) {
                        resourceSignal.at(cone) += intensity;
                        resourceCount.at(cone) += 1;
                    }
                    if (cell.occupant) {
                        agentSignal.at(cone) += intensity;
                        agentCount.at(cone) += 1;
                    }
                }

                // TODO: other vision modes??
            }
        }
        
        // Compute averages
        if (visionMode == VisionMode::AVERAGE) {
            for (int i = 0; i < 3; ++i) {
                if (resourceCount.at(i) > 0) resourceSignal.at(i) /= resourceCount.at(i);
                if (agentCount.at(i) > 0) agentSignal.at(i) /= agentCount.at(i);
            }
        }

        // Concatenate
        resourceSignal.insert(resourceSignal.end(), agentSignal.begin(), agentSignal.end());
        return resourceSignal;  // [R_left, R_center, R_right, A_left, A_center, A_right]
    }
};

