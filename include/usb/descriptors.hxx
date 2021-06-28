// SPDX-License-Identifier: BSD-3-Clause
#ifndef USB_DESCRIPTORS___HXX
#define USB_DESCRIPTORS___HXX

#include <cstdint>
#include <cstddef>
#include <string_view>
#include "usb/constants.hxx"
#include "usb/types.hxx"

namespace usb::descriptors
{
	enum class usbDescriptor_t : uint8_t
	{
		invalid = 0x00U,
		device = 0x01U,
		configuration = 0x02U,
		string = 0x03U,
		interface = 0x04U,
		endpoint = 0x05U,
		deviceQualifier = 0x06U,
		otherSpeed = 0x07U,
		interfacePower = 0x08U, // Speed, Power?
		otg = 0x09U,
		debug = 0x0AU,
		interfaceAssociation = 0x0BU,
		security = 0x0CU,
		key = 0x0DU,
		encryptionType = 0x0EU,
		deviceCapability = 0x10U,
		wirelessEndpoint = 0x11U,
		hid = 0x21U,
		report = 0x22U,
		physicalDesc = 0x23U
	};

	enum class usbClass_t : uint8_t
	{
		none = 0x00U,
		audio = 0x01U,
		cdcACM = 0x02U,
		hid = 0x03U,
		physical = 0x05U,
		image = 0x06U,
		printer = 0x07U,
		massStorage = 0x08U,
		hub = 0x09U,
		cdcData = 0x0AU,
		smartCard = 0x0BU,
		contentSecurity = 0x0DU,
		video = 0x0EU,
		healthcare = 0x0FU,
		audioVisual = 0x10U,
		billboard = 0x11U,
		typeCBridge = 0x12U,
		diagnostic = 0xDCU,
		wireless = 0xE0U,
		misc = 0xEFU,
		application = 0xFEU,
		vendor = 0xFFU
	};

	struct usbDeviceDescriptor_t
	{
		uint8_t length;
		usbDescriptor_t descriptorType;
		uint16_t usbVersion;
		usbClass_t deviceClass;
		uint8_t deviceSubClass;
		uint8_t deviceProtocol;
		uint8_t maxPacketSize0;
		uint16_t vendorID;
		uint16_t productID;
		uint16_t deviceVersion;
		uint8_t strMfrIndex;
		uint8_t strProductIndex;
		uint8_t strSerialNoIndex;
		uint8_t numConfigurations;
	};

	enum class usbConfigAttr_t : uint8_t
	{
		defaults = 0x80U,
		selfPowered = 0x40U,
		remoteWakeup = 0x20U,
		hostNegotiationProto = 0x02U,
		sessionRequestProto = 0x01U
	};

	struct [[gnu::packed]] usbConfigDescriptor_t
	{
		uint8_t length;
		usbDescriptor_t descriptorType;
		uint16_t totalLength;
		uint8_t numInterfaces;
		uint8_t configurationValue;
		uint8_t strConfigurationIndex;
		usbConfigAttr_t attributes;
		uint8_t maxPower;
	};

	struct usbInterfaceDescriptor_t
	{
		uint8_t length;
		usbDescriptor_t descriptorType;
		uint8_t interfaceNumber;
		uint8_t alternateSetting;
		uint8_t numEndpoints;
		usbClass_t interfaceClass;
		uint8_t interfaceSubClass;
		uint8_t interfaceProtocol;
		uint8_t strInterfaceIdx;
	};

	enum class usbEndpointType_t : uint8_t
	{
		control = 0,
		isochronous = 1,
		bulk = 2,
		interrupt = 3
	};

	using usbEndpointDir_t = usb::types::endpointDir_t;

	constexpr static const uint8_t endpointDirMask{0x7F};
	constexpr inline uint8_t endpointAddress(const usbEndpointDir_t dir, const uint8_t number) noexcept
		{ return uint8_t(dir) | (number & endpointDirMask); }

	struct [[gnu::packed]] usbEndpointDescriptor_t
	{
		uint8_t length;
		usbDescriptor_t descriptorType;
		uint8_t endpointAddress;
		usbEndpointType_t endpointType;
		uint16_t maxPacketSize;
		uint8_t interval;
	};

	namespace subclasses
	{
		enum class device_t : uint8_t
		{
			none = 0
		};

		enum class hid_t : uint8_t
		{
			none = 0,
			bootInterface = 1
		};

		enum class application_t : uint8_t
		{
			dfu = 1
		};

		enum class vendor_t : uint8_t
		{
			none = 0
		};
	} // namespace subclasses

	namespace protocols
	{
		enum class device_t : uint8_t
		{
			none = 0
		};

		enum class hid_t : uint8_t
		{
			none = 0,
			keyboard = 1,
			mouse = 2
		};

		enum class application_t : uint8_t
		{
			runtime = 1
		};

		enum class vendor_t : uint8_t
		{
			none = 0,
			flashprog = 1
		};
	} // namespace protocols

