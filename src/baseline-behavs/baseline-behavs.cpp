/* Include the controller definition */
#include "baseline-behavs.h"
/* Function definitions for XML parsing */
#include <argos3/core/utility/configuration/argos_configuration.h>
/* 2D vector definition */
#include <argos3/core/utility/math/vector2.h>

/****************************************/
/****************************************/

CBehavior::SensoryData CBehavior::m_sSensoryData;
CBehavior::RobotData CBehavior::m_sRobotData;

/****************************************/
/****************************************/

CBaselineBehavs::ExperimentToRun::ExperimentToRun() :
    SBehavior(SWARM_AGGREGATION),
    FBehavior(FAULT_NONE),
    id_FaultyRobotInSwarm("-1")
{
}

/****************************************/
/****************************************/

void CBaselineBehavs::ExperimentToRun::Init(TConfigurationNode& t_node)
{
    std::string errorbehav;

    try
    {
        GetNodeAttribute(t_node, "swarm_behavior", swarmbehav);
        GetNodeAttribute(t_node, "fault_behavior", errorbehav);
        GetNodeAttribute(t_node, "id_faulty_robot", id_FaultyRobotInSwarm);
    }
    catch(CARGoSException& ex)
            THROW_ARGOSEXCEPTION_NESTED("Error initializing type of experiment to run, and fault to simulate.", ex);

    if (swarmbehav.compare("SWARM_AGGREGATION") == 0)
        SBehavior = SWARM_AGGREGATION;
    else if (swarmbehav.compare("SWARM_DISPERSION") == 0)
        SBehavior = SWARM_DISPERSION;
    else if (swarmbehav.compare("SWARM_FLOCKING") == 0)
        SBehavior = SWARM_FLOCKING;
    else if (swarmbehav.compare("SWARM_HOMING") == 0)
        SBehavior = SWARM_HOMING;
    else if (swarmbehav.compare("SWARM_STOP") == 0)
        SBehavior = SWARM_STOP;
    else
    {
        std::cerr << "invalid swarm behavior";
        assert(-1);
    }

    if (errorbehav.compare("FAULT_NONE") == 0)
        FBehavior = FAULT_NONE;
    else if  (errorbehav.compare("FAULT_STRAIGHTLINE") == 0)
        FBehavior = FAULT_STRAIGHTLINE;
    else if  (errorbehav.compare("FAULT_RANDOMWALK") == 0)
        FBehavior = FAULT_RANDOMWALK;
    else if  (errorbehav.compare("FAULT_CIRCLE") == 0)
        FBehavior = FAULT_CIRCLE;
    else if  (errorbehav.compare("FAULT_STOP") == 0)
        FBehavior = FAULT_STOP;


    else if  (errorbehav.compare("FAULT_PROXIMITYSENSORS_SETMIN") == 0)
        FBehavior = FAULT_PROXIMITYSENSORS_SETMIN;
    else if  (errorbehav.compare("FAULT_PROXIMITYSENSORS_SETMAX") == 0)
        FBehavior = FAULT_PROXIMITYSENSORS_SETMAX;
    else if  (errorbehav.compare("FAULT_PROXIMITYSENSORS_SETRANDOM") == 0)
        FBehavior = FAULT_PROXIMITYSENSORS_SETRANDOM;
    else if  (errorbehav.compare("FAULT_PROXIMITYSENSORS_SETOFFSET") == 0)
        FBehavior = FAULT_PROXIMITYSENSORS_SETOFFSET;


    else if  (errorbehav.compare("FAULT_RABSENSOR_SETOFFSET") == 0)
        FBehavior = FAULT_RABSENSOR_SETOFFSET;
    else if  (errorbehav.compare("FAULT_RABSENSOR_MISSINGRECEIVERS") == 0)
        FBehavior = FAULT_RABSENSOR_MISSINGRECEIVERS;


    else if  (errorbehav.compare("FAULT_ACTUATOR_LWHEEL_SETZERO") == 0)
        FBehavior = FAULT_ACTUATOR_LWHEEL_SETZERO;
    else if  (errorbehav.compare("FAULT_ACTUATOR_RWHEEL_SETZERO") == 0)
        FBehavior = FAULT_ACTUATOR_RWHEEL_SETZERO;
    else if  (errorbehav.compare("FAULT_ACTUATOR_BWHEELS_SETZERO") == 0)
        FBehavior = FAULT_ACTUATOR_BWHEELS_SETZERO;

    else if  (errorbehav.compare("FAULT_SOFTWARE") == 0)
        FBehavior = FAULT_SOFTWARE;

    else if  (errorbehav.compare("FAULT_POWER_FAILURE") == 0)
        FBehavior = FAULT_POWER_FAILURE;

    else
    {
        std::cerr << "invalid fault behavior";
        assert(-1);
    }
}

