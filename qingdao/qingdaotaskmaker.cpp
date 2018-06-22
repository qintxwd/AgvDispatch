#include "QingdaoTaskMaker.h"
#include "../taskmanager.h"
#include "qingdaonodetingput.h"
#include "qingdaonodetingget.h"
#include "qingdaonodetingcharge.h"

QingdaoTaskMaker::QingdaoTaskMaker()
{
}


QingdaoTaskMaker::~QingdaoTaskMaker()
{
}

void QingdaoTaskMaker::init()
{
	//nothing to do
}

void QingdaoTaskMaker::makeTask(qyhnetwork::TcpSessionPtr conn, const Json::Value &request)
{
	AgvTaskPtr task(new AgvTask());

	//1.指定车辆
	int agvId = request["agv"].asInt();
	task->setAgv(agvId);

	//2.优先级
	int priority = request["priority"].asInt();
	task->setPriority(priority);

	//3.额外的参数
	if (!request["extra_params"].isNull()) {
		Json::Value extra_params = request["extra_params"];
		Json::Value::Members mem = extra_params.getMemberNames();
		for (auto iter = mem.begin(); iter != mem.end(); iter++)
		{
			task->setExtraParam(*iter, extra_params[*iter].asString());
		}		
	}

	//4.节点
	if (!request["nodes"].isNull()) {
		Json::Value nodes = request["nodes"];
		for (int i = 0; i < nodes.size(); ++i) {
			Json::Value one_node = nodes[i];
			int station = one_node["station"].asInt();
			int doWhat = one_node["dowhat"].asInt();
			std::string node_params_str = one_node["params"].asString();
			std::vector<std::string> node_params = split(node_params_str, ";");

			//根据客户端的代码，
			//dowhat列表为
			// 0 --> pick
			// 1 --> put
			// 2 --> charge
			AgvTaskNodePtr node_node(new AgvTaskNode());
			node_node->setStation(station);

			if (doWhat == 0) {
				AgvTaskNodeDoThingPtr getThing(new QingdaoNodeTingGet(node_params));
				node_node->push_backDoThing(getThing);
			}else if (doWhat == 1) {
				AgvTaskNodeDoThingPtr putThing(new QingdaoNodeTingPut(node_params));
				node_node->push_backDoThing(putThing);
			}else if (doWhat == 2) {
				AgvTaskNodeDoThingPtr chargeThing(new QingdaoNodeTingCharge(node_params));
				node_node->push_backDoThing(chargeThing);
			}
			task->push_backNode(node_node);
		}
	}

	//5.产生时间 
	task->setProduceTime(getTimeStrNow());

	combined_logger->info(" getInstance()->addTask ");

	TaskManager::getInstance()->addTask(task);
}