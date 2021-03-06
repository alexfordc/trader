#ifndef STRATEGYTEMPLATE_H
#define STRATEGYTEMPLATE_H
#include<map>
#include<set>
#include<memory>
#include<atomic>
#include<vector>
#include"structs.h"
#include"CTAAPI.h"
#include"json11.h"
#include"AlgorithmOrder.h"
#include"MongoCxx.h"
#define BAR_MODE "barmode"
#define TICK_MODE "tickmode"

class algorithmOrder;
class MongoCxx;
class CTPAPI;

class StrategyData
{
public:
	void insertparam(std::string key, std::string value);					//插入参数
	void insertvar(std::string key, std::string value);						//插入变量
	std::string getparam(std::string);										//获取参数
	std::string getvar(std::string);										//获取变量
	std::map<std::string, std::string>getallparam();						//获取所有
	std::map<std::string, std::string>getallvar();							//获取所有
private:
	std::mutex m_mtx;					//变量锁
	std::map<std::string, std::string>m_parammap;//参数列表
	std::map<std::string, std::string>m_varmap;//变量列表
};

class StrategyTemplate
{
public:
	StrategyTemplate(CTAAPI *ctamanager);
	~StrategyTemplate();
	/******************************策略参数和变量*********************************************/
	//基本参数
	std::string gatewayname;				//CTP
	std::string tickDbName;					//tick数据库名
	std::string BarDbName;					//Bar数据库名
	//基础变量
	std::string trademode;					//交易模式
	bool inited;							//初始化控制
	bool trading;							//交易控制
	int initDays;							//加载历史数据的天数

	//算法交易部分变量						默认值，可以不修改在策略实例中调用setlimit函数来重新设置
	double unitLimit;						//算法下单
	Mode TradingMode;				//回测还是实盘

	/******************************CTAMANAGER控制策略***********************************************/
	//初始化
	virtual void onInit();
	//开始 
	virtual void onStart();
	//停止
	virtual void onStop();
	//撤所有单，停止策略时使用
	void cancelallorder();
	//给参数赋值
	void checkparam(const char* paramname, const char* paramvalue);
	//给pos赋值
	void checkSymbol(const char* symbolname);
	//获取参数的值
	std::string getparam(std::string paramname);
	//加载策略数据到界面
	void putGUI();
	//更新参数到界面
	virtual void putEvent();
	//tradeevent更新持仓
	void changeposmap(std::string symbol, double pos);
	/*******************************实现策略主要函数**************************************************/
	//TICK
	virtual void onTick(TickData Tick);
	//BAR
	virtual void onBar(BarData Bar);
	//报单回调
	virtual void onOrder(std::shared_ptr<Event_Order>e);
	//成交回调
	virtual void onTrade(std::shared_ptr<Event_Trade>e);

	//发单函数

	//做多
	std::vector<std::string> buy(std::string symbol, double price, double volume);
	//平多
	std::vector<std::string> sell(std::string symbol, double price, double volume);
	//做空
	std::vector<std::string> sellshort(std::string symbol, double price, double volume);
	//平空
	std::vector<std::string> buycover(std::string symbol, double price, double volume);

	//撤单函数
	void cancelOrder(std::string orderID, std::string gatewayname);

	//算法交易
	double getpos(std::string symbol);							//给算法交易模块获取持仓外部接口
	std::map<std::string, double>getposmap();					//获取全部持仓，给算法交易设置用

	//提供给backtestengine的外部接口
	std::map<std::string, std::string> GetVarPlotMap();
	std::map<std::string, std::string> GetIndicatorMap();


	//算法交易
	algorithmOrder *m_algorithm;
protected:
	//读取历史数据
	std::vector<TickData>loadTick(std::string symbol, int days);
	std::vector<BarData>loadBar(std::string symbol, int days);

	//利用mongocxx模板保存和读取mongodb
	virtual void savepostomongo();
	virtual void loadposfrommongo();
	MongoCxx *m_MongoCxx;

