#pragma once

#include "../agvtasknodedothing.h"

class QingdaoNodeTingPut :
	public AgvTaskNodeDoThing
{
public:
	QingdaoNodeTingPut(std::vector<std::string> _params);
	virtual ~QingdaoNodeTingPut();

	enum { Type = AgvTaskNodeDoThing::Type + 21 };

	int type() { return Type; }

	void beforeDoing(AgvPtr agv);
	void doing(AgvPtr agv);
	void afterDoing(AgvPtr agv);
	bool result(AgvPtr agv);

	std::string discribe() {
		return std::string("Qingdao Agv PUT_GOOD");
	}
private:
	bool bresult;//Ö´ÐÐ½á¹û
};

