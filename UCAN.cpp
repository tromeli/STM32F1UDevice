/*
 * UCAN.cpp
 *
 *  Created on: 2018年4月3日
 *      Author: Romeli
 */

#include <UCAN.h>
#include <cstring>

UCAN::UCAN(UIT_Typedef& it, uint8_t rxBufSize, uint16_t idH, uint16_t idL,
		uint16_t maskIdH, uint16_t maskIdL) :
		_it(it) {
	ReceiveEvent = nullptr;

	_idH = idH;
	_idL = idL;
	_maskIdH = maskIdH;
	_maskIdL = maskIdL;

	_ePool = nullptr;
	_rxBufSize = rxBufSize;
	_rxBuf = new Data_Typedef[_rxBufSize]();
	_rxBufStart = 0;
	_rxBufEnd = 0;
}

UCAN::~UCAN() {
	delete[] _rxBuf;
}

void UCAN::Init() {
	GPIOInit();
	NVICInit();
	CANInit();
}

void UCAN::Send(Data_Typedef& data) {
	Send(data.id, data.data, data.dataSize);
}

void UCAN::Send(uint32_t id, uint8_t* data, uint8_t size) {
	CanTxMsg msg;
	msg.StdId = id;
	msg.DLC = size;
	memcpy(msg.Data, data, msg.DLC);
	msg.IDE = 0;
	msg.RTR = 0;
	CAN_Transmit(CAN1, &msg);
}

void UCAN::Read(Data_Typedef& data) {
	data.id = _rxBuf[_rxBufStart].id;
	data.dataSize = _rxBuf[_rxBufStart].dataSize;
	memcpy(data.data, _rxBuf[_rxBufStart].data, data.dataSize);
	if (_rxBufStart != _rxBufEnd) {
		(++_rxBufStart) %= _rxBufSize;
	}
}

uint8_t UCAN::Available() {
	return uint8_t(
			_rxBufStart <= _rxBufEnd ?
					_rxBufEnd - _rxBufStart :
					_rxBufSize - _rxBufStart + _rxBufEnd);
}

void UCAN::SetEventPool(voidFun rcvEvent, UEventPool* pool) {
	ReceiveEvent = rcvEvent;
	_ePool = pool;
}

void UCAN::IRQ() {
	CanRxMsg msg;
	CAN_Receive(CAN1, CAN_FIFO0, &msg);
	_rxBuf[_rxBufEnd].id = msg.StdId;
	_rxBuf[_rxBufEnd].dataSize = msg.DLC;

	memcpy(_rxBuf[_rxBufEnd].data, msg.Data, _rxBuf[_rxBufEnd].dataSize);
	(++_rxBufEnd) %= _rxBufSize;
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0); // 清除中断

	if (ReceiveEvent != nullptr) {
		if (_ePool != nullptr) {
			_ePool->Insert(ReceiveEvent);
		} else {
			ReceiveEvent();
		}
	}
}

void UCAN::GPIOInit() {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;      //can rx  pa11
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;      //can tx  pa12
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void UCAN::NVICInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _it.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_it.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _it.SubPriority;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void UCAN::CANInit() {
	CAN_InitTypeDef CAN_InitStructure;
	CAN_FilterInitTypeDef CAN_FilterInitStructure;

	CAN_DeInit(CAN1);

	// 波特率 = Fpclk1/((tbs1+1+tbs2+1+1)*brp);
	// 125K = 36000K / 16 * (8 + 9 + 1);
	CAN_InitStructure.CAN_TTCM = DISABLE; // 非时间触发
	CAN_InitStructure.CAN_ABOM = DISABLE; // 软件自动离线管理
	CAN_InitStructure.CAN_AWUM = DISABLE; // 睡眠模式通过软件唤醒(清楚CAN->MCR的sleep位)
	CAN_InitStructure.CAN_NART = ENABLE;  // 使能报文自动重发
	CAN_InitStructure.CAN_RFLM = DISABLE; // 报文不锁定，新的覆盖旧的
	CAN_InitStructure.CAN_TXFP = DISABLE; // 优先级由报文标识符决定
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal; // 模式设置 0:普通模式, 1:换回模式
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; // 重新同步跳跃宽度
	CAN_InitStructure.CAN_BS1 = CAN_BS1_9tq; // 时间段1的时间单元
	CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq; // 时间段2的时间单元
	CAN_InitStructure.CAN_Prescaler = 2; // 分频系数
	CAN_Init(CAN1, &CAN_InitStructure);

	CAN_FilterInitStructure.CAN_FilterNumber = 0;
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh = _idH;
	CAN_FilterInitStructure.CAN_FilterIdLow = _idL;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = _maskIdH;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = _maskIdL;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}