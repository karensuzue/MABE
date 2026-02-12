#pragma once

#define _USE_MATH_DEFINES

#include <vector>
#include <array>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <string>
#include "../../Utilities/Random.h"
#include "../../Organism/Organism.h"

// TODO: This world currently supports a single resource type. 
// Add support for multiple resource types later.
// How should they be differentiated? Hmm...
// Perhaps by associating resources with logical operators?

// TODO: Consistent naming convention (but I love snake case...)

// ---------------------------------------------------
// Interface for Organisms to interact with the world 
// ---------------------------------------------------
class Agent {
public:
    std::shared_ptr<Organism> org; // pointer to the actual evolving Organism (contains genome, brain)
    int row, col;

    // SIZE RULE NOTES:
    // Fitness is accumulated resources, but currently doubles as
    // "stored calories" in a prey that gets copied to the predator that consumed it
    // Once dead, an agent can't increase their fitness, but still retains what they've consumed while alive
    int fitness = 0; 
    int size = 0; // for size rule
    bool alive = true; // book keeping
    // int energy = 100; // TODO: some kind of energy cool down
    
    // 0 is North, 1 is East, 2 is South, 3 is West
    int facingDir = 0;


    // TODO: randomize facingDir? 
    Agent(std::shared_ptr<Organism> o , int r = 0, int c = 0, int facing = 0) 
        : org(o) , row(r), col(c), facingDir(facing) {}

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
    // Agent* occupant = nullptr; // nullptr if empty, only 1 occupant per cell
    int occupant_id = -1;// -1 means empty, otherwise index into agents
    // int cooldown = 0; // number of steps until resources can regrow at this cell
};

// ---------------------------------------------------
// Toroidal world map
// ---------------------------------------------------

// Read as (dRow, dCol)
constexpr std::array<int, 4> dRow = {-1, 0, 1, 0}; // N, E, S, W
constexpr std::array<int, 4> dCol = { 0, 1, 0, -1};

enum class VisionMode {
    NEAREST, // vision sensors only display values for nearest resources/agents 
    AVERAGE // vision sensors compute an average for resources/agents seen within the radius
};

class WorldMap {
public:
    int width, height;

    // std::vector<std::vector<Cell>> grid; // vector of rows of Cells
    std::vector<Cell> grid; // flat representation
    std::vector<Agent> agents;

    double resDensity; // resource density probability
    // double resGrowthRate; // global resource regrowth rate
    int totalResCount = 0; // total resource count

    // Number of world steps before a resource reappears in a cell
    int minResCooldown; 
    int maxResCooldown; 

    VisionMode visionMode; // [Nearest, Average]

    bool allowSizeRule;
    bool allowAgeRule;
    int eatingCost;


    WorldMap() : width(0), height(0), resDensity(0.0), /*resGrowthRate(0.0),*/ 
        visionMode(VisionMode::NEAREST) {}

    WorldMap(int w, int h, double density, int minCool, int maxCool, 
            std::string vismode, bool size, bool age/*, int eatCost*/) 
        : width(w), height(h), 
        resDensity(density), /*resGrowthRate(grow),*/
        minResCooldown(minCool), maxResCooldown(maxCool),
        allowSizeRule(size), allowAgeRule(age),
        // eatingCost(eatCost),
        grid(w * h)
    {   
        assert(w > 0 && h > 0 && "Grid dimensions cannot be 0!");
        assert(minResCooldown > 0 && "Minimum resource cooldown cannot be 0!");
        assert(maxResCooldown > 0 && "Maximum resource cooldown cannot be 0!");

        if (vismode == "Nearest") visionMode = VisionMode::NEAREST;
        else if (vismode == "Average") visionMode = VisionMode::AVERAGE;
    }

    // Easy access for flat world representation
    Cell & at(int r, int c) { return grid.at(r * width + c); }
    const Cell & at(int r, int c) const { return grid.at(r * width + c); } 

    bool isOccupied(const Cell & c) const { return c.occupant_id != -1; }

    // Rain in resources at random locations
    // void initResourcesByShuffle() {
    //     const int N = width * height;

    //     // Clamp in case std::lround exceeds N.... I don't know
    //     // std::clamp expects int instead of long int, which is what std::lround returns
    //     const int expectedResCount = std::clamp(static_cast<int>(std::lround(resDensity * N)), 0, N);

