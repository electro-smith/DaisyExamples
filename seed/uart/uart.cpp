#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

#define RX
//#define TX

DaisyField hw;
UartHandler uart;

int main(void)
{
	hw.Init();

	UartHandler::Config uart_config;
	uart_config.baudrate = 31250;
	uart_config.parity = UartHandler::Config::Parity::PARITY_NONE;
	uart_config.periph = UartHandler::Config::Peripheral::USART_1;
	uart_config.stopbits = UartHandler::Config::StopBits::STOP_BITS_1;
	uart_config.pin_config.rx = {DSY_GPIOB, 7};
	uart_config.pin_config.tx = {DSY_GPIOB, 6};

	#ifdef TX
		uart_config.mode = UartHandler::Config::Mode::MODE_TX;
	#else
		uart_config.mode = UartHandler::Config::Mode::MODE_RX;
	#endif

	uart.Init(uart_config);

	hw.StartAdc();

	while(1) {
		//onboard led on if no errors
		//rx will throw overrun error since we're reading too slow, but that's OK
		hw.seed.SetLed(uart.CheckError() == HAL_UART_ERROR_NONE);

		#ifdef TX
			uint8_t buff[] = {0, 11, 27, 13, 104};
			uart.PollTx(buff, 5);
		#endif

		#ifdef RX
			if(!uart.RxActive())
			{
				uart.FlushRx();
				uart.StartRx();
			}

			else{
				while(uart.Readable()){
					hw.display.Fill(false);
					char cstr[10];
					sprintf(cstr, "rx: %d", uart.PopRx());
					hw.display.SetCursor(0,0);
					hw.display.WriteString(cstr, Font_7x10, true);
					hw.display.Update();
					hw.seed.DelayMs(300);
				}
			}
		#endif
	}
}
