/****************************************/
/****************************************/
/* ARGoS-related headers */
#include <argos3/core/simulator/simulator.h>
#include <argos3/core/simulator/loop_functions.h>

#include <src/evolution/evol_loop_functions.h>
#include <src/exec_tools.h>

#include <src/evolution/descriptors.h>

/****************************************/
/****************************************/


Descriptor* init_analysis_descriptor(EvolutionLoopFunctions& cLoopFunctions, size_t individual_index,std::string filename)
{
    std::map<std::string,Descriptor*> slaves;
    slaves["sdbc"] = new SDBC(&cLoopFunctions, "cvt_Gomes_sdbc_walls_and_robots_std",10);
    slaves["sa_history"] = new SubjectiveHistoryDescriptor(cLoopFunctions.output_folder + "/sa_history"+std::to_string(individual_index));
    slaves["xy_history"] = new ObjectiveHistoryDescriptor(cLoopFunctions.output_folder +  "/xy_history"+std::to_string(individual_index));
    return new AnalysisDescriptor(individual_index, filename, slaves);
}


int main(int argc, char **argv)
{
    
    std::vector<std::string> cmd_args;
    for (int i = 0; i < argc; i++)
        cmd_args.push_back(std::string(argv[i]));
    std::vector<std::string>::iterator individual_it = std::find(cmd_args.begin(), cmd_args.end(), "-n");
    if(individual_it == cmd_args.end())
    {
        std::cerr << "Argument -n individual is missing. Exiting ...";
        exit(-1);
    }


#if ARGOS_PARALLEL
    /* note: needs to be in a cpp */
    /* Initialize ARGoS */
    /* Redirect LOG and argos::LOGERR to dedicated files to prevent clutter on the screen */
    std::ofstream cLOGFile(std::string("ARGoS_LOG_" + argos::ToString(getpid())).c_str(), std::ios::out);
    argos::LOG.DisableColoredOutput();
    argos::LOG.GetStream().rdbuf(cLOGFile.rdbuf());
    std::ofstream cLOGERRFile(std::string("ARGoS_LOGERR_" + argos::ToString(getpid())).c_str(), std::ios::out);
    argos::LOGERR.DisableColoredOutput();
    argos::LOGERR.GetStream().rdbuf(cLOGERRFile.rdbuf());
    argos::LOG << "starting "<< argv[1] << std::endl;// tell which job it is
    argos::LOGERR << "starting "<< argv[1] << std::endl;// tell which job it is
#endif

    /*
     * Initialize ARGoS
     */
    /* The CSimulator class of ARGoS is a singleton. Therefore, to
    * manipulate an ARGoS experiment, it is enough to get its instance */
    argos::CSimulator &cSimulator = argos::CSimulator::GetInstance();
    /* Set the .argos configuration file
     * This is a relative path which assumed that you launch the executable
     * from argos3-examples (as said also in the README) */
    cSimulator.SetExperimentFileName(argv[1]);

    /* Load it to configure ARGoS */
    cSimulator.LoadExperiment();

// #ifdef ARGOS_PARALLEL
//     init_shared_mem<EAEAParams>();
//     configure_and_run_ea<parallel_ea_t>(argc,argv);
// #else
// #endif


    static EvolutionLoopFunctions &cLoopFunctions = dynamic_cast<EvolutionLoopFunctions &>(cSimulator.GetLoopFunctions());
    /* process arguments*/
    size_t individual_index = std::atoi((*(individual_it+1)).c_str()); 
    cLoopFunctions.descriptor = init_analysis_descriptor(cLoopFunctions,individual_index,std::string(cLoopFunctions.output_folder)+std::string("/analysis_"));

    configure_and_run_ea<serial_ea_t>(argc,argv);
   


   /*
    * Dispose of ARGoS stuff
    */
    argos::CSimulator::GetInstance().Destroy();

    /* All is OK */
    return 0;
}

/****************************************/
/****************************************/