    //     // Reset
    //     totalResCount = 0;
    //     for (int i = 0; i < N; ++i) {
    //         grid.at(i).resource = false;
    //         grid.at(i).cooldown = Random::getInt(minResCooldown, maxResCooldown);
    //     }

    //     // Shuffle indices
    //     std::vector<int> idxs(N);
    //     std::iota(idxs.begin(), idxs.end(), 0);
    //     std::shuffle(idxs.begin(), idxs.end(), Random::getCommonGenerator());

    //     // Set first K cells to have resources
    //     for (int j = 0; j < expectedResCount; ++j) {
    //         Cell & cell = grid.at(idxs.at(j));
    //         cell.resource = true;
    //         ++totalResCount;
    //         cell.cooldown = 0; 
    //     }
    // }

    // Add agents randomly and link them to existing Organisms
    // void initAgentsByShuffle(const std::vector<std::shared_ptr<Organism>> & population) {
    //     const int N = width * height;
    //     const int popSize = static_cast<int>(population.size());
    //     assert(popSize <= N && "There are too many organisms and too few cells!");
        
    //     // Reset
    //     agents.clear();
    //     for (int i = 0; i < N; ++i) {
    //         grid.at(i).occupant = nullptr;
    //     }

    //     // Shuffle grid indices
    //     std::vector<int> idxs(N);
    //     std::iota(idxs.begin(), idxs.end(), 0);
    //     std::shuffle(idxs.begin(), idxs.end(), Random::getCommonGenerator());

    //     // Assign first popSize indices to agents
    //     for (int i = 0; i < popSize; ++i) {
    //         int idx = idxs.at(i);
    //         int r = idx / width;
    //         int c = idx % width;

    //         agents.emplace_back(population.at(i), r, c);
    //         grid.at(idx).occupant = &agents.back();

    //         // If spawned in cell has resource (lucky you!!)
    //         if (grid.at(idx).resource) {
    //             grid.at(idx).resource = false; // consumed
    //             --totalResCount; 
    //             grid.at(idx).cooldown = Random::getInt(minResCooldown, maxResCooldown);
    //             agents.back().fitness += 1; // TODO: we can vary this later
    //         }
    //     }
    // }

    // Resources initialize in the left side of the map
    // Cooldown is turned off here
    void initResourcesLeft(int gapW) {
        assert(gapW <= width - 2 && "Gap too large, need at least 1 column per side.");

        const int zoneW = (width - gapW) / 2; // width of resource zone
        assert(zoneW > 0 && "Resource zone width is 0.");

        const int N = width * height;

        // Clamp in case std::lround exceeds N.... I don't know
        // std::clamp expects int instead of long int, which is what std::lround returns
        const int expectedResCount = std::clamp(static_cast<int>(std::lround(resDensity * N)), 0, N);
        assert(expectedResCount <= zoneW * height && "There are too many resources and too few cells at initialization!");

        // Reset
        totalResCount = 0;
        for (int i = 0; i < N; ++i) {
            grid.at(i).resource = false;
            // grid.at(i).cooldown = Random::getInt(minResCooldown, maxResCooldown);
        }

        // Find indices that sit in the left side of the map
        std::vector<int> grid_idxs;
        grid_idxs.reserve(zoneW * height);
        for (int i = 0; i < N; ++i) {
            int col = i % width;
            if (col < zoneW) grid_idxs.push_back(i);
        }
        std::shuffle(grid_idxs.begin(), grid_idxs.end(), Random::getCommonGenerator());

        // Assign resources to the first 'expectedResCount' cells
        for (int i = 0; i < expectedResCount; ++i) {
            int idx = grid_idxs.at(i);
            Cell & cell = grid.at(idx);
            cell.resource = true;
            ++totalResCount;
        }
    }

