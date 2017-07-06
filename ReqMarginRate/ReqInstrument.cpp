// ReqInstrument.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <boost/thread/thread.hpp>
#include "ReqInstrument.h"
#include<fstream>
#include<omp.h>
#include<ctime>
using namespace std;

static omp_lock_t c_lock;
ofstream ofile;
ofstream ofile1;
ofstream ofile2;
ofstream ofile3;
ofstream ofile4;

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
	boost::this_thread::sleep(boost::posix_time::seconds(6));
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
		ofile2 << "�û�ID: " << pRspUserLogin->UserID << endl;
		ofile3 << "�û�ID: " << pRspUserLogin->UserID << endl;
		ofile4 << "�û�ID: " << pRspUserLogin->UserID << endl;
		ReqQryInstrument();
		//ReqQryExchange();
		//ReqQryInstrumentMarginRate();
	}
}


void ReqInstrument::ReqQryInstrument()
{
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	int iResult = this->ctp_trade->ReqQryInstrument(&req, ++this->trade_request_id_);
	cout << "�����ѯ��Լ: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	//boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//cout << "��ѯ��Լ������ļ�Instrument.txt��" << endl;
	if (pInstrument!=NULL && !IsErrorRspInfo(pRspInfo))
	{
		CThostFtdcInstrumentField* ctp_instrument = new CThostFtdcInstrumentField();
		*ctp_instrument = *pInstrument;
		query_buffer_.push_back(ctp_instrument);

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
	}
	if (bIsLast)
	{
		cout << "��ѯ��Լ������ļ�Instrument.txt��" << endl;
		query_cv_.notify_one();
		query_finished_ = true;
		ReqQryExchange();
		boost::this_thread::sleep(boost::posix_time::seconds(1));
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
			ReqQryProduct();
		}
	}
}

bool ReqInstrument::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		cout << "ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
	return bResult;
}

