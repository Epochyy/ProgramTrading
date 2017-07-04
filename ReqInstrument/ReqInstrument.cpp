// ReqInstrument.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <boost/thread/thread.hpp>
#include "ReqInstrument.h"
#include<fstream>
#include<omp.h>
#include<ctime>
#include <thread>
#include <mutex>
using namespace std;

static omp_lock_t c_lock;
ofstream ofile;
ofstream ofile1;

void ReqInstrument::load_connection_from_file()
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

void ReqInstrument::OnFrontConnected()
{
	cout << "���ӳɹ�" << endl;
	ReqUserLogin();
}

vector<CtpConnectionInfo> ReqInstrument::getConn()
{
	return this->connection_infos;
}

void ReqInstrument::UnInitialUserApi()
{
	if (this->ctp_trade){
		this->ctp_trade->RegisterSpi(NULL);
		this->ctp_trade->Release();
		this->ctp_trade = NULL;
	}
}

void ReqInstrument::initialize(CtpConnectionInfo conn)
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
	char *p = new char[len];
	ctp_trade_front_address_.copy(p, len, 0);
	ctp_trade->RegisterFront(p);
	omp_set_lock(&c_lock);
	cout << "connection_id_ : " << conn.connection_id_ << endl;
	omp_unset_lock(&c_lock);
	omp_set_lock(&c_lock);
	cout << "account_name_ : " << conn.account_name_ << endl;
	omp_unset_lock(&c_lock);
	this->ctp_trade->Init();
	//this->ctp_trade->Join();
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::ReqUserLogin()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	ctp_broker_id_.copy(req.BrokerID, ctp_broker_id_.length(), 0);
	ctp_investor_id_.copy(req.UserID, ctp_investor_id_.length(), 0);
	ctp_trade_password_.copy(req.Password, ctp_trade_password_.length(), 0);
	int iResult = this->ctp_trade->ReqUserLogin(&req, ++this->trade_request_id_);
	cout << "��¼" << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
}

void ReqInstrument::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		this->front_id_ = pRspUserLogin->FrontID;
		this->session_id_ = pRspUserLogin->SessionID;
		strcpy_s(order_ref, pRspUserLogin->MaxOrderRef);
		Ref = atoi(pRspUserLogin->MaxOrderRef);
		ofile << "�û�ID: " << pRspUserLogin->UserID << endl;
		ofile1 << "�û�ID: " << pRspUserLogin->UserID << endl;
		ReqQryInstrument();
	}
}

void ReqInstrument::ReqQryOrder()
{
	CThostFtdcQryOrderField req;
	memset(&req, 0, sizeof(req));
	ctp_broker_id_.copy(req.BrokerID, ctp_broker_id_.length(), 0);
	ctp_investor_id_.copy(req.InvestorID, ctp_investor_id_.length(), 0);
	int iResult = ctp_trade->ReqQryOrder(&req, ++this->trade_request_id_);
	cout << "�����ѯ����: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
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

void ReqInstrument::ReqQryInstrument()
{
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	int iResult = this->ctp_trade->ReqQryInstrument(&req, ++this->trade_request_id_);
	cout << "�����ѯ��Լ: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//cout << "��ѯ��Լ������ļ�Instrument.txt��" << endl;
	if (!IsErrorRspInfo(pRspInfo))
	{
		ofile << "��Լ����:" << pInstrument->InstrumentID << "\t";
		ofile << "����������:" << pInstrument->ExchangeID << "\t";
		ofile << "��Լ����:" << pInstrument->InstrumentName << "\t";
		ofile << "��Լ�ڽ������Ĵ���:" << pInstrument->ExchangeInstID << "\t";
		ofile << "��Ʒ����:" << pInstrument->ProductID << "\t";
		ofile << "��Ʒ����:" << pInstrument->ProductClass << "\t";
		ofile << "�������:" << pInstrument->DeliveryYear << "\t";
		ofile << "������:" << pInstrument->DeliveryMonth << "\t";
		ofile << "�г�����µ���:" << pInstrument->MaxMarketOrderVolume << "\t";
		ofile << "�г���С�µ���:" << pInstrument->MinMarketOrderVolume << "\t";
		ofile << "�޼۵�����µ���:" << pInstrument->MaxLimitOrderVolume << "\t";
		ofile << "�޼۵���С�µ���:" << pInstrument->MinLimitOrderVolume << "\t";
		ofile << "��Լ��������:" << pInstrument->VolumeMultiple << "\t";
		ofile << "��С�䶯��λ" << pInstrument->PriceTick << "\t";
		ofile << "������:" << pInstrument->CreateDate << "\t";
		ofile << "������:" << pInstrument->OpenDate << "\t";
		ofile << "������:" << pInstrument->ExpireDate << "\t";
		ofile << "��ʼ������:" << pInstrument->StartDelivDate << "\t";
		ofile << "����������:" << pInstrument->EndDelivDate << "\t";
		ofile << "��Լ��������״̬:" << pInstrument->InstLifePhase << "\t";
		ofile << "��ǰ�Ƿ���:" << pInstrument->IsTrading << "\t";
		ofile << "�ֲ�����:" << pInstrument->PositionType << "\t";
		ofile << "�ֲ���������:" << pInstrument->PositionDateType << "\t";
		ofile << "��ͷ��֤����:" << pInstrument->LongMarginRatio << "\t";
		ofile << "��ͷ��֤����:" << pInstrument->ShortMarginRatio << "\t";
		ofile << "�Ƿ�ʹ�ô��߱�֤���㷨:" << pInstrument->MaxMarginSideAlgorithm << "\t";
		ofile << "������Ʒ����:" << pInstrument->UnderlyingInstrID;
		ofile << "ִ�м�:" << pInstrument->StrikePrice << "\t";
		ofile << "��Ȩ����:" << pInstrument->OptionsType;
		ofile << "��Լ������Ʒ����:" << pInstrument->UnderlyingMultiple << "\t";
		ofile << "�������:" << pInstrument->CombinationType << endl;
		//boost::this_thread::sleep(boost::posix_time::seconds(1));
		if (bIsLast)
		{
			cout << "��ѯ��Լ������ļ�Instrument.txt��" << endl;
			//boost::this_thread::sleep(boost::posix_time::seconds(1));
			ReqQryExchange();
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}
}

void ReqInstrument::ReqQryExchange()
{
	//boost::this_thread::sleep(boost::posix_time::seconds(1));
	CThostFtdcQryExchangeField req;
	memset(&req, 0, sizeof(req));
	int iResult = this->ctp_trade->ReqQryExchange(&req, ++this->trade_request_id_);
	cout << "�����ѯ������: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		ofile1 << "����������: " << pExchange->ExchangeID << endl;
		ofile1 << "����������: " << pExchange->ExchangeName << endl;
		ofile1 << "����������: " << pExchange->ExchangeProperty << endl;
		if (bIsLast)
		{
			cout << "��ѯ�����������Exchange.txt��" << endl;
		}
	}
}

void ReqInstrument::remove_all_orders()
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
		cout << "threads: " << omp_get_thread_num() << endl;
		CThostFtdcOrderField *p = static_cast<CThostFtdcOrderField*> (query_buffer_[i]);

		if (IsTradingOrder(static_cast<CThostFtdcOrderField*>(p)))
		{
			ReqOrderAction(p);
		}
	}
	query_buffer_.clear();
	query_finished_ = false;
	query_lock.unlock();
	omp_set_lock(&c_lock);
	cout << " REMOVE ORDERS FINISHED " << __FUNCTION__ << endl;
	omp_unset_lock(&c_lock);
}