	namespace hid
	{
		enum class countryCode_t : uint8_t
		{
			notSupported = 0,
			arabic = 1,
			belgian = 2,
			canadianBi = 3,
			canadianFrench = 4,
			czech = 5,
			danish = 6,
			finnish = 7,
			french = 8,
			german = 9,
			greek = 10,
			hebrew = 11,
			hungary = 12,
			iso = 13,
			italian = 14,
			japanese = 15,
			korean = 16,
			latinAmerican = 17,
			dutch = 18,
			norwegian = 19,
			persian = 20,
			polish = 21,
			portuguese = 22,
			russian = 23,
			slovak = 24,
			spanish = 25,
			swissFrench = 27,
			swissGerman = 28,
			swiss = 29,
			taiwanese = 30,
			turkishQ = 31,
			english = 32,
			american = 33,
			balkan = 34,
			turkishF = 35
		};

		struct hidDescriptor_t
		{
			uint8_t length;
			usbDescriptor_t descriptorType;
			uint16_t hidVersion;
			countryCode_t countryCode;
			uint8_t numDescriptors;
		};

		struct reportDescriptor_t
		{
			usbDescriptor_t descriptorType;
			uint16_t length;
		};
	} // namespace hid

	namespace dfu
	{
		enum class descriptor_t : uint8_t
		{
			functional = 0x21U
		};

		enum class willDetach_t : uint8_t
		{
			no = 0x00U,
			yes = 0x08U
		};

		enum class manifestationTolerant_t : uint8_t
		{
			no = 0x00U,
			yes = 0x04U
		};

		enum class canUpload_t : uint8_t
		{
			no = 0x00U,
			yes = 0x02U
		};

		enum class canDownload_t : uint8_t
		{
			no = 0x00U,
			yes = 0x01U
		};

		struct attributes_t final
		{
		private:
			uint8_t value;

		public:
			constexpr attributes_t(const willDetach_t willDetach, const manifestationTolerant_t manifTolerant,
				const canUpload_t canUpload, const canDownload_t canDownload) noexcept :
					value
					{
						uint8_t(uint8_t(willDetach) | uint8_t(manifTolerant) |
						uint8_t(canUpload) | uint8_t(canDownload))
					} { }

			[[nodiscard]] constexpr willDetach_t willDetach() const noexcept
				{ return static_cast<willDetach_t>(value & uint8_t(willDetach_t::yes)); }

			[[nodiscard]] constexpr manifestationTolerant_t manifestationTolerant() const noexcept
				{ return static_cast<manifestationTolerant_t>(value & uint8_t(manifestationTolerant_t::yes)); }

			[[nodiscard]] constexpr canUpload_t canUpload() const noexcept
				{ return static_cast<canUpload_t>(value & uint8_t(canUpload_t::yes)); }

			[[nodiscard]] constexpr canDownload_t canDownload() const noexcept
				{ return static_cast<canDownload_t>(value & uint8_t(canDownload_t::yes)); }
		};

		struct [[gnu::packed]] functionalDescriptor_t
		{
			uint8_t length;
			descriptor_t descriptorType;
			attributes_t attributes;
			uint16_t detachTimeout;
			uint16_t transferSize;
			uint16_t dfuVersion;
		};
	} // namespace dfu

	struct usbMultiPartDesc_t
	{
		uint8_t length;
		const void *descriptor;
	};

	struct usbMultiPartTable_t
	{
	private:
		const usbMultiPartDesc_t *_begin;
		const usbMultiPartDesc_t *_end;

	public:
		constexpr usbMultiPartTable_t(const usbMultiPartDesc_t *const begin,
			const usbMultiPartDesc_t *const end) noexcept : _begin{begin}, _end{end} { }
		[[nodiscard]] constexpr auto begin() const noexcept { return _begin; }
		[[nodiscard]] constexpr auto end() const noexcept { return _end; }
		[[nodiscard]] constexpr auto count() const noexcept { return _end - _begin; }

		[[nodiscard]] constexpr auto &part(const std::size_t index) const noexcept
		{
			if (_begin + index >= _end)
				return *_end;
			return _begin[index];
		}
		constexpr auto &operator [](const std::size_t index) const noexcept { return part(index); }

		[[nodiscard]] constexpr auto totalLength() const noexcept
		{
			// TODO: Convert to std::accumulate() later.
			std::size_t count{};
			for (const auto &descriptor : *this)
				count += descriptor.length;
			return count;
		}
	};

	struct [[gnu::packed]] usbStringDesc_t
	{
		uint8_t length;
		usbDescriptor_t descriptorType;
		const char16_t *const string;

		constexpr usbStringDesc_t(const std::u16string_view data) noexcept :
			length{uint8_t(baseLength() + (data.length() * 2))},
			descriptorType{usbDescriptor_t::string}, string{data.data()} { }

		[[nodiscard]] constexpr static uint8_t baseLength() noexcept
			{ return sizeof(usbStringDesc_t) - sizeof(char16_t *); }
		[[nodiscard]] constexpr uint8_t stringLength() const noexcept
			{ return length - baseLength(); }

		[[nodiscard]] constexpr auto asParts() const noexcept
		{
			const std::array<usbMultiPartDesc_t, 2> parts
			{{
				{
					baseLength(),
					this
				},
				{
					stringLength(),
					string
				}
			}};
			return parts;
		}
	};

	extern const std::array<usbMultiPartTable_t, usb::constants::configsCount> usbConfigDescriptors;
} // namespace usb::descriptors

#endif /*USB_DESCRIPTORS___HXX*/
