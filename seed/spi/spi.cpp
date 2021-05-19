#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField hw;
SpiHandle spi;
uint8_t counter;

int main(void)
{
	hw.Init();

	SpiHandle::Config spi_config;
	spi_config.periph = SpiHandle::Config::Peripheral::SPI_1;
	spi_config.mode = SpiHandle::Config::Mode::MASTER;
	spi_config.direction = SpiHandle::Config::Direction::TWO_LINES;
	spi_config.clock_polarity = SpiHandle::Config::ClockPolarity::HIGH;
	spi_config.clock_phase = SpiHandle::Config::ClockPhase::ONE_EDGE;
	spi_config.nss = SpiHandle::Config::NSS::HARD_INPUT;
	spi_config.baud_prescaler = SpiHandle::Config::BaudPrescaler::BAUDRATEPRESCALER_64;
	spi_config.datasize = 8;

	spi_config.pin_config.sclk = {DSY_GPIOA, 5};
	spi_config.pin_config.miso = {DSY_GPIOA, 6};
	spi_config.pin_config.mosi = {DSY_GPIOA, 7};
	spi_config.pin_config.nss = {DSY_GPIOA, 4};

	spi.Init(spi_config);

	counter = 0;

	hw.StartAdc();
	while(1) {
		//onboard led on if no errors
		// hw.seed.SetLed(spi.CheckError() == HAL_SPI_ERROR_NONE);
		
		//transmit
		uint8_t buff[] = {counter};
		spi.BlockingTransmit(buff, 1);
		counter++;
		hw.DelayMs(10);

		//receive
		uint8_t rxbuff;
		spi.PollReceive(&rxbuff, 1, 10);

		//oled
		hw.display.Fill(false);				

		char cstr[10];
		sprintf(cstr, "rx: %d", rxbuff);
			
		hw.display.SetCursor(0,0);
		hw.display.WriteString(cstr, Font_7x10, true);
			
		hw.display.Update();
	}		
}