#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod pod;
SpiHandle spi;
uint8_t counter;

int main(void)
{
	pod.Init();

	SpiHandle::Config spi_config;
	spi_config.periph = SpiHandle::Config::Peripheral::SPI_1;
	spi_config.direction = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;
	spi_config.clock_polarity = SpiHandle::Config::ClockPolarity::LOW;
	spi_config.clock_phase = SpiHandle::Config::ClockPhase::ONE_EDGE;
	spi_config.nss = SpiHandle::Config::NSS::HARD_OUTPUT;
	spi_config.baud_prescaler = SpiHandle::Config::BaudPrescaler::BAUDRATEPRESCALER_8;
	spi_config.datasize = 8;
	spi_config.mode = SpiHandle::Config::Mode::MASTER;
	
	spi_config.pin_config.sclk = {DSY_GPIOG, 11};
	spi_config.pin_config.miso = {DSY_GPIOB, 4};
	spi_config.pin_config.mosi = {DSY_GPIOB, 5};
	spi_config.pin_config.nss = {DSY_GPIOG, 10};

	spi.Init(spi_config);

	counter = 0;

	pod.StartAdc();
	while(1) {
		//onboard led on if no errors
		// hw.seed.SetLed(spi.CheckError() == HAL_SPI_ERROR_NONE);
		
		uint8_t buff[] = {counter};
		spi.BlockingTransmit(buff, 1);
		pod.DelayMs(10);
		counter++;
	}		
}