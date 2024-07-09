import React, { Dispatch, SetStateAction, useEffect, useState, useRef } from 'react';
import { StyleSheet, View, StyleProp, ViewStyle, PlatformColor, Animated, } from 'react-native';
import ConfigButton from './ConfigButton';
import { status } from './StatusBar';
import s from './Style';
import appleModeCapabilities from './Capabilities';

export default function ConfigSelector({ style, config, setConfig, setStatus, appReady, appleMode }: {
  style?: StyleProp<ViewStyle>;
  config: number;
  setConfig: Dispatch<SetStateAction<number>>;
  setStatus: Dispatch<SetStateAction<status>>;
  appReady: boolean;
  appleMode: number;
}) {
  const [isReady, setIsReady] = useState(false);
  const [selectorDims, setSelectorDims] = useState({ x: 0, y: 0, width: 0, height: 0 });

  const selectButtonLeft = useRef(new Animated.Value(0)).current;
  const [configurationPending, setConfigurationPending] = useState(false);

  const configCapabilities = appleModeCapabilities[appleMode];

  const selectButtonBg = configurationPending ?
    s.buttonBgActivePress : s.buttonBgActive;

  //move button when config changes
  useEffect(() => {
    if (!isReady) return;
    const buttonLeft = (config - 1) * (selectorDims.width / 7);
    Animated.timing(selectButtonLeft, {
      toValue: buttonLeft,
      duration: 200,
      useNativeDriver: false,
    }).start();
  }, [config])

  function submitConfigurationRequest(ind: number) {
    if (!isReady) return;
    if (!appReady) {
      setStatus({
        message: "The app is not ready yet!",
        category: 'error'
      })
      return;
    }
    if (configurationPending) {
      setStatus({
        message: "Already a pending configuration request!",
        category: 'error'
      })
      return;
    }
    if (ind == config) {
      setStatus({
        message: "Device already in this configuration!",
        category: 'attention'
      })
      return;
    }
    if (configCapabilities[ind - 1].reduce((a, b) => a + b, 0) == 0) {
      setStatus({
        message: "Config not supported in current mode.",
        category: 'attention'
      })
      return;
    }
    setStatus({
      message: "Setting configuration...",
      category: 'warning'
    })
    setConfigurationPending(true);

    //fake change configuration request
    const timeout = new Promise(r => setTimeout(r, 100));
    timeout.then(() => {
      setStatus({
        message: "Succesfully set configuration.",
        category: 'success'
      })
      setConfigurationPending(false);
      setConfig(ind);
    });
  }

  return (
    <View
      style={[style, styles.configTray, !isReady && s.hide]}
      onLayout={(e) => { setIsReady(true); setSelectorDims(e.nativeEvent.layout) }}
    >
      {[...Array(7).keys()].map(x => ++x).map((ind, _) => (
        <ConfigButton
          key={ind} ind={ind}
          style={styles.configButton}
          onPressed={() => {
            submitConfigurationRequest(ind);
          }}
          enabled={Boolean(configCapabilities[ind - 1].reduce((a, b) => a + b, 0))} //enable if one ok
        />
      ))}
      <Animated.View style={[styles.psSelectButton, {
        left: selectButtonLeft,
        top: 0,
        width: selectorDims.width / 7,
        height: selectorDims.height,
      }, selectButtonBg]}
      />
    </View>
  )
}

const styles = StyleSheet.create({
  configTray: {
    position: 'relative',
    flexDirection: "row",
    marginVertical: 6,
    borderRadius: 4,
  },
  configButton: {
    flexGrow: 1
  },
  psSelectButton: {
    position: 'absolute',
    zIndex: -1,
  },
})
