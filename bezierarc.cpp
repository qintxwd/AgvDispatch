#include "bezierarc.h"

#define _USE_MATH_DEFINES
#include <math.h>
#define sqr(x) (x * x)

#define _ABS(x) (x < 0 ? -x : x)

const double TOLERANCE = 0.0000001;  // Application specific tolerance

extern double sqrt(double);

double q1, q2, q3, q4, q5;

struct point2d {
    double x, y;
};

BezierArc::BezierArc()
{

}
double BezierArc::BezierArcLength(PointF p1, PointF p2, PointF p3, PointF p4)
{
    PointF k1, k2, k3, k4;

    k1 = -p1 + 3*(p2 - p3) + p4;
    k2 = 3*(p1 + p3) - 6*p2;
    k3 = 3*(p2 - p1);
    k4 = p1;

    q1 = 9.0*(sqr(k1.x()) + sqr(k1.y()));
    q2 = 12.0*(k1.x()*k2.x() + k1.y()*k2.y());
    q3 = 3.0*(k1.x()*k3.x() + k1.y()*k3.y()) + 4.0*(sqr(k2.x()) + sqr(k2.y()));
    q4 = 4.0*(k2.x()*k3.x() + k2.y()*k3.y());
    q5 = sqr(k3.x()) + sqr(k3.y());

    double result = Simpson(balf, 0, 1, 1024, 0.001);
    return result;
}

double BezierArc::BezierArcLength(PointF p1, PointF p2, PointF p3)
{
    PointF a,b;
    a.setX(p1.x() - 2*p2.x() + p3.x());
    a.setY(p1.y() - 2*p2.y() + p3.y());
    b.setX(2*p2.x() - 2*p1.x());
    b.setY(2*p2.y() - 2*p1.y());
    float A = 4*(a.x()*a.x() + a.y()*a.y());
    float B = 4*(a.x()*b.x() + a.y()*b.y());
    float C = b.x()*b.x() + b.y()*b.y();

    float Sabc = 2*sqrt(A+B+C);
    float A_2 = sqrt(A);
    float A_32 = 2*A*A_2;
    float C_2 = 2*sqrt(C);
    float BA = B/A_2;

    return ( A_32*Sabc +
             A_2*B*(Sabc-C_2) +
             (4*C*A-B*B)*log( (2*A_2+BA+Sabc)/(BA+C_2) )
             )/(4*A_32);
}


BezierArc::POSITION_POSE BezierArc::BezierArcPoint(PointF p1, PointF p2, PointF p3, PointF p4,double t)
{
    //计算坐标
    BezierArc::POSITION_POSE pp;
    pp.pos.setX(
                (p1.x()*(1 - t)*(1 - t)*(1 - t)
                           + 3 * p2.x()*t*(1 - t)*(1 - t)
                           + 3 * p3.x()*t*t*(1 - t)
                           + p4.x() * t*t*t)
                );

    pp.pos.setY(
                (p1.y()*(1 - t)*(1 - t)*(1 - t)
                           + 3 * p2.y()*t*(1 - t)*(1 - t)
                           + 3 * p3.y()*t*t*(1 - t)
                           + p4.y() * t*t*t)
                );

    PointF slope = 3*(1-t)*(1-t)*(p2-p1)+6*(1-t)*t*(p3-p2)+3*t*t*(p4-p3);

    pp.angle = (atan2(slope.y(),slope.x()) * 180 / M_PI);
    return pp;
}

BezierArc::POSITION_POSE BezierArc::BezierArcPoint(PointF p1, PointF p2, PointF p3,double t)
{
    //计算坐标
    BezierArc::POSITION_POSE pp;
    pp.pos.setX(
                (p1.x()*(1 - t)*(1 - t)
                           + 2 * p2.x()*t*(1 - t)
                           + p3.x() * t*t)
                );

    pp.pos.setY(
                (p1.y()*(1 - t)*(1 - t)
                           + 2 * p2.y()*t*(1 - t)
                           + p3.y() * t*t)
                );

    PointF slope = 2*(1-t)*(p2-p1)+2*t*(p3-p2);

    pp.angle = (atan2(slope.y(),slope.x()) * 180 / M_PI);
    return pp;
}

