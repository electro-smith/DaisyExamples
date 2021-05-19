#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField field;
SpiHandle spi;
uint8_t counter;

int main(void)
{
	field.Init();

	SpiHandle::Config spi_config;
	spi_config.periph = SpiHandle::Config::Peripheral::SPI_1;
	spi_config.direction = SpiHandle::Config::Direction::TWO_LINES;
	spi_config.clock_polarity = SpiHandle::Config::ClockPolarity::HIGH;
	spi_config.clock_phase = SpiHandle::Config::ClockPhase::ONE_EDGE;
	spi_config.nss = SpiHandle::Config::NSS::HARD_INPUT;
	spi_config.baud_prescaler = SpiHandle::Config::BaudPrescaler::BAUDRATEPRESCALER_64;
	spi_config.datasize = 8;
	spi_config.mode = SpiHandle::Config::Mode::SLAVE;
	
	spi_config.pin_config.sclk = {DSY_GPIOA, 5};
	spi_config.pin_config.miso = {DSY_GPIOA, 6};
	spi_config.pin_config.mosi = {DSY_GPIOA, 7};
	spi_config.pin_config.nss = {DSY_GPIOA, 4};

	spi.Init(spi_config);

	counter = 0;

	field.StartAdc();
	while(1) {
		//onboard led on if no errors
		// field.seed.SetLed(spi.CheckError() == HAL_SPI_ERROR_NONE);
		
		uint8_t rxbuff;
		spi.PollReceive(&rxbuff, 1, 10);

		//oled
		field.display.Fill(false);				

		char cstr[10];
		sprintf(cstr, "rx: %d", rxbuff);
			
		field.display.SetCursor(0,0);
		field.display.WriteString(cstr, Font_7x10, true);
			
		field.display.Update();
	}		
}