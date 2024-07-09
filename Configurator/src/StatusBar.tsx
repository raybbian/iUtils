import { StyleProp, ViewStyle, View, Text, StyleSheet, PlatformColor, useWindowDimensions } from "react-native";
// @ts-ignore: no ts definitions
import Icon from 'react-native-vector-icons/AntDesign';

export type statusType = 'success' | 'attention' | 'warning' | 'error' | 'neutral';
export type status = { message: string, category: statusType };

export default function StatusBar({ status, style }: {
  status: status;
  style?: StyleProp<ViewStyle>;
}) {
  let statusBg: StyleProp<ViewStyle>;
  let statusBadgeBg: StyleProp<ViewStyle>;
  let iconString: string;
  switch (status.category) {
    case "success":
      statusBg = styles.successBg;
      statusBadgeBg = styles.successBadgeBg;
      iconString = "checkcircle";
      break;
    case "attention":
      statusBg = styles.attentionBg;
      statusBadgeBg = styles.attentionBadgeBg;
      iconString = 'infocirlce'
      break;
    case "warning":
      statusBg = styles.warningBg;
      statusBadgeBg = styles.warningBadgeBg;
      iconString = "exclamationcircle"
      break;
    case "error":
      statusBg = styles.errorBg;
      statusBadgeBg = styles.errorBadgeBg;
      iconString = "closecircle";
      break;
    case "neutral":
      statusBg = styles.neutralBg;
      statusBadgeBg = styles.neutralBadgeBg;
      iconString = "pluscircle";
      break;
  }

  return (
    <View style={[style, styles.statusBar, statusBg]}>
      <View style={[styles.statusMessage]}>
        <View style={styles.statusIcon}>
          <Icon name={iconString} size={20} color={statusBadgeBg.backgroundColor} />
        </View>
        <Text style={styles.statusText}>{status.message}</Text>
      </View>
    </View>
  )
}

const styles = StyleSheet.create({
  statusText: {
    flexGrow: 0,
    fontSize: 13
  },
  statusIcon: {
    width: 32
  },
  statusMessage: {
    flexDirection: "row",
    alignItems: 'center',
    paddingHorizontal: 16
  },
  statusBar: {
    borderRadius: 4,
    borderTopLeftRadius: 0,
    borderTopRightRadius: 0,
    flexDirection: "column",
    justifyContent: 'center'
  },
  successBg: {
    backgroundColor: PlatformColor("SystemFillColorSuccessBackgroundBrush")
  },
  warningBg: {
    backgroundColor: PlatformColor("SystemFillColorCautionBackgroundBrush")
  },
  errorBg: {
    backgroundColor: PlatformColor("SystemFillColorCriticalBackgroundBrush")
  },
  attentionBg: {
    backgroundColor: PlatformColor("SystemFillColorAttentionBackgroundBrush")
  },
  neutralBg: {
    backgroundColor: PlatformColor("SystemFillColorNeutralBackgroundBrush")
  },
  successBadgeBg: {
    backgroundColor: PlatformColor("SystemFillColorSuccessBrush")
  },
  warningBadgeBg: {
    backgroundColor: PlatformColor("SystemFillColorCautionBrush")
  },
  errorBadgeBg: {
    backgroundColor: PlatformColor("SystemFillColorCriticalBrush")
  },
  attentionBadgeBg: {
    backgroundColor: PlatformColor("SystemFillColorAttentionBrush")
  },
  neutralBadgeBg: {
    backgroundColor: PlatformColor("SystemFillColorNeutralBrush")
  },
})
