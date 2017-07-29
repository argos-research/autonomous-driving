class Map {
private:		    
	/*
     * Information for parking maneuver
     */
    double _D_l;                    // longitudinal displacement              
    double _D_w;                    // lateral displacement

    double _x;						// x coordinate of the car
    double _y;						// y coordinate of the car
    double _angle;					// angle of the car; 0 implies the car's orientation is parallel to the parking lot

public:
	/*
	 * Constructor creates empty map
	 */
	Map() : _D_l(0), _D_w(0), _x(0), _y(0), _angle(0) { }

	~Map() { }

	/*
	 * Getter- and setter-methods
	 */
	double getLongitudinalDisplacement() { return _D_l; }

	void setLongitudinalDisplacement(double d) { _D_l = d; }

	double getLateralDisplacement() { return _D_w; }

	void setLateralDisplacement(double d) { _D_w = d; }

	double getX() { return _x; }

	void setX(double x) { _x = x; }

	double getY() { return _y; }

	double get_angle() { return _angle; }

};
