// include/utils.h
#ifndef UTILS_H
#define UTILS_H

#include <time.h>


int findFreeVehicle();
int findAppropriateRoad(int, struct JunctionStruct *);
int getRandomInteger(int, int);
time_t getCurrentSeconds();
int findIndexOfMinimum(double *, char *, int);

#endif // UTILS_H
