#ifndef BEZIERARC_H
#define BEZIERARC_H

class PointF
{
public:
    PointF();
    PointF(double xpos, double ypos);
    PointF(int xpos, int ypos);


    double manhattanLength() const;

    double x() const;
    double y() const;
    void setX(double x);
    void setY(double y);

    double &rx();
    double &ry();

    PointF &operator+=(const PointF &p);
    PointF &operator-=(const PointF &p);
    PointF &operator*=(double c);
    PointF &operator/=(double c);

    static double dotProduct(const PointF &p1, const PointF &p2)
    { return p1.xp * p2.xp + p1.yp * p2.yp; }

    friend bool operator==(const PointF &, const PointF &);
    friend bool operator!=(const PointF &, const PointF &);
    friend const PointF operator+(const PointF &, const PointF &);
    friend const PointF operator-(const PointF &, const PointF &);
    friend const PointF operator*(double, const PointF &);
    friend const PointF operator*(const PointF &, double);
    friend const PointF operator+(const PointF &);
    friend const PointF operator-(const PointF &);
    friend const PointF operator/(const PointF &, double);
    friend const double getDistance(const PointF &, const PointF &);
private:
    double xp;
    double yp;
};


class BezierArc
{
public:
    BezierArc();

    //计算长度
    static double BezierArcLength(PointF p1, PointF p2, PointF p3, PointF p4);
    static double BezierArcLength(PointF p1, PointF p2, PointF p3);

    typedef struct position_pose{
        PointF pos;
        double angle;
    }POSITION_POSE;

    //计算位姿
    static POSITION_POSE BezierArcPoint(PointF p1, PointF p2, PointF p3, PointF p4, double t);
    static POSITION_POSE BezierArcPoint(PointF p1, PointF p2, PointF p3, double t);
private:
    static double Simpson (
            double (*f)(double),
            double a,
            double b,
            int n_limit,
            double TOLERANCE);
    static double balf(double t);
};

#endif // BEZIERARC_H