/****************************************/
/****************************************/

void CBaselineBehavs::SWheelTurningParams::Init(TConfigurationNode& t_node)
{
    try
    {
        GetNodeAttribute(t_node, "max_speed", MaxSpeed);
    }
    catch(CARGoSException& ex)
            THROW_ARGOSEXCEPTION_NESTED("Error initializing controller wheel turning parameters.", ex);

    //std::cout << " MaxSpeed " << MaxSpeed << std::endl;
}

/****************************************/
/****************************************/


CBaselineBehavs::CBaselineBehavs() :
    m_pcWheels(NULL),
    m_pcProximity(NULL),
    m_pcGround(NULL),
    m_pcLeds(NULL),
    m_pcRABA(NULL),
    m_pcRABS(NULL),
    m_pcRNG(CRandom::CreateRNG("argos")),
    b_damagedrobot(false)
{
    m_fInternalRobotTimer = 0.0f;

}

/****************************************/
/****************************************/

void CBaselineBehavs::Init(TConfigurationNode& t_node)
{
    try
    {
        /*
    * Get sensor/actuator handles
    *
    * The passed string (ex. "differential_steering") corresponds to the
    * XML tag of the device whose handle we want to have. For a list of
    * allowed values, type at the command prompt:
    *
    * $ argos3 -q actuators
    *
    * to have a list of all the possible actuators, or
    *
    * $ argos3 -q sensors
    *
    * to have a list of all the possible sensors.
    *
    * NOTE: ARGoS creates and initializes actuators and sensors
    * internally, on the basis of the lists provided the configuration
    * file at the <controllers><Thymio_diffusion><actuators> and
    * <controllers><Thymio_diffusion><sensors> sections. If you forgot to
    * list a device in the XML and then you request it here, an error
    * occurs.
    */
        m_pcWheels    = GetActuator<CCI_DifferentialSteeringActuator>("differential_steering");
        m_pcWheelsEncoder = GetSensor <CCI_DifferentialSteeringSensor >("differential_steering");
        m_pcLeds      = GetActuator<CCI_ThymioLedsActuator          >("thymio_led");
        m_pcProximity = GetSensor  <CCI_ThymioProximitySensor       >("Thymio_proximity");
        m_pcGround    = GetSensor  <CCI_ThymioGroundSensor          >("Thymio_ground");
        m_pcRABA      = GetActuator<CCI_RangeAndBearingActuator     >("range_and_bearing" );
        m_pcRABS      = GetSensor <CCI_RangeAndBearingSensor        >("range_and_bearing" );

        /*
   * Parse XML parameters
   */
        /* Experiment to run */
        m_sExpRun.Init(GetNode(t_node, "experiment_run"));
        /* Wheel turning */
        m_sWheelTurningParams.Init(GetNode(t_node, "wheel_turning"));
    }
    catch(CARGoSException& ex)
            THROW_ARGOSEXCEPTION_NESTED("Error initializing the controller for robot \"" << GetId() << "\"", ex);

    Reset();

    m_sRobotDetails.SetKinematicDetails(m_sWheelTurningParams.MaxSpeed, m_sWheelTurningParams.MaxSpeed);

    CopyRobotDetails(m_sRobotDetails);

    m_pFlockingBehavior = new CFlockingBehavior(m_sRobotDetails.iterations_per_second * 1.0f); // 5.0f

    if(this->GetId().compare("thymio"+m_sExpRun.id_FaultyRobotInSwarm) == 0)
        b_damagedrobot = true;
}

