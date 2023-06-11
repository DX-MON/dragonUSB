// SPDX-License-Identifier: BSD-3-Clause
#include "usb/platform.hxx"
#include "usb/internal/core.hxx"
#include "usb/platforms/stm32f1/core.hxx"
#include "usb/device.hxx"
#include <substrate/index_sequence>

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
	namespace internal
	{
		auto &epBufferCtrlFor(const uint8_t endpoint)
		{
			auto &epTable{*reinterpret_cast<stm32::usbEPTable_t *>(stm32::packetBufferBase +
				static_cast<uintptr_t>(usbCtrl.bufferTablePtr))};
			return epTable[endpoint];
		}

		volatile uint16_t *epBufferPtr(const uint32_t address)
			{ return reinterpret_cast<volatile uint16_t *>(stm32::packetBufferBase + address); }
	} // namespace internal

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

	void reset() noexcept
	{
		// Set up only EP0.
		resetEPs(epReset_t::all);
		// Because of the 512 byte memory limit of the peripheral buffers,
		// we overlay the TX and RX buffers on each other for EP0.
		internal::setupEndpoint(vals::usb::endpoint(vals::usb::endpointDir_t::controllerOut, 0),
			usbEndpointType_t::control, 0, 64);
		internal::setupEndpoint(vals::usb::endpoint(vals::usb::endpointDir_t::controllerIn, 0),
			usbEndpointType_t::control, 0, 64);
		// Enable the endpoint for receiving SETUP packets
		usbCtrl.epCtrlStat[0] = (usbCtrl.epCtrlStat[0] & vals::usb::epCtrlRXMask) | vals::usb::epCtrlRXValid;

		// Once we get done, idle the peripheral
		usbCtrl.address = 0;
		usbState = deviceState_t::attached;
		usbCtrl.ctrl |= vals::usb::controlSOFItrEn | vals::usb::controlCorrectXferItrEn |
			vals::usb::controlWakeupItrEn;
		usb::device::activeConfig = 0;
	}

	void resetEPs(const epReset_t what) noexcept
	{
		for (const auto endpoint : substrate::indexSequence_t{vals::usb::endpoints})
		{
			if (what == epReset_t::user && endpoint == 0)
				continue;
			usbCtrl.epCtrlStat[endpoint] &= vals::usb::epClearMask;
			usbCtrl.epCtrlStat[endpoint] |= vals::usb::epAddress(endpoint);
		}
		usb::core::common::resetEPs(what);
	}

	namespace internal
	{
		void setupEndpoint(const uint8_t endpoint, const usbEndpointType_t type, const uint16_t bufferAddress,
			const uint16_t bufferLength) noexcept
		{
			const auto direction{static_cast<endpointDir_t>(endpoint & ~vals::usb::endpointDirMask)};
			const auto endpointNumber{uint8_t(endpoint & vals::usb::endpointDirMask)};

			auto epCtrl{usbCtrl.epCtrlStat[endpointNumber]};
			auto &epBufferCtrl{internal::epBufferCtrlFor(endpointNumber)};

			// NB: we assume both IN and OUT endpoints have a consistent type here as there are only 8 endpoint
			// registers and the types are shared between the two halves
			epCtrl &= vals::usb::epCtrlTypeMask;
			epCtrl |= [&]()
			{
				switch (type)
				{
					case usbEndpointType_t::control:
						return vals::usb::epCtrlTypeControl;
					case usbEndpointType_t::bulk:
						return vals::usb::epCtrlTypeBulk;
					case usbEndpointType_t::interrupt:
						return vals::usb::epCtrlTypeInterrupt;
					case usbEndpointType_t::isochronous:
						return vals::usb::epCtrlTypeIsochronous;
				}
				// This should never bit hit.. but.. just in case.
				return vals::usb::epCtrlTypeBulk;
			}();

			if (direction == endpointDir_t::controllerIn)
			{
				epCtrl &= vals::usb::epCtrlTXMask;
				epCtrl |= vals::usb::epCtrlTXNack;
				epBufferCtrl.txAddress = sizeof(stm32::usbEPTable_t) + bufferAddress;
			}
			else
			{
				epCtrl &= vals::usb::epCtrlRXMask;
				epCtrl |= vals::usb::epCtrlRXNack;
				epBufferCtrl.rxAddress = sizeof(stm32::usbEPTable_t) + bufferAddress;
				epBufferCtrl.rxCount = vals::usb::rxBufferSize(bufferLength);
			}

			usbCtrl.epCtrlStat[endpointNumber] = epCtrl;
		}
	} // namespace internal

	void wakeup() noexcept
	{
		usbSuspended = false;
		// Clear forced suspend (low-power mode is already cancelled at this point by hardware)
		usbCtrl.ctrl &= ~vals::usb::controlForceSuspend;
		// Switch over the interrupt source being used
		usbCtrl.ctrl = (usbCtrl.ctrl & ~vals::usb::controlWakeupItrEn) | vals::usb::controlSuspendItrEn;
	}

	void suspend() noexcept
	{
		// Suspend the controller, setting it in low power mode
		usbCtrl.ctrl |= vals::usb::controlForceSuspend | vals::usb::controlLowPowerMode;
		// Switch over the interrupt source being used
		usbCtrl.ctrl = (usbCtrl.ctrl & ~vals::usb::controlSuspendItrEn) | vals::usb::controlWakeupItrEn;
		usbSuspended = true;
	}

	uint8_t *recvData(volatile const uint16_t *const usbBuffer, uint8_t *const buffer, const uint16_t length) noexcept
	{
		for (uint16_t offset = 0; offset < length; offset += 2U)
		{
			// Because of how the packet buffer is laid out on the CPU side of the bus, and
			// because of how the packet buffer is laid out to the USB core, only every other
			// uint16_t maps to a packet buffer entry.
			// That is, for every uint32_t visible from the CPU, we access just one uint16_t
			// of the packet buffer. The USB core writes by uint16_t.
			const auto data{usbBuffer[offset]};
			const auto amount{std::min<uint16_t>(2U, length - offset)};
			std::memcpy(buffer + offset, &data, amount);
		}
		return buffer + length;
	}

	/*!
	* @returns true when the all the data to be read has been retreived,
	* false if there is more left to fetch.
	*/
	bool readEP(const uint8_t endpoint) noexcept
	{
		auto &epStatus{epStatusControllerOut[endpoint]};
		auto &epBufferCtrl{internal::epBufferCtrlFor(endpoint)};
		auto readCount{static_cast<uint16_t>(epBufferCtrl.rxCount & vals::usb::rxCountByteMask)};
		// Bounds sanity and then adjust how much is left to transfer
		if (readCount > epStatus.transferCount)
			readCount = epStatus.transferCount;
		epStatus.transferCount -= readCount;
		// Grab the data associated with this transfer
		epStatus.memBuffer = recvData(internal::epBufferPtr(epBufferCtrl.rxAddress),
			static_cast<uint8_t *>(epStatus.memBuffer), uint16_t(readCount));
		return !epStatus.transferCount;
	}

	void handleIRQ() noexcept
	{
		const auto status{usbCtrl.intStatus & vals::usb::itrStatusMask};
		usbCtrl.intStatus &= vals::usb::itrStatusClearMask;

		if (usbState == deviceState_t::attached)
		{
			usbCtrl.ctrl |= vals::usb::controlSuspendItrEn;
			usbState = deviceState_t::powered;
		}

		if (status & vals::usb::itrStatusWakeup)
			wakeup();
		else if (usbSuspended)
			return;

		if (status & vals::usb::itrStatusReset)
		{
			reset();
			usbState = deviceState_t::waiting;
			return;
		}

		if (status & vals::usb::itrStatusSuspend)
			suspend();

		if (usbState == deviceState_t::detached ||
			usbState == deviceState_t::attached ||
			usbState == deviceState_t::powered)
			return;
	}
}
