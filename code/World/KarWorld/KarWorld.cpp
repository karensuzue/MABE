//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "KarWorld.h"

shared_ptr<ParameterLink<int>> KarWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_Kar-evaluationsPerGeneration", 100,
    "How many steps to simulate per generation?");

shared_ptr<ParameterLink<int>> KarWorld::mapWidthPL =
    Parameters::register_parameter("WORLD_Kar-mapWidth", 50,
    "Width of world map.");

shared_ptr<ParameterLink<int>> KarWorld::mapHeightPL =
    Parameters::register_parameter("WORLD_Kar-mapHeight", 50,
    "Height of world map.");

shared_ptr<ParameterLink<double>> KarWorld::resDensityPL =
    Parameters::register_parameter("WORLD_Kar-resDensity", 0.05,
    "Starting resource density.");

shared_ptr<ParameterLink<double>> KarWorld::resGrowthRatePL =
    Parameters::register_parameter("WORLD_Kar-resGrowthRate", 0.01,
    "Rate of resource growth.");

shared_ptr<ParameterLink<double>> KarWorld::visionRadiusPL =
    Parameters::register_parameter("WORLD_Kar-visionRadius", 5.0,
    "Agent's vision radius.");

shared_ptr<ParameterLink<string>> KarWorld::visionModePL =
    Parameters::register_parameter("WORLD_Kar-visionMode", string("Nearest"), 
    "Agent's vision mode, [Nearest, Average]"); // [Nearest, Average]

// shared_ptr<ParameterLink<int>> KarWorld::numAgentsPL = 
//     Parameters::register_parameter("WORLD_Kar-numAgents", 2,
//     "Number of coexisting agents.");

KarWorld::KarWorld(shared_ptr<ParametersTable> PT) : AbstractWorld(PT) {
    evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);

    mapWidth = mapWidthPL->get(PT);
    mapHeight = mapHeightPL->get(PT);
    resDensity = resDensityPL->get(PT);
    resGrowthRate = resGrowthRatePL->get(PT);
    visionRadius = visionRadiusPL->get(PT);
    visionMode = visionModePL->get(PT);
    // numAgents = numAgentsPL->get(PT);
    
    // Initialize world map with resources, no agents added yet
    worldMap = WorldMap(mapWidth, mapHeight, resDensity, resGrowthRate, visionMode);

	popFileColumns.clear();
    popFileColumns.push_back("score");
}

// For each generation, we simulate the current population's lifetime.
// At the end of the generation, we collect fitness scores and allow MABE to update the population.
// We then reset the world, including resource placement and agent-organism linkages.
auto KarWorld::evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) -> void {
    std::vector<std::shared_ptr<Organism>> population = groups[groupName]->population;
    int popSize = population.size();

    assert(popSize < mapWidth * mapHeight && "Too many organisms!");

    worldMap.resetMap(); // reset resource placement, clear agents
    worldMap.addAgents(population); // reset agent positions, and form new agent-organism linkages

    assert(worldMap.agents.size() == popSize && "The number of agents is not equal to the population size!");

    // Reset all brains
    for (int i = 0; i < popSize; ++i) {
        population.at(i)->brains[brainName]->resetBrain();
    }

    // Let it rip
    for (int t = 0; t < evaluationsPerGeneration; ++t) {
        if (visualize /*&& t % 1000*/) {
            std::cout << worldMap;
        }

        // --------------------------------------------
        // Phase 1: sense and decide (don't move yet!!)
        // --------------------------------------------

        std::vector<int> rotationCmds(popSize, 0);
        std::vector<int> forwardCmds (popSize, 0);

        for (int i = 0; i < popSize; ++i) {
            Agent & agent = worldMap.agents.at(i);        
            Organism & org = *agent.org;
            auto & brain = org.brains[brainName]; // too lazy to figure out variable type

            assert(agent.org == population.at(i) && "Agent's org pointer must match the one in the population");

            // Sense the world, and set brain inputs
            auto inputs = worldMap.senseWorld(agent, visionRadius);
            for (int j = 0; j < inputs.size(); ++j) {
                brain->setInput(j, inputs[j]); // set input j
            }

            // Convert inputs to outputs
            brain->update();
            
            // Grab brain outputs (I think these are double values in CGP?)
            double rotateOut = brain->readOutput(0);
            double forwardOut = brain->readOutput(1);

            // Convert brain outputs into actual commands (I don't know if this is going to work!)
            int rotationCmd = Trit(rotateOut); // -1 (left), 0 (no turn), 1 (right)
            int forwardCmd = Bit(forwardOut); // 0 (don't move), 1 (move one step forward)

            // Store the commands for now
            rotationCmds.at(i) = rotationCmd;
            forwardCmds.at(i) = forwardCmd;
        }

        // --------------------------------------------
        // Phase 2: move
        // --------------------------------------------

        for (int i = 0; i < popSize; ++i) {
            worldMap.stepAgent(worldMap.agents.at(i), rotationCmds.at(i), forwardCmds.at(i));
        }

        // Random resource regrowth
        worldMap.growResource();
    } 

    for (int i = 0; i < popSize; ++i) {
        population.at(i)->dataMap.set("score", worldMap.agents.at(i).fitness);
    }

}

// The requiredGroups function lets MABE know how to set up populations of organisms that this world needs
// 6 inputs for vision, 2 outputs for rotation and forward commands
auto KarWorld::requiredGroups() -> unordered_map<string,unordered_set<string>> {
	return { { groupName, { "B:" + brainName + ",6,2" } } }; 
}