/****************************************/
/****************************************/

void CBaselineBehavs::CopyRobotDetails(RobotDetails& robdetails)
{

    CBehavior::m_sRobotData.MaxSpeed                    = robdetails.MaxLinearSpeed * robdetails.iterations_per_second; // max speed in cm/s to control behavior
    CBehavior::m_sRobotData.iterations_per_second       = robdetails.iterations_per_second;
    CBehavior::m_sRobotData.seconds_per_iterations      = 1.0f / robdetails.iterations_per_second;
    CBehavior::m_sRobotData.HALF_INTERWHEEL_DISTANCE    = robdetails.HALF_INTERWHEEL_DISTANCE;
    CBehavior::m_sRobotData.INTERWHEEL_DISTANCE         = robdetails.INTERWHEEL_DISTANCE;
    CBehavior::m_sRobotData.WHEEL_RADIUS                = robdetails.WHEEL_RADIUS;

    CBehavior::m_sRobotData.m_cNoTurnOnAngleThreshold   = robdetails.m_cNoTurnOnAngleThreshold;
    CBehavior::m_sRobotData.m_cSoftTurnOnAngleThreshold = robdetails.m_cSoftTurnOnAngleThreshold;

    CBehavior::m_sRobotData.BEACON_SIGNAL_MARKER           = BEACON_SIGNAL;
    CBehavior::m_sRobotData.NEST_BEACON_SIGNAL_MARKER      = NEST_BEACON_SIGNAL;
    CBehavior::m_sRobotData.SELF_INFO_PACKET_MARKER        = SELF_INFO_PACKET;
    CBehavior::m_sRobotData.SELF_INFO_PACKET_FOOTER_MARKER = SELF_INFO_PACKET_FOOTER;
    CBehavior::m_sRobotData.RELAY_PACKET_MARKER            = RELAY_PACKET;
    CBehavior::m_sRobotData.RELAY_PACKET_FOOTER_MARKER     = RELAY_PACKET_FOOTER;
    CBehavior::m_sRobotData.VOTER_PACKET_MARKER            = VOTER_PACKET;
    CBehavior::m_sRobotData.VOTER_PACKET_FOOTER_MARKER     = VOTER_PACKET_FOOTER;
    CBehavior::m_sRobotData.DATA_BYTE_BOUND_MARKER         = DATA_BYTE_BOUND;
}

/****************************************/
/****************************************/

