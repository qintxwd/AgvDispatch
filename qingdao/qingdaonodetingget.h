#pragma once

#include "../agvtasknodedothing.h"

class QingdaoNodeTingGet :
	public AgvTaskNodeDoThing
{
public:
	QingdaoNodeTingGet(std::vector<std::string> _params);
	virtual ~QingdaoNodeTingGet();

	enum { Type = AgvTaskNodeDoThing::Type + 22 };

	int type() { return Type; }

	void beforeDoing(AgvPtr agv);
	void doing(AgvPtr agv);
	void afterDoing(AgvPtr agv);
	bool result(AgvPtr agv);

	std::string discribe() {
		return std::string("Qingdao Agv GET_GOOD");
	}
private:
    bool bresult;
};

