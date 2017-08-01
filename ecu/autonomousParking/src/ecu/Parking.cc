/*
 * Genode includes
 */
#include <math.h>

/*
 * Parking inlcudes
 */
#include "Parking.h"

#include <base/printf.h>

/* etc */
#include <cstdio>
#include <cstring>

Parking::Parking(CarInformation info) : _info(info), _traveled_distance(0), 
                                        _free_space(0),
                                        _state(SEARCHING), _direction(1),
                                        _side(-1),
                                        _maneuver_timestamp(0),
                                        _sampling_period(0)
{ 
    _T_star = 12;                           // magic number; works for current scenario
    _T = _T_star;                           // first estimation of T
    _local_steer_max = _info.steer_max;     // first value for _local_steer_max is max possible steer value
}

bool Parking::_findParkingLot(double sensor_right, double spin_velocity) {
    // check if sensor data indicates that the distance to the right is too narrow
    if(sensor_right < _info.parkingLotWidth) {

    // check if the potential parking lot has sufficient length
        if(_free_space >= _info.parkingLotLength) {
            return true;
        }
        // reset amount of free space and update displacements as potetial parking lot was too small
        _free_space = 0;
        _map.setLongitudinalDisplacement(0);
        _map.setLateralDisplacement(0);
    } else {
        // add passed distance to free space and update displacements as the end of the potential parking lot is not reached
        _free_space += spin_velocity * _sampling_period * _info.wheelRadius;
        _map.setLongitudinalDisplacement(_free_space);
        // the width of the parking lot should be approximated with the most narrow distance sensed
        _map.setLateralDisplacement(fmin(sensor_right, _old_sensor_right));
    }

    // update x coordinate of the car
    _traveled_distance += spin_velocity * _sampling_period * _info.wheelRadius;
    _map.setX(_traveled_distance);

    // safe old right sensor data
    _old_sensor_right = sensor_right;

    return false;
}

double Parking::_a(double t) {
    // calculate time of turning points
    double t_prime = (_T - _T_star) / 2.0;

    double result = 0;
    // calculate ratio of steering with regards to the timestamp
    if(t >= 0 && t < t_prime) {
        result = 1;
    } else if(t >= t_prime && t <= (_T - t_prime)) {
        result = cos((M_PI * (t - t_prime)) / _T_star);
    } else if(t > (_T - t_prime) && t < _T) {
        result = -1;
    }
    return result;
}

double Parking::_b(double t) {
    // calculate ratio of velocity with regards to the timestamp
    return 0.5 * (1 - cos((4 * M_PI * t) / _T));
}

double Parking::_steering_angle(double t) {
    // calculate steering angle based on max steering angle and ratio method
    return _local_steer_max * _side * _a(t);
}

double Parking::_velocity(double t) {
    // calculate velocity based on max speed and ratio method
    return _info.velocity_max * _direction * _b(t);
}

void Parking::_calculate_T() {
    double s_angle;
    double velo;
    
    // set calculation coordinates to start coordinates
    _x = _map.getX();
    _y = _map.getY();
    _phi = _map.get_angle();

    // discrete integration of motion equation system; end coordinates are calculated
    do {
        for(double ts = 0; ts <= _T; ts += _sampling_period){
            s_angle = _steering_angle(ts);
            velo = _velocity(ts);

            if(s_angle == 0){
                _phi_old = _phi;
                _phi = _phi;
                _x = _x + (velo * _sampling_period * cos(s_angle));
                _y = _y + (velo * _sampling_period * sin(s_angle));
            } else {
                _phi_old = _phi;
                _phi = _phi + (((velo * _sampling_period) / _info.length_car) * sin(s_angle));
                _x = _x + ((_info.length_car / tan(s_angle)) * (sin(_phi) - sin(_phi_old)));
                _y = _y - ((_info.length_car / tan(s_angle)) * (cos(_phi) - cos(_phi_old)));;
            }
        }

        // increase T
        _T += _sampling_period;

    // check if lateral condition is still fullfilled; if yes another optimization step (increase of T) can be performed
    } while(_longitudinalCondition(_map.getX(), _x, _map.getY(), _y, _map.get_angle()));

    // decrease T as it was increased before checking the condition
    _T -= _sampling_period;
}

