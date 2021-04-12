#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
UartHandler uart;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();

	UartHandler::Config uart_config;
	uart_config.baudrate = 31250;
	uart_config.mode = UartHandler::Config::Mode::MODE_TX_RX;
	uart_config.parity = UartHandler::Config::Parity::PARITY_EVEN;
	uart_config.periph = UartHandler::Config::Peripheral::USART_1;
	uart_config.stopbits = UartHandler::Config::StopBits::STOP_BITS_1;
	uart_config.pin_config.rx = {DSY_GPIOB, 7};
	uart_config.pin_config.tx = {DSY_GPIOB, 6};

	uart.Init(uart_config);
	uart.StartRx();

	uint8_t buff[] = {0,12,37,49};

	hw.StartAudio(AudioCallback);
	while(1) {
		//onboard led on if no errors
		hw.SetLed(uart.CheckError() == HAL_UART_ERROR_NONE);
		uart.PollTx(buff, 4);
	}
}