void ReqInstrument::ReqQryProduct()
{
	CThostFtdcQryProductField req;
	memset(&req, 0, sizeof(req));
	int iResult = ctp_trade->ReqQryProduct(&req, ++this->trade_request_id_);
	cout << "�����Ʒ��ѯ: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo))
	{
		ofile2 << "��Ʒ����: " << pProduct->ProductID << "\t";
		ofile2 << "��Ʒ����: " << pProduct->ProductName << "\t";
		ofile2 << "����������: " << pProduct->ExchangeID << "\t";
		ofile2 << "��Ʒ����: " << pProduct->ProductClass << "\t";
		ofile2 << "��Լ��������: " << pProduct->VolumeMultiple << "\t";
		ofile2 << "��С�䶯��λ: " << pProduct->PriceTick << "\t";
		ofile2 << "�м۵�����µ���: " << pProduct->MaxMarketOrderVolume << "\t";
		ofile2 << "�м۵���С�µ���: " << pProduct->MinMarketOrderVolume << "\t";
		ofile2 << "�޼۵�����µ���: " << pProduct->MaxLimitOrderVolume << "\t";
		ofile2 << "�޼۵���С�µ���: " << pProduct->MinLimitOrderVolume << "\t";
		ofile2 << "�ֲ�����: " << pProduct->PositionType << "\t";
		ofile2 << "�ֲ���������: " << pProduct->PositionDateType << "\t";
		ofile2 << "ƽ�ִ�������: " << pProduct->CloseDealType << "\t";
		ofile2 << "���ױ�������: " << pProduct->TradeCurrencyID << "\t";
		ofile2 << "��Ѻ�ʽ���÷�Χ: " << pProduct->MortgageFundUseRange << "\t";
		ofile2 << "��������Ʒ����: " << pProduct->ExchangeProductID << "\t";
		ofile2 << "��Լ������Ʒ����: " << pProduct->UnderlyingMultiple << endl;
		if (bIsLast)
		{
			cout << "��ѯ������ļ�Product.txt��" << endl;
			cout << "�����ѯ��Լ����:" << endl;
			string instrument;
			cin >> instrument;
			ReqQryInstrumentMarginRate(instrument);
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}
}

void ReqInstrument::ReqQryInstrumentMarginRate(string instrument)
{
	//CThostFtdcInstrumentField *p = static_cast<CThostFtdcInstrumentField*> (query_instrument_[i]);

	CThostFtdcQryInstrumentMarginRateField Ins;
	int len = ctp_broker_id_.size();
	ctp_broker_id_.copy(Ins.BrokerID, len, 0);
	Ins.BrokerID[len] = '\0';
	len = ctp_investor_id_.size();
	ctp_investor_id_.copy(Ins.InvestorID, len, 0);
	Ins.InvestorID[len] = '\0';
	len = instrument.size();
	instrument.copy(Ins.InstrumentID,len,0);
	Ins.InstrumentID[len] = '\0';
	//Ͷ���ױ�Ins.HedgeFlag---'1'����Ͷ��;'2'��������;'3'�����ױ�;
	Ins.HedgeFlag = THOST_FTDC_HF_Hedge;
	int result=this->ctp_trade->ReqQryInstrumentMarginRate(&Ins,++trade_request_id_);
	cout << ((result == 0) ? "�ɹ�" : "ʧ��") << endl;
	/*unique_lock<mutex> query_lock(query_mutex_);
	//ReqQryInstrument();
	while (false == query_finished_)
	{
		query_cv_.wait(query_lock);
	}
	int s = query_buffer_.size();
	cout << s << endl;
	for (int i = 0; i < 60; i++)
	{
		CThostFtdcInstrumentField *p = static_cast<CThostFtdcInstrumentField*> (query_buffer_[i]);
		CThostFtdcQryInstrumentMarginRateField Ins;
		int len = ctp_broker_id_.length();
		ctp_broker_id_.copy(Ins.BrokerID, len, 0);
		len = ctp_investor_id_.length();
		ctp_investor_id_.copy(Ins.InvestorID, len, 0);
		strcpy_s(Ins.InstrumentID, p->InstrumentID);
		//Ͷ���ױ�Ins.HedgeFlag---'1'����Ͷ��;'2'��������;'3'�����ױ�;
		Ins.HedgeFlag = THOST_FTDC_HF_Hedge;
		this->ctp_trade->ReqQryInstrumentMarginRate(&Ins, ++trade_request_id_);
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}
	query_buffer_.clear();
	query_finished_ = false;
	query_lock.unlock();*/
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "��ѯ��֤���Ӧ!" << endl;
	if (pInstrumentMarginRate!=NULL && !IsErrorRspInfo(pRspInfo))
	{
		ofile3 << "��Լ����: " << pInstrumentMarginRate->InstrumentID << "\t";
		ofile3 << "Ͷ���߷�Χ: " << pInstrumentMarginRate->InvestorRange << "\t";
		ofile3 << "���͹�˾����: " << pInstrumentMarginRate->BrokerID << "\t";
		ofile3 << "Ͷ���ߴ���: " << pInstrumentMarginRate->InvestorID << "\t";
		ofile3 << "Ͷ���ױ���־: " << pInstrumentMarginRate->HedgeFlag << "\t";
		ofile3 << "��ͷ��֤����: " << pInstrumentMarginRate->LongMarginRatioByMoney << "\t";
		ofile3 << "��ͷ��֤���: " << pInstrumentMarginRate->LongMarginRatioByVolume << "\t";
		ofile3 << "��ͷ��֤����: " << pInstrumentMarginRate->ShortMarginRatioByMoney << "\t";
		ofile3 << "��ͷ��֤���: " << pInstrumentMarginRate->ShortMarginRatioByVolume << "\t";
		ofile3 << "�Ƿ���Խ�������ȡ: " << pInstrumentMarginRate->IsRelative << endl;
		if (bIsLast)
		{
			cout << "��ѯ������ļ�MarginRate.txt��" << endl;
			cout << "�����ѯ��Լ����:" << endl;
			string instrument;
			cin >> instrument;
			ReqQryInstrumentCommissionRate(instrument);
		}
	}
}

void ReqInstrument::ReqQryInstrumentCommissionRate(string instrument)
{
	CThostFtdcQryInstrumentCommissionRateField Ins;
	int len = ctp_broker_id_.size();
	ctp_broker_id_.copy(Ins.BrokerID, len, 0);
	Ins.BrokerID[len] = '\0';
	len = ctp_investor_id_.size();
	ctp_investor_id_.copy(Ins.InvestorID, len, 0);
	Ins.InvestorID[len] = '\0';
	len = instrument.size();
	instrument.copy(Ins.InstrumentID, len,0);
	Ins.InstrumentID[len] = '\0';
	//����������	ExchangeID
	int result = this->ctp_trade->ReqQryInstrumentCommissionRate(&Ins, ++trade_request_id_);
	cout << ((result == 0) ? "�ɹ�" : "ʧ��") << endl;
		//boost::this_thread::sleep(boost::posix_time::seconds(1));
	boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void ReqInstrument::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cout << "��ѯ�������ʻ�Ӧ!" << endl;
	if (pInstrumentCommissionRate!=NULL && !IsErrorRspInfo(pRspInfo))
	{
		ofile4 << "��Լ����: " << pInstrumentCommissionRate->InstrumentID << "\t";
		ofile4 << "Ͷ���߷�Χ: " << pInstrumentCommissionRate->InvestorRange << "\t";
		ofile4 << "���͹�˾����: " << pInstrumentCommissionRate->BrokerID << "\t";
		ofile4 << "Ͷ���ߴ���: " << pInstrumentCommissionRate->InvestorID << "\t";
		ofile4 << "������������: " << pInstrumentCommissionRate->OpenRatioByMoney << "\t";
		ofile4 << "����������: " << pInstrumentCommissionRate->OpenRatioByVolume << "\t";
		ofile4 << "ƽ����������: " << pInstrumentCommissionRate->CloseRatioByMoney << "\t";
		ofile4 << "ƽ��������: " << pInstrumentCommissionRate->CloseRatioByVolume << "\t";
		ofile4 << "ƽ����������: " << pInstrumentCommissionRate->CloseTodayRatioByMoney << "\t";
		ofile4 << "ƽ��������: " << pInstrumentCommissionRate->CloseTodayRatioByVolume << "\t";
		ofile4 << "����������: " << pInstrumentCommissionRate->ExchangeID << "\t";
		ofile4 << "ҵ������: " << pInstrumentCommissionRate->BizType << endl;
		if (bIsLast)
		{
			cout << "��ѯ������ļ�CommissionRate.txt��" << endl;
		}
	}
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
	ofile2.open("Product.txt");
	ofile3.open("MarginRate.txt");
	ofile4.open("CommissionRate.txt");
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
		boost::this_thread::sleep(boost::posix_time::seconds(10));
		r1.UnInitialUserApi();
	}
	ofile.close();
	return 0;
}