void Parking::_calculate_local_max_steer() {
    // calculation coordinates are already calculated due to the calculation of T; therefore we can instantly check the lateral condition
    if(_lateralCondition(_map.getX(), _x, _map.getY(), _y, _map.get_angle())) return;

    double s_angle;
    double velo;

    // we need to perform new calculation with adaoted steering angle; therefore reset the already calculated calculation coordinates
    _x = _map.getX();
    _y = _map.getY();
    _phi = _map.get_angle();

    do {
        // decrease _max_steering angle
        _local_steer_max -= 0.0872665;

        // perform discrete integration again to calculate new calculation coordinates
        for(double ts = 0; ts <= _T; ts += _sampling_period){
            s_angle = _steering_angle(ts);
            velo = _velocity(ts);

            if(s_angle == 0){
                _phi_old = _phi;
                _phi = _phi;
                //_x = _x + (velo * _sampling_period * cos(s_angle));                           // commented out for showcase
                _y = _y + (velo * _sampling_period * sin(s_angle));
            } else {
                _phi_old = _phi;
                _phi = _phi + (((velo * _sampling_period) / _info.length_car) * sin(s_angle));
                //_x = _x + ((_info.length_car / tan(s_angle)) * (sin(_phi) - sin(_phi_old)));  // commented out for showcase
                _y = _y - ((_info.length_car / tan(s_angle)) * (cos(_phi) - cos(_phi_old)));;
            }
        }
    // check whether lateral condition is still harmed
    } while(!_lateralCondition(_map.getX(), _x, _map.getY(), _y, _map.get_angle()));

}

bool Parking::_longitudinalCondition(double startX, double endX, double startY, double endY, double start_angle){
    // during the parking maneuver the car's x-coordinate must not exceed the longitudinal size of the parking lot
    return fabs(((endX - startX) * cos(start_angle)) + ((endY - startY) * sin(start_angle))) < _map.getLongitudinalDisplacement();
}

bool Parking::_lateralCondition(double startX, double endX, double startY, double endY, double start_angle){
    // during the parking maneuver the car's y-coordinate must not exceed the lateral size of the parking lot
    return fabs(((startX - endX) * sin(start_angle)) + ((endY - startY) * cos(start_angle))) < _map.getLateralDisplacement();
}

void Parking::receiveData(double sensor_front, double sensor_right, double sensor_back, double spin_velocity, double timestamp, Publisher *publisher){
    // update sampling period to latest timestamp received
    _sampling_period = timestamp;

    // check sensor data for imminent collision
    if((sensor_front <= _info.safetyDistanceLength) || (sensor_right <= _info.safetyDistanceWidth) || (sensor_back <= _info.safetyDistanceLength*3)){
        // car should stop if a upcoming collision is detected
        _actuator_steering = 0;
        _actuator_velocity = 0;
        _state = PARKED;
    }

    char buffer1[1024] = { 0 };
    char buffer2[1024] = { 0 };


        // check state of the algorithm
        switch(_state){

        case SEARCHING      : if(_findParkingLot(sensor_right, spin_velocity)){
                                //if a parking lot is found the car stops and changes to calculating phase
                                _actuator_steering = 0;
                                _actuator_velocity = 0;
                                _direction = -1;
                                _state = CALCULATING;
                              } else {
                                // if no parking lot is found the car keeps moving with constant velocity
                                _actuator_steering = 0;
                                _actuator_velocity = _info.velocity_max * _direction;
                              }
                              // jump to publish
                              break;
        
        case CALCULATING    : if(spin_velocity != 0){
                                // wait for the car to stand still
                                _actuator_steering = 0;
				                _actuator_velocity = 0;
				                break;
			                  }
                              // perform calculation of T and _local_steer_max
			                  _calculate_T();
                              _calculate_local_max_steer();

                             
                              sprintf(buffer1, "%f",_T);
                              PDBG("%s", buffer1);
                            
                              sprintf(buffer2, "%f",_local_steer_max);
                              PDBG("%s", buffer2);
                            
                              // change state to controlling and jump instantly to first evaluation of controlling phase
                              _state = CONTROLLING;
              
        case CONTROLLING    : _actuator_steering = (_steering_angle(_maneuver_timestamp) / _info.steer_max);
                              _actuator_velocity = _velocity(_maneuver_timestamp);
                              _maneuver_timestamp += _sampling_period;
                              // after calculating next steering angle + velocity and updating the _maneuver_timestamp jump to publish
                              break;

        case PARKED         : _actuator_steering = 0;
                              _actuator_velocity = 0;
                              // in parked state car stands still; jump to publish
                              break;
        }

    // publish the calculated actuator data
	publisher->my_publish("0", _actuator_steering);
	publisher->my_publish("4", _actuator_velocity);
	if(_state==PARKED)
	{
		publisher->my_publish("3", 0);
	}

        return;
    }
