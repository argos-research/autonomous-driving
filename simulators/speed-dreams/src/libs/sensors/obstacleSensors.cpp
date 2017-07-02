/***************************************************************************

    file                 : ObstacleSensors.cpp
    copyright            : (C) 2008 Lugi Cardamone, Daniele Loiacono, Matteo Verzola
						   (C) 2013 Wolf-Dieter Beelitz
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#define __DEBUG_OPP_SENS__


#include "obstacleSensors.h"

void SingleObstacleSensor::init(tCarElt *car,double start_angle, double angle_covered,double range)
{
	this->car = car;
	sensor_range=range;
	sensor_angle_covered = angle_covered;
	sensor_angle_start = start_angle;
	sensor_out=0;   //value ranges from 0 to 1: 0="No obstacle in sight" 1="Obstacle too close (collision)!"

}

void SingleObstacleSensor::setSingleObstacleSensor(double sens_value)
{
	sensor_out=sens_value;
}

ObstacleSensors::ObstacleSensors(int sensors_number, tTrack* track, tCarElt* car, tSituation *situation, int range )
{
	sensors_num=sensors_number;
	anglePerSensor = 360.0 / (double)sensors_number;
	obstacles_in_range = new Obstacle[situation->_ncars];

	all_obstacles = new Obstacle[situation->_ncars];

	sensors = new SingleObstacleSensor[sensors_number];

	for (int i = 0; i < sensors_number; i++) {
		sensors[i].init(car,i*anglePerSensor, anglePerSensor, range);

	}

	myc = car;
	sensorsRange= range;

}

ObstacleSensors::~ObstacleSensors()
{
	delete [] sensors;
	delete [] obstacles_in_range;
	delete [] all_obstacles;
}

double ObstacleSensors::getObstacleSensorOut(int sensor_id)
{
	return sensors[sensor_id].getSingleObstacleSensorOut();
}

double ObstacleSensors::distance(double x1, double y1, double x2, double y2) {
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

bool ObstacleSensors::is_between(double xc1, double xc2, double xcross) {
	if ((xc1 < xc2) && (xcross >= xc1) && (xcross <= xc2)) return true;
	else if ((xc2 < xc1) && (xcross >= xc2) && (xcross <= xc1)) return true;
	else return false;
}

void ObstacleSensors::sensors_update(tSituation *situation)
{
	double dist = 200;
	double candidate;
	double pointx = -1;
	double pointy = -1;
	double sens_x = -1;
	double sens_y = -1;

	// iterate over all cars
	for (int i = 0; i < situation->_ncars && situation->_ncars != 1; i++) {
		tCarElt *obst = situation->cars[i];
		if (myc->_pos_X == obst->_pos_X && myc->_pos_Y == obst->_pos_Y) // ignore own car
			continue;

		/* calculate slope of own and obstacle car */
		double m = tan(myc->_yaw); // own car

		double mo = tan(obst->_yaw); // obstacle car
		double mon = tan(obst->_yaw + PI/2); // rotate 90°

		/* build 4 straight lines of obstacle car
		 *
		 * y = m * x + t
		 * => t = y - m * x
		 */
		double t1 = obst->_corner_y(1) - mo * obst->_corner_x(1); // left side
		double t2 = obst->_corner_y(1) - mon * obst->_corner_x(1); // front side
		double t3 = obst->_corner_y(2) - mo * obst->_corner_x(2); // right side
		double t4 = obst->_corner_y(2) - mon * obst->_corner_x(2); // back side

		/* straight line for own car */
		double t = myc->_pos_Y - m * myc->_pos_X;

		/* position sensor in front of car */
		//sens_x = myc->_pos_X + cos(m) * (0 - myc->_dimension_x/2);
		//sens_y = myc->_pos_Y + sin(m) * (0 - myc->_dimension_x/2);
		sens_x = (myc->_corner_x(0) + myc->_corner_x(1))/2;
		sens_y = (myc->_corner_y(0) + myc->_corner_y(1))/2;

		/* DEBUG print points of sensor and car corners */
		printf("A=(%f,%f)\n", sens_x, sens_y);
		printf("F=(%f,%f)\n", myc->_corner_x(0), myc->_corner_y(0));
		printf("G=(%f,%f)\n", myc->_corner_x(1), myc->_corner_y(1));
		printf("H=(%f,%f)\n", myc->_corner_x(2), myc->_corner_y(2));
		printf("I=(%f,%f)\n", myc->_corner_x(3), myc->_corner_y(3));

		/* DEBUG print calculated straight lines */
		printf("f(x)=%f*x+%f\n", m, t); // own car
		printf("k(x)=%f*x+%f\n", mo, t1); // obstacle car
		printf("l(x)=%f*x+%f\n", mon, t2);
		printf("m(x)=%f*x+%f\n", mo, t3);
		printf("n(x)=%f*x+%f\n", mon, t4);

		/* calculate intersections
		 *
		 * m_1 * x + t_1 = m_2 * x + t_2
		 * => m_1 * x - m_2 * x = t_2 - t_1
		 * => x * (m_1 - m_2) = t_2 - t_1
		 * => x = (t_2 - t_1) / (m_1 - m2)
		 */
		double cross_x1 = (t1 - t)/(m - mo);
		double cross_y1 = m * cross_x1 + t;
		double cross_x2 = (t2 - t)/(m - mon);
		double cross_y2 = m * cross_x2 + t;
		double cross_x3 = (t3 - t)/(m - mo);
		double cross_y3 = m * cross_x3 + t;
		double cross_x4 = (t4 - t)/(m - mon);
		double cross_y4 = m * cross_x4 + t;

		// left
		if (is_between(obst->_corner_x(3), obst->_corner_x(1), cross_x1)) {
			printf("intersection at (%f, %f)\n", cross_x1, cross_y1);
			candidate = distance(sens_x, sens_y, cross_x1, cross_y1);
			if (candidate < dist) {
				pointx = cross_x1;
				pointy = cross_y1;
				dist = candidate;
			}
			//printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x1, cross_y1, distance(myc->_pos_X, myc->_pos_Y, cross_x1, cross_y1));
		}
		// front
		if (is_between(obst->_corner_x(1), obst->_corner_x(0), cross_x2)) {
			printf("intersection at (%f, %f)\n", cross_x2, cross_y2);
			candidate = distance(sens_x, sens_y, cross_x2, cross_y2);
			if (candidate < dist) {
				pointx = cross_x2;
				pointy = cross_y2;
				dist = candidate;
			}
			//printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x2, cross_y2, distance(myc->_pos_X, myc->_pos_Y, cross_x2, cross_y2));
		}
		// right
		if (is_between(obst->_corner_x(0), obst->_corner_x(2), cross_x3)) {
			printf("intersection at (%f, %f)\n", cross_x3, cross_y3);
			candidate = distance(sens_x, sens_y, cross_x3, cross_y3);
			if (candidate < dist) {
				pointx = cross_x3;
				pointy = cross_y3;
				dist = candidate;
			}
			//printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x3, cross_y3, distance(myc->_pos_X, myc->_pos_Y, cross_x3, cross_y3));
		}
		// back
		if (is_between(obst->_corner_x(2), obst->_corner_x(3), cross_x4)) {
			printf("intersection at (%f, %f)\n", cross_x4, cross_y4);
			candidate = distance(sens_x, sens_y, cross_x4, cross_y4);
			if (candidate < dist) {
				pointx = cross_x4;
				pointy = cross_y4;
				dist = candidate;
			}
			//printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x4, cross_y4, distance(myc->_pos_X, myc->_pos_Y, cross_x4, cross_y4));
		}

		/*if (situation->cars[i]->_corner_x(0) < situation->cars[i]->_corner_x(3)) {
			cross_x1 > situation->cars[i]->_corner_x(0) && cross_x1 < situation->cars[i]->_corner_x(3)
		}

		if (cross_x1 < )
		if (, situation->cars[i]->_corner_y(0), cross_x1, cross_y1, situation->cars[i]->_corner_x(3), situation->cars[i]->_corner_y(3)))
			printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x1, cross_y1, distance(myc->_pos_X, cross_x1, myc->_pos_Y, cross_y1));
		if (is_between(situation->cars[i]->_corner_x(0), situation->cars[i]->_corner_y(0), cross_x2, cross_y2, situation->cars[i]->_corner_x(1), situation->cars[i]->_corner_y(1)))
			printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x2, cross_y2, distance(myc->_pos_X, cross_x2, myc->_pos_Y, cross_y2));
		if (is_between(situation->cars[i]->_corner_x(1), situation->cars[i]->_corner_y(1), cross_x3, cross_y3, situation->cars[i]->_corner_x(2), situation->cars[i]->_corner_y(2)))
			printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x3, cross_y3, distance(myc->_pos_X, cross_x3, myc->_pos_Y, cross_y3));
		if (is_between(situation->cars[i]->_corner_x(2), situation->cars[i]->_corner_y(2), cross_x4, cross_y4, situation->cars[i]->_corner_x(3), situation->cars[i]->_corner_y(3)))
			printf("Schnittpunkt bei (%f,%f) mit Distanz %f\n", cross_x4, cross_y4, distance(myc->_pos_X, cross_x4, myc->_pos_Y, cross_y4));*/
	}
	printf("Distance from sensor (%f, %f) to nearest intersection (%f, %f): %f\n", sens_x, sens_y, pointx, pointy, dist);
}

