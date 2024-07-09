import { Pressable, StyleProp, ViewStyle, StyleSheet, Text, PlatformColor } from "react-native";
import { useState, Dispatch, SetStateAction, useEffect } from 'react';
import s from './Style'
import { status } from './StatusBar'
// @ts-ignore: no ts definitions
import Icon from 'react-native-vector-icons/Feather';

export default function FeatureButton({ label, iconString, style, featureInd, appReady, config, setStatus, enabled }: {
  label: string,
  iconString: string,
  style?: StyleProp<ViewStyle>;
  featureInd: number;
  appReady: boolean;
  config: number;
  setStatus: Dispatch<SetStateAction<status>>;
  enabled: boolean;
}) {
  const [isPressed, setIsPressed] = useState(false);
  const [isHovered, setIsHovered] = useState(false);
  const [isActive, setIsActive] = useState(false);
  const [featurePending, setFeaturePending] = useState(false);

  let bgStyle: StyleProp<ViewStyle>;
  if (!enabled) {
    bgStyle = s.buttonBgDisabled;
  } else if (isActive && featurePending) {
    bgStyle = s.buttonBgActivePress;
  } else if (!isActive && featurePending) {
    bgStyle = s.buttonBgPress;
  } else if (isActive) {
    bgStyle = s.buttonBgActive;
  } else if (isPressed) {
    bgStyle = s.buttonBgPress;
  } else if (isHovered) {
    bgStyle = s.buttonBgHover;
  } else {
    bgStyle = s.buttonBg;
  }

  //disable all features when config changes
  useEffect(() => {
    setIsActive(false);
  }, [config])

  function requestToggleFeature(on: boolean) {
    //fake toggle feature request
    if (!appReady) {
      setStatus({
        message: "The app is not ready yet!",
        category: 'error'
      })
      return;
    }
    if (featurePending) {
      setStatus({
        message: "Already a pending feature request!",
        category: 'error'
      })
      return;
    }

    setStatus({
      message: `${on ? 'Enabling' : 'Disabling'} feature...`,
      category: 'warning'
    });
    setFeaturePending(true);

    const timeout = new Promise(r => setTimeout(r, 200));
    timeout.then(() => {
      setStatus({
        message: `Succesfully ${on ? 'enabled' : 'disabled'} feature.`,
        category: 'success'
      })
      setFeaturePending(false);
      setIsActive(on);
    });
  }

  return (
    <Pressable
      style={[style, styles.featureButton, bgStyle]}
      onPressIn={() => {
        if (!enabled) {
          setStatus({
            message: "Feature not available in current config.",
            category: "attention"
          })
          return;
        }
        setIsPressed(true)
      }}
      onPressOut={() => {
        if (!enabled) return;
        setIsPressed(false);
        const nextMode = !isActive;
        requestToggleFeature(nextMode);
      }}
      onHoverIn={() => setIsHovered(true)}
      onHoverOut={() => setIsHovered(false)}
    >
      <Icon name={iconString} size={24} color={PlatformColor('TextFillColorPrimaryBrush')} />
      <Text>{label}</Text>
    </Pressable>
  )
}

const styles = StyleSheet.create({
  featureButton: {
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    flexGrow: 1,
    borderRadius: 4
  }
})
