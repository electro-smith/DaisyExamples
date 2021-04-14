#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField hw;
UartHandler uart;
uint8_t counter;

int main(void)
{
	hw.Init();

	UartHandler::Config uart_config;
	uart_config.baudrate = 31250;
	uart_config.parity = UartHandler::Config::Parity::NONE;
	uart_config.periph = UartHandler::Config::Peripheral::USART_1;
	uart_config.stopbits = UartHandler::Config::StopBits::BITS_1;
	uart_config.pin_config.rx = {DSY_GPIOB, 7};
	uart_config.pin_config.tx = {DSY_GPIOB, 6};
	uart_config.mode = UartHandler::Config::Mode::TX_RX;
	uart_config.wordlength = UartHandler::Config::WordLength::BITS_8;

	uart.Init(uart_config);

	counter = 0;

	hw.StartAdc();
	while(1) {
		//onboard led on if no errors
		hw.seed.SetLed(uart.CheckError() == HAL_UART_ERROR_NONE);

		//transmit
		uint8_t buff[] = {counter};
		uart.PollTx(buff, 1);
		counter++;
		hw.seed.DelayMs(300);

		//receive
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
			}
		}		
	}
}
