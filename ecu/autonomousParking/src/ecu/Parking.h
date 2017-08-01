/*
 * Parking includes
 */
#include "CarInformation.h"
#include "Map.h"

/*pub*/
#include <publisher.h>

enum STATE { SEARCHING, CALCULATING, CONTROLLING, PARKED };

class Parking {
private:

	/*
	 * Information about the car and its environment
	 */
	CarInformation _info;			// holds information about the car as well as info imnpacted by both Parking and detection

	Map _map;						// contains information about the environment (parking lot) and the position of the car


	/*
	 * Information for parking lot detection
	 */
	double _old_sensor_right;       // value of right sensor from last simulation step
	double _traveled_distance;      // distance traveled since start of searching procedure
    double _free_space;             // current length of potential parking lot


	/*
	 * Information for parking maneuver
	 */
	STATE _state;					// current state of the parking algorithm

	double _direction;				// forward = 1 ; backwards = -1
	double _side;					// side on which the parking lot is; right = -1 ; left = 1

	double _maneuver_timestamp;     // timestamp during parking maneuver; start of maneuver needs to be timestamp 0; therefore an own ts is introduced
	double _sampling_period;		// time of 1 sampling period

	double _T_star;					// time for steering negative max to positive max (or vice versa)
	double _T;						// estimated time for whole parking maneuver as we only support one iteration right now

	double _local_steer_max; 		// optimized value for max steering; determined in calculation phase


	/*
	 * Data for calculating phase
	 */
	double _x;						// x coordinate during calculation phase
	double _y;						// y coordinate during calculation phase
	double _phi;					// car orientation angle during calculation phase (current step)
	double _phi_old;				// car orientation angle of previous calculation step


	/*
	 * Data for actuators
	 */
	double _actuator_steering;		// actuator data to determine velocity; this value is published
	double _actuator_velocity;		// actuator data to determine steering angle; this value is published


	/*
	 * finds a parking lot with appropriate size and builds a map of the environment(parking lot) of th car + stores car's position in map
	 */
	bool _findParkingLot(double sensor_right, double spin_velocity);

	/*
	 * helper functions for _steering_angle() and _veloctity; calculate ratio of velocity and steering
	 */
	double _a(double t);

	double _b(double t);


	/*
	 * calculates steering angle for next simulation step
	 */
	double _steering_angle(double t);


	/*
	 * calculates velocity for next simulation step
	 */
	double _velocity(double t);


	/*
	 * calculate time for parking maneuver and max steer angle
	 */
	void _calculate_T();

	void _calculate_local_max_steer();


	/*
	 *  condition methods are used to check the distance conditions
	 */
	bool _longitudinalCondition(double startX, double endX, double startY, double endY, double start_angle);

	bool _lateralCondition(double startX, double endX, double startY, double endY, double start_angle);

public:

	Parking(CarInformation info);

	~Parking() { }

	/*
	 * main method of the algorithm; calculate the actuator data based on the state while processing the range data from sensors
	 */
	void receiveData(double sensor_front, double sensor_right, double sensor_back, double spin_velocity, double timestamp, Publisher *publisher);

};
