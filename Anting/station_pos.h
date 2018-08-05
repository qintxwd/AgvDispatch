#ifndef STATION_POS_H
#define STATION_POS_H
class StationPos
{
public:
    int m_id;
    int m_level;
    int m_pickPos1;
    int m_pickPos2;
    int m_putPos1;
    int m_putPos2;
    int m_x;
    int m_y;
    int m_t;
    StationPos(int _id, int _level, int _pickPos1, int _pickPos2, int _putPos1, int _putPos2,int _x,int _y,int _t)
    {
        m_id = _id;
        m_level = _level;
        m_pickPos1 = _pickPos1;
        m_pickPos2 = _pickPos2;
        m_putPos1 = _putPos1;
        m_putPos2 = _putPos2;
        m_x = _x;
        m_y = _y;
        m_t = _t;
    }
    StationPos(){}
};
#endif // STATION_POS_H
