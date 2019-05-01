

#include <src/obsavoid/evol_loop_functions.h>
#include <src/obsavoid/descriptors.h>
#include <src/obsavoid/statistics.h>
#include <iterator>

#define SENSOR_ACTIVATION_THRESHOLD 0.5

#ifdef THREE_D_BEHAV
const size_t Descriptor::behav_dim = 3;
#endif
#ifdef SIX_D_BEHAV
const size_t Descriptor::behav_dim = 6;
#endif

// const size_t SDBC::behav_dim=3;

void Descriptor::before_trials(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	for (size_t t = 0; t < behav_dim; ++t)
		bd[t].resize(cLoopFunctions.m_unNumberTrials, 0.0f);
	current_trial = -1; // will add +1 at start of each new trial
}

void Descriptor::start_trial()
{
	num_updates = 0;
	++current_trial;
}
std::vector<float> Descriptor::after_trials(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{

	std::vector<float> final_bd;
	final_bd.resize(behav_dim);
	for (size_t i = 0; i < behav_dim; ++i)
	{
		final_bd[i] = StatFuns::mean(this->bd[i]);
		if (final_bd[i] < 0 || final_bd[i] > 1)
		{
			throw std::runtime_error("bd not in [0,1]");
		}
	}
	return final_bd;
}

void AverageDescriptor::set_input_descriptor(size_t robot_index, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{

	for (size_t i = 0; i < cLoopFunctions.inputs.size() - 1; ++i)
	{
		this->bd[i][current_trial] += (cLoopFunctions.inputs[i] >= SENSOR_ACTIVATION_THRESHOLD) ? 1.0 : 0.0;
	}
	++num_updates;
}

/*end the trial*/
void AverageDescriptor::end_trial(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{

	for (size_t i = 0; i < cLoopFunctions.inputs.size() - 1; ++i)
	{
		this->bd[i][current_trial] /= (float)num_updates;
		if (this->bd[i][current_trial] < 0 || this->bd[i][current_trial] > 1)
		{
			throw std::runtime_error("bd not in [0,1]");
		};
	}
}

IntuitiveHistoryDescriptor::IntuitiveHistoryDescriptor(CLoopFunctions *cLoopFunctions)
{

	//define member variables
	center = cLoopFunctions->GetSpace().GetArenaCenter();

	// initialise grid (for calculating coverage and uniformity)
	argos::CVector3 max = cLoopFunctions->GetSpace().GetArenaSize();
	argos::CVector3 min = center - 0.5 * max;
	max_deviation = StatFuns::get_minkowski_distance(max, center);

	if (static_cast<CObsAvoidEvolLoopFunctions *>(cLoopFunctions)->m_unNumberRobots != 1)
	{
		throw std::runtime_error("number of robots should be equal to 1 when choosing IntuitiveHistoryDescriptor");
	}
	total_size = max.GetX() * max.GetY() / grid_step;
}

/*reset BD at the start of a trial*/
void IntuitiveHistoryDescriptor::start_trial()
{
	Descriptor::start_trial();
	deviation = 0.0f;
	//velocity_stats=RunningStat();  // dropped the velocity stats
}
void IntuitiveHistoryDescriptor::set_input_descriptor(size_t robot_index, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	//add to the deviation (to get the mean after all trials have finished)
	CVector3 pos = cLoopFunctions.get_position(cLoopFunctions.m_pcvecRobot[robot_index]);
	deviation += StatFuns::get_minkowski_distance(pos, center); // assume single robot

	//count the bin
	std::tuple<int, int, int> bin = get_bin(pos);
	auto find_result = unique_visited_positions.find(bin);
	if (find_result == unique_visited_positions.end())
	{
		unique_visited_positions.insert(std::pair<std::tuple<int, int, int>, size_t>(bin, 1));
	}
	else
	{
		unique_visited_positions[bin] += 1;
	}
	++num_updates;
}

void IntuitiveHistoryDescriptor::set_output_descriptor(size_t robot_index, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	//velocity_stats->push(cLoopFunctions.curr_lin_speed);
}

void IntuitiveHistoryDescriptor::end_trial(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	/*add behavioural metrics */

	//uniformity of probabilities
	std::vector<float> probabilities = get_probs();
	float uniformity = StatFuns::uniformity(probabilities);
	this->bd[0][current_trial] = uniformity;
	//deviation from the center
	float avg_deviation = deviation / (max_deviation * (float)num_updates);
	this->bd[1][current_trial] = avg_deviation;
	//coverage
	float max_visited_positions = std::min(total_size, (float)num_updates);
	float coverage = (float)unique_visited_positions.size() / (float)max_visited_positions;
	this->bd[2][current_trial] = coverage;
	// //variability in the speed]
	// float velocity_sd=velocity_stats.std()/max_velocitysd;
	// this->bd[3] +=velocity_sd;
	//

#ifdef PRINTING
	std::cout << "uniformity" << uniformity << std::endl;
	std::cout << "Max deviation" << max_deviation << std::endl;
	std::cout << "deviation" << avg_deviation << std::endl;
	std::cout << "coverage" << coverage << std::endl;
	//std::cout<<"velocity_sd"<<velocity_sd<<std::endl;
#endif

	unique_visited_positions.clear();
}

float Entity::distance(const Entity e1, const Entity e2)
{
	return StatFuns::get_minkowski_distance(e1.position, e2.position);
}

SDBC::SDBC(CLoopFunctions *cLoopFunctions, std::string init_type) : Descriptor()
{
	if (init_type.find("sdbc_walls_and_robots") == 0)
	{
		init_walls_and_robots(cLoopFunctions);
	}
	else if (init_type.find("sdbc_robots") == 0)
	{
		init_walls_and_robots(cLoopFunctions);
	}
	else
	{
		throw std::runtime_error("init type " + init_type + "not found");
	}
	include_std = init_type.find("std") == 0 ? true : false; // will calculate standard deviations as well

	num_groups = entity_groups.size();
	for (auto &kv : entity_groups)
	{

		if (kv.second.max_size > 1 && kv.first == "robots")
		{
			within_comparison_groups.push_back(kv.first); // within-group distance computations
		}

		between_comparison_groups.push_back(kv.first); // between-group distance computations

		if (kv.second.max_size != kv.second.min_size)
		{
			variable_groups.push_back(kv.first);
		}
	}
	argos::CVector3 max = cLoopFunctions->GetSpace().GetArenaSize();
	maxX = max.GetX();
	maxY = max.GetY();
	maxdist = StatFuns::get_minkowski_distance(max, CVector3::ZERO);
}
void SDBC::init_walls_and_robots(CLoopFunctions *cLoopFunctions)
{
	SDBC::init_robots(cLoopFunctions);
	// 4 walls, each with 0 state features, but two constant positional features
	std::vector<Entity> boxes;

	CSpace::TMapPerType &argos_boxes = cLoopFunctions->GetSpace().GetEntitiesByType("box");
	for (CSpace::TMapPerType::iterator it = argos_boxes.begin(); it != argos_boxes.end(); ++it) //!TODO: Make sure the CSpace::TMapPerType does not change during a simulation (i.e it is not robot-position specific)
	{
		CBoxEntity &cBody = *any_cast<CBoxEntity *>(it->second);
		CVector3 position = cBody.GetEmbodiedEntity().GetOriginAnchor().Position;
		Entity e = Entity();
		e.position = CVector3(position);
		boxes.push_back(e);
	}
	std::pair<std::string, Entity_Group> wallpair = {"boxes", Entity_Group(0, boxes.size(), boxes.size(), boxes)};
	entity_groups.insert(wallpair);
}
void SDBC::init_robots(CLoopFunctions *cLoopFunctions)
{
	// robot here has 5 features: x,y,orientation,wheelvelocity1,wheelvelocity2
	// here it is assumed fixed number of robots
	std::vector<Entity> robots;
	size_t num_robots = static_cast<CObsAvoidEvolLoopFunctions *>(cLoopFunctions)->m_unNumberRobots;
	for (size_t i = 0; i < num_robots; ++i)
	{
		robots.push_back(Entity());
	}
	std::pair<std::string, Entity_Group> robotpair = {"robots",
													  Entity_Group(5, num_robots, num_robots, robots)}; // 5 features
	entity_groups.insert(robotpair);
}

/* group sizes are the first BD dimensions*/
void SDBC::add_group_sizes()
{

	for (std::string key : variable_groups)
	{
		Entity_Group &group = entity_groups[key];
		float size = group.get_size();

#ifdef PRINTING
		std::cout << "group: " << key << std::endl;
		std::cout << "size : " << size << std::endl;
#endif
		bd[bd_index][current_trial] += size; // add to average out later
		bd_index++;
	}
}

/* group mean attribute vectors, the second set of BD dimensions*/
void SDBC::add_group_meanstates()
{
	for (auto &kv : entity_groups)
	{
		Entity_Group &group = kv.second;
#ifdef PRINTING
		std::cout << "group: " << kv.first << std::endl;
#endif
		for (int i = 0; i < group.kappa; ++i)
		{

			float mean_state = group.mean_state_vec(i);

			bd[bd_index][current_trial] += mean_state;
			if (include_std)
			{
				bd_index++;
				float sd_state = group.sd_state_vec(i, mean_state);
				bd[bd_index][current_trial] += sd_state;
			}
			bd_index++;
#ifdef PRINTING
			std::cout << "attribute " << i << ": " << mean_state << std::endl;
#endif
		}
	}
}

/* avg pair-wise distance within groups, the third set of  BD dimensions*/
void SDBC::add_within_group_dispersion()
{
	float sum;
	for (std::string key : within_comparison_groups)
	{

		Entity_Group &group = entity_groups[key];
#ifdef PRINTING
		std::cout << "group: " << key << std::endl;
#endif
		if (group.max_size <= 1)
		{
			continue; //ignore
		}
		else if (group.get_absolute_size() <= 1)
		{
			// only at the moment, may change later
			++bd_index;
			continue;
		}
		else
		{
			sum = 0.0f;
			for (int i = 0; i < group.get_absolute_size(); ++i)
			{
				for (int j = 1; j < group.get_absolute_size() && j != i; ++j)
				{
					sum += Entity::distance(group[i], group[j]);
				}
			}
		}

		this->bd[bd_index][current_trial] += sum / ((float)(group.get_absolute_size() - 1) * (group.get_absolute_size() - 1) * maxdist);
		++bd_index;
	}
}

/* avg pair-wise distance between groups, the final BD dimensions*/
void SDBC::add_between_group_dispersion()
{
	float sum;
	for (size_t i = 0; i < between_comparison_groups.size(); ++i)
	{
		std::string key = between_comparison_groups[i];
		Entity_Group &group = entity_groups[key];
		for (size_t j = 1; j < between_comparison_groups.size() && j != i; ++j)
		{
			std::string key2 = between_comparison_groups[j];
			Entity_Group &group2 = entity_groups[key2];
#ifdef PRINTING
			std::cout << "group1: " << key << std::endl;
			std::cout << "group2: " << key2 << std::endl;
#endif
			if (group.max_size <= 1)
			{
				continue; //ignore
			}
			else if (group.get_absolute_size() <= 1)
			{
				++bd_index;
				continue;
			}
			else
			{
				sum = 0.0f;
				for (Entity e1 : group.entities)
				{
					for (Entity e2 : group2.entities)
					{
						sum += Entity::distance(e1, e2);
					}
				}
			}
			bd[bd_index][current_trial] += sum / ((float)(group.get_absolute_size() * group2.get_absolute_size()) * maxdist); // divide by product of group sizes; add for now, we will average the number of times
			++bd_index;
		}
	}
}

/* prepare for trials*/
void SDBC::before_trials(argos::CSimulator &cSimulator)
{
}
/*reset BD at the start of a trial*/
void SDBC::start_trial()
{
	Descriptor::start_trial();
	bd_index = 0;
}
/*after getting inputs, can update the descriptor if needed*/
void SDBC::set_input_descriptor(size_t robot_index, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
}
/*after getting outputs, can update the descriptor if needed*/
void SDBC::set_output_descriptor(size_t robot_index, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	// here just set the attributes of robot at index; let end
	CVector3 pos = cLoopFunctions.get_position(cLoopFunctions.m_pcvecRobot[robot_index]);

	float x = pos.GetX() / maxX;
	float y = pos.GetY() / maxY;
	float theta = cLoopFunctions.curr_theta.UnsignedNormalize() / CRadians::TWO_PI; // normalise radians to [0,1]
	float wheel1 = (10.0f + cLoopFunctions.outf[0]) / 20.0f;
	float wheel2 = (10.0f + cLoopFunctions.outf[1]) / 20.0f;

	std::vector<float> new_vec = {x, y, theta, wheel1, wheel2};
	entity_groups["robots"][robot_index].set_attributes(new_vec, pos);
#ifdef PRINTING
	std::cout << "x,y,theta,w1,w2=" << x << "," << y << "," << theta << "," << wheel1 << "," << wheel2 << std::endl;
#endif
}
/*after the looping over robots*/
void SDBC::after_robotloop(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	add_group_sizes();
	add_group_meanstates();
	add_between_group_dispersion();
	add_within_group_dispersion();
	bd_index = 0;
	++num_updates;
}
/*end the trial*/
void SDBC::end_trial(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{

	for (size_t i = 0; i < behav_dim; ++i)
	{
		this->bd[i][current_trial] /= (float)(num_updates + 1);
		if (this->bd[i][current_trial] < 0 || this->bd[i][current_trial] > 1)
		{
			throw std::runtime_error("bd" + std::to_string(i) + " not in [0,1]: " + std::to_string(bd[i][current_trial]));
		};
	}
}

/* given a group of sensory activations, sum their activation, and get the correspinding bin*/
size_t get_group_inputactivation(size_t num_bins, std::vector<size_t> group, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
	float sum = 0.0f;
	for (const size_t &idx : group)
	{
		sum += cLoopFunctions.inputs[idx];
	}
	sum /= group.size();
	float dx = 1. / (float)num_bins;
	float s = dx;
	for (size_t i; i < num_bins; ++i)
	{
		if (sum <= dx)
		{
			return i;
		}
		s += dx;
	}
	throw std::runtime_error("not assigned to any bin");
}
/* prepare for trials*/
void MutualinfoDescriptor::before_trials(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
}
/*reset BD at the start of a trial*/
void MutualinfoDescriptor::start_trial()
{
}
/*after getting inputs, can update the descriptor if needed*/
void MutualinfoDescriptor::set_input_descriptor(size_t robot_index, CObsAvoidEvolLoopFunctions &cLoopFunctions)
{

	for (const std::vector<size_t> &group : sensory_groups)
	{
		// get the activation bin
		size_t bin = get_group_inputactivation(num_bins, group, cLoopFunctions);
		// push it to the sensory group's history
	}
}

/*after the looping over robots*/
void MutualinfoDescriptor::after_robotloop(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
}

/*end the trial*/
void MutualinfoDescriptor::end_trial(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
}

/*summarise BD at the end of trials*/
std::vector<float> MutualinfoDescriptor::after_trials(CObsAvoidEvolLoopFunctions &cLoopFunctions)
{
}