void CBaselineBehavs::ControlStep()
{
    m_pcRABA->ClearData(); // clear the channel at the start of each control cycle
    m_uRABDataIndex = 0;

    bool b_RunningGeneralFaults(false);
    if(b_damagedrobot && (m_sExpRun.FBehavior == ExperimentToRun::FAULT_STRAIGHTLINE ||
                          m_sExpRun.FBehavior == ExperimentToRun::FAULT_RANDOMWALK ||
                          m_sExpRun.FBehavior == ExperimentToRun::FAULT_CIRCLE ||
                          m_sExpRun.FBehavior == ExperimentToRun::FAULT_STOP))
    {
        b_RunningGeneralFaults = true;
        RunGeneralFaults();
    }

    else if(m_sExpRun.SBehavior == ExperimentToRun::SWARM_AGGREGATION ||
            m_sExpRun.SBehavior == ExperimentToRun::SWARM_DISPERSION  ||
            m_sExpRun.SBehavior == ExperimentToRun::SWARM_FLOCKING    ||
            m_sExpRun.SBehavior == ExperimentToRun::SWARM_HOMING      ||
            m_sExpRun.SBehavior == ExperimentToRun::SWARM_STOP)
        RunHomogeneousSwarmExperiment();


    if(!b_damagedrobot || b_RunningGeneralFaults || m_sExpRun.FBehavior == ExperimentToRun::FAULT_NONE)
        CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
    else
    {
        if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_PROXIMITYSENSORS_SETMIN)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_PROXIMITYSENSORS_SETMAX)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_PROXIMITYSENSORS_SETRANDOM)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_PROXIMITYSENSORS_SETOFFSET)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));


        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_RABSENSOR_SETOFFSET)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_RABSENSOR_MISSINGRECEIVERS)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));


        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_ACTUATOR_LWHEEL_SETZERO)
        {
            // does not affect the sensors - they stay the same
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        }
        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_ACTUATOR_RWHEEL_SETZERO)
        {
            // does not affect the sensors - they stay the same
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        }
        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_ACTUATOR_BWHEELS_SETZERO)
        {
            // does not affect the sensors - they stay the same
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
        }


        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_SOFTWARE)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));

        else if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_POWER_FAILURE)
            CBehavior::m_sSensoryData.SetSensoryData(m_pcRNG, m_fInternalRobotTimer, GetIRSensorReadings(b_damagedrobot, m_sExpRun.FBehavior), GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior));
    }

    /*For flocking behavior - to compute relative velocity*/
    CBehavior::m_sSensoryData.SetWheelSpeedsFromEncoders(m_pcWheelsEncoder->GetReading().VelocityLeftWheel, m_pcWheelsEncoder->GetReading().VelocityRightWheel);

    /*The robot has to continually track the velocity of its neighbours - since this is done over a period of time. It can't wait until the flocking behavior is activated to start tracking neighbours*/
    m_pFlockingBehavior->SimulationStep();


    Real leftSpeed = 0.0, rightSpeed = 0.0;
    bool bControlTaken = false;
    for (TBehaviorVectorIterator i = m_vecBehaviors.begin(); i != m_vecBehaviors.end(); i++)
    {
        if (!bControlTaken)
        {
            bControlTaken = (*i)->TakeControl();
            if (bControlTaken)
            {
                (*i)->Action(leftSpeed, rightSpeed);
            }
        } else
            (*i)->Suppress();
    }




    if(b_damagedrobot && m_sExpRun.FBehavior == ExperimentToRun::FAULT_ACTUATOR_LWHEEL_SETZERO)
        leftSpeed  = 0.0f;

    if(b_damagedrobot && m_sExpRun.FBehavior == ExperimentToRun::FAULT_ACTUATOR_RWHEEL_SETZERO)
        rightSpeed = 0.0f;

    if(b_damagedrobot && m_sExpRun.FBehavior == ExperimentToRun::FAULT_ACTUATOR_BWHEELS_SETZERO)
    {
        leftSpeed = 0.0f;
        rightSpeed = 0.0f;
    }

    m_pcWheels->SetLinearVelocity(leftSpeed, rightSpeed); // in cm/s

    //std::cout << "LS:  " << leftSpeed << " RS:  " << rightSpeed << std::endl;

    //CCI_RangeAndBearingSensor::TReadings rabsensor_readings = GetRABSensorReadings(b_damagedrobot, m_sExpRun.FBehavior);

    m_uRobotId = RobotIdStrToInt();

    SenseCommunicate(RobotIdStrToInt(), m_pcRABA, m_uRABDataIndex);












//    m_pcRABA->SetData(0, 100); // send test-value of 100 on RAB medium

//    /* Get readings from proximity sensor */
//    const CCI_ThymioProximitySensor::TReadings& tProxReads = m_pcProximity->GetReadings();
//    /* Get readings from ground sensor */
//    const CCI_ThymioGroundSensor::TReadings& tGroundReads = m_pcGround->GetReadings();

//    m_pcLeds->SetProxHIntensity(tProxReads);

//    //   LOG << tProxReads;
//    //   LOG << tProxReads[2].Value<< tProxReads[2].Angle.GetValue();
//    //   std::cout << tProxReads;

