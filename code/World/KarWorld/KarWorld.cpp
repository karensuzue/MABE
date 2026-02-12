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
    Parameters::register_parameter("WORLD_Kar-evaluationsPerGeneration", 10,
    "How many lifetimes to simulate per generation?");

shared_ptr<ParameterLink<int>> KarWorld::worldStepsPL =
    Parameters::register_parameter("WORLD_Kar-worldSteps", 100,
    "How many world steps to simulate per lifetime?");

shared_ptr<ParameterLink<int>> KarWorld::mapWidthPL =
    Parameters::register_parameter("WORLD_Kar-mapWidth", 50,
    "Width of world map.");

shared_ptr<ParameterLink<int>> KarWorld::mapHeightPL =
    Parameters::register_parameter("WORLD_Kar-mapHeight", 50,
    "Height of world map.");

shared_ptr<ParameterLink<double>> KarWorld::resDensityPL =
    Parameters::register_parameter("WORLD_Kar-resDensity", 0.05,
    "Starting resource density.");

shared_ptr<ParameterLink<int>> KarWorld::gapWidthPL =
    Parameters::register_parameter("WORLD_Kar-gapWidth", 1,
    "Number of columns between resource and agent zones at initialization.");

// shared_ptr<ParameterLink<double>> KarWorld::resGrowthRatePL =
//     Parameters::register_parameter("WORLD_Kar-resGrowthRate", 0.01,
//     "Rate of resource growth.");

shared_ptr<ParameterLink<int>> KarWorld::minResCooldownPL =
    Parameters::register_parameter("WORLD_Kar-minResCooldown", 10,
    "Minimum number of world steps before a resource can reappear in a cell.");

shared_ptr<ParameterLink<int>> KarWorld::maxResCooldownPL =
    Parameters::register_parameter("WORLD_Kar-maxResCooldown", 50,
    "Maximum number of world steps before a resource can reappear in a cell.");

shared_ptr<ParameterLink<double>> KarWorld::resSiteFidelityPL = 
    Parameters::register_parameter("WORLD_Kar-resSiteFidelity", 1.0,
    "Probability of resource spawning in the original cell.");

shared_ptr<ParameterLink<double>> KarWorld::visionRadiusPL =
    Parameters::register_parameter("WORLD_Kar-visionRadius", 5.0,
    "Agent's vision radius.");

shared_ptr<ParameterLink<string>> KarWorld::visionModePL =
    Parameters::register_parameter("WORLD_Kar-visionMode", string("Nearest"), 
    "Agent's vision mode, [Nearest, Average]");

shared_ptr<ParameterLink<bool>> KarWorld::allowSizeRulePL =
    Parameters::register_parameter("WORLD_Kar-allowSizeRule", false, 
    "Toggle size-based predation rule, [true, false]");

shared_ptr<ParameterLink<bool>> KarWorld::allowAgeRulePL =
    Parameters::register_parameter("WORLD_Kar-allowAgeRule", false, 
    "Toggle age-based predation rule, [true, false]");

// shared_ptr<ParameterLink<int>> KarWorld::eatingCostPL =
//     Parameters::register_parameter("WORLD_Kar-eatingCost", 1,
//     "Fitness cost of consuming another agent. Only applies when one of the predation rules is enabled");


// shared_ptr<ParameterLink<int>> KarWorld::numAgentsPL = 
//     Parameters::register_parameter("WORLD_Kar-numAgents", 2,
//     "Number of coexisting agents.");

KarWorld::KarWorld(shared_ptr<ParametersTable> PT) : AbstractWorld(PT) {
    evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT);
    worldSteps = worldStepsPL->get(PT);

    mapWidth = mapWidthPL->get(PT);
    mapHeight = mapHeightPL->get(PT);
    resDensity = resDensityPL->get(PT);
    gapWidth = gapWidthPL->get(PT);
    // resGrowthRate = resGrowthRatePL->get(PT);

    minResCooldown = minResCooldownPL->get(PT);
    maxResCooldown = maxResCooldownPL->get(PT);
    assert(maxResCooldown < worldSteps && "maxResCooldown cannot be larger or equal to worldSteps!");

    visionRadius = visionRadiusPL->get(PT);
    visionMode = visionModePL->get(PT);

    allowSizeRule = allowSizeRulePL->get(PT);
    allowAgeRule = allowAgeRulePL->get(PT);
    // numAgents = numAgentsPL->get(PT);
    
    // Initialize world map with resources, no agents added yet
    worldMap = WorldMap(mapWidth, mapHeight, resDensity, minResCooldown, maxResCooldown, 
                        visionMode, allowSizeRule, allowAgeRule);

	popFileColumns.clear();
    popFileColumns.push_back("score");
}

