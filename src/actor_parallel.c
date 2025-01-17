#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "mpi.h"
#include <assert.h>
#include <string.h>

#include "../include/pool.h"
#include "../include/data_structures.h"
#include "../include/utils.h"
#include "../include/actor_parallel.h"

int main(int argc, char *argv[])
{
    int i, detachsize;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // if argc is not 2, print error message and exit
    if (argc != 2)
    {
        fprintf(stderr, "Error: You need to provide the roadmap file as the only argument\n");
        exit(-1);
    }
    // srand is used to generate random numbers
    srand(time(NULL));
    map_filename = argv[1];

    // Main function of the program
    int statusCode = processPoolInit();
    if (statusCode == 1)
    {
        workerCode();
    }
    else if (statusCode == 2)
    {
        createInitialActor(0);
        createInitialActor(1);
        for (int i = 0; i < size - 3; i++)
        {
            createInitialActor(2);
        }

        int masterStatus = masterPoll();
        while (masterStatus)
        {
            masterStatus = masterPoll();
        }
    }

    processPoolFinalise();
    MPI_Finalize();
    return 0;
}

static void createInitialActor(int type)
{
    int data[1];
    int workerPid = startWorkerProcess();
    data[0] = type;
    MPI_Send(data, 1, MPI_INT, workerPid, 0, MPI_COMM_WORLD);
}

