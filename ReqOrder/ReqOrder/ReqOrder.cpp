// ReqOrder.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <boost/thread/thread.hpp>
#include "ReadFile.h"
#include<fstream>
#include<omp.h>
#include<ctime>
#include <thread>
#include <mutex>
using namespace std;

static omp_lock_t c_lock;
ofstream ofile;


void ReadFile::OnFrontConnected()
{
	//cout << "���ӳɹ�" << endl;
	ReqUserLogin();
}

vector<CtpConnectionInfo> ReadFile::getConn()
{
	return this->connection_infos;
}

void ReadFile::UnInitialUserApi()
{
	if (this->ctp_trade){
		this->ctp_trade->RegisterSpi(NULL);
		this->ctp_trade->Release();
		this->ctp_trade = NULL;
	}
}

void ReadFile::initialize(CtpConnectionInfo conn)
{
	omp_init_lock(&c_lock);
	//for (auto &conn : connection_infos)
	this->ctp_trade = CThostFtdcTraderApi::CreateFtdcTraderApi();
	//CtpConnectionInfo conn = connection_infos[i];
	this->ctp_trade_front_address_ = conn.ctp_trade_front_address_;
	this->ctp_broker_id_ = conn.ctp_trade_broker_id_;
	this->ctp_investor_id_ = conn.ctp_trade_investor_id_;
	this->ctp_trade_password_ = conn.ctp_trade_password_;
	this->ctp_trade->RegisterSpi(this);
	this->ctp_trade->SubscribePublicTopic(THOST_TERT_QUICK);
	this->ctp_trade->SubscribePrivateTopic(THOST_TERT_QUICK);
	int len = ctp_trade_front_address_.length();
	char *p=new char[len];
	ctp_trade_front_address_.copy(p, len, 0);
	ctp_trade->RegisterFront(p);
	omp_set_lock(&c_lock);
	cout << "connection_id_ : " << conn.connection_id_ << endl;
	omp_unset_lock(&c_lock);
	omp_set_lock(&c_lock);
	cout << "account_name_ : " << conn.account_name_ << endl;
	omp_unset_lock(&c_lock);
	this->ctp_trade->Init();
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReadFile::ReqUserLogin()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	ctp_broker_id_.copy(req.BrokerID, ctp_broker_id_.length(), 0);
	ctp_investor_id_.copy(req.UserID, ctp_investor_id_.length(), 0);
	ctp_trade_password_.copy(req.Password, ctp_trade_password_.length(), 0);
	int iResult = this->ctp_trade->ReqUserLogin(&req, ++this->trade_request_id_);
}

void ReadFile::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		this->front_id_ = pRspUserLogin->FrontID;
		this->session_id_ = pRspUserLogin->SessionID;
		strcpy_s(order_ref, pRspUserLogin->MaxOrderRef);
		Ref = atoi(pRspUserLogin->MaxOrderRef);
		ofile << "�û�ID: " << pRspUserLogin->UserID << endl;
		ReqQryInstrument();
	}
}

