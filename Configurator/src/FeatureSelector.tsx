import { View, StyleProp, ViewStyle, StyleSheet } from 'react-native';
import FeatureButton from './FeatureButton';
import { Dispatch, SetStateAction } from 'react';
import { status } from './StatusBar'
import appleModeCapabilities from './Capabilities';

export default function FeatureSelector({ style, appReady, setStatus, config, appleMode }: {
  style?: StyleProp<ViewStyle>;
  appReady: boolean;
  setStatus: Dispatch<SetStateAction<status>>;
  config: number;
  appleMode: number;
}) {
  const featureCapabilities = appleModeCapabilities[appleMode][config - 1];

  return (
    <View style={[style, styles.featureCard]}>
      <View style={styles.featureRow}>
        <FeatureButton
          label='Photo Transfer'
          iconString='image'
          featureInd={0}
          appReady={appReady}
          config={config}
          setStatus={setStatus}
          enabled={Boolean(featureCapabilities[0])}
        />
        <FeatureButton
          label='iTunes Audio'
          iconString='music'
          featureInd={1}
          appReady={appReady}
          config={config}
          setStatus={setStatus}
          enabled={Boolean(featureCapabilities[1])}
        />
      </View>
      <View style={styles.featureRow}>
        <FeatureButton
          label='iTunes USB'
          iconString='sliders'
          featureInd={2}
          appReady={appReady}
          config={config}
          setStatus={setStatus}
          enabled={Boolean(featureCapabilities[2])}
        />
        <FeatureButton
          label='Mobile Tethering'
          iconString='wifi'
          featureInd={3}
          appReady={appReady}
          config={config}
          setStatus={setStatus}
          enabled={Boolean(featureCapabilities[3])}
        />
      </View>
      <View style={styles.featureRow}>
        <FeatureButton
          label='Ethernet Transfer'
          iconString='upload-cloud'
          featureInd={4}
          appReady={appReady}
          config={config}
          setStatus={setStatus}
          enabled={Boolean(featureCapabilities[4])}
        />
        <FeatureButton
          label='Screen Share'
          iconString='airplay'
          featureInd={5}
          appReady={appReady}
          config={config}
          setStatus={setStatus}
          enabled={Boolean(featureCapabilities[5])}
        />
      </View>
    </View>
  )
}

const styles = StyleSheet.create({
  featureCard: {
    flexDirection: "column",
    marginVertical: 6,
    gap: 6
  },
  featureRow: {
    flexGrow: 1,
    flexDirection: "row",
    gap: 6,
  },
})
