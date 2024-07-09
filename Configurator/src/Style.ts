'use strict';
import { StyleSheet, PlatformColor } from 'react-native';

export default StyleSheet.create({
	expand: {
		width: "100%",
		height: "100%"
	},
	hide: {
		opacity: 0,
	},
	buttonBg: {
		backgroundColor: PlatformColor("ControlFillColorDefaultBrush"),
	},
	buttonBgHover: {
		backgroundColor: PlatformColor("ControlFillColorSecondaryBrush"),
	},
	buttonBgPress: {
		backgroundColor: PlatformColor("ControlFillColorTertiaryBrush"),
	},
	buttonBgActive: {
		backgroundColor: PlatformColor("SystemAccentColor")
	},
	buttonBgActivePress: {
		backgroundColor: PlatformColor("SystemAccentColorDark1")
	},
	buttonBgDisabled: {
		backgroundColor: PlatformColor("ControlAltFillColorSecondaryBrush")
	}
});