void ReadFile::load_connection_from_file()
{
	try
	{
		boost::property_tree::ptree pt;
		boost::property_tree::read_json("connection_50etf.txt", pt);
		BOOST_FOREACH(const boost::property_tree::ptree::value_type& v, pt.get_child("accounts"))
		{
			CtpConnectionInfo conn;
			conn.connection_id_ = v.second.get<int>("connection_id_");
			conn.account_name_ = v.second.get<string>("account_name_");
			conn.is_market_sim_ = v.second.get<bool>("is_market_sim_");
			conn.ctp_market_front_address_ = v.second.get<string>("ctp_market_front_address_");
			conn.ctp_market_broker_id_ = v.second.get<string>("ctp_market_broker_id_");
			conn.ctp_market_investor_id_ = v.second.get<string>("ctp_market_investor_id_");
			conn.ctp_market_password_ = v.second.get<string>("ctp_market_password_");
			conn.is_trade_sim_ = v.second.get<bool>("is_market_sim_");
			conn.ctp_trade_front_address_ = v.second.get<string>("ctp_trade_front_address_");
			conn.ctp_trade_broker_id_ = v.second.get<string>("ctp_trade_broker_id_");
			conn.ctp_trade_investor_id_ = v.second.get<string>("ctp_trade_investor_id_");
			conn.ctp_trade_password_ = v.second.get<string>("ctp_trade_password_");
			this->connection_infos.emplace_back(conn);
		}
		this->ctp_market_front_address_ = pt.get<string>("lts_market_front_address_");
		this->ctp_trade_front_address_ = pt.get<string>("lts_trade_front_address_");
		this->ctp_broker_id_ = pt.get<string>("lts_broker_id_");
		this->ctp_investor_id_ = pt.get<string>("lts_investor_id_");
		this->ctp_market_password_ = pt.get<string>("lts_market_password_");
		this->ctp_trade_password_ = pt.get<string>("lts_trade_password_");
	}
	catch (exception ex)
	{
		assert(0);
		cout << __FUNCTION__ << ex.what() << endl;
	}
}

void ReadFile::ReqQryOrder()
{
	CThostFtdcQryOrderField req;
	memset(&req, 0, sizeof(req));
	ctp_broker_id_.copy(req.BrokerID, ctp_broker_id_.length(), 0);
	ctp_investor_id_.copy(req.InvestorID, ctp_investor_id_.length(), 0);
	int iResult = ctp_trade->ReqQryOrder(&req, ++this->trade_request_id_);
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReadFile::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
	if ((pOrder != NULL) && (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded)
		&& (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) && (pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing))
	{
		CThostFtdcOrderField* ctp_order = new CThostFtdcOrderField();
		*ctp_order = *pOrder;
		query_buffer_.push_back(ctp_order);
	}

	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		query_cv_.notify_one();
		query_finished_ = true;
	}
	ReqQryInstrument();
}

void ReadFile::ReqQryInstrument()
{
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	//strcpy_s(req.ExchangeID, "DCE");
	int iResult = this->ctp_trade->ReqQryInstrument(&req, ++this->trade_request_id_);
	cout << "�����ѯ��Լ: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReadFile::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "��ѯ��Լ���:" << endl;
	if (!IsErrorRspInfo(pRspInfo))
	{
		cout << "����������:" << pInstrument->ExchangeID << endl;
		cout << "��Լ����:" << pInstrument->InstrumentID << endl;
		cout << "��Լ�ڽ������Ĵ���:" << pInstrument->ExchangeInstID << endl;
		cout << "ִ�м�:" << pInstrument->StrikePrice << endl;
		cout << "������:" << pInstrument->EndDelivDate << endl;
		cout << "��ǰ����״̬:" << pInstrument->IsTrading << endl;
		ReqQryExchange();
	}
}

void ReadFile::ReqQryExchange()
{
	CThostFtdcQryExchangeField req;
	memset(&req, 0, sizeof(req));
	int iResult = this->ctp_trade->ReqQryExchange(&req, ++this->trade_request_id_);
	cout << "�����ѯ������: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReadFile::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "��ѯ���������:" << endl;
	if (!IsErrorRspInfo(pRspInfo))
	{
		cout << "����������" << pExchange->ExchangeID << endl;
		cout << "����������" << pExchange->ExchangeName << endl;
		cout << "����������" << pExchange->ExchangeProperty << endl;
	}
}

