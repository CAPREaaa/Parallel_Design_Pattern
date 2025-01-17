// include/data_structures.h
#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <time.h>

#define VERBOSE_ROUTE_PLANNER 0
#define LARGE_NUM 99999999.0
#define BUFFER_SIZE 1024 * 1024 * 1024

#define CONTROL_RANK 1
#define ROADJUNCTION_RANK 2

#define UPDATED_RESULTS_TAG 1
#define VEHICLE_CREATED_TAG 2
#define RANDOM_CREATE_TAG 3
#define UPDATE_JUNCTION_TAG 4
#define UPDATED_JUNCTION_TAG 5
#define UPDATE_VEHICLES_TAG 6
#define FINISHED_UPDATED_JUNCTION_TAG 7
#define FINISHED_UPDATED_VEHICLES_TAG 8
#define PLAN_ROUTE_TAG 9
#define ACTIVE_0 10
#define TRAFFICLIGHT_ENABLED_TAG 11
#define ROAD_SPEED_TAG 12
#define VEHICLE_INDEX_TAG 13
#define BEGIN_ROADS_UPDATE_TAG 14
#define LOOP_BEGIN_TAG 15
#define LOOP_END_TAG 16
#define VEHICLE_INDEX_RETURN_ID_TAG 17
#define FILE_WRITE_TAG 18
#define BREAK_MESSAGE_TAG 19
#define FINISH_WRITE_TAG 20

#define MAX_ROAD_LEN 100
#define MAX_VEHICLES 500
#define MAX_MINS 100
#define MIN_LENGTH_SECONDS 2
#define MAX_NUM_ROADS_PER_JUNCTION 50
#define SUMMARY_FREQUENCY 5
#define INITIAL_VEHICLES 50

#define BUS_PASSENGERS 80
#define BUS_MAX_SPEED 50
#define BUS_MIN_FUEL 10
#define BUS_MAX_FUEL 100
#define CAR_PASSENGERS 4
#define CAR_MAX_SPEED 100
#define CAR_MIN_FUEL 1
#define CAR_MAX_FUEL 40
#define MINI_BUS_MAX_SPEED 80
#define MINI_BUS_PASSENGERS 15
#define MINI_BUS_MIN_FUEL 2
#define MINI_BUS_MAX_FUEL 75
#define COACH_MAX_SPEED 60
#define COACH_PASSENGERS 40
#define COACH_MIN_FUEL 20
#define COACH_MAX_FUEL 100
#define MOTOR_BIKE_MAX_SPEED 120
#define MOTOR_BIKE_PASSENGERS 2
#define MOTOR_BIKE_MIN_FUEL 1
#define MOTOR_BIKE_MAX_FUEL 20
#define BIKE_MAX_SPEED 10
#define BIKE_PASSENGERS 1
#define BIKE_MIN_FUEL 2
#define BIKE_MAX_FUEL 10


enum ReadMode
{
    NONE,
    ROADMAP,
    TRAFFICLIGHTS
};

enum VehicleType
{
    CAR,
    BUS,
    MINI_BUS,
    COACH,
    MOTORBIKE,
    BIKE
};

struct JunctionStruct
{
    int id, num_roads, num_vehicles;
    char hasTrafficLights;
    int trafficLightsRoadEnabled;
    int total_number_crashes, total_number_vehicles;
    struct RoadStruct *roads;
};

struct RoadStruct
{
    struct JunctionStruct *from, *to;
    int id; // Add road id
    int roadLength, maxSpeed, numVehiclesOnRoad, currentSpeed;
    int total_number_vehicles, max_concurrent_vehicles;
};

struct VehicleStruct
{
    int id;
    int passengers, source, dest, maxSpeed;
    // Distance is in meters
    int speed, arrived_road_time, fuel;
    time_t last_distance_check_secs, start_t;
    double remaining_distance;
    char active;
    struct JunctionStruct *currentJunction;
    struct RoadStruct *roadOn;
};

struct JunctionStruct *roadMap;
int num_junctions, num_roads;

struct VehicleStruct *vehicles;
char *map_filename;

int total_vehicles;
int passengers_delivered;
int vehicles_exhausted_fuel;
int passengers_stranded;
int vehicles_crashed ;

static int size;
static int rank;

#endif // DATA_STRUCTURES_H
