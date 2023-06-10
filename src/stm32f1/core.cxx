// SPDX-License-Identifier: BSD-3-Clause
#include "usb/platform.hxx"
#include "usb/internal/core.hxx"
#include "usb/device.hxx"

/*!
 * USB pinout:
 * PA11 - D-
 * PA12 - D+
 *
 * PA8 - FS Pull-Up resistor
 * PA15 - VBus
 */

using namespace usb::constants;
using namespace usb::types;
using namespace usb::core::internal;

namespace usb::core
{
	void init() noexcept
	{
		// Enable the clocks for the USB peripheral
		rcc.apb1PeriphClockEn |= vals::rcc::apb1PeriphClockEnUSB;
		// Enable the clocks for the GPIO Port A peripheral
		rcc.apb2PeriphClockEn |= vals::rcc::apb2PeriphClockEnGPIOPortA;

		// Set up the port pins used by the USB controller so they're in the right modes
		// with the USB controller connected through.
		vals::gpio::clear(gpioA, vals::gpio_t::pin8);
		vals::gpio::config<vals::gpio_t::pin8>(gpioA, vals::gpio::mode_t::input, vals::gpio::config_t::inputFloating);

		// Having configured the pins, we now need to set up the USB controller
		// Clear the control register, releasing power down and forced reset on the controller,
		// set the buffer table pointer to the start of the USB SRAM, and clear pending interrupts.
		usbCtrl.ctrl &= vals::usb::controlMask;
		usbCtrl.bufferTablePtr = 0;
		usbCtrl.intStatus &= vals::usb::itrStatusClearMask;

		// Enable the USB NVIC slot we use
		nvic.enableInterrupt(vals::irqs::usbLowPriority);

		// Initialise the state machine
		usbState = deviceState_t::detached;
		usbCtrlState = ctrlState_t::idle;
		usbDeferalFlags = 0;
	}

	void attach() noexcept
	{
		// Reset all USB interrupts
		usbCtrl.ctrl &= vals::usb::controlItrMask;
		// And their flags
		usbCtrl.intStatus &= vals::usb::itrStatusClearMask;

		// Ensure the device address is 0
		usbCtrl.address = 0 | vals::usb::addressUSBEnable;
		// Ensure we're in the unconfigured configuration
		usb::device::activeConfig = 0;
		// Ensure we can respond to reset interrupts
		usbCtrl.ctrl |= vals::usb::controlResetItrEn;
		// Attach to the bus
		vals::gpio::set(gpioA, vals::gpio_t::pin8);
		vals::gpio::config<vals::gpio_t::pin8>(gpioA, vals::gpio::mode_t::output2MHz,
			vals::gpio::config_t::outputNormalPushPull);
	}

	void detach() noexcept
	{
		// Detach from the bus
		usbCtrl.address = 0;
		// Reset all USB interrupts
		usbCtrl.ctrl &= vals::usb::controlItrMask;
		// Ensure that the current configuration is torn down
		deinitHandlers();
		// Switch to the unconfigured configuration
		usb::device::activeConfig = 0;
	}

	void address(const uint8_t value) noexcept
	{
		usbCtrl.address = (usbCtrl.address & vals::usb::addressClrMask) |
			(value & vals::usb::addressMask);
	}

	uint8_t address() noexcept { return usbCtrl.address & vals::usb::addressMask; }
}
