/*
 * UCAN.h
 *
 *  Created on: 2018年4月3日
 *      Author: Romeli
 */

#ifndef UCAN_H_
#define UCAN_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>
#include <Event/UEventPool.h>

class UCAN {
public:
	struct Data_Typedef {
		uint32_t id;
		uint8_t data[8];
		uint8_t dataSize;
	};

	voidFun ReceiveEvent;

	UCAN(UIT_Typedef& it, uint8_t rxBufSize, uint16_t idH = 0,
			uint16_t idL = 0, uint16_t maskIdH = 0, uint16_t maskIdL = 0);
	virtual ~UCAN();

	void Init();
	void Send(Data_Typedef& data);
	void Send(uint32_t id, uint8_t* data, uint8_t size);
	void Read(Data_Typedef& data);

	uint8_t Available();
	void SetEventPool(voidFun rcvEvent, UEventPool* pool);
	void IRQ();
private:
	uint16_t _idH, _idL;
	uint16_t _maskIdH, _maskIdL;

	UIT_Typedef& _it;
	UEventPool* _ePool;
	Data_Typedef* _rxBuf;
	uint8_t _rxBufSize;
	uint8_t _rxBufStart, _rxBufEnd;

	void GPIOInit();
	void NVICInit();
	void CANInit();
};

#endif /* UCAN_H_ */