void  ReqInstrument::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	char str[10];
	sprintf_s(str, "%d", pOrder->OrderSubmitStatus);
	int orderState = atoi(str) - 48;

	//cout << "����Ӧ��!" << endl;

	if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled){
		omp_set_lock(&c_lock);
		cout << "�ѳ�����" << endl;
		omp_unset_lock(&c_lock);
		ofile << "��Լ����: " << pOrder->InstrumentID << "\t";
		ofile << "����������: " << pOrder->ExchangeID << "\t";
		ofile << "����������: " << pOrder->ExchangeID << "\t";
		ofile << "��������: " << pOrder->Direction << "\t";
		ofile << "ƽ���۸�: " << pOrder->LimitPrice << "\t";
		ofile << "����: " << pOrder->VolumeTotalOriginal << "\t";
		ofile << "��������: " << pOrder->TradingDay << endl;
	}
}

void ReqInstrument::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	cout << "�����ɹ��ɽ�!" << endl;
	cout << "�ɽ�ʱ�䣺 " << pTrade->TradeTime << endl;
	cout << "��Լ���룺 " << pTrade->InstrumentID << endl;
	cout << "�ɽ��۸� " << pTrade->Price << endl;
	cout << "�ɽ����� " << pTrade->Volume << endl;
	cout << "��ƽ�ַ��� " << pTrade->Direction << endl;
}

bool ReqInstrument::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

//pOrderָ�򱨵���Ϣ�ṹ�ĵ�ַ
//ָʾ�ôη����Ƿ�Ϊ��� nRequestID �����һ�η���
void  ReqInstrument::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "OnRspOrderAction" << endl;
	if (!IsErrorRspInfo(pRspInfo)){
		assert(0);
	}
}

bool ReqInstrument::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		cout << "ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
	return bResult;
}

//��������
void ReqInstrument::ReqOrderAction(CThostFtdcOrderField *pOrder)
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

void ReqInstrument::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "OnRspError" << endl;
	cout << pRspInfo->ErrorID << ":" << pRspInfo->ErrorMsg << endl;
	IsErrorRspInfo(pRspInfo);
}

int _tmain(int argc, _TCHAR* argv[])
{
	ReqInstrument r;
	r.load_connection_from_file();
	ofile.open("Instrument.txt");
	ofile1.open("Exchange.txt");
	vector<CtpConnectionInfo> conns = r.getConn();
	int s = conns.size();
	//omp_set_num_threads(2);
	//omp_set_dynamic(4);
	ReqInstrument r1;
	//#pragma omp parallel for private(r1) 
	for (int i = 0; i < s; i++)
	{
		ReqInstrument r1;
		r1.initialize(conns[i]);
		//r1.remove_all_orders();
		boost::this_thread::sleep(boost::posix_time::seconds(6));
		r1.UnInitialUserApi();
	}
	ofile.close();
	return 0;
}

