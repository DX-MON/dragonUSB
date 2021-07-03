#include "usb/types.hxx"
#include "usb/core.hxx"
#include "usb/device.hxx"
#include "usb/drivers/dfu.hxx"
#include "usb/drivers/dfuTypes.hxx"

using namespace usb::core;
using namespace usb::device;
using namespace usb::types;
using namespace usb::dfu::types;
using usb::device::packet;

namespace usb::dfu
{
	static config_t config{};

	static substrate::span<zone_t> zones{};

	static_assert(sizeof(config_t) == 6);

	static void init()
	{
		config.state = dfuState_t::applicationIdle;
		config.status = dfuStatus_t::ok;
	}

	void detached(const bool state) noexcept
	{
		if (state)
			config.state = dfuState_t::dfuIdle;
		else
			config.state = dfuState_t::applicationIdle;
	}

	[[noreturn]] static void detach()
	{
		usb::core::detach();
		config.state = dfuState_t::applicationDetach;
		reboot();
	}

	static answer_t handleDownload() noexcept
	{
		return {response_t::zeroLength, nullptr, 0};
	}

	static answer_t handleDFURequest(const std::size_t interface) noexcept
	{
		const auto &requestType{packet.requestType};
		if (requestType.recipient() != setupPacket::recipient_t::interface ||
			requestType.type() != setupPacket::request_t::typeClass ||
			packet.index != interface)
			return {response_t::unhandled, nullptr, 0};

		const auto request{static_cast<types::request_t>(packet.request)};
		switch (request)
		{
			case types::request_t::detach:
				if (packet.requestType.dir() == endpointDir_t::controllerIn)
					return {response_t::stall, nullptr, 0};
				usb::device::setupCallback = detach;
				return {response_t::zeroLength, nullptr, 0};
			case types::request_t::download:
				if (packet.requestType.dir() == endpointDir_t::controllerIn)
					return {response_t::stall, nullptr, 0};
				return handleDownload();
			case types::request_t::getStatus:
				if (packet.requestType.dir() == endpointDir_t::controllerOut)
					return {response_t::stall, nullptr, 0};
				return {response_t::data, &config, sizeof(config)};
			case types::request_t::clearStatus:
				if (packet.requestType.dir() == endpointDir_t::controllerIn)
					return {response_t::stall, nullptr, 0};
				if (config.state == dfuState_t::error)
				{
					config.state = dfuState_t::dfuIdle;
					config.status = dfuStatus_t::ok;
				}
				return {response_t::zeroLength, nullptr, 0};
			case types::request_t::getState:
				if (packet.requestType.dir() == endpointDir_t::controllerOut)
					return {response_t::stall, nullptr, 0};
				return {response_t::data, &config.state, sizeof(dfuState_t)};
			case types::request_t::abort:
				if (packet.requestType.dir() == endpointDir_t::controllerIn)
					return {response_t::stall, nullptr, 0};
				config.state = dfuState_t::dfuIdle;
				return {response_t::zeroLength, nullptr, 0};
		}

		return {response_t::stall, nullptr, 0};
	}

	void registerHandlers(const substrate::span<zone_t> flashZones,
		const uint8_t interface, const uint8_t config) noexcept
	{
		init();
		zones = flashZones;
		usb::device::registerHandler(interface, config, handleDFURequest);
	}
} // namespace usb::dfu
