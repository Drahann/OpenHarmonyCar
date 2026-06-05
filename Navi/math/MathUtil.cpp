
#include "MathUtil.h"



double MathUtil::epsilon = 0.000000001;
double MathUtil::twopi_inv = 0.5/PI;
double MathUtil::twopi = 2.0*PI;

MathUtil::MathUtil(void)
{
}


MathUtil::~MathUtil(void)
{
}
//从弧度变化到角度
double MathUtil::toAngle(double theta)
{
	return (theta * 180.0 / PI);
}

//从角度变化到弧度
double MathUtil::toRadians(double angle)
{
	return (angle * PI / 180.0);
}
    // only good for positive numbers.
double MathUtil::mod2pi_pos(double vin)
    {
        double q = vin * twopi_inv + 0.5;
        int qi = (int) q;

        return vin - qi*twopi;
    }

    /** Ensure that v is [-PI, PI] **/
double MathUtil::mod2pi(double vin)
    {
        double v;

        if (vin < 0)
            v = -mod2pi_pos(-vin);
        else
            v = mod2pi_pos(vin);

        // Validation test:
        //	if (v < -Math.PI || v > Math.PI)
        //		System.out.printf("%10.3f -> %10.3f\n", vin, v);

        return v;
    }

    /** Returns a value of v wrapped such that ref and v differ by no
     * more +/-PI
     **/
 double MathUtil::mod2pi(double ref, double v)
    {
        return ref + mod2pi(v-ref);
    }

double MathUtil::exp(double xin)
    {
        if (xin>=0)
            return exp_pos(xin);

        return 1/(exp_pos(-xin));
    }

    /** Quickly compute e^x for positive x.
     **/
double MathUtil::exp_pos(double xin)
    {
        // our algorithm: compute 2^(x/log(2)) by breaking exponent
        // into integer and fractional parts.  The integer part can be
        // done with a bit shift operation. The fractional part, which
        // has bounded magnitude, can be computed with a polynomial
        // approximation. We then multiply together the two parts.

        // prevent deep recursion that would just return INF anyway...
        // e^709 > Double.MAX_VALUE;
        if (xin>709)
            return Double_MAX_VALUE;

        if (xin>43) // recursively handle values which would otherwise blow up.
	    {
            // the value 43 was determined emperically
            return 4727839468229346561.4744575*exp_pos(xin-43);
	    }

        double x = 1.44269504088896*xin; // now we compute 2^x
        int wx = (int) x; // integer part
        double rx = x-wx;    // fractional part

        rx*=0.69314718055995; // scale fractional part by log(2)

        double b = 1L<<wx; // 2^integer part
        double rx2 = rx*rx;
        double rx3 = rx2*rx;
        double rx4 = rx3*rx;

        double r = 1+rx+rx2/2+rx3/6+rx4/24; // polynomial approximation for bounded rx.

        return b*r;
    }
