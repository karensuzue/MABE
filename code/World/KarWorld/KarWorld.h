//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once    // directive to insure that this .h file is only included one time

#include <World/AbstractWorld.h> // AbstractWorld defines all the basic function templates for worlds
#include <string>
#include <memory> // shared_ptr
#include <map>
#include <vector>
#include <cassert>

#include "WorldMap.hpp"

using std::shared_ptr;
using std::string;
using std::map;
using std::unordered_map;
using std::unordered_set;
using std::to_string;

class KarWorld : public AbstractWorld {

public:
    static shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;

    static shared_ptr<ParameterLink<int>> mapWidthPL;
    static shared_ptr<ParameterLink<int>> mapHeightPL;
    static shared_ptr<ParameterLink<double>> resDensityPL;
    static shared_ptr<ParameterLink<double>> resGrowthRatePL;
    static shared_ptr<ParameterLink<double>> visionRadiusPL;
    static shared_ptr<ParameterLink<string>> visionModePL;
    
    // static shared_ptr<ParameterLink<int>> numAgentsPL;
    
    int evaluationsPerGeneration;
    int mapWidth, mapHeight;
    double resDensity;
    double resGrowthRate;
    double visionRadius;
    string visionMode;
    // int numAgents;

    WorldMap worldMap;
    
    string groupName = "root::";
    string brainName = "root::";
    
    KarWorld(shared_ptr<ParametersTable> PT);
    virtual ~KarWorld() = default;

    virtual auto evaluate(map<string, shared_ptr<Group>>& /*groups*/, int /*analyze*/, int /*visualize*/, int /*debug*/) -> void override;

    virtual auto requiredGroups() -> unordered_map<string,unordered_set<string>> override;

};