//---------------------------------------------------------------------------
double BezierArc::balf(double t)                   // Bezier Arc Length Function
{
    double result = q5 + t*(q4 + t*(q3 + t*(q2 + t*q1)));
    if(result<0)result*=-1;
    result = sqrt(result);
    return result;
}

//---------------------------------------------------------------------------
// NOTES:       TOLERANCE is a maximum error ratio
//                      if n_limit isn't a power of 2 it will be act like the next higher
//                      power of two.
double BezierArc::Simpson (
        double (*f)(double),
        double a,
        double b,
        int n_limit,
        double TOLERANCE)
{
    int n = 1;
    double multiplier = (b - a)/6.0;
    double endsum = f(a) + f(b);
    double interval = (b - a)/2.0;
    double asum = 0;
    double bsum = f(a + interval);
    double est1 = multiplier * (endsum + 2 * asum + 4 * bsum);
    double est0 = 2 * est1;

    while(n < n_limit
          && (_ABS(est1) > 0 && _ABS((est1 - est0) / est1) > TOLERANCE)) {
        n *= 2;
        multiplier /= 2;
        interval /= 2;
        asum += bsum;
        bsum = 0;
        est0 = est1;
        double interval_div_2n = interval / (2.0 * n);

        for (int i = 1; i < 2 * n; i += 2) {
            double t = a + i * interval_div_2n;
            bsum += f(t);
        }

        est1 = multiplier*(endsum + 2*asum + 4*bsum);
    }

    return est1;
}



PointF::PointF() : xp(0), yp(0) { }

PointF::PointF(double xpos, double ypos) : xp(xpos), yp(ypos) { }
PointF::PointF(int xpos, int ypos) : xp(1.0*xpos), yp(1.0*ypos) { }

double PointF::manhattanLength() const
{
    return fabsl(x())+fabsl(y());
}

double PointF::x() const
{
    return xp;
}

double PointF::y() const
{
    return yp;
}

void PointF::setX(double xpos)
{
    xp = xpos;
}

void PointF::setY(double ypos)
{
    yp = ypos;
}

double &PointF::rx()
{
    return xp;
}

double &PointF::ry()
{
    return yp;
}

PointF &PointF::operator+=(const PointF &p)
{
    xp+=p.xp;
    yp+=p.yp;
    return *this;
}

PointF &PointF::operator-=(const PointF &p)
{
    xp-=p.xp; yp-=p.yp; return *this;
}

PointF &PointF::operator*=(double c)
{
    xp*=c; yp*=c; return *this;
}

bool operator==(const PointF &p1, const PointF &p2)
{
    return p1.xp == p2.xp && p1.yp == p2.yp;
}

bool operator!=(const PointF &p1, const PointF &p2)
{
    return p1.xp != p2.xp || p1.yp != p2.yp;
}

const PointF operator+(const PointF &p1, const PointF &p2)
{
    return PointF(p1.xp+p2.xp, p1.yp+p2.yp);
}

const PointF operator-(const PointF &p1, const PointF &p2)
{
    return PointF(p1.xp-p2.xp, p1.yp-p2.yp);
}

const PointF operator*(const PointF &p, double c)
{
    return PointF(p.xp*c, p.yp*c);
}

const PointF operator*(double c, const PointF &p)
{
    return PointF(p.xp*c, p.yp*c);
}

const PointF operator+(const PointF &p)
{
    return p;
}

const PointF operator-(const PointF &p)
{
    return PointF(-p.xp, -p.yp);
}

PointF &PointF::operator/=(double divisor)
{
    xp/=divisor;
    yp/=divisor;
    return *this;
}

const PointF operator/(const PointF &p, double divisor)
{
    return PointF(p.xp/divisor, p.yp/divisor);
}

const double getDistance(const PointF &p1, const PointF &p2)
{
    return sqrt((p1.x()-p2.x())*(p1.x()-p2.x())+(p1.y()-p2.y())*(p1.y()-p2.y()));
}
