#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstdlib>

#include "Tree.h"
#include "Node.h"
#include "MbRandom.h"
#include "MCMC.h"
#include "SpExModel.h"
#include "Settings.h"
#include "TraitModel.h"
//#include "Autotune.h"
#include "FastSimulatePrior.h"
#include "Prior.h"
#include "SpExDataWriter.h"
#include "TraitDataWriter.h"


void printAbout();
const char* currentTime();
void exitWithMessageUsage();
void exitWithErrorUnknownArgument(const std::string& arg);
void exitWithErrorNoControlFile();

#include "Log.h"

int main (int argc, char* argv[])
{
    printAbout();

    if (argc == 1) {
        exitWithMessageUsage();
    }

    std::vector<UserParameter> commandLineParameters;
    std::string controlFilename;

    // Process command-line arguments
    int i = 1;
    while (i < argc) {
        std::string arg = std::string(argv[i]);
        if (arg == "-h" || arg == "--help" || arg == "-help") {
            exitWithMessageUsage();
        } else if (arg == "-c" || arg == "--control" || arg == "-control") {
            if (++i < argc) {
                controlFilename = std::string(argv[i]);
            } else {
                exitWithErrorNoControlFile();
            }
        // Let the Settings class (below) process the remaining arguments
        } else {
            if (++i < argc) {
                // First, make sure argument starts with "--"
                if ((arg[0] != '-') || (arg[1] != '-')) {
                    log(Error) << "Invalid command-line option <<"
                        << arg << ">>.\n";
                    std::exit(1);
                }
                arg = arg.substr(2);    // Cut out the starting "--"
                std::string arg_value = std::string(argv[i]);
                UserParameter param(arg, arg_value);
                commandLineParameters.push_back(param);
            } else {
                log(Error) << "Missing a command-line argument.\n";
                std::exit(1);
            }
        }

        i++;
    }

    // Load settings from control file
    Settings mySettings(controlFilename, commandLineParameters);

    MbRandom myRNG(mySettings.getSeed());

    Prior myPrior(&myRNG, &mySettings);
    
    std::string commandLine(argv[0]);
    for (int i = 1; i < argc; i++) {
        commandLine += std::string(" ") + argv[i];
    }

    std::ofstream runInfoFile(mySettings.getRunInfoFilename().c_str());
    log(Message, runInfoFile) << "Command line: " << commandLine << "\n";

    //log(Message, runInfoFile) << "Git commit id: " << GIT_COMMIT_ID << "\n";
    if (mySettings.getSeed() == -1) {
        log(Message) << "Random seed (clock): " << myRNG.getSeed() << "\n";
        log(Message, runInfoFile) << "Random seed (clock): " <<
            myRNG.getSeed() << "\n";
    } else {
        log(Message) << "Random seed: " << myRNG.getSeed() << "\n";
        log(Message, runInfoFile) << "Random seed: " << myRNG.getSeed() << "\n";
    }
    log(Message, runInfoFile) << "Start time: " << currentTime() << "\n";

    if (mySettings.getModeltype() == "speciationextinction") {
        log(Message) << "\nModel type: Speciation/Extinction\n";

        mySettings.printCurrentSettings(runInfoFile);

        std::string treefile = mySettings.getTreeFilename();
        Tree intree(treefile, &myRNG);

        if (mySettings.getUseGlobalSamplingProbability()) {
            intree.initializeSpeciationExtinctionModel
                (mySettings.getGlobalSamplingFraction());
        } else {
            // TODO: Code should be supported for this but need to check
            intree.initializeSpeciationExtinctionModel
                (mySettings.getSampleProbsFilename());
        }
        
        intree.setCanNodeHoldEventByDescCount
            (mySettings.getMinCladeSizeForShift());
        intree.setTreeMap(intree.getRoot());

        if (mySettings.getInitializeModel()) {
            SpExModel model(&myRNG, &intree, &mySettings, &myPrior);

            if (mySettings.getRunMCMC()) {
                int numberOfGenerations = mySettings.getNGENS();
                SpExDataWriter dataWriter(mySettings, model);
                MCMC mcmc(myRNG, model, numberOfGenerations, dataWriter);
                mcmc.run();
            }
/*
            // TODO: Is this doing anything
            if (mySettings.getAutotune()) {
                Autotune autotune(&myRNG, &model, &mySettings);
            }
*/
        }

    } else if (mySettings.getModeltype() == "trait") {
        log(Message) << "\nModel type: Trait\n";
        
        mySettings.printCurrentSettings(runInfoFile);

        std::string treefile = mySettings.getTreeFilename();
        Tree intree(treefile, &myRNG);

        intree.setAllNodesCanHoldEvent();
        intree.setTreeMap(intree.getRoot());

        intree.getPhenotypesMissingLatent(mySettings.getTraitFile());
        intree.initializeTraitValues();
        
        if (mySettings.getInitializeModel()) {
            TraitModel model(&myRNG, &intree, &mySettings, &myPrior);

            if (mySettings.getRunMCMC()) {
                int numberOfGenerations = mySettings.getNGENS();
                TraitDataWriter dataWriter(mySettings, model);
                MCMC mcmc(myRNG, model, numberOfGenerations, dataWriter);
                mcmc.run();
            }
/*
            if (mySettings.getAutotune()) {
                log(Error) << "Autotune option not yet supported for "
                    << "phenotypic (trait) analysis.\n";
                std::exit(1);
            }
*/
        }
    }

    if (mySettings.getSimulatePriorShifts()){
        FastSimulatePrior fsp(&myRNG, &mySettings);
    }

    log(Message, runInfoFile) << "End time: " << currentTime() << "\n";
    runInfoFile.close();

    return 0;
}


void printAbout()
{
    log(Message) << "\
+--------------------------------------------------------------------------+\n\
|   BAMM: Bayesian Analysis of Macroevolutionary Mixtures                  |\n\
+--------------------------------------------------------------------------+\n\
|                                                                          |\n\
|   Daniel Rabosky <drabosky@umich.edu>                                    |\n\
|   University of Michigan, Ann Arbor, MI, USA                             |\n\
|                                                                          |\n\
|   Authors: Carlos Anderson, Joseph Brown, Michael Grundler,              |\n\
|            Daniel Rabosky, Jeff Shi, Pascal Title                        |\n\
|                                                                          |\n\
|   Copyright (C) 2012-2014 Daniel Rabosky                                 |\n\
|   See LICENSE for details.                                               |\n\
|                                                                          |\n\
+--------------------------------------------------------------------------+\n\
\n";
}


const char* currentTime()
{
    time_t curTime;
    time(&curTime);
    return std::ctime(&curTime);
}


void exitWithMessageUsage()
{
    std::cout << "Usage: ./bamm -c control_filename\n";
    std::exit(0);
}


void exitWithErrorUnknownArgument(const std::string& arg)
{
    std::cout << "Unknown argument " << arg << ".\n";
    std::exit(1);
}


void exitWithErrorNoControlFile()
{
    std::cout << "ERROR: No control file specified.\n";
    std::cout << "Fix by specifying a control file name.\n";
    std::exit(1);
}
