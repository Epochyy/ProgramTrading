#include<string>
#include<iostream>
#include <condition_variable>
#include <thread>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <ThostFtdcTraderApi.h>

#pragma comment (lib, "thostmduserapi.lib")
#pragma comment (lib, "thosttraderapi.lib")

using namespace std;

/*
将报单一个一个取出来进行撤单
参考remove all;
*/

TThostFtdcOrderRefType	order_ref;	//报单引用
static int Ref;

struct CtpConnectionInfo
{
	int connection_id_;
	string account_name_;
	bool is_market_sim_;
	string ctp_market_front_address_;
	string ctp_market_broker_id_;
	string ctp_market_investor_id_;
	string ctp_market_password_;
	bool is_trade_sim_;
	string ctp_trade_front_address_;
	string ctp_trade_broker_id_;
	string ctp_trade_investor_id_;
	string ctp_trade_password_;
};

class ReadFile : public CThostFtdcTraderSpi
{
public:
	void load_connection_from_file();
	ReadFile(){};
	void initialize(CtpConnectionInfo conn);
	void remove_all_orders();
	void ReqUserLogin();
	void ReqQryOrder();
	void ReqOrderAction(CThostFtdcOrderField *pOrder);
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
	virtual void OnFrontConnected();
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
	virtual bool IsTradingOrder(CThostFtdcOrderField *pOrder);
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
	vector<CtpConnectionInfo> getConn();
	void UnInitialUserApi();
private:
	CThostFtdcTraderApi * ctp_trade;
	string ctp_market_front_address_;
	string ctp_trade_front_address_;
	string ctp_broker_id_;
	string ctp_investor_id_;
	string ctp_password_;
	string ctp_market_password_;
	string ctp_trade_password_;
	vector<CtpConnectionInfo> connection_infos;
	int trade_request_id_;
	int front_id_;
	int session_id_;
	vector<void*> query_buffer_;
	condition_variable query_cv_;
	mutex query_mutex_;
	bool query_finished_;
};

