#pragma once

#include "../type.h"
class MathUtil
{

	static double epsilon;
    static double twopi_inv ;
    static double twopi ;


public:
	MathUtil(void);
	~MathUtil(void);


	static double toRadians(double angle);
	static double toAngle(double theta);
	static double mod2pi_pos(double vin);
    /** Ensure that v is [-PI, PI] **/
	static double mod2pi(double vin);
	static double mod2pi(double ref, double v);
	static double exp(double xin);
	static double exp_pos(double xin);

};

