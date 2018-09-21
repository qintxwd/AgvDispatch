#pragma once

#include "../agvtasknodedothing.h"

class QingdaoNodeTingCharge :
	public AgvTaskNodeDoThing
{
public:
	QingdaoNodeTingCharge(std::vector<std::string> _params);
	virtual ~QingdaoNodeTingCharge();

	enum { Type = AgvTaskNodeDoThing::Type + 22 };

	int type() { return Type; }

	void beforeDoing(AgvPtr agv);
	void doing(AgvPtr agv);
	void afterDoing(AgvPtr agv);
	bool result(AgvPtr agv);

	std::string discribe() {
		return std::string("Qingdao Agv CHARGE");
	}
private:
    bool bresult;
};