//    m_pcWheels->SetLinearVelocity(m_fWheelVelocity, m_fWheelVelocity);


//    /* Sum them together */
//    CVector2 cAccumulator;
//    for(size_t i = 0; i < tProxReads.size(); ++i)
//    {
//        cAccumulator += CVector2(tProxReads[i].Value, tProxReads[i].Angle);
//    }
//    cAccumulator /= tProxReads.size();

//    short cground = 0;
//    for(size_t i = 0; i < tGroundReads.size(); ++i)
//    {
//        cground += tGroundReads[i].Value;
//    }

//    /* If the angle of the vector is small enough and the closest obstacle
//    * is far enough, continue going straight, otherwise curve a little
//    */
//    CRadians cAngle = cAccumulator.Angle();
//    LOG << cAngle.GetValue();
//    if(m_cGoStraightAngleRange.WithinMinBoundIncludedMaxBoundIncluded(cAngle) &&
//            cAccumulator.Length() < m_fDelta )
//    {
//        /* Go straight */
//        m_pcWheels->SetLinearVelocity(m_fWheelVelocity, m_fWheelVelocity);
//    }
//    else
//    {
//        /* Turn, depending on the sign of the angle */
//        if(cAngle.GetValue() < 0)
//        {
//            m_pcWheels->SetLinearVelocity(m_fWheelVelocity, 0);
//        }
//        else
//        {
//            m_pcWheels->SetLinearVelocity(0, m_fWheelVelocity);
//        }
//    }

//    CCI_RangeAndBearingSensor::TReadings sensor_readings = m_pcRABS->GetReadings();
//    std::cout << "Robots in RAB range of " << GetId() << " is " << sensor_readings.size() << std::endl;
//    for(size_t i = 0; i < sensor_readings.size(); ++i)
//    {
//        std::cout << "RAB range " << sensor_readings[i].Range << " Bearing "  << sensor_readings[i].HorizontalBearing << " Message size " << sensor_readings[i].Data.Size() << std::endl;
//        for(size_t j = 0; j < sensor_readings[i].Data.Size(); ++j)
//            std::cout << "Data-Packet at index " << j << " is " << sensor_readings[i].Data[j] << std::endl;
//    }
}

CBaselineBehavs::~CBaselineBehavs()
{
    m_pcWheels->SetLinearVelocity(0.0f, 0.0f);

    // delete all behaviors
}

/****************************************/
/****************************************/


void CBaselineBehavs::RunGeneralFaults()
{
    //m_pcLEDs->SetAllColors(CColor::RED);

    m_vecBehaviors.clear();
    if(m_sExpRun.FBehavior == ExperimentToRun::FAULT_STRAIGHTLINE)
    {
        CRandomWalkBehavior* pcStraightLineBehavior = new CRandomWalkBehavior(0.0f);
        m_vecBehaviors.push_back(pcStraightLineBehavior);
    }
    else if (m_sExpRun.FBehavior == ExperimentToRun::FAULT_RANDOMWALK)
    {
        CRandomWalkBehavior* pcRandomWalkBehavior = new CRandomWalkBehavior(0.0017f);  // 0.05f
        m_vecBehaviors.push_back(pcRandomWalkBehavior);
    }

    else if (m_sExpRun.FBehavior == ExperimentToRun::FAULT_CIRCLE)
    {
        CCircleBehavior* pcCircleBehavior = new CCircleBehavior();
        m_vecBehaviors.push_back(pcCircleBehavior);
    }

    else //m_sExpRun.FBehavior == ExperimentToRun::FAULT_STOP
    {}
}

/****************************************/
/****************************************/

