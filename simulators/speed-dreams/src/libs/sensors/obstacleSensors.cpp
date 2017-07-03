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

double ObstacleSensors::distance(point p1, point p2) {
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

bool ObstacleSensors::is_between(double xc1, double xc2, double xcross) {
	if ((xc1 < xc2) && (xcross >= xc1) && (xcross <= xc2)) return true;
	else if ((xc2 < xc1) && (xcross >= xc2) && (xcross <= xc1)) return true;
	else return false;
}

bool ObstacleSensors::is_infront(double midx, double midy, double sensx, double sensy, double crossx, double crossy) {
	double eps = 0.1;
	double dist1 = distance(midx, midy, sensx, sensy) + distance(sensx, sensy, crossx, crossy);
	double dist2 = distance(midx, midy, crossx, crossy);
	printf("%f vs. %f\n", dist1, dist2);
	return (dist1 >= dist2 - eps && dist1 <= dist2 + eps);
}

bool ObstacleSensors::is_infront(point middle, point sensor, point intersection) {
	double eps = 0.1;
	double dist1 = distance(middle.x, middle.y, sensor.x, sensor.y) + distance(sensor.x, sensor.y, intersection.x, intersection.y);
	double dist2 = distance(middle.x, middle.y, intersection.x, intersection.y);
	return (dist1 >= dist2 - eps && dist1 <= dist2 + eps);
}

void ObstacleSensors::sensors_update(tSituation *situation)
{
	point sensorPosition = { 0, 0 }; // position of the sensor
	double obstacleDistance = 200; // distance to nearest obstacle
	point obstacleIntersection = { 0, 0 }; // point of intersection with nearest obstacle

	/* calculate slope of own car */
	double m = tan(myc->_yaw);
	/* straight line for the sensor */
	double t = myc->_pos_Y - m * myc->_pos_X;

	/* iterate over all cars */
	for (int i = 0; i < situation->_ncars && situation->_ncars != 1; i++) {
		tCarElt *obstacleCar = situation->cars[i];
		if (myc == obstacleCar) continue; // ignore own car

		/* calculate slope of own and obstacle car */
		double m_obst = tan(obstacleCar->_yaw);
		double m_obst_90 = tan(obstacleCar->_yaw + PI/2); // 90° turned

		/*
		 * corners:
		 *
		 * 1   front   0
		 *   +-------+
		 * l |       | r
		 * e |       | i
		 * f |       | g
		 * t |       | h
		 *   |       | t
		 *   +-------+
		 * 3   back    2
		 */

		/* build 4 straight lines (the 4 sides of the car) for obstacleCar */
		double t_left = obstacleCar->_corner_y(1) - m_obst * obstacleCar->_corner_x(1);
		double t_front = obstacleCar->_corner_y(1) - m_obst_90 * obstacleCar->_corner_x(1);
		double t_right = obstacleCar->_corner_y(2) - m_obst * obstacleCar->_corner_x(2);
		double t_back = obstacleCar->_corner_y(2) - m_obst_90 * obstacleCar->_corner_x(2);

		/* position sensor in front of car
		 *
		 * midpoint between two points
		 * ((x1 + x2) / 2, (y1 + y2) / 2)
		 */
		sensorPosition = { (myc->_corner_x(0) + myc->_corner_x(1))/2,
											 (myc->_corner_y(0) + myc->_corner_y(1))/2 };

		/* calculate intersections
		 *
		 * m_1 * x + t_1 = m_2 * x + t_2
		 * => m_1 * x - m_2 * x = t_2 - t_1
		 * => x * (m_1 - m_2) = t_2 - t_1
		 * => x = (t_2 - t_1) / (m_1 - m2)
		 */
		 point i_left = { (t_left - t) / (m - m_obst), m * i_left.x + t };
		 point i_front = { (t_front - t) / (m - m_obst_90), m * i_front.x + t };
		 point i_right = { (t_right - t) / (m - m_obst), m * i_right.x + t };
		 point i_back = { (t_back - t) / (m - m_obst_90), m * i_back.x + t };

		 /* find nearest intersection point, in front of sensor */
		 /* check if found intersection is in domain */
		 double distanceCandidate = -1;
		 if (is_between(obstacleCar->_corner_x(3), obstacleCar->_corner_x(1), i_left.x) &&
	 			 is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_left)) {
					 distanceCandidate = distance(sensorPosition, i_left);
					 if (distanceCandidate < obstacleDistance) {
						 obstacleDistance = distanceCandidate;
						 obstacleIntersection = i_left;
					 }
		 }
		 if (is_between(obstacleCar->_corner_x(1), obstacleCar->_corner_x(0), i_front.x) &&
				 is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_front)) {
					 distanceCandidate = distance(sensorPosition, i_front);
					 if (distanceCandidate < obstacleDistance) {
						 obstacleDistance = distanceCandidate;
						 obstacleIntersection = i_front;
					 }
		 }
		 if (is_between(obstacleCar->_corner_x(0), obstacleCar->_corner_x(2), i_right.x) &&
				 is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_right)) {
					 distanceCandidate = distance(sensorPosition, i_right);
					 if (distanceCandidate < obstacleDistance) {
						 obstacleDistance = distanceCandidate;
						 obstacleIntersection = i_right;
					 }
		 }
		 if (is_between(obstacleCar->_corner_x(2), obstacleCar->_corner_x(3), i_back.x) &&
				 is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_back)) {
					 distanceCandidate = distance(sensorPosition, i_back);
					 if (distanceCandidate < obstacleDistance) {
						 obstacleDistance = distanceCandidate;
						 obstacleIntersection = i_back;
					 }
		 }
		 printf("Distance: %f\n", obstacleDistance);
	}
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
