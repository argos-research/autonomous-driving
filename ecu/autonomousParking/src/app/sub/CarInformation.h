#include <math.h>

struct CarInformation {

    /*
     * Properties of the Car
     */
    double length_car;               // length of the car
    double width_car;                // width of the car

    double wheelRadius;
    //double wheelCircumference;      // used to calculate minRotationsNeeded

    double safetyDistanceLength;    // distance between the parking car and the obstacle in front/back
    double safetyDistanceWidth;     // distance between obstacle to the right

    double steer_max;               // max possible steering angle
    double velocity_max;            // max possible velocity

    /*
     * Properties of a suitable parking lot
     */
    double parkingLotWidth;         // minimum width of a sufficiently wide parking lot
    double parkingLotLength;        // minimum length of a sufficiently large parking lot

    /*
     * Information for parking lot detection
     */
    //double minRotationsNeeded;      // amount of wheel rotations needed to pass a parking lot which has a sufficient length

    CarInformation(double len, double wid, double rad, double steer, double velo) : length_car(len), width_car(wid), wheelRadius(rad),
                                                         safetyDistanceLength(0.5), safetyDistanceWidth(0.2),
                                                         steer_max(steer), velocity_max(velo) 
    {
        //wheelCircumference = (wheelRadius* 2 * M_PI);

        parkingLotLength = length_car + safetyDistanceLength * 2;       // safety distance to the car in front and to the car in the back is applied
        parkingLotWidth = width_car + safetyDistanceWidth;              // safety distance to the right is applieds

        //minRotationsNeeded = parkingLotLength/wheelCircumference;
    }
};