void ObstacleSensors::printSensors()
{
	int tabsBefore;
	int tabsAfter;
	if(sensors_num % 2 == 0)
	{
		for(int curLevel=0; curLevel<sensors_num/2; curLevel++)
		{
			tabsBefore=sensors_num/2 - curLevel - 1;
			tabsAfter= curLevel*2+1;
			for(int i=0; i<tabsBefore; i++)
				printf("\t");
			printf("%.2f",sensors[sensors_num/2-1-curLevel].getSingleObstacleSensorOut());
			for(int i=0; i<tabsAfter; i++)
				printf("\t");
			printf("%.2f",sensors[sensors_num/2+curLevel].getSingleObstacleSensorOut());
			printf("\n");
		}

	}
	else
	{
		for(int curLevel=0; curLevel<(sensors_num/2+1); curLevel++)
		{
			tabsBefore=sensors_num/2 - curLevel ;
			tabsAfter= curLevel*2 ;
			for(int i=0; i<tabsBefore; i++)
				printf("\t");
			printf("%.2f",sensors[sensors_num/2-1-curLevel].getSingleObstacleSensorOut());
			if(curLevel>0)
			{
				for(int i=0; i<tabsAfter; i++)
					printf("\t");
				if(curLevel==0)
					printf(" ");
				printf("%.2f",sensors[sensors_num/2+curLevel].getSingleObstacleSensorOut());
			}
			printf("\n");
		}
	}
}
