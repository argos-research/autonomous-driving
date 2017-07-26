/*
 * Genode includes
 */
#include <math.h>

/*
 * Parking inlcudes
 */
#include "Parking.h"

Parking::Parking(CarInformation info) : _info(info), _lastRotationValue(0), 
                                        _rotationsWhileEnoughSpace(0),
                                        _state(SEARCHING), _direction(1),
                                        _side(1), _timestamp(0),
                                        _sampling_period(1)
{ 
    _T_star = 3;    // TODO : find appropriate magic number
    _T = _T_star;   // first estimation of T
}

bool Parking::_findParkingLot(double sensor_right, double rotations) {
    // check if sensor data indicates that the distance to the right is too narrow
    if(sensor_right < _info.parkingLotWidth) {
        // reset number of wheel rotations while enough space for parking and update displacements
        _rotationsWhileEnoughSpace = 0;
        _map.setLongitudinalDisplacement(0);
        _map.setLateralDisplacement(0);
    } else {
        // add rotations and update displacements
        _rotationsWhileEnoughSpace += (rotations - _lastRotationValue);
        _map.setLongitudinalDisplacement(_rotationsWhileEnoughSpace * _info.wheelCircumference);
        _map.setLateralDisplacement(fmin(sensor_right, _old_sensor_right));
    }
        
    _lastRotationValue = rotations;
    _map.setX(rotations * _info.wheelCircumference);

    // check if the potential parking lot has sufficient length
    if(_rotationsWhileEnoughSpace >= _info.minRotationsNeeded) {
        return true;
    }
    return false;
}

double Parking::_a(double t) {
    double t_prime = (_T - _T_star) / 2.0;

    double result = 0;
    // calculate percentage of steering with regards to the timestamp
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
    return 0.5 * (1 - cos((4 * M_PI * t) / _T));
}

double Parking::_steering_angle(double t) {
    return _local_steer_max * _side * _a(t);
}

double Parking::_velocity(double t) {
    return _info.velocity_max * _direction * _b(t);
}

void Parking::_calculate_T() {
    double s_angle;
    double velo;
    
    _x = _map.getX();
    _y = _map.getY();
    _phi = _map.get_angle();

    do {
        for(double ts = 0; ts <= _T; ts += _timestamp){
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
        _T += _timestamp;
    } while(_longitudinalCondition(_map.getX(), _x, _map.getY(), _y, _phi));

    _T -= _timestamp;
}

void Parking::_calculate_local_max_steer() {
    if(_lateralCondition(_map.getX(), _x, _map.getY(), _y, _phi)) return;

    double s_angle;
    double velo;

    _x = _map.getX();
    _y = _map.getY();
    _phi = _map.get_angle();

    do {
        _local_steer_max -= 0.0872665;

        for(double ts = 0; ts <= _T; ts += _timestamp){
            s_angle = _steering_angle(ts);
            velo = _velocity(ts);

            if(s_angle == 0){
                _phi_old = _phi;
                _phi = _phi;
                //_x = _x + (velo * _sampling_period * cos(s_angle));
                _y = _y + (velo * _sampling_period * sin(s_angle));
            } else {
                _phi_old = _phi;
                _phi = _phi + (((velo * _sampling_period) / _info.length_car) * sin(s_angle));
                //_x = _x + ((_info.length_car / tan(s_angle)) * (sin(_phi) - sin(_phi_old)));
                _y = _y - ((_info.length_car / tan(s_angle)) * (cos(_phi) - cos(_phi_old)));;
            }
        }
    } while(!_lateralCondition(_map.getX(), _x, _map.getY(), _y, _phi));

}

bool Parking::_longitudinalCondition(double startX, double endX, double startY, double endY, double end_angle){
    return fabs(((endX - startX) * cos(_phi)) + ((endY - startY) * sin(_phi))) < _map.getLongitudinalDisplacement();
}

bool Parking::_lateralCondition(double startX, double endX, double startY, double endY, double end_angle){
    return fabs(((startX - endX) * sin(_phi)) + ((endY - startY) * cos(_phi))) < _map.getLateralDisplacement();
}

void Parking::receiveData(double sensor_front, double sensor_right, double sensor_back, double rotations){

        // TODO - process sensor data to determine potential collisions

        switch(_state){

        case SEARCHING      : if(_findParkingLot(sensor_right, rotations)){
                                _actuator_steering = 0;
                                _actuator_velocity = 0;
                                _direction = -1;
                                _state = PARKED;
                              } else {
                                _actuator_steering = 0;
                                _actuator_velocity = _info.velocity_max * _direction;
                              }
                              break;
        
        case CALCULATING    : _calculate_T();
                              _calculate_local_max_steer();
                              _state = CONTROLLING;
              
        case CONTROLLING    : if(true){
                                _state = PARKED;
                                _actuator_steering = 0;
                                _actuator_velocity = 0;
                              } else {
                                _actuator_steering = (_steering_angle(_timestamp) / _local_steer_max);
                                _actuator_velocity = _velocity(_timestamp);
                                _timestamp += _sampling_period;
                              }
                              break;

        case PARKED         : _actuator_steering = 0;
                              _actuator_velocity = 0;
                              break;
        }

        // TODO : send actuator data

        return;
    }