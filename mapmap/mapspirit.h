#ifndef MAPSPIRIT_H
#define MAPSPIRIT_H

#include <string>

class MapSpirit
{
public:
    enum Map_Spirit_Type
    {
        Map_Sprite_Type_Point = 0,
        Map_Sprite_Type_Path,
        Map_Sprite_Type_Floor,
        Map_Sprite_Type_Background,
        Map_Sprite_Type_Block,
        Map_Sprite_Type_Group,
    };

    MapSpirit(int _id,std::string _name,Map_Spirit_Type _spirit_type);

    MapSpirit(const MapSpirit &s) = delete;

    virtual ~MapSpirit();

    virtual MapSpirit *clone();

    bool operator ==(const MapSpirit &s){
        return id == s.id;
    }

    Map_Spirit_Type getSpiritType() const
    {
        return spirit_type;
    }
    void setSpiritType(Map_Spirit_Type __type){spirit_type = __type;}

    int getId() const{return id;}
    void setId(int _id){id=_id;}

    std::string getName() const {return name;}
    void setName(std::string _name){name=_name;}
private:
    int id;
    std::string name;
    Map_Spirit_Type spirit_type;
};

#endif // MAPSPIRIT_H
