
"""
combine basic perturbations for different agents
"""

import numpy as np
import pickle
import os

home=os.environ["HOME"]

BASIC_PERTURBATIONS=["FAULT_NONE","FAULT_PROXIMITYSENSORS_SETMIN", "FAULT_PROXIMITYSENSORS_SETMAX", "FAULT_PROXIMITYSENSORS_SETRANDOM",
"FAULT_ACTUATOR_LWHEEL_SETHALF", "FAULT_ACTUATOR_RWHEEL_SETHALF", "FAULT_ACTUATOR_BWHEELS_SETHALF"]


def random_combinations(num_agents):
    return np.random.randint(0,7,num_agents)

def write_superset(filename,basic_perturbations,num_agents,num_elements):
    superset=[]
    for i in range(num_elements):
        with open(filename+"p"+str(i)+".txt","w+") as f:
            indexes = random_combinations(num_agents)
            combined_perturbations=""
            for i in range(len(indexes) - 1):
                ind = indexes[i]
                combined_perturbations+=basic_perturbations[ind]+","
            combined_perturbations += basic_perturbations[indexes[-1]]
            f.write(combined_perturbations)


if __name__ == "__main__":
    for run in range(1,6):
        write_superset(home+"/argos-sferes/experiments/perturbations/run"+str(run)+"_",BASIC_PERTURBATIONS,10,100)

