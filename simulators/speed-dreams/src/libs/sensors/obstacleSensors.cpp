/*
* \author Alexander Weidinger
* \date   2017-07-06
*/

//#define __DEBUG_OPP_SENS__
#define __OPP_NOISE_STD__ 0.02

#include "obstacleSensors.h"

SingleObstacleSensor::SingleObstacleSensor(tCarElt *car, double angle, double move_x, double move_y, double range) {
	this->car = car;
	this->angle = angle;
	this->range = range;
	this->move_x = move_x;
	this->move_y = move_y;
}

SingleObstacleSensor::~SingleObstacleSensor() {
}

void ObstacleSensors::addSensor(tCarElt *car, double angle, double move_x, double move_y, double range) {
	SingleObstacleSensor sens(car, angle, move_x, move_y, range);
	sensors.push_back(sens);
}

ObstacleSensors::ObstacleSensors(tTrack* track, tCarElt* car)
{
	myc = car;
}

ObstacleSensors::~ObstacleSensors() {
}

/* calculate distance between two points */
double ObstacleSensors::distance(point p1, point p2) {
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

/* check if xcross is between xc1 and xc2 */
bool ObstacleSensors::is_between(double xc1, double xc2, double xcross) {
	if ((xc1 < xc2) && (xcross >= xc1) && (xcross <= xc2)) return true;
	else if ((xc2 < xc1) && (xcross >= xc2) && (xcross <= xc1)) return true;
	else return false;
}

/* middle <-> sensor + sensor <-> intersection = middle <-> intersection */
bool ObstacleSensors::is_infront(point middle, point sensor, point intersection) {
	double eps = 0.1;
	double dist1 = distance(middle, sensor) + distance(sensor, intersection);
	double dist2 = distance(middle, intersection);
	return (dist1 >= dist2 - eps && dist1 <= dist2 + eps);
}

/* method taken from patch files of
* http://cs.adelaide.edu.au/~optlog/SCR2015/index.html
*/
double ObstacleSensors::normRand(double avg,double std)
{
	double x1, x2, w, y1, y2;

	do {
		x1 = 2.0 * rand()/(double(RAND_MAX)) - 1.0;
		x2 = 2.0 * rand()/(double(RAND_MAX)) - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );

	w = sqrt( (-2.0 * log( w ) ) / w );
	y1 = x1 * w;
	y2 = x2 * w;
	return y1*std + avg;
}

void ObstacleSensors::sensors_update(tSituation *situation)
{
	for (auto it=sensors.begin(); it != sensors.end(); ++it) {

		point sensorPosition = { 0, 0 }; // position of the sensor
		double obstacleDistance = (*it).getRange(); // distance to nearest obstacle
		point obstacleIntersection = { 0, 0 }; // point of intersection with nearest obstacle

		/* calculate slope of own car */
		double m = tan(myc->_yaw);

		/* position sensor at desired location */
		double dis = distance(point{0, 0}, point{(*it).getMove_x(), (*it).getMove_y()}); // total distance of point on own car
		double phi = atan2((*it).getMove_y(), (*it).getMove_x()); // angle in which direction we need to move the sensor point
		sensorPosition = { myc->_pos_X + cos(myc->_yaw + phi) * dis,
			myc->_pos_Y + sin(myc->_yaw + phi) * dis
		}; // calculate distance for x and y coordinates and add it to middle point

		/* calculate slope of sensor */
		m = tan(myc->_yaw + (*it).getAngle() * PI / 180); // add custom angle of sensor (in degree for convenience)
		/* straight line for the sensor
		* y = m * x + t
		* => t = y - m * x
		*/
		double t = sensorPosition.y - m * sensorPosition.x;

		#ifdef __DEBUG_OPP_SENS__
		printf("points={(%f,%f),(%f,%f),(%f,%f),(%f,%f),(%f,%f)}\n",
		myc->_corner_x(0), myc->_corner_y(0), // front right
		myc->_corner_x(1), myc->_corner_y(1), // front left
		myc->_corner_x(2), myc->_corner_y(2), // back right
		myc->_corner_x(3), myc->_corner_y(3), // back left
		sensorPosition.x, sensorPosition.y);	// sensor
		printf("f(x)=%f * x + %f\n", m, t);		// straight line of the sensor
		#endif

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

			/* find nearest intersection point in front of the sensor
			* and check if the found intersection is in the domain
			*/
			double distanceCandidate = 9000;
			/* left side of obstacle car */
			if (is_between(obstacleCar->_corner_x(3), obstacleCar->_corner_x(1), i_left.x) &&
			is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_left)) {
				distanceCandidate = distance(sensorPosition, i_left);
				if (distanceCandidate < obstacleDistance) {
					obstacleDistance = distanceCandidate;
					obstacleIntersection = i_left;
				}
			}
			/* front side of obstacle car */
			if (is_between(obstacleCar->_corner_x(1), obstacleCar->_corner_x(0), i_front.x) &&
			is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_front)) {
				distanceCandidate = distance(sensorPosition, i_front);
				if (distanceCandidate < obstacleDistance) {
					obstacleDistance = distanceCandidate;
					obstacleIntersection = i_front;
				}
			}
			/* right side of obstacle car */
			if (is_between(obstacleCar->_corner_x(0), obstacleCar->_corner_x(2), i_right.x) &&
			is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_right)) {
				distanceCandidate = distance(sensorPosition, i_right);
				if (distanceCandidate < obstacleDistance) {
					obstacleDistance = distanceCandidate;
					obstacleIntersection = i_right;
				}
			}
			/* back side of obstacle car */
			if (is_between(obstacleCar->_corner_x(2), obstacleCar->_corner_x(3), i_back.x) &&
			is_infront(point { myc->_pos_X, myc->_pos_Y }, sensorPosition, i_back)) {
				distanceCandidate = distance(sensorPosition, i_back);
				if (distanceCandidate < obstacleDistance) {
					obstacleDistance = distanceCandidate;
					obstacleIntersection = i_back;
				}
			}

			obstacleDistance *= normRand(1, __OPP_NOISE_STD__); // add noise
			(*it).setDistance(obstacleDistance);

			#ifdef __DEBUG_OPP_SENS__
			printf("sensor #%d: %f\n", std::distance(sensors.begin(), it), obstacleDistance);
			#endif
		}
	}
}
