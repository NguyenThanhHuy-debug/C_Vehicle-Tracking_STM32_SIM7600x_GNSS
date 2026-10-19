
#include "davitedSensor.h"

#include "ringbuffer.h"

#define RING_UART_LEN 64
#define dataLen		22
uint8_t buff_uart[RING_UART_LEN];
RingBuff ring_buff;

uint16_t uart_available()
{
	return ring_buff_available(&ring_buff);
}

void uart_init(void)
{
	ring_buff_init(&ring_buff, buff_uart, RING_UART_LEN);
}

void processingdata(uint8_t *data_rxResponse, uint8_t length, uint16_t *data_t,uint32_t *data_F, uint32_t *data_N)
{
	// declare variable processing UART data
	uint8_t rxBuffer[dataLen];
	uint8_t data_Dec[dataLen];
	int DataPosision = -1;
	for(int i = 0; i < length ; i++)
	{
		ring_buff_push(&ring_buff, data_rxResponse[i]);
		rxBuffer[i] = ring_buff_pop(&ring_buff);
		if(rxBuffer[i] == 70)
		{
			DataPosision = i;
		}
	}
	if(DataPosision != -1)
	{
		int cnt = 0;
		while(cnt < length)
		{
			data_Dec[cnt] = rxBuffer[DataPosision];
			++ DataPosision;
			if(DataPosision == 22) DataPosision = 0;
			++ cnt;
		}

	}
	*data_F = ((data_Dec[2]-'0')*1000)+ ((data_Dec[3]-'0')*100)+ ((data_Dec[4]-'0')*10)+ (data_Dec[5]-'0');
	*data_N = ((data_Dec[14]-'0')*1000)+ ((data_Dec[15]-'0')*100)+ ((data_Dec[16]-'0')*10)+ (data_Dec[17]-'0');
	*data_t = ((data_Dec[9]-'0')*10)+ (data_Dec[10]-'0');
}