// For each generation, we run several simulations of the current population's lifetime.
// At the end of the generation, we collect average fitness scores and allow MABE to update the population.
// We then reset the world, including resource placement and agent-organism linkages.
auto KarWorld::evaluate(map<string, shared_ptr<Group>>& groups, int analyze, int visualize, int debug) -> void {
    // std::cout << "Global::update = " << Global::update << "\n";
    // std::cout << "visualize = " << visualize << " analyze = " << analyze << "\n";

    std::vector<std::shared_ptr<Organism>> population = groups[groupName]->population;
    const int popSize = static_cast<int>(population.size());

    // Sum of agent scores across lifetime simulations (fitness per generation of an organism)
    std::vector<int> agentScores(popSize, 0);

    // Simulate several 'lifetimes' to correct for chance
    for (int t = 0; t < evaluationsPerGeneration; ++t) { 
        // Reset and initialize map with resources and agent-organism linkages
        // worldMap.initResourcesByShuffle();
        // worldMap.initAgentsByShuffle(population);
        worldMap.initResourcesLeft(gapWidth);
        worldMap.initAgentsRight(gapWidth, population); 

        // Make sure all organisms are linked with an agent
        assert(worldMap.agents.size() == popSize && "The number of agents is not equal to the population size!");

        // Reset all brains
        for (int i = 0; i < popSize; ++i) {
            population.at(i)->brains[brainName]->resetBrain();
        }

        // Display the initial world once at the beginning of a lifetime
        if (visualize) {
            // std::cout << worldMap;
            // visualizeData = "** Simulation " + std::to_string(t) + "**\n"; 
            visualizeData += "**Initialize World**\n";
            ss << worldMap;
            visualizeData += ss.str();
            FileManager::openAndWriteToFile("KarWorldData-G" 
                                + std::to_string(Global::update) 
                                + "-L" 
                                + std::to_string(t) + ".txt", visualizeData);

            ss.str(""); // clear internal buffer
            ss.clear(); // clear error state flags
            visualizeData = ""; // sure why not
        }
        
        // Step through a single lifetime
        for (int step = 0; step < worldSteps; ++step) {
            // --------------------------------------------
            // Phase 1: sense and decide (don't move yet!!)
            // --------------------------------------------

            // Rotation commands are -1, 0, 1 (left, no turn, right)
            // Forward commands are 0, 1 (stay or move forward one cell)
            std::vector<int> rotationCmds(popSize, 0);
            std::vector<int> forwardCmds (popSize, 0);

            for (int i = 0; i < popSize; ++i) {
                Agent & agent = worldMap.agents.at(i);        
                Organism & org = *(agent.org);
                auto & brain = org.brains[brainName]; // too lazy to figure out variable type

                assert(agent.org == population.at(i) && "Agent's org pointer must match the one in the population!");

                // COMMENT OUT IF HUMAN 
                // Write per-agent information to file
                // if (visualize && t == 0/*&& (i % 10 == 0) && (step % 10 == 0)*/) {
                //     visualizeData = "**Agent " + std::to_string(i) + " Information**\n";
                //     visualizeData += "Row: " + std::to_string(agent.row) + "\n";
                //     visualizeData += "Col: " + std::to_string(agent.col) + "\n";
                //     visualizeData += "Fitness: " + std::to_string(agent.fitness) + "\n";

                //     FileManager::openAndWriteToFile("KarWorldData.txt", visualizeData);
                //     visualizeData = "";
                // }

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

                // Convert brain outputs into actual commands
                // TODO: Trit is probably not a good idea here, since it's biased toward allowing the agent to turn...
                // Probably the same for Bit
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
                worldMap.stepAgent(i, rotationCmds.at(i), forwardCmds.at(i));
            }

            // Random resource regrowth every world step
            // worldMap.growResource();
            // worldMap.updateResourcesPerStep(resSiteFidelity);
            worldMap.updateResourcesPerStep2();

            if (visualize/*&& (step % 10) == 0*/) {
                // std::cout << "\x1B[2J\x1B[H"; // clear screen
                // std::cout << worldMap; // for Human Brain
                visualizeData = "**World Step: " + std::to_string(step) + "**\n";
                ss << worldMap;
                visualizeData += ss.str();
                FileManager::openAndWriteToFile("KarWorldData-G" 
                                                + std::to_string(Global::update) 
                                                + "-L" 
                                                + std::to_string(t) + ".txt", visualizeData);

                ss.str("");
                ss.clear();
                visualizeData = "";
            }
        } 
    
        // Agent's fitness is the amount of resources it consumes in a single lifetime
        for (int i = 0; i < popSize; ++i) {
            agentScores.at(i) += worldMap.agents.at(i).fitness; 
        }
    }

    // We average over evaluationsPerGeneration lifetimes
    for (int i = 0; i < popSize; ++i) {
        population.at(i)->dataMap.set("score", agentScores.at(i) / evaluationsPerGeneration);
    }
}

// The requiredGroups function lets MABE know how to set up populations of organisms that this world needs
// 6 inputs for vision, 2 outputs for rotation and forward commands
auto KarWorld::requiredGroups() -> unordered_map<string,unordered_set<string>> {
	return { { groupName, { "B:" + brainName + ",6,2" } } }; 
}