void CBaselineBehavs::RunHomogeneousSwarmExperiment()
{
    m_vecBehaviors.clear();

    if(m_sExpRun.SBehavior == ExperimentToRun::SWARM_AGGREGATION)
    {
        CDisperseBehavior* pcDisperseBehavior = new CDisperseBehavior(0.1f);    // 0.1f reflects a distance of about 4.5cm
        m_vecBehaviors.push_back(pcDisperseBehavior);

        CAggregateBehavior* pcAggregateBehavior = new CAggregateBehavior(100.0f); //range threshold in cm //60.0
        m_vecBehaviors.push_back(pcAggregateBehavior);

        CRandomWalkBehavior* pcRandomWalkBehavior = new CRandomWalkBehavior(0.0017f); //0.05f
        m_vecBehaviors.push_back(pcRandomWalkBehavior);

        //m_pcLEDs->SetAllColors(CColor::GREEN);
    }

    else if(m_sExpRun.SBehavior == ExperimentToRun::SWARM_DISPERSION)
    {
        CDisperseBehavior* pcDisperseBehavior = new CDisperseBehavior(0.1f);
        m_vecBehaviors.push_back(pcDisperseBehavior);

        CRandomWalkBehavior* pcRandomWalkBehavior = new CRandomWalkBehavior(0.0017f); //0.05f
        m_vecBehaviors.push_back(pcRandomWalkBehavior);

        //m_pcLEDs->SetAllColors(CColor::RED);
    }

    else if(m_sExpRun.SBehavior == ExperimentToRun::SWARM_FLOCKING)
    {
        CDisperseBehavior* pcDisperseBehavior = new CDisperseBehavior(0.1f);
        m_vecBehaviors.push_back(pcDisperseBehavior);

        m_vecBehaviors.push_back(m_pFlockingBehavior);

        CRandomWalkBehavior* pcRandomWalkBehavior = new CRandomWalkBehavior(0.0017f); //0.05f
        m_vecBehaviors.push_back(pcRandomWalkBehavior);
    }

    else if(m_sExpRun.SBehavior == ExperimentToRun::SWARM_HOMING)
    {
        if(this->GetId().compare("thymio0") == 0)
        {
            // thymio0 is the beacon robot
            /* Sends out data 'BEACON_SIGNAL' with RABS that you are a beacon. Neighbouring robots will use this data to home in on your position */
            // BEACON_SIGNAL is way above the DATA_BYTE_BOUND

            m_pcRABA->SetData(0, BEACON_SIGNAL);
            m_uRABDataIndex++;
            //m_pcLEDs->SetAllColors(CColor::YELLOW);
        }
        else
        {
            CDisperseBehavior* pcDisperseBehavior = new CDisperseBehavior(0.1f);    // 0.1f reflects a distance of about 4.5cm
            m_vecBehaviors.push_back(pcDisperseBehavior);

            Real MAX_BEACON_SIGNAL_RANGE = 1.0f; //1m
            CHomingToFoodBeaconBehavior* pcHomingToFoodBeaconBehavior = new CHomingToFoodBeaconBehavior(BEACON_SIGNAL, MAX_BEACON_SIGNAL_RANGE);
            m_vecBehaviors.push_back(pcHomingToFoodBeaconBehavior);

            CRandomWalkBehavior* pcRandomWalkBehavior = new CRandomWalkBehavior(0.0017f); //0.05f
            m_vecBehaviors.push_back(pcRandomWalkBehavior);
        }
    }
    else if(m_sExpRun.SBehavior == ExperimentToRun::SWARM_STOP)
    {
    }
}

/****************************************/
/****************************************/

unsigned CBaselineBehavs::RobotIdStrToInt()
{
    std::string id = GetId();
    id.erase(0, 6); // remove the first six characters 'thymio'

    std::string::size_type sz;   // alias of size_t
    unsigned u_id = std::stoi(id, &sz);
    return u_id;
}

/****************************************/
/****************************************/

/*
 * This statement notifies ARGoS of the existence of the controller.
 * It binds the class passed as first argument to the string passed as
 * second argument.
 * The string is then usable in the configuration file to refer to this
 * controller.
 * When ARGoS reads that string in the configuration file, it knows which
 * controller class to instantiate.
 * See also the configuration files for an example of how this is used.
 */
REGISTER_CONTROLLER(CBaselineBehavs, "baseline-behavs")