static void workerCode()
{
    int workerStatus = 1, data[1];
    while (workerStatus)
    {
        int parentId = getCommandData();
        MPI_Recv(data, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (data[0] == 0)
        {
            control();
        }
        else if (data[0] == 1)
        {
            RoadJunction();
        }
        else if (data[0] == 2)
        {
            Vehicle();
        }
        workerStatus = workerSleep();
    }
}

static void control()
{
    // Initialise time
    time_t seconds = 0;                         // Stores the current time in seconds
    time_t start_seconds = getCurrentSeconds(); // Capture the start time
    int elapsed_mins = 0;                       // Counter for elapsed minutes in the simulation

    // Update the total vehicles
    total_vehicles += INITIAL_VEHICLES; // Increment the total vehicle count by the initial vehicles

    MPI_Status status; // Status variable for MPI operations
    // Main loop to continue until the maximum minutes are reached

    // record the time
    int round = 0;
    double total_time = 0.0, start_time, end_time;

    while (elapsed_mins < MAX_MINS)
    {
        // record the start time
        start_time = MPI_Wtime();

        time_t current_seconds = getCurrentSeconds(); // Get the current time in seconds
        // Check if a second has passed
        if (current_seconds != seconds)
        {
            seconds = current_seconds;
            // Check if it's time to output status
            if (seconds - start_seconds > 0)
            {
                // Output status every MIN_LENGTH_SECONDS seconds
                if ((seconds - start_seconds) % MIN_LENGTH_SECONDS == 0)
                {
                    elapsed_mins++; // Increment the elapsed minutes
                    // Ask roadjunction to randomly generate vehicles
                    int total_new_vehicles = getRandomInteger(100, 200); // Random number of vehicles to create
                    int participating_processes = size - 3;              // Number of processes participating (excluding 3 reserved ranks)

                    // Determine the number of vehicles for each process to create
                    int vehicles_per_process = total_new_vehicles / participating_processes;
                    int extra_vehicles = total_new_vehicles % participating_processes;

                    // Distribute vehicle creation tasks among the processes
                    for (int i = 3; i < size; i++)
                    {
                        int vehicles_to_create = vehicles_per_process + (i - 3 < extra_vehicles);
                        MPI_Send(&vehicles_to_create, 1, MPI_INT, i, RANDOM_CREATE_TAG, MPI_COMM_WORLD);
                    }

                    // Receive the number of vehicles created by each process
                    int total_created_vehicles = 0;
                    for (int i = 3; i < size; i++)
                    {
                        int created_vehicles = 0;
                        MPI_Recv(&created_vehicles, 1, MPI_INT, i, VEHICLE_CREATED_TAG, MPI_COMM_WORLD, &status);
                        total_created_vehicles += created_vehicles;
                    }
                    // Update total vehicles count
                    total_vehicles += total_created_vehicles;

                    // Print summary information every SUMMARY_FREQUENCY minutes
                    if (elapsed_mins % SUMMARY_FREQUENCY == 0)
                    {
                        printf("[Time: %d mins] %d vehicles, %d passengers delivered, %d stranded passengers, %d crashed vehicles, %d vehicles exhausted fuel\n",
                               elapsed_mins, total_vehicles, passengers_delivered, passengers_stranded, vehicles_crashed, vehicles_exhausted_fuel);
                    }
                }
            }
        }

        // Command roadjunction to update each intersection
        MPI_Send(&elapsed_mins, 1, MPI_INT, ROADJUNCTION_RANK, UPDATE_JUNCTION_TAG, MPI_COMM_WORLD);
        // Wait for the completion message from roadjunction
        int finished_Road = 0;
        MPI_Recv(&finished_Road, 1, MPI_INT, ROADJUNCTION_RANK, FINISHED_UPDATED_JUNCTION_TAG, MPI_COMM_WORLD, &status);

        // Request vehicle status updates
        for (int i = 3; i < size; i++)
        {
            MPI_Send(&elapsed_mins, 1, MPI_INT, i, UPDATE_VEHICLES_TAG, MPI_COMM_WORLD);
        }

        // Receive updated results from the vehicle processes
        for (int i = 3; i < size; i++)
        {
            int data[4];              // Buffer to hold received data
            int finished_Vehicle = 0; // Flag to indicate vehicle status update is finished
            // Receive the data from vehicle process
            MPI_Recv(data, 4, MPI_INT, i, UPDATED_RESULTS_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_SOURCE == i) // Check the source of the message
            {
                // Aggregate results from the vehicle processes
                vehicles_exhausted_fuel += data[0];
                passengers_stranded += data[1];
                vehicles_crashed += data[2];
                passengers_delivered += data[3];
            }
            // Wait for the completion message from vehicle
            MPI_Recv(&finished_Vehicle, 1, MPI_INT, i, FINISHED_UPDATED_VEHICLES_TAG, MPI_COMM_WORLD, &status);
        }

        // record the end time
        end_time = MPI_Wtime();
        // increment the total time
        total_time += (end_time - start_time);
        round++;

        // 每隔100轮输出平均时间
        if (round % 100 == 0 || round % 50 == 0)
        {
            printf("After %d loops, average time per loop is: %f seconds\n", round, total_time / round);
        }
    }
    // Send a final write command to all vehicle processes
    for (int i = 3; i < size; i++)
    {
        int final_write = 1;
        MPI_Send(&final_write, 1, MPI_INT, i, FILE_WRITE_TAG, MPI_COMM_WORLD);
    }

    // Final summary printout
    printf("Finished after %d mins: %d vehicles, %d passengers delivered, %d passengers stranded, %d crashed vehicles, %d vehicles exhausted fuel\n",
           elapsed_mins, total_vehicles, passengers_delivered, passengers_stranded, vehicles_crashed, vehicles_exhausted_fuel);

    // Print the total time taken for the simulation
    printf("Total time for %d loops is: %f seconds\n", round, total_time);
    printf("Average time per loop is: %f seconds\n", total_time / round);

    // Shut down the MPI worker pool before exiting
    shutdownPool();
}

static void RoadJunction()
{
    // Load the road map from the file
    loadRoadMap(map_filename);

    while (1)
    {

        MPI_Status status;
        int flag1 = 0;
        // Check if there's a message from control to update junctions
        MPI_Iprobe(CONTROL_RANK, UPDATE_JUNCTION_TAG, MPI_COMM_WORLD, &flag1, &status);
        if (flag1)
        {
            int elapsed_mins = 0;
            // Receive the elapsed minutes from control
            MPI_Recv(&elapsed_mins, 1, MPI_INT, CONTROL_RANK, UPDATE_JUNCTION_TAG, MPI_COMM_WORLD, &status);
            // If the message was from control, proceed to update

            if (status.MPI_SOURCE == CONTROL_RANK)
            {
                // Inform vehicles to begin road updates
                for (int count = 3; count < size; count++)
                {
                    int begin = 1;
                    MPI_Send(&begin, 1, MPI_INT, count, BEGIN_ROADS_UPDATE_TAG, MPI_COMM_WORLD);
                }

                // Loop through all junctions to update their status
                for (int i = 0; i < num_junctions; i++)
                {
                    // Signal the start of a junction update to vehicles
                    for (int count = 3; count < size; count++)
                    {
                        int loop_begin = 1;
                        MPI_Send(&loop_begin, 1, MPI_INT, count, LOOP_BEGIN_TAG, MPI_COMM_WORLD);
                    }

                    // Check and update traffic lights if necessary
                    if (roadMap[i].hasTrafficLights && roadMap[i].num_roads > 0)
                    {
                        for (int count = 3; count < size; count++)
                        {
                            roadMap[i].trafficLightsRoadEnabled = elapsed_mins % roadMap[i].num_roads;
                            int data[2] = {i, roadMap[i].trafficLightsRoadEnabled};
                            MPI_Send(data, 2, MPI_INT, count, TRAFFICLIGHT_ENABLED_TAG, MPI_COMM_WORLD);
                        }
                    }

                    // Iterate over each road connected to the current junction
                    for (int j = 0; j < roadMap[i].num_roads; j++)
                    {
                        struct RoadStruct *road = &roadMap[i].roads[j];
                        int num_vehicles_on_road = 0;
                        // Request updates for vehicles on this road from all vehicles
                        for (int count = 3; count < size; count++)
                        {
                            for (int k = 0; k < MAX_VEHICLES; k++)
                            {
                                // Send road ID to vehicles to get status
                                MPI_Send(&road->id, 1, MPI_INT, count, VEHICLE_INDEX_TAG, MPI_COMM_WORLD);
                                int flag = 0;
                                // Receive vehicle presence confirmation
                                MPI_Recv(&flag, 1, MPI_INT, count, VEHICLE_INDEX_RETURN_ID_TAG, MPI_COMM_WORLD, &status);
                                if (flag)
                                    num_vehicles_on_road++;
                            }
                        }

                        // Adjust road speed based on the number of vehicles (congestion)
                        road->currentSpeed = road->maxSpeed - num_vehicles_on_road;
                        // Send the updated speed to all vehicles
                        for (int count = 3; count < size; count++)
                        {
                            int data[3] = {i, j, road->currentSpeed};
                            MPI_Send(data, 3, MPI_INT, count, ROAD_SPEED_TAG, MPI_COMM_WORLD);
                        }

                        if (road->currentSpeed < 10)
                        {
                            road->currentSpeed = 10;
                            // Send the updated minimum speed to all vehicles
                            for (int count = 3; count < size; count++)
                            {
                                int data[3] = {i, j, road->currentSpeed};
                                MPI_Send(data, 3, MPI_INT, count, ROAD_SPEED_TAG, MPI_COMM_WORLD);
                            }
                        }
                    }

                    // Signal the end of a junction update to vehicles
                    for (int i = 3; i < size; i++)
                    {
                        int loop_end = 1;
                        MPI_Send(&loop_end, 1, MPI_INT, i, LOOP_END_TAG, MPI_COMM_WORLD);
                    }
                }

                // Signal to control that junction update is completed
                int finished = 1;
                MPI_Send(&finished, 1, MPI_INT, CONTROL_RANK, FINISHED_UPDATED_JUNCTION_TAG, MPI_COMM_WORLD);
            }
        }

        // Check for break messages from vehicles to exit the loop
        int flag2 = 0;
        MPI_Iprobe(3, BREAK_MESSAGE_TAG, MPI_COMM_WORLD, &flag2, &status);
        if (flag2)
        {
            int break_msg;
            // Receive the break message from a vehicle
            MPI_Recv(&break_msg, 1, MPI_INT, 3, BREAK_MESSAGE_TAG, MPI_COMM_WORLD, &status);
            if (break_msg)
            {
                // Break out of the infinite loop to terminate the road junction process
                break;
            }
        }
    }
}

static void Vehicle()
{
    // load the road map
    loadRoadMap(map_filename);

    // init vehicle
    vehicles = (struct VehicleStruct *)malloc(sizeof(struct VehicleStruct) * MAX_VEHICLES);
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        vehicles[i].active = 0;
        vehicles[i].roadOn = NULL;
        vehicles[i].currentJunction = NULL;
        vehicles[i].maxSpeed = 0;
    }
    // Calculate the number of vehicles that should be initialized per process
    int participating_processes = size - 3;
    int vehicles_per_process = INITIAL_VEHICLES / participating_processes;
    int extra_vehicles = INITIAL_VEHICLES % participating_processes;
    if (rank != size - 1)
    {
        // Normal share of vehicles to initialize
        int count = initVehicles(vehicles_per_process);
    }
    else
    {
        // Last rank takes extra vehicles if the division is not even
        int count = initVehicles(vehicles_per_process + extra_vehicles);
    }

    while (1)
    {
        // Accept messages from control for randomly generated vehicles
        MPI_Status status;
        int flag1 = 0; // Used to mark whether a message has arrived
        MPI_Iprobe(CONTROL_RANK, RANDOM_CREATE_TAG, MPI_COMM_WORLD, &flag1, &status);
        if (flag1)
        {
            int num_new_vehicles = 0;
            MPI_Recv(&num_new_vehicles, 1, MPI_INT, CONTROL_RANK, RANDOM_CREATE_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_SOURCE == CONTROL_RANK)
            {
                int count = 0; // Counter for successfully activated vehicles
                for (int i = 0; i < num_new_vehicles; i++)
                {
                    enum VehicleType vehicleType;
                    vehicleType = activateRandomVehicle();
                    int res = activateVehicle(vehicleType);
                    if (res != -1)
                    {
                        count++;
                    }
                }
                // Report back the number of vehicles successfully created
                MPI_Send(&count, 1, MPI_INT, CONTROL_RANK, VEHICLE_CREATED_TAG, MPI_COMM_WORLD);
            }
        }

        // Receive news from raodjunction for road updates
        int flag2 = 0;
        MPI_Iprobe(ROADJUNCTION_RANK, BEGIN_ROADS_UPDATE_TAG, MPI_COMM_WORLD, &flag2, &status);
        if (flag2)
        {
            int begin;
            MPI_Recv(&begin, 1, MPI_INT, ROADJUNCTION_RANK, BEGIN_ROADS_UPDATE_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_SOURCE == ROADJUNCTION_RANK)
            {
                for (int i = 0; i < num_junctions; i++)
                {
                    int loop_begin;
                    MPI_Recv(&loop_begin, 1, MPI_INT, ROADJUNCTION_RANK, LOOP_BEGIN_TAG, MPI_COMM_WORLD, &status);
                    while (1)
                    {
                        int end = 0;
                        MPI_Status status;
                        int flag = 0;
                        // Use MPI_Iprobe to check if a message has arrived
                        MPI_Iprobe(ROADJUNCTION_RANK, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
                        if (flag)
                        {
                            switch (status.MPI_TAG)
                            {
                            case TRAFFICLIGHT_ENABLED_TAG: // Handle traffic light enabled state change
                            {
                                int data[2];
                                MPI_Recv(data, 2, MPI_INT, ROADJUNCTION_RANK, TRAFFICLIGHT_ENABLED_TAG, MPI_COMM_WORLD, &status);
                                roadMap[data[0]].trafficLightsRoadEnabled = data[1];
                                break;
                            }
                            case VEHICLE_INDEX_TAG: // Handle vehicle index update
                            {
                                int id;
                                MPI_Recv(&id, 1, MPI_INT, ROADJUNCTION_RANK, VEHICLE_INDEX_TAG, MPI_COMM_WORLD, &status);
                                // Traverse the entire vehicle array, find the vehicle, and see if there is a vehicle whose roadOn is this road
                                int index = 0;
                                for (int i = 0; i < MAX_VEHICLES; i++)
                                {
                                    if (vehicles[i].active && vehicles[i].roadOn != NULL && vehicles[i].roadOn->id == id)
                                    {
                                        index = 1;
                                        break;
                                    }
                                }
                                MPI_Send(&index, 1, MPI_INT, ROADJUNCTION_RANK, VEHICLE_INDEX_RETURN_ID_TAG, MPI_COMM_WORLD);
                                break;
                            }
                            case ROAD_SPEED_TAG: // Handle road speed update
                            {
                                int data[3];
                                MPI_Recv(data, 3, MPI_INT, ROADJUNCTION_RANK, ROAD_SPEED_TAG, MPI_COMM_WORLD, &status);

                                roadMap[data[0]].roads[data[1]].currentSpeed = data[2];

                                break;
                            }
                            case LOOP_END_TAG:
                            {
                                int loop_end;
                                MPI_Recv(&loop_end, 1, MPI_INT, ROADJUNCTION_RANK, LOOP_END_TAG, MPI_COMM_WORLD, &status);
                                if (loop_end)
                                {
                                    end = 1;
                                }
                                break;
                            }
                            }
                        }

                        if (end)
                        {
                            break;
                        }
                    }
                }
            }
        }

        // Accept messages from control to update the vehicle
        int flag3 = 0;
        MPI_Iprobe(CONTROL_RANK, UPDATE_VEHICLES_TAG, MPI_COMM_WORLD, &flag3, &status);
        if (flag3)
        {
            int elapsed_mins;
            MPI_Recv(&elapsed_mins, 1, MPI_INT, CONTROL_RANK, UPDATE_VEHICLES_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_SOURCE == CONTROL_RANK)
            {
                for (int i = 0; i < MAX_VEHICLES; i++)
                {
                    if (vehicles[i].active)
                    {
                        handleVehicleUpdate(i);
                    }
                }

                // Pack and send these four data to control
                int data[4];
                data[0] = vehicles_exhausted_fuel;
                data[1] = passengers_stranded;
                data[2] = vehicles_crashed;
                data[3] = passengers_delivered;
                MPI_Send(data, 4, MPI_INT, CONTROL_RANK, UPDATED_RESULTS_TAG, MPI_COMM_WORLD);
                // restart the variable
                vehicles_exhausted_fuel = 0;
                passengers_stranded = 0;
                vehicles_crashed = 0;
                passengers_delivered = 0;
                // Send to control, task completed
                int finished = 1;
                MPI_Send(&finished, 1, MPI_INT, CONTROL_RANK, FINISHED_UPDATED_VEHICLES_TAG, MPI_COMM_WORLD);
            }
        }

        int flag4 = 0;
        MPI_Iprobe(CONTROL_RANK, FILE_WRITE_TAG, MPI_COMM_WORLD, &flag4, &status);
        if (flag4)
        {
            int final_write;
            MPI_Recv(&final_write, 1, MPI_INT, CONTROL_RANK, FILE_WRITE_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_SOURCE == CONTROL_RANK)
            {
                // Send data information to process 3
                if (rank != 3)
                {
                    for (int i = 0; i < num_junctions; i++)
                    {
                        // Send data for each intersection
                        MPI_Send(&roadMap[i].total_number_vehicles, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
                        MPI_Send(&roadMap[i].total_number_crashes, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);

                        for (int j = 0; j < roadMap[i].num_roads; j++)
                        {
                            // Send data for each road
                            int from_id = roadMap[i].roads[j].from->id;
                            int to_id = roadMap[i].roads[j].to->id;
                            MPI_Send(&from_id, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
                            MPI_Send(&to_id, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
                            MPI_Send(&roadMap[i].roads[j].total_number_vehicles, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
                            MPI_Send(&roadMap[i].roads[j].max_concurrent_vehicles, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
                        }
                    }
                }
                // Statistical information
                if (rank == 3)
                {

                    MPI_Status status;
                    for (int source = 4; source < size; source++)
                    {
                        for (int i = 0; i < num_junctions; i++)
                        {
                            // Receive data for each intersection
                            int total_number_vehicles, total_number_crashes;
                            MPI_Recv(&total_number_vehicles, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);
                            MPI_Recv(&total_number_crashes, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);
                            roadMap[i].total_number_vehicles += total_number_vehicles;
                            roadMap[i].total_number_crashes += total_number_crashes;

                            for (int j = 0; j < roadMap[i].num_roads; j++)
                            {
                                // Receive data for each road
                                int from_id, to_id, road_vehicles, max_concurrent;
                                MPI_Recv(&road_vehicles, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);
                                MPI_Recv(&max_concurrent, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);
                                roadMap[i].roads[j].total_number_vehicles += road_vehicles;
                                roadMap[i].roads[j].max_concurrent_vehicles += max_concurrent;
                            }
                        }
                    }
                }
                if (rank == 3)
                {
                    // writeDetailedInfo();
                    // Send a message to roadjunction to ask him to break
                    int msg = 1;
                    MPI_Send(&msg, 1, MPI_INT, ROADJUNCTION_RANK, BREAK_MESSAGE_TAG, MPI_COMM_WORLD);
                    break;
                }
                break;
            }
        }
    }
}
static void handleVehicleUpdate(int i)
{

    if (getCurrentSeconds() - vehicles[i].start_t > vehicles[i].fuel)
    {
        vehicles_exhausted_fuel++;
        passengers_stranded += vehicles[i].passengers;

        vehicles[i].active = 0;
        return;
    }

    // If the vehicle is on a certain road rather than at a certain intersection
    if (vehicles[i].roadOn != NULL && vehicles[i].currentJunction == NULL)
    {
        // Means that the vehicle is currently on a road
        time_t sec = getCurrentSeconds();
        int latest_time = sec - vehicles[i].last_distance_check_secs;
        if (latest_time < 1)
            return;
        vehicles[i].last_distance_check_secs = sec;
        double travelled_length = latest_time * vehicles[i].speed;
        vehicles[i].remaining_distance -= travelled_length;
        if (vehicles[i].remaining_distance <= 0)
        {
            // Left the road and arrived at the target junction
            vehicles[i].arrived_road_time = 0;
            vehicles[i].last_distance_check_secs = 0;
            vehicles[i].remaining_distance = 0;
            vehicles[i].speed = 0;
            vehicles[i].currentJunction = vehicles[i].roadOn->to;
            vehicles[i].currentJunction->num_vehicles++;
            vehicles[i].currentJunction->total_number_vehicles++;
            vehicles[i].roadOn->numVehiclesOnRoad--;
            vehicles[i].roadOn = NULL;
        }
    }

    // If the vehicle is at a certain intersection

    if (vehicles[i].currentJunction != NULL)
    {
        // If the vehicle is at an intersection and is not on the road
        if (vehicles[i].roadOn == NULL)
        {
            // If the road is NULL then the vehicle is on a junction and not on a road
            if (vehicles[i].currentJunction->id == vehicles[i].dest)
            {
                // Arrived! Job done!
                passengers_delivered += vehicles[i].passengers;
                vehicles[i].active = 0;
            }
            else
            {
                int next_junction_target = planRoute(vehicles[i].currentJunction->id, vehicles[i].dest, roadMap, num_junctions, num_roads);
                if (next_junction_target != -1)
                {
                    int road_to_take = findAppropriateRoad(next_junction_target, vehicles[i].currentJunction);
                    assert(vehicles[i].currentJunction->roads[road_to_take].to->id == next_junction_target);

                    vehicles[i].roadOn = &(vehicles[i].currentJunction->roads[road_to_take]);
                    vehicles[i].roadOn->numVehiclesOnRoad++;
                    vehicles[i].roadOn->total_number_vehicles++;
                    // If the number of vehicles on the road exceeds the maximum number of vehicles on the road, update the maximum number of vehicles
                    if (vehicles[i].roadOn->max_concurrent_vehicles < vehicles[i].roadOn->numVehiclesOnRoad)
                    {
                        vehicles[i].roadOn->max_concurrent_vehicles = vehicles[i].roadOn->numVehiclesOnRoad;
                    }
                    // The remaining distance of the vehicle on this road is the length of the selected road
                    vehicles[i].remaining_distance = vehicles[i].roadOn->roadLength;
                    // The vehicle's speed is updated to the minimum of the vehicle's maximum speed and the current speed of the road
                    vehicles[i].speed = vehicles[i].roadOn->currentSpeed;
                    if (vehicles[i].speed > vehicles[i].maxSpeed)
                        vehicles[i].speed = vehicles[i].maxSpeed;
                }
                else
                {
                    // Report error (this should never happen)
                    fprintf(stderr, "No longer a viable route\n");
                    exit(-1);
                }
            }
        }
        // Here we have selected a junction, now it's time to determine if the vehicle can be released from the junction
        char take_road = 0;
        if (vehicles[i].currentJunction->hasTrafficLights)
        {
            // Need to check that we can go, otherwise need to wait until road enabled by traffic light
            take_road = vehicles[i].roadOn == &vehicles[i].currentJunction->roads[vehicles[i].currentJunction->trafficLightsRoadEnabled];
        }
        else
        {
            // If not traffic light then there is a chance of collision
            int collision = getRandomInteger(0, 8) * vehicles[i].currentJunction->num_vehicles;
            if (collision > 20)
            {
                // Vehicle has crashed!
                passengers_stranded += vehicles[i].passengers;
                vehicles_crashed++;
                vehicles[i].active = 0;
                vehicles[i].currentJunction->total_number_crashes++;
            }
            take_road = 1;
        }
        // If take the road then clear the junction
        if (take_road)
        {
            vehicles[i].last_distance_check_secs = getCurrentSeconds();
            vehicles[i].currentJunction->num_vehicles--;
            vehicles[i].currentJunction = NULL;
        }
    }
}

static int initVehicles(int num_initial)
{
    int count = 0;
    for (int i = 0; i < num_initial; i++)
    {
        enum VehicleType vehicleType;
        vehicleType = activateRandomVehicle();
        int res = activateVehicle(vehicleType);
        if (res != -1)
        {
            count++;
        }
    }

    return count;
}

static void loadRoadMap(char *filename)
{
    enum ReadMode currentMode = NONE;
    char buffer[MAX_ROAD_LEN];
    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        fprintf(stderr, "Error opening roadmap file '%s'\n", filename);
        exit(-1);
    }

    while (fgets(buffer, MAX_ROAD_LEN, f))
    {
        if (buffer[0] == '%')
            continue;
        if (buffer[0] == '#')
        {
            if (strncmp("# Road layout:", buffer, 14) == 0)
            {
                char *s = strstr(buffer, ":");
                num_junctions = atoi(&s[1]);
                roadMap = (struct JunctionStruct *)malloc(sizeof(struct JunctionStruct) * num_junctions);
                for (int i = 0; i < num_junctions; i++)
                {
                    roadMap[i].id = i;
                    roadMap[i].num_roads = 0;
                    roadMap[i].num_vehicles = 0;
                    roadMap[i].hasTrafficLights = 0;
                    roadMap[i].total_number_crashes = 0;
                    roadMap[i].total_number_vehicles = 0;
                    // Not ideal to allocate all roads size here
                    roadMap[i].roads = (struct RoadStruct *)malloc(sizeof(struct RoadStruct) * MAX_NUM_ROADS_PER_JUNCTION);
                }
                currentMode = ROADMAP;
            }
            if (strncmp("# Traffic lights:", buffer, 17) == 0)
            {
                currentMode = TRAFFICLIGHTS;
            }
        }
        else
        {
            if (currentMode == ROADMAP)
            {
                char *space = strstr(buffer, " ");
                *space = '\0';
                int from_id = atoi(buffer);
                char *nextspace = strstr(&space[1], " ");
                *nextspace = '\0';
                int to_id = atoi(&space[1]);
                char *nextspace2 = strstr(&nextspace[1], " ");
                *nextspace = '\0';
                int roadlength = atoi(&nextspace[1]);
                int speed = atoi(&nextspace2[1]);
                if (roadMap[from_id].num_roads >= MAX_NUM_ROADS_PER_JUNCTION)
                {
                    fprintf(stderr, "Error: Tried to create road %d at junction %d, but maximum number of roads is %d, increase 'MAX_NUM_ROADS_PER_JUNCTION'",
                            roadMap[from_id].num_roads, from_id, MAX_NUM_ROADS_PER_JUNCTION);
                    exit(-1);
                }
                roadMap[from_id].roads[roadMap[from_id].num_roads].id = num_roads;
                roadMap[from_id].roads[roadMap[from_id].num_roads].from = &roadMap[from_id];
                roadMap[from_id].roads[roadMap[from_id].num_roads].to = &roadMap[to_id];
                roadMap[from_id].roads[roadMap[from_id].num_roads].roadLength = roadlength;
                roadMap[from_id].roads[roadMap[from_id].num_roads].maxSpeed = speed;
                roadMap[from_id].roads[roadMap[from_id].num_roads].numVehiclesOnRoad = 0;
                roadMap[from_id].roads[roadMap[from_id].num_roads].currentSpeed = speed;
                roadMap[from_id].roads[roadMap[from_id].num_roads].total_number_vehicles = 0;
                roadMap[from_id].roads[roadMap[from_id].num_roads].max_concurrent_vehicles = 0;
                roadMap[from_id].num_roads++;
                num_roads++;
            }
            else if (currentMode == TRAFFICLIGHTS)
            {
                int id = atoi(buffer);
                if (roadMap[id].num_roads > 0)
                    roadMap[id].hasTrafficLights = 1;
            }
        }
    }
    fclose(f);
}

/**
 * Activates a vehicle and sets its type and route randomly
 **/
static int activateRandomVehicle()
{
    int random_vehicle_type = getRandomInteger(0, 5);
    enum VehicleType vehicleType;
    if (random_vehicle_type == 0)
    {
        vehicleType = BUS;
    }
    else if (random_vehicle_type == 1)
    {
        vehicleType = CAR;
    }
    else if (random_vehicle_type == 2)
    {
        vehicleType = MINI_BUS;
    }
    else if (random_vehicle_type == 3)
    {
        vehicleType = COACH;
    }
    else if (random_vehicle_type == 4)
    {
        vehicleType = MOTORBIKE;
    }
    else if (random_vehicle_type == 5)
    {
        vehicleType = BIKE;
    }
    // 返回激活的车辆的索引
    return vehicleType;
}

/**
 * Activates a vehicle with a specific type, will find an idle vehicle data
 * element and then initialise this with a random (but valid) route between
 * two junction. The new vehicle's index is returned,
 * or -1 if there are no free slots.
 **/
static int activateVehicle(enum VehicleType vehicleType)
{
    int id = findFreeVehicle();
    if (id >= 0)
    {
        vehicles[id].id = id;
        vehicles[id].active = 1;
        vehicles[id].start_t = getCurrentSeconds();
        vehicles[id].last_distance_check_secs = 0;
        vehicles[id].speed = 0;
        vehicles[id].remaining_distance = 0;
        vehicles[id].arrived_road_time = 0;
        vehicles[id].source = vehicles[id].dest = getRandomInteger(0, num_junctions);
        while (vehicles[id].dest == vehicles[id].source)
        {
            // Ensure that the source and destination are different
            vehicles[id].dest = getRandomInteger(0, num_junctions);
            if (vehicles[id].dest != vehicles[id].source)
            {
                // See if there is a viable route between the source and destination
                int next_jnct = planRoute(vehicles[id].source, vehicles[id].dest, roadMap, num_junctions, num_roads);
                if (next_jnct == -1)
                {
                    // Regenerate source and dest
                    vehicles[id].source = vehicles[id].dest = getRandomInteger(0, num_junctions);
                }
            }
        }
        vehicles[id].currentJunction = &roadMap[vehicles[id].source];
        vehicles[id].currentJunction->num_vehicles++;
        vehicles[id].currentJunction->total_number_vehicles++;
        vehicles[id].roadOn = NULL;
        if (vehicleType == CAR)
        {
            vehicles[id].maxSpeed = CAR_MAX_SPEED;
            vehicles[id].passengers = getRandomInteger(1, CAR_PASSENGERS);
            vehicles[id].fuel = getRandomInteger(CAR_MIN_FUEL, CAR_MAX_FUEL);
        }
        else if (vehicleType == BUS)
        {
            vehicles[id].maxSpeed = BUS_MAX_SPEED;
            vehicles[id].passengers = getRandomInteger(1, BUS_PASSENGERS);
            vehicles[id].fuel = getRandomInteger(BUS_MIN_FUEL, BUS_MAX_FUEL);
        }
        else if (vehicleType == MINI_BUS)
        {
            vehicles[id].maxSpeed = MINI_BUS_MAX_SPEED;
            vehicles[id].passengers = getRandomInteger(1, MINI_BUS_PASSENGERS);
            vehicles[id].fuel = getRandomInteger(MINI_BUS_MIN_FUEL, MINI_BUS_MAX_FUEL);
        }
        else if (vehicleType == COACH)
        {
            vehicles[id].maxSpeed = COACH_MAX_SPEED;
            vehicles[id].passengers = getRandomInteger(1, COACH_PASSENGERS);
            vehicles[id].fuel = getRandomInteger(COACH_MIN_FUEL, COACH_MAX_FUEL);
        }
        else if (vehicleType == MOTORBIKE)
        {
            vehicles[id].maxSpeed = MOTOR_BIKE_MAX_SPEED;
            vehicles[id].passengers = getRandomInteger(1, MOTOR_BIKE_PASSENGERS);
            vehicles[id].fuel = getRandomInteger(MOTOR_BIKE_MIN_FUEL, MOTOR_BIKE_MAX_FUEL);
        }
        else if (vehicleType == BIKE)
        {
            vehicles[id].maxSpeed = BIKE_MAX_SPEED;
            vehicles[id].passengers = getRandomInteger(1, BIKE_PASSENGERS);
            vehicles[id].fuel = getRandomInteger(BIKE_MIN_FUEL, BIKE_MAX_FUEL);
        }
        else
        {
            fprintf(stderr, "Unknown vehicle type\n");
        }
        return id;
    }
    return -1;
}

static int planRoute(int source_id, int dest_id, struct JunctionStruct *roadMap, int num_junctions, int num_roads)
{
    if (VERBOSE_ROUTE_PLANNER)
        printf("Search for route from %d to %d\n", source_id, dest_id);
    double *dist = (double *)malloc(sizeof(double) * num_junctions);
    char *active = (char *)malloc(sizeof(char) * num_junctions);
    struct JunctionStruct **prev = (struct JunctionStruct **)malloc(sizeof(struct JunctionStruct *) * num_junctions);

    int activeJunctions = num_junctions;
    for (int i = 0; i < num_junctions; i++)
    {
        active[i] = 1;
        prev[i] = NULL;
        if (i != source_id)
        {
            dist[i] = LARGE_NUM;
        }
    }
    dist[source_id] = 0;
    while (activeJunctions > 0)
    {
        int v_idx = findIndexOfMinimum(dist, active, num_junctions);
        if (v_idx == dest_id)
            break;
        struct JunctionStruct *v = &roadMap[v_idx];
        active[v_idx] = 0;
        activeJunctions--;

        for (int i = 0; i < v->num_roads; i++)
        {
            if (active[v->roads[i].to->id] && dist[v_idx] != LARGE_NUM)
            {
                double alt = dist[v_idx] + v->roads[i].roadLength / (v->id == source_id ? v->roads[i].currentSpeed : v->roads[i].maxSpeed);
                if (alt < dist[v->roads[i].to->id])
                {
                    dist[v->roads[i].to->id] = alt;
                    prev[v->roads[i].to->id] = v;
                }
            }
        }
    }
    free(dist);
    free(active);
    int u_idx = dest_id;
    int *route = (int *)malloc(sizeof(int) * num_junctions);
    int route_len = 0;
    if (prev[u_idx] != NULL || u_idx == source_id)
    {
        if (VERBOSE_ROUTE_PLANNER)
            printf("Start at %d\n", u_idx);
        while (prev[u_idx] != NULL)
        {
            route[route_len] = u_idx;
            u_idx = prev[u_idx]->id;
            if (VERBOSE_ROUTE_PLANNER)
                printf("Route %d\n", u_idx);
            route_len++;
        }
    }
    free(prev);
    if (route_len > 0)
    {
        int next_jnct = route[route_len - 1];
        if (VERBOSE_ROUTE_PLANNER)
            printf("Found next junction is %d\n", next_jnct);
        free(route);
        return next_jnct;
    }
    if (VERBOSE_ROUTE_PLANNER)
        printf("Failed to find route between %d and %d\n", source_id, dest_id);
    free(route);
    return -1;
}

void writeDetailedInfo()
{
    FILE *f = fopen("../result/results", "w");
    for (int i = 0; i < num_junctions; i++)
    {
        fprintf(f, "Junction %d: %d total vehicles and %d crashes\n", i, roadMap[i].total_number_vehicles, roadMap[i].total_number_crashes);
        for (int j = 0; j < roadMap[i].num_roads; j++)
        {
            fprintf(f, "--> Road from %d to %d: Total vehicles %d and %d maximum concurrently\n", roadMap[i].roads[j].from->id,
                    roadMap[i].roads[j].to->id, roadMap[i].roads[j].total_number_vehicles, roadMap[i].roads[j].max_concurrent_vehicles);
        }
    }
    fclose(f);
}
