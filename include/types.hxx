// SPDX-License-Identifier: BSD-3-Clause
#ifndef USB_TYPES___HXX
#define USB_TYPES___HXX

#include <cstdint>
#include <tuple>

namespace usb::descriptors { struct usbMultiPartTable_t; }

namespace usb::types
{
	// Reserve space for EP0 In + Out, and EP1 In + Out.
	constexpr static const uint8_t endpointCount{2};
	constexpr static const uint8_t configsCount{1};
	constexpr static const uint8_t epBufferSize{64};

	constexpr static const uint16_t vid{0x1209};
	constexpr static const uint16_t pid{0xAB0C};

	constexpr static const uint8_t interfaceDescriptorCount{1};
	constexpr static const uint8_t endpointDescriptorCount{2};
	constexpr static const uint8_t stringCount{4};

	enum class ctrlState_t
	{
		idle,
		wait,
		dataTX,
		dataRX,
		statusTX,
		statusRX
	};

	enum class deviceState_t
	{
		detached,
		attached,
		powered,
		waiting,
		addressing,
		addressed,
		configured
	};

	enum class response_t
	{
		data,
		zeroLength,
		unhandled,
		stall
	};

	enum class endpointDir_t : uint8_t
	{
		controllerOut = 0x00U,
		controllerIn = 0x80U
	};

	struct usbEP_t final
	{
	private:
		uint8_t value{};

	public:
		usbEP_t() = default;

		void endpoint(const uint8_t num) noexcept
		{
			value &= 0xF0U;
			value |= uint8_t(num & 0x0FU);
		}

		[[nodiscard]] uint8_t endpoint() const noexcept { return value & 0x0FU; }

		void dir(const endpointDir_t dir) noexcept
		{
			value &= 0x7FU;
			value |= uint8_t(dir);
		}

		[[nodiscard]] endpointDir_t dir() const noexcept { return static_cast<endpointDir_t>(value & 0x80U); }
	};

	template<typename buffer_t> struct usbEPStatus_t final
	{
	private:
		uint8_t value{};

	public:
		buffer_t *memBuffer{nullptr};
		usbEP_t ctrl{};
		uint16_t transferCount{};
		// Multi-part fields
		uint8_t partNumber{};
		const usb::descriptors::usbMultiPartTable_t *partsData{nullptr};

		usbEPStatus_t() = default;

		void transferTerminated(const bool terminated) noexcept
		{
			value &= 0xFEU;
			value |= terminated ? 0x01U : 0x00U;
		}

		[[nodiscard]] bool transferTerminated() const noexcept { return value & 0x01U; }

		void needsArming(const bool needed) noexcept
		{
			value &= 0xFDU;
			value |= uint8_t(needed ? 0x02U : 0x00U);
		}

		[[nodiscard]] bool needsArming() const noexcept { return value & 0x02U; }

		void stall(const bool needed) noexcept
		{
			value &= 0xFBU;
			value |= uint8_t(needed ? 0x04U : 0x00U);
		}

		[[nodiscard]] bool stall() const noexcept { return value & 0x04U; }

		void isMultiPart(const bool multiPart) noexcept
		{
			value &= 0xF7U;
			value |= uint8_t(multiPart ? 0x08U : 0x00U);
		}

		[[nodiscard]] bool isMultiPart() const noexcept { return value & 0x08U; }
		void resetStatus() noexcept { value = 0; }
	};

	using answer_t = std::tuple<response_t, const void *, std::uint16_t>;
} // namespace usb::types

#endif /*USB_TYPES___HXX*/