	//CTA管理器
	CTAAPI *m_ctamanager;

	//Bar推送                                   tick级可以不用 ， 如果是bar的就要用
	std::mutex m_hourminutemtx;					//可以在继承的策略直接用这些变量，省着声明了 
	int m_minute;
	int m_hour;
	BarData m_bar;

	//两个参数列表
	StrategyData *m_strategydata;

	/**********************************_(:з」∠)_回测专用神器***********************************************/
	std::map<std::string, std::string>m_VarPlot;	std::map<std::string, std::string>m_indicatorPlot;			std::mutex m_VarPlotmtx;
private:
	//总报单开平函数公用
	std::vector<std::string> sendOrder(std::string symbol, std::string orderType, double price, double volume);

	/*******************************************************/
	std::map<std::string, double>m_pos_map;				//持仓
	std::mutex m_Pos_mapmtx;

	//OrderList
	std::mutex m_orderlistmtx;							//撤单锁
	std::vector<std::string>m_orderlist;				//普通报单列表
};

class BarGreater
{
public:
	bool operator ()(const BarData& bar1, const BarData& bar2)
	{
		return bar1.unixdatetime < bar2.unixdatetime;
	}
};

class TickGreater
{
public:
	bool operator ()(const TickData& tick1, const TickData& tick2)
	{
		return tick1.unixdatetime < tick2.unixdatetime;
	}
};

#endif

/*
.h

std::map<std::string,int> m_barMinute;
std::map<std::string, int> m_barHour;
std::map<std::string, BarData> m_barmap;









.cpp

if (m_barMinute.find(Tick.symbol) == m_barMinute.end())
{
m_barMinute.insert(std::pair<std::string, int>(Tick.symbol, 99));
}
if (m_barHour.find(Tick.symbol) == m_barHour.end())
{
m_barHour.insert(std::pair<std::string, int>(Tick.symbol, 99));
}
putEvent();

int tickMinute = Tick.getminute();
int tickHour = Tick.gethour();

m_hourminutemtx.lock();

if ((tickMinute != m_barMinute[Tick.symbol]) || (tickHour != m_barHour[Tick.symbol]))
{
if (m_barmap.find(Tick.symbol) != m_barmap.end())
{//判断这个合约是否要存分钟bar
onBar(m_barmap[Tick.symbol]);
}
BarData bar;

bar.symbol = Tick.symbol;
bar.exchange = Tick.exchange;
bar.open = Tick.lastprice;
bar.high = Tick.lastprice;
bar.low = Tick.lastprice;
bar.close = Tick.lastprice;

bar.openPrice = Tick.openPrice;//今日开
bar.highPrice = Tick.highPrice;//今日高
bar.lowPrice = Tick.lowPrice;//今日低
bar.preClosePrice = Tick.preClosePrice;//昨收

bar.upperLimit = Tick.upperLimit;//涨停
bar.lowerLimit = Tick.lowerLimit;//跌停

bar.volume = Tick.volume;
bar.openInterest = Tick.openInterest;

bar.date = Tick.date;
bar.time = Tick.time;
bar.unixdatetime = Tick.unixdatetime;
m_barmap[Tick.symbol] = bar;
m_barMinute[Tick.symbol] = tickMinute;
m_barHour[Tick.symbol] = tickHour;

}
else
{
m_barmap[Tick.symbol].high = std::max(m_barmap[Tick.symbol].high, Tick.lastprice);
m_barmap[Tick.symbol].low = std::min(m_barmap[Tick.symbol].low, Tick.lastprice);
m_barmap[Tick.symbol].close = Tick.lastprice;
m_barmap[Tick.symbol].highPrice = Tick.highPrice;
m_barmap[Tick.symbol].lowPrice = Tick.lowPrice;
m_barmap[Tick.symbol].volume = Tick.volume;
}
m_hourminutemtx.unlock();




































*/