    // Agents initialize in the right side of the map
    // Cooldown is turned off here
    void initAgentsRight(int gapW, const std::vector<std::shared_ptr<Organism>> & population) {
        assert(gapW <= width - 2 && "Gap too large, need at least 1 column per side.");

        const int zoneW = (width - gapW) / 2; // width of agents zone
        assert(zoneW > 0 && "Agent zone width is 0.");

        const int popSize = static_cast<int>(population.size());
        assert(popSize <= zoneW * height && "There are too many organisms and too few cells at initialization!");

        const int N = width * height;

        // Reset
        agents.clear();
        agents.reserve(popSize);  
        for (int i = 0; i < N; ++i) {
            grid.at(i).occupant_id = -1; // -1 signals empty cell
        }

        // Find indices that sit in the right side of the map
        std::vector<int> grid_idxs;
        grid_idxs.reserve(zoneW * height);
        for (int i = 0; i < N; ++i) {
            int col = i % width;
            if (col >= zoneW + gapW) { 
                grid_idxs.push_back(i);
            }
        }
        std::shuffle(grid_idxs.begin(), grid_idxs.end(), Random::getCommonGenerator());

        // Assign first popSize grid_idxs to agents
        for (int i = 0; i < popSize; ++i) {
            int idx = grid_idxs.at(i);
            int r = idx / width;
            int c = idx % width;

            agents.emplace_back(population.at(i), r, c);
            grid.at(idx).occupant_id = i;
        }
    }

    // TODO: better visualizer in python, play frames from .txt file, wait between frames
    // even better: arrow key to swithc between frames
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
                const Cell & cell = (*this).at(r, c);

                char symbol = ' '; // default

                if (isOccupied(cell)) {
                    const Agent & agent = agents.at(cell.occupant_id);
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

    // Helper function for updateResourcesPerStep
    // Picks a random cell INDEX to respawn resources in the map
    int pickRandomResourceCell() {
        std::vector<int> candidateIdxs;
        candidateIdxs.reserve(width * height); // increase capacity

        // Record unoccupied cells without resources
        for (int i = 0; i < width * height; ++i) {
            const Cell & cell = grid.at(i);
            if (!cell.resource && !isOccupied(cell)) { 
                candidateIdxs.push_back(i);
            }
        }
        if (candidateIdxs.empty()) {
            return -1;  // signal failure 
        }

        int chosen = Random::getIndex(candidateIdxs.size());
        return candidateIdxs[chosen];
    }

    // Resources must wait for cooldown and can't rain in locations occupied by agents
    // Resources also has a pSameCell chance of respawning in the same location
    // void updateResourcesPerStep(double pSameCell) {
    //     const int N = width * height;

    //     // If the world is saturated with resources, return
    //     double currentDensity = static_cast<double>(totalResCount) / 
    //                             static_cast<double>(width * height);
    //     if (currentDensity >= resDensity) {
    //         return; 
    //     }

    //     for (int i = 0; i < N; ++i) {
    //         Cell & cell = grid.at(i);

    //         // Just making sure...
    //         if (cell.resource) {  
    //             assert(cell.cooldown == 0 && "A cell that contains a resource cannot have a cooldown.");
    //             continue;
    //         }

    //         // Cell is not ready to regrow
    //         if (!cell.resource && cell.cooldown > 0) {
    //             cell.cooldown--;
    //         }

    //         // Cell is ready to regrow
    //         else if (!cell.resource && cell.cooldown == 0) { 
    //             bool sameCell = Random::P(pSameCell);
    //             if (sameCell && cell.occupant == nullptr) { // respawn resource in the same cell
    //                 cell.resource = true;
    //                 ++totalResCount;   
    //             }
    //             else { // find a different cell
    //                 int newCellID = pickRandomResourceCell();
    //                 if (newCellID != -1) {
    //                     grid.at(newCellID).resource = true; 
    //                     ++totalResCount;   
    //                 }
    //             }
    //         }
    //     }
    // }

    std::array<int, 4> getNeighbors(int center) {
        assert(center >= 0 && center < width * height);
        int row = center / width;
        int col = center % width;
        
        std::array<int, 4> neighbors;
        neighbors.at(0) = ((row - 1 + height) % height) * width + col; // up
        neighbors.at(1) = ((row + 1) % height) * width + col; // down
        neighbors.at(2) = row * width + ((col - 1 + width) % width); // left
        neighbors.at(3) = row * width + ((col + 1) % width); // right

        return neighbors;
    }

    // Resources respawn in patches by seeding one empty cell,
    // then expanding outward using a BFS‑like growth heuristic.
    // This occurs when resource levels fall below the target.
    // Goes with initResourcesLeft() and initAgentsRight()
    // Cooldown is turned off here
    void updateResourcesPerStep2() {
        const int N = width * height;
        const int expectedResCount = std::clamp(static_cast<int>(std::lround(resDensity * N)), 0, N);

        if (totalResCount >= expectedResCount) return;

        int neededResCount = expectedResCount - totalResCount;
        
        // Our "seed" (an empty, unoccupied cell)
        const int seed = pickRandomResourceCell();
        if (seed == -1) return; 
        grid.at(seed).resource = true;
        ++totalResCount;
        --neededResCount;
        if (neededResCount <= 0) return;

        // Grow outward!
        std::vector<int> frontier;
        frontier.reserve(std::min(N, neededResCount + 1));
        frontier.push_back(seed);
        while (neededResCount > 0 && !frontier.empty()) {
            const int f_idx = Random::getIndex(static_cast<int>(frontier.size()));
            const int current = frontier.at(f_idx);

            std::array<int, 4> neighbors = getNeighbors(current);
            std::shuffle(neighbors.begin(), neighbors.end(), Random::getCommonGenerator());

            bool placed = false;
            for (int neighbor : neighbors) {
                // Take the first valid neighbor only for randomness 
                Cell & nc = grid.at(neighbor);
                if (!nc.resource && !isOccupied(nc)) {
                    nc.resource = true;
                    ++totalResCount;
                    --neededResCount;
                    frontier.push_back(neighbor);
                    placed = true;
                    break;
                }
            }

            if (!placed) {
                // 'f_idx' may not be .back(), doing this retains a copy of the last (and potentially good) candidate
                frontier[f_idx] = frontier.back();
                frontier.pop_back();
            }
        }
    }

    // TODO: Rotation and forward commands are continuous values output from Organism brains?
    // Rotation commands are -1, 0, 1 (left, no turn, right)
    // Forward commands are 0, 1 (stay or move forward one cell)
    // Let agent move if there is no other agent blocking it, otherwise only rotation is possible
    void stepAgent(int agent_id, int rotationCmd, int forwardCmd) {
        Agent & agent = agents.at(agent_id);

        // Rotate agent
        agent.facingDir = (agent.facingDir + rotationCmd + 4) % 4;

        // Ensure toroidal wrap for new location
        int new_row = (agent.row + dRow[agent.facingDir] * forwardCmd + height) % height;
        int new_col = (agent.col + dCol[agent.facingDir] * forwardCmd + width) % width;

        Cell & dest = (*this).at(new_row, new_col);

        // Ensure new location is not occupied
        if (!isOccupied(dest)) {
            // Remove agent from its old position
            (*this).at(agent.row, agent.col).occupant_id = -1;

            // Update the agent's internal position
            agent.col = new_col;
            agent.row = new_row;
            
            // Check for resource to consume
            if (dest.resource) { 
                // Consume resource
                agent.fitness += 1; // TODO: we can vary this later
                dest.resource = false;

                // Random resource cooldown
                // c.cooldown = Random::getInt(minResCooldown, maxResCooldown);
                --totalResCount;
                // std::cout << "ATE \n" << std::endl;
            }

            // Let agent occupy new cell
            dest.occupant_id = agent_id;
        }
    }

    // Agents can perceive what's ahead and to the sides (via a 180 degrees arc)
    // Their field of view is divided into three cones (6 inputs for Organisms)
    // This function returns the "intensity" of nearby resources and agents in each cone
    std::vector<double> senseWorld(Agent & agent, double visionRadius) {
        std::vector<double> resourceSignal(3, 0.0); // TODO: maybe change this to std::array
        std::vector<double> agentSignal(3, 0.0);

        // Record the number of resources/agents in each cone (for AVERAGE vision)
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
                Cell & cell = (*this).at(rWrapped, cWrapped);

                // Retain "intensity" of NEAREST resources and agents in each cone
                // NOTE: If a resource or agent is 1 square away, intensity is 0.5
                if (visionMode == VisionMode::NEAREST) {
                    if (cell.resource) {
                        // Replace with nearest (largest) signals
                        resourceSignal.at(cone) = std::max(resourceSignal.at(cone), intensity);
                    }     
                    if (isOccupied(cell)) {
                        agentSignal.at(cone) = std::max(agentSignal.at(cone), intensity);
                    }
                }

                // Average over all resources and agents seen in each cone
                else if (visionMode == VisionMode::AVERAGE) {
                    if (cell.resource) {
                        resourceSignal.at(cone) += intensity;
                        resourceCount.at(cone) += 1;
                    }
                    if (isOccupied(cell)) {
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

        // Concatenate resourceSignal and agentSignal
        resourceSignal.insert(resourceSignal.end(), agentSignal.begin(), agentSignal.end());
        return resourceSignal;  // [R_left, R_center, R_right, A_left, A_center, A_right]
    }



};

