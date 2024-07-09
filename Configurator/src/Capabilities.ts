const appleModeCapabilities = [
	[ // APPLE_MODE_UNKNOWN
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
	],
	[ // APPLE_MODE_BASE
		[1, 0, 0, 0, 0, 0], //PTP
		[0, 1, 0, 0, 0, 0], //Audio
		[1, 0, 1, 0, 0, 0], //PTP + USBMUX
		[1, 0, 1, 0, 1, 0], //PTP + USBMUX + TETHER
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
	],
	[ // APPLE_MODE_BASE_VALERIA
		[1, 0, 0, 0, 0, 0], //PTP
		[0, 1, 0, 0, 0, 0], //Audio
		[1, 0, 1, 0, 0, 0], //PTP + USBMUX
		[1, 0, 1, 0, 1, 0], //PTP + USBMUX + TETHER
		[1, 0, 1, 0, 0, 1], //PTP + USBMUX + VALERIA
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
	],
	[ // APPLE_MODE_BASE_NETWORK
		[1, 0, 0, 0, 0, 0], //PTP
		[0, 1, 0, 0, 0, 0], //Audio
		[1, 0, 1, 0, 0, 0], //PTP + USBMUX
		[1, 0, 1, 0, 1, 0], //PTP + USBMUX + TETHER
		[1, 0, 1, 1, 0, 0], //PTP + USBMUX + NETWORK
		[0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0],
	],
	[ // APPLE_MODE_BASE_NETWORK_TETHER
		[1, 0, 0, 0, 0, 0], //PTP
		[0, 1, 0, 0, 0, 0], //Audio
		[1, 0, 1, 0, 0, 0], //PTP + USBMUX
		[1, 0, 1, 0, 1, 0], //PTP + USBMUX + TETHER
		[1, 0, 1, 1, 0, 0], //PTP + USBMUX + NETWORK
		[1, 0, 1, 1, 1, 0], //PTP + USBMUX + NETWORK + TETHER
		[0, 0, 0, 0, 0, 0],
	],
	[ // APPLE_MODE_BASE_NETWORK_VALERIA
		[1, 0, 0, 0, 0, 0], //PTP
		[0, 1, 0, 0, 0, 0], //Audio
		[1, 0, 1, 0, 0, 0], //PTP + USBMUX
		[1, 0, 1, 0, 1, 0], //PTP + USBMUX + TETHER
		[1, 0, 1, 1, 0, 0], //PTP + USBMUX + NETWORK
		[1, 0, 1, 0, 0, 1], //PTP + USBMUX + VALERIA
		[0, 0, 0, 0, 0, 0],
	],
	[ // APPLE_MODE_BASE_NETWORK_TETHER_VALERIA
		[1, 0, 0, 0, 0, 0], //PTP
		[0, 1, 0, 0, 0, 0], //Audio
		[1, 0, 1, 0, 0, 0], //PTP + USBMUX
		[1, 0, 1, 0, 1, 0], //PTP + USBMUX + TETHER
		[1, 0, 1, 1, 0, 0], //PTP + USBMUX + NETWORK
		[1, 0, 1, 1, 1, 0], //PTP + USBMUX + NETWORK + TETHER
		[1, 0, 1, 0, 0, 1], //PTP + USBMUX + VALERIA
	],
];

export default appleModeCapabilities;