void ReadFile::remove_all_orders()
{
	unique_lock<mutex> query_lock(query_mutex_);
	ReqQryOrder();
	while (false == query_finished_)
	{
		query_cv_.wait(query_lock);
	}
	int s = query_buffer_.size();
	#pragma omp parallel num_threads(4)
	for (int i = 0; i < s; i++)
	{
		cout <<"threads: "<< omp_get_thread_num()<<endl;
		CThostFtdcOrderField *p = static_cast<CThostFtdcOrderField*> (query_buffer_[i]);

		if (IsTradingOrder(static_cast<CThostFtdcOrderField*>(p)))
		{
			ReqOrderAction(p);
		}
	}
	/*
	for (void* p : query_buffer_)
	{
		if (IsTradingOrder(static_cast<CThostFtdcOrderField*>(p)))
		{
			//cout << "�����ȴ��ɽ��У�" << endl;
			ReqOrderAction(static_cast<CThostFtdcOrderField*>(p));
		}
	}*/
	query_buffer_.clear();
	query_finished_ = false;
	query_lock.unlock();
	omp_set_lock(&c_lock);
	cout << " REMOVE ORDERS FINISHED " << __FUNCTION__ << endl;
	omp_unset_lock(&c_lock);
}

void  ReadFile::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	char str[10];
	sprintf_s(str, "%d", pOrder->OrderSubmitStatus);
	int orderState = atoi(str) - 48;

	//cout << "����Ӧ��!" << endl;
	
	if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled){
		omp_set_lock(&c_lock);
		cout << "�ѳ�����" << endl;
		omp_unset_lock(&c_lock);
		ofile << "����������: " << pOrder->ExchangeID << "\t";
		ofile <<"��Լ����: "<< pOrder->InstrumentID << "\t";
		ofile << "��������: " << pOrder->Direction << "\t";
		ofile << "ƽ���۸�: " << pOrder->LimitPrice << "\t";
		ofile << "����: " << pOrder->VolumeTotalOriginal << "\t";
		ofile << "��������: " << pOrder->TradingDay << endl;
	}
}

void ReadFile::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	cout << "�����ɹ��ɽ�!" << endl;
	cout << "�ɽ�ʱ�䣺 " << pTrade->TradeTime << endl;
	cout << "��Լ���룺 " << pTrade->InstrumentID << endl;
	cout << "�ɽ��۸� " << pTrade->Price << endl;
	cout << "�ɽ����� " << pTrade->Volume << endl;
	cout << "��ƽ�ַ��� " << pTrade->Direction << endl;
}

bool ReadFile::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

//pOrderָ�򱨵���Ϣ�ṹ�ĵ�ַ
//ָʾ�ôη����Ƿ�Ϊ��� nRequestID �����һ�η���
void  ReadFile::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "OnRspOrderAction" << endl;
	if (!IsErrorRspInfo(pRspInfo)){
		assert(0);
	}
}

bool ReadFile::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		cout << "ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
	return bResult;
}

//��������
void ReadFile::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, pOrder->BrokerID);
	strcpy_s(req.InvestorID, pOrder->InvestorID);
	strcpy_s(req.OrderRef, pOrder->OrderRef);
	req.FrontID = pOrder->FrontID;
	req.SessionID = pOrder->SessionID;
	strcpy_s(req.ExchangeID, pOrder->ExchangeID);
	strcpy_s(req.OrderSysID, pOrder->OrderSysID);
	req.ActionFlag = THOST_FTDC_AF_Delete;
	strcpy_s(req.InstrumentID, pOrder->InstrumentID);
	int iResult = ctp_trade->ReqOrderAction(&req, ++this->trade_request_id_);
}

int _tmain(int argc, _TCHAR* argv[])
{
	ReadFile r;
	r.load_connection_from_file();
	ofile.open("Canceled.txt");
	vector<CtpConnectionInfo> conns=r.getConn();
	int s = conns.size();
	omp_set_num_threads(2);
	omp_set_dynamic(4);
	ReadFile r1;
//#pragma omp parallel for private(r1) 
	for (int i = 0; i < s; i++)
	{
		ReadFile r1;
		r1.initialize(conns[i]);
		//r1.remove_all_orders();
		r1.UnInitialUserApi();
	}
	ofile.close();
	return 0;
}

