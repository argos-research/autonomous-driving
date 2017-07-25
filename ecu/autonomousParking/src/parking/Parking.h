/*
 * Parking includes
 */
#include "CarInformation.h"
#include "Map.h"

enum STATE { SEARCHING, CALCULATING, CONTROLLING, PARKED };

class Parking {
private:

	/*
	 * Information about the car and its environment
	 */
	CarInformation _info;			// holds information about the car as well as info imnpacted by both Parking and detection

	Map _map;						// contains information about the environment to the right


	/*
	 * Information for parking lot detection
	 */
	double _old_sensor_right;
	double _lastRotationValue;          // number of wheel rotations of the last simulation step
    double _rotationsWhileEnoughSpace;  // number of wheel rotations while enough space for parking (width of parking lot is sufficiently large)


	/*
	 * Information for parking maneuver
	 */
	STATE _state;

	double _direction;				// forward = 1 ; backwards = -1
	double _side;					// right = 1 ; left = 0 --- we only support parking to the parking lot on the right

	double _timestamp;				// elapsed time since start of parking maneuver
	double _sampling_period;		// time of 1 sampling period
	double _T_star;					// time for steering negative max to positive max
	double _T;						// estimated time for iteration/parking maneuver as we inly support one iteration right now

	double _local_steer_max; 		// adjustable value for max steering; used in calculation phase


	/*
	 * Data for calculating phase
	 */
	double _x;
	double _y;
	double _phi;
	double _phi_old;


	/*
	 * Data for actuators
	 */
	double _actuator_steering;
	double _actuator_velocity;


	/*
	 * findParkingLot - finds a parking lot wiht appropriate size and builds a map of the environment to the right of th car
	 */
	bool _findParkingLot(double sensor_right, double rotations);

	/*
	 * _a and _b - helper functions for _steering_angle() and _veloctity
	 */
	double _a(double t);

	double _b(double t);


	/*
	 * _steering_angle - calculates steering angle for next simulation step
	 */
	double _steering_angle(double t);


	/*
	 * _velocity - calculates velocity for next simulation step
	 */
	double _velocity(double t);


	/*
	 * Calculate time for parking maneuver and max steer angle
	 */
	void _calculate_T();

	void _calculate_local_max_steer();


	/*
	 *  condition methods are used to check the distance conditions
	 */
	bool _longitudinalCondition(double startX, double endX);

	bool _lateralCondition(double startY, double endY);

public:

	Parking(CarInformation info);

	~Parking() { }

	void receiveData(double sensor_front, double sensor_back, double sensor_right, double rotations);

};
