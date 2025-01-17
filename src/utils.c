// src/utils.c
#include "../include/data_structures.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
// Implement the functions declared in utils.h
/**
 * Iterates through the vehicles array and finds the first free entry, returns
 * the index of this or -1 if none is found
 **/
 int findFreeVehicle()
{
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (!vehicles[i].active)
            return i;
    }
    return -1;
}
/**
 * Finds the road index out of the junction's roads that leads to a specific
 * destination junction
 **/
 int findAppropriateRoad(int dest_junction, struct JunctionStruct *junction)
{
    for (int j = 0; j < junction->num_roads; j++)
    {
        if (junction->roads[j].to->id == dest_junction)
            return j;
    }
    return -1;
}


 int getRandomInteger(int from, int to)
{
    return (rand() % (to - from)) + from;
}

 time_t getCurrentSeconds()
{
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    time_t current_seconds = curr_time.tv_sec;
    return current_seconds;
}

 int findIndexOfMinimum(double *dist, char *active, int num_junctions)
{
    double min_dist = LARGE_NUM + 1;
    int current_min = -1;
    for (int i = 0; i < num_junctions; i++)
    {
        if (active[i] && dist[i] < min_dist)
        {
            min_dist = dist[i];
            current_min = i;
        }
    }
    return current_